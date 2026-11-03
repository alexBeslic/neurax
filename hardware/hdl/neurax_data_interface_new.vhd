-- neurax_data_interface: Dual-port RAM with mSGDMA-compatible Avalon-ST
-- sink/source and accelerator access on Port B.
--
-- Uses Altera altsyncram for guaranteed M10K mapping. True dual-port mode:
-- Port A is owned by the Avalon-ST sink/source, Port B by the accelerator.
--
-- Operation lifecycle (driven by an external CSR module):
--
--   1. CSR programs the mSGDMA (MM->ST read master) descriptor; HPS streams
--      one frame to the sink. Sink writes RAM starting at address 0. When
--      endofpacket arrives, frame_len_o latches the word count and
--      load_done_o pulses for one cycle. If any beat carried error,
--      frame_error_o pulses instead (frame is left in RAM but flagged bad).
--
--   2. CSR commands the accelerator to compute. Accelerator owns Port B and
--      may read inputs and write results anywhere in RAM.
--
--   3. CSR pulses start_readback_i with readback_len_i words to play back.
--      The source streams RAM[0 .. readback_len_i-1] to the mSGDMA (ST->MM
--      write master) with startofpacket on the first beat and endofpacket
--      on the last. readback_done_o pulses for one cycle when the eop beat
--      is accepted by the downstream sink.
--
-- Notes for Platform Designer integration (_hw.tcl):
--   - readyLatency = 0, readyAllowance = 0 on both ST interfaces.
--   - symbolsPerBeat = 1, symbolWidth = g_DATA_WIDTH (no `empty` signal).
--   - rst_i is async-asserted; wrap it through altera_reset_synchronizer
--     or mark the reset as async in the _hw.tcl.
--   - The HPS-side flow MUST serialize load -> compute -> readback. The
--     sink is backpressured (ready = 0) while a readback is in progress.
--
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library altera_mf;
use altera_mf.altera_mf_components.all;

entity neurax_data_interface is
    generic (
        g_DATA_WIDTH : natural := 32;
        g_ADDR_WIDTH : natural := 15;     -- ceil(log2(g_RAM_SIZE))
        g_RAM_SIZE   : natural := 23000   -- max words; must be < 2**g_ADDR_WIDTH
    );
    port (
        clk_i : in std_logic;
        rst_i : in std_logic;

        ------ Avalon-ST Sink (HPS -> RAM, fed by mSGDMA MM->ST) ------
        asi_data_i          : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        asi_valid_i         : in  std_logic;
        asi_ready_o         : out std_logic;
        asi_startofpacket_i : in  std_logic;
        asi_endofpacket_i   : in  std_logic;
        asi_error_i         : in  std_logic;

        ------ Avalon-ST Source (RAM -> HPS, drained by mSGDMA ST->MM) ------
        aso_data_o          : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        aso_valid_o         : out std_logic;
        aso_ready_i         : in  std_logic;
        aso_startofpacket_o : out std_logic;
        aso_endofpacket_o   : out std_logic;
        aso_error_o         : out std_logic;

        ------ Control hooks (to/from external CSR / accelerator wrapper) ------
        load_done_o      : out std_logic;                                  -- 1-cycle pulse on sink eop (good frame)
        frame_error_o    : out std_logic;                                  -- 1-cycle pulse on sink eop (tainted frame)
        frame_len_o      : out std_logic_vector(g_ADDR_WIDTH - 1 downto 0);-- words written in last frame
        start_readback_i : in  std_logic;                                  -- 1-cycle pulse to begin source readback
        readback_len_i   : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0);-- words to stream out (latched at start)
        readback_done_o  : out std_logic;                                  -- 1-cycle pulse when source eop is accepted
        busy_o           : out std_logic;                                  -- high while sink or source is active

        ------ Accelerator RAM Port B ------
        ram_b_rdaddress_i : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_b_q_o         : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        ram_b_wraddress_i : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_b_data_i      : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        ram_b_wren_i      : in  std_logic
    );
end neurax_data_interface;

architecture arch of neurax_data_interface is

    -- =========================================================================
    -- RAM port signals
    -- =========================================================================
    signal port_a_address : std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
    signal port_a_wren    : std_logic;
    signal port_a_wrdata  : std_logic_vector(g_DATA_WIDTH - 1 downto 0);
    signal port_a_q       : std_logic_vector(g_DATA_WIDTH - 1 downto 0);

    signal port_b_q       : std_logic_vector(g_DATA_WIDTH - 1 downto 0);
    signal port_b_address : std_logic_vector(g_ADDR_WIDTH - 1 downto 0);

    constant c_MAX_WORDS : unsigned(g_ADDR_WIDTH - 1 downto 0)
                         := to_unsigned(g_RAM_SIZE, g_ADDR_WIDTH);

    -- =========================================================================
    -- Sink (write) FSM state
    -- =========================================================================
    signal write_index    : unsigned(g_ADDR_WIDTH - 1 downto 0) := (others => '0');
    signal in_frame       : std_logic := '0';   -- between accepted sop and eop
    signal err_taint      : std_logic := '0';   -- any beat carried error
    signal frame_len_r    : unsigned(g_ADDR_WIDTH - 1 downto 0) := (others => '0');
    signal load_done_r    : std_logic := '0';
    signal frame_error_r  : std_logic := '0';

    signal sink_handshake : std_logic;          -- valid AND ready
    signal write_addr_mux : std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
    signal at_capacity    : std_logic;          -- write_index == c_MAX_WORDS

    -- =========================================================================
    -- Source (read) FSM state + skid buffer
    -- =========================================================================
    signal reading_active : std_logic := '0';
    signal read_index     : unsigned(g_ADDR_WIDTH - 1 downto 0) := (others => '0');
    signal read_len       : unsigned(g_ADDR_WIDTH - 1 downto 0) := (others => '0');

    signal read_issue  : std_logic := '0';
    signal sop_issue   : std_logic := '0';
    signal eop_issue   : std_logic := '0';

    signal ram_pending  : std_logic := '0';     -- stage-1: address presented to RAM
    signal ram_sop_pend : std_logic := '0';
    signal ram_eop_pend : std_logic := '0';

    signal ram_valid_reg : std_logic := '0';    -- stage-2: RAM data captured
    signal ram_sop_reg   : std_logic := '0';
    signal ram_eop_reg   : std_logic := '0';
    signal ram_data_reg  : std_logic_vector(g_DATA_WIDTH - 1 downto 0) := (others => '0');

    -- 2-entry skid buffer between the RAM pipeline and the ST source.
    -- Lets the read pipeline keep draining when aso_ready_i drops, and keeps
    -- valid+data stable on the bus until ready is sampled high.
    type t_skid_entry is record
        data : std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        sop  : std_logic;
        eop  : std_logic;
    end record;
    type t_skid_array is array (0 to 1) of t_skid_entry;

    signal skid       : t_skid_array := (others => (data => (others => '0'),
                                                    sop => '0', eop => '0'));
    signal skid_count : unsigned(1 downto 0) := (others => '0');  -- 0..2
    signal in_flight  : unsigned(1 downto 0) := (others => '0');  -- 0..2

    signal aso_valid_int : std_logic := '0';
    signal aso_data_int  : std_logic_vector(g_DATA_WIDTH - 1 downto 0) := (others => '0');
    signal aso_sop_int   : std_logic := '0';
    signal aso_eop_int   : std_logic := '0';

    signal readback_done_r : std_logic := '0';

begin

    -- =========================================================================
    -- True dual-port M10K RAM
    -- =========================================================================
    u_dpram : altsyncram
    generic map (
        operation_mode          => "BIDIR_DUAL_PORT",
        width_a                 => g_DATA_WIDTH,
        widthad_a               => g_ADDR_WIDTH,
        numwords_a              => 2**g_ADDR_WIDTH,
        width_b                 => g_DATA_WIDTH,
        widthad_b               => g_ADDR_WIDTH,
        numwords_b              => 2**g_ADDR_WIDTH,
        lpm_type                => "altsyncram",
        outdata_reg_a           => "UNREGISTERED",
        outdata_reg_b           => "UNREGISTERED",
        address_aclr_a          => "NONE",
        address_aclr_b          => "NONE",
        indata_aclr_a           => "NONE",
        indata_aclr_b           => "NONE",
        wrcontrol_aclr_a        => "NONE",
        wrcontrol_aclr_b        => "NONE",
        outdata_aclr_a          => "NONE",
        outdata_aclr_b          => "NONE",
        read_during_write_mode_port_a => "NEW_DATA_NO_NBE_READ",
        read_during_write_mode_port_b => "NEW_DATA_NO_NBE_READ",
        power_up_uninitialized  => "FALSE",
        intended_device_family  => "Cyclone V",
        clock_enable_input_a    => "BYPASS",
        clock_enable_input_b    => "BYPASS",
        clock_enable_output_a   => "BYPASS",
        clock_enable_output_b   => "BYPASS"
    )
    port map (
        clock0    => clk_i,
        clock1    => clk_i,
        address_a => port_a_address,
        data_a    => port_a_wrdata,
        wren_a    => port_a_wren,
        q_a       => port_a_q,
        address_b => port_b_address,
        data_b    => ram_b_data_i,
        wren_b    => ram_b_wren_i,
        q_b       => port_b_q,
        aclr0     => '0',
        aclr1     => '0',
        byteena_a => (others => '1'),
        byteena_b => (others => '1'),
        clocken0  => '1',
        clocken1  => '1',
        addressstall_a => '0',
        addressstall_b => '0',
        rden_a    => '1',
        rden_b    => '1'
    );

    ram_b_q_o      <= port_b_q;
    port_b_address <= ram_b_wraddress_i when ram_b_wren_i = '1'
                      else ram_b_rdaddress_i;

    -- =========================================================================
    -- Port A combinational drivers
    --
    -- The sink owns Port A whenever the source isn't reading. We do NOT block
    -- the sink while a frame is loaded-but-unread; the HPS is allowed to
    -- overwrite by issuing a new descriptor whose first beat carries sop=1.
    -- We DO block the sink while a readback is actively in progress, to keep
    -- writes from racing the read pointer through the M10K block.
    -- =========================================================================
    asi_ready_o   <= '0' when (rst_i = '1' or reading_active = '1') else '1';
    sink_handshake <= asi_valid_i and (not reading_active);

    -- On an sop beat the first word always lands at address 0, regardless of
    -- any leftover write_index value from a previous frame.
    write_addr_mux <= (others => '0') when asi_startofpacket_i = '1'
                      else std_logic_vector(write_index);

    at_capacity <= '1' when write_index = c_MAX_WORDS else '0';

    -- Write only when (a) sink owns Port A, (b) the beat is part of a frame
    -- (sop, or already mid-frame), and (c) we haven't run off the end of RAM.
    port_a_wren <= sink_handshake
                   and (asi_startofpacket_i or in_frame)
                   and (not at_capacity);

    port_a_wrdata <= asi_data_i;

    -- Port A address mux: read pointer during readback, write address otherwise.
    port_a_address <= std_logic_vector(read_index) when reading_active = '1'
                      else write_addr_mux;

    -- =========================================================================
    -- Sink (write) FSM
    --
    -- Avalon-ST semantics: error is a per-beat qualifier, not a stop request.
    -- We accept every beat of the descriptor (so mSGDMA can finish), but if
    -- any beat in the frame asserts error_i we taint the frame and pulse
    -- frame_error_o on eop instead of load_done_o.
    -- =========================================================================
    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            write_index   <= (others => '0');
            in_frame      <= '0';
            err_taint     <= '0';
            frame_len_r   <= (others => '0');
            load_done_r   <= '0';
            frame_error_r <= '0';

        elsif rising_edge(clk_i) then
            load_done_r   <= '0';
            frame_error_r <= '0';

            if sink_handshake = '1' then
                if asi_startofpacket_i = '1' then
                    -- Start a new frame; first beat lands at address 0.
                    write_index <= to_unsigned(1, g_ADDR_WIDTH);
                    in_frame    <= '1';
                    err_taint   <= asi_error_i;

                    if asi_endofpacket_i = '1' then
                        -- Single-beat frame: open and close on the same edge.
                        frame_len_r <= to_unsigned(1, g_ADDR_WIDTH);
                        in_frame    <= '0';
                        if asi_error_i = '0' then
                            load_done_r <= '1';
                        else
                            frame_error_r <= '1';
                        end if;
                    end if;

                elsif in_frame = '1' then
                    -- Mid-frame beat.
                    if at_capacity = '0' then
                        write_index <= write_index + 1;
                    else
                        -- Frame exceeds RAM capacity: stop advancing, taint.
                        err_taint <= '1';
                    end if;

                    if asi_error_i = '1' then
                        err_taint <= '1';
                    end if;

                    if asi_endofpacket_i = '1' then
                        in_frame <= '0';
                        if at_capacity = '0' then
                            frame_len_r <= write_index + 1;
                        else
                            frame_len_r <= write_index;  -- last beat was dropped
                        end if;

                        if err_taint = '0' and asi_error_i = '0'
                           and at_capacity = '0' then
                            load_done_r <= '1';
                        else
                            frame_error_r <= '1';
                        end if;
                    end if;
                end if;
                -- A non-sop beat with in_frame='0' is an upstream protocol
                -- violation: silently dropped (no write, no advance).
            end if;
        end if;
    end process;

    -- =========================================================================
    -- Control / status outputs
    -- =========================================================================
    load_done_o     <= load_done_r;
    frame_error_o   <= frame_error_r;
    frame_len_o     <= std_logic_vector(frame_len_r);
    readback_done_o <= readback_done_r;

    busy_o <= '1' when (in_frame      = '1'
                        or reading_active = '1'
                        or ram_pending    = '1'
                        or ram_valid_reg  = '1'
                        or skid_count /= 0)
              else '0';

    aso_data_o          <= aso_data_int;
    aso_valid_o         <= aso_valid_int;
    aso_startofpacket_o <= aso_sop_int;
    aso_endofpacket_o   <= aso_eop_int;
    aso_error_o         <= '0';

    -- =========================================================================
    -- Source (read) FSM with 2-entry skid buffer
    --
    -- Pipeline (one beat per stage):
    --   read_issue  -->  ram_*_pend (address at RAM)
    --               -->  ram_*_reg  (RAM q captured)
    --               -->  skid buffer (1-2 entries)
    --               -->  aso_* outputs (held until ready)
    --
    -- Backpressure invariant: a new read is issued only when
    --     in_flight + skid_count < 2
    -- so any beat that enters the pipeline always has a slot to land in,
    -- even if the downstream sink keeps ready low for arbitrary cycles.
    -- =========================================================================
    process(clk_i, rst_i)
        variable v_skid       : t_skid_array;
        variable v_skid_count : unsigned(1 downto 0);
        variable v_in_flight  : unsigned(1 downto 0);
        variable v_pop        : boolean;
        variable v_can_issue  : boolean;
        variable v_popped_eop : std_logic;
    begin
        if rst_i = '1' then
            reading_active <= '0';
            read_index     <= (others => '0');
            read_len       <= (others => '0');

            read_issue   <= '0';
            sop_issue    <= '0';
            eop_issue    <= '0';

            ram_pending  <= '0';
            ram_sop_pend <= '0';
            ram_eop_pend <= '0';

            ram_valid_reg <= '0';
            ram_sop_reg   <= '0';
            ram_eop_reg   <= '0';
            ram_data_reg  <= (others => '0');

            skid       <= (others => (data => (others => '0'),
                                       sop => '0', eop => '0'));
            skid_count <= (others => '0');
            in_flight  <= (others => '0');

            aso_valid_int <= '0';
            aso_data_int  <= (others => '0');
            aso_sop_int   <= '0';
            aso_eop_int   <= '0';

            readback_done_r <= '0';

        elsif rising_edge(clk_i) then
            v_skid       := skid;
            v_skid_count := skid_count;
            v_in_flight  := in_flight;
            v_popped_eop := '0';

            readback_done_r <= '0';

            ------------------------------------------------------------------
            -- 1) Downstream pop: if the beat we showed last cycle was taken,
            --    drop it off the head of the skid buffer.
            ------------------------------------------------------------------
            v_pop := (aso_valid_int = '1') and (aso_ready_i = '1');
            if v_pop then
                v_popped_eop := v_skid(0).eop;
                v_skid(0)    := v_skid(1);
                v_skid(1)    := (data => (others => '0'), sop => '0', eop => '0');
                v_skid_count := v_skid_count - 1;
            end if;

            ------------------------------------------------------------------
            -- 2) Capture RAM output from stage-1 into stage-2.
            ------------------------------------------------------------------
            ram_valid_reg <= ram_pending;
            ram_sop_reg   <= ram_sop_pend;
            ram_eop_reg   <= ram_eop_pend;
            ram_data_reg  <= port_a_q;

            if ram_pending = '1' then
                v_in_flight := v_in_flight - 1;
            end if;

            ------------------------------------------------------------------
            -- 3) Push stage-2 result into the skid buffer.
            ------------------------------------------------------------------
            if ram_valid_reg = '1' then
                v_skid(to_integer(v_skid_count)) :=
                    (data => ram_data_reg,
                     sop  => ram_sop_reg,
                     eop  => ram_eop_reg);
                v_skid_count := v_skid_count + 1;
            end if;

            ------------------------------------------------------------------
            -- 4) Upstream: issue a new read if pipeline+skid have room.
            ------------------------------------------------------------------
            read_issue <= '0';
            sop_issue  <= '0';
            eop_issue  <= '0';

            v_can_issue := (v_in_flight + v_skid_count) < to_unsigned(2, 2);

            if reading_active = '1' and v_can_issue then
                read_issue <= '1';

                if read_index = 0 then
                    sop_issue <= '1';
                end if;

                if read_index + 1 = read_len then
                    -- last beat to issue for this readback
                    eop_issue      <= '1';
                    reading_active <= '0';
                    read_index     <= (others => '0');
                else
                    read_index <= read_index + 1;
                end if;

                v_in_flight := v_in_flight + 1;
            end if;

            ram_pending  <= read_issue;
            ram_sop_pend <= sop_issue;
            ram_eop_pend <= eop_issue;

            ------------------------------------------------------------------
            -- 5) Start a readback when the CSR pulses start_readback_i.
            --    Ignored unless the pipeline is fully idle and length > 0.
            ------------------------------------------------------------------
            if start_readback_i = '1'
               and reading_active = '0'
               and v_in_flight = 0 and v_skid_count = 0
               and ram_valid_reg = '0'
               and unsigned(readback_len_i) /= 0 then
                reading_active <= '1';
                read_index     <= (others => '0');
                read_len       <= unsigned(readback_len_i);
            end if;

            ------------------------------------------------------------------
            -- 6) Commit working copies and drive the registered outputs.
            ------------------------------------------------------------------
            skid       <= v_skid;
            skid_count <= v_skid_count;
            in_flight  <= v_in_flight;

            if v_skid_count > 0 then
                aso_valid_int <= '1';
                aso_data_int  <= v_skid(0).data;
                aso_sop_int   <= v_skid(0).sop;
                aso_eop_int   <= v_skid(0).eop;
            else
                aso_valid_int <= '0';
                aso_data_int  <= (others => '0');
                aso_sop_int   <= '0';
                aso_eop_int   <= '0';
            end if;

            -- Pulse readback_done when the eop beat is accepted by the sink.
            if v_popped_eop = '1' then
                readback_done_r <= '1';
            end if;
        end if;
    end process;

end arch;
