-- neurax_data_interface: Dual-port RAM with Avalon-ST interface and accelerator access
--
-- Uses Altera altsyncram megafunction (M10K) in BIDIR_DUAL_PORT mode.
-- Port A: Avalon-ST sink for HPS->RAM writes and a DMA-controlled packet source
--         for RAM->HPS reads.
-- Port B: Direct RAM access for the FPGA accelerator (full address range).
--
-- Sink path (HPS -> RAM):
--   Ready only while IDLE (asi_ready_o = '1' when rd_state = IDLE).  Writes are
--   blocked during PREFETCH, SEND, and FLUSH to prevent Port A address conflicts.
--   Each accepted word is written at wr_ptr; the pointer wraps at 2**g_ADDR_WIDTH.
--
-- Source path (RAM -> HPS), packet transmitter:
--   Idle until rd_start_i is pulsed for one cycle.  The start address and length
--   are latched internally at that moment; subsequent changes on rd_start_addr_i
--   or rd_length_i do not affect the running transfer.  The machine transitions:
--
--     IDLE -> PREFETCH -> SEND -> FLUSH -> IDLE
--
--   PREFETCH (1 cycle): registers rd_ptr on Port A so q_a = RAM[rd_ptr] is
--     available at the start of SEND.
--   SEND: asserts aso_valid_o and streams one word per successful handshake.
--     Look-ahead addressing keeps the RAM output pipeline one step ahead,
--     sustaining 1-word/cycle throughput under continuous ready.
--   FLUSH (1 cycle): asserts rd_done_o and guarantees aso_valid_o is deasserted
--     before returning to IDLE.  Occurs only after the last word is accepted so
--     it does not reduce transfer throughput.
--
-- Avalon-MM register integration:
--   rd_start_i      maps to CONTROL.START  (write-1-to-start)
--   rd_start_addr_i maps to READ_START_ADDRESS register
--   rd_length_i     maps to READ_LENGTH register
--   rd_busy_o       maps to CONTROL.BUSY   (read-only status)
--   rd_done_o       maps to CONTROL.DONE   (self-clearing one-cycle pulse)
--   Replacing direct input ports with Avalon-MM registers requires only wiring
--   register write-data to the input ports and the write strobe to rd_start_i.
--
-- RAM note: the full 2**g_ADDR_WIDTH = 32768 words of physical RAM are used.
--   The upper 32768-23000 = 9768 addresses are unused but harmless.
--   g_RAM_SIZE has been removed to avoid implying a software-enforced wrap.
--
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library altera_mf;
use altera_mf.altera_mf_components.all;

entity neurax_data_interface is
    generic (
        g_DATA_WIDTH : natural := 32;
        g_ADDR_WIDTH : natural := 15    -- 15 bits => 32768-word RAM (covers 23000 words)
    );
    port (
        clk_i : in  std_logic;
        rst_i : in  std_logic;

        ------ Avalon-ST Sink (HPS -> RAM) ------
        asi_channel_i : in  std_logic;
        asi_data_i    : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        asi_valid_i   : in  std_logic;
        asi_ready_o   : out std_logic;
        asi_error_i   : in  std_logic;

        ------ Avalon-ST Source (RAM -> HPS, packet mode) ------
        aso_data_o    : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        aso_valid_o   : out std_logic;
        aso_ready_i   : in  std_logic;
        aso_channel_o : out std_logic;
        aso_error_o   : out std_logic;
        aso_sop_o     : out std_logic;   -- startofpacket: high on first word
        aso_eop_o     : out std_logic;   -- endofpacket:   high on last  word

        ------ Read control (maps to Avalon-MM register block) ------
        rd_start_i      : in  std_logic;                                    -- CONTROL.START (one-cycle pulse)
        rd_start_addr_i : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0); -- READ_START_ADDRESS
        rd_length_i     : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0); -- READ_LENGTH (words)
        rd_busy_o       : out std_logic;                                    -- CONTROL.BUSY
        rd_done_o       : out std_logic;                                    -- CONTROL.DONE (one-cycle pulse)

        ------ RAM Port B (accelerator access, unchanged) ------
        ram_b_rdaddress_i : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_b_q_o         : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        ram_b_wraddress_i : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_b_data_i      : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        ram_b_wren_i      : in  std_logic;

        ------ Port B diagnostic trace (software-readable) ------
        diag_trace_control_i : in  std_logic_vector(31 downto 0);
        diag_trace_phase_i   : in  std_logic_vector(3 downto 0);
        diag_trace_data_o    : out std_logic_vector(31 downto 0)
    );
end neurax_data_interface;

architecture arch of neurax_data_interface is

    -- Port A signals (shared between sink and source)
    signal port_a_address : std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
    signal port_a_wren    : std_logic;
    signal port_a_wrdata  : std_logic_vector(g_DATA_WIDTH - 1 downto 0);
    signal port_a_q       : std_logic_vector(g_DATA_WIDTH - 1 downto 0);

    -- Port B signals (accelerator)
    signal port_b_address : std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
    signal port_b_q       : std_logic_vector(g_DATA_WIDTH - 1 downto 0);
    signal port_b_ram_data : std_logic_vector(g_DATA_WIDTH - 1 downto 0);
    signal port_b_ram_wren : std_logic;
    signal port_b_diag_write : std_logic;
    signal diag_trace_read_address : std_logic_vector(g_ADDR_WIDTH - 1 downto 0);

    -- Eight-cycle snapshot around the first read of weight word 0 (0x2710).
    -- Three 32-bit words per sample: {phase, requested address},
    -- {write-enable, muxed Port B address}, and accelerator-facing q.
    type diag_trace_mem_t is array (0 to 23) of std_logic_vector(31 downto 0);
    signal diag_trace_mem       : diag_trace_mem_t;
    signal diag_trace_count     : natural range 0 to 8;
    signal diag_trace_active    : std_logic;
    signal diag_trace_triggered : std_logic;

    -- Keep the physical Port B path visible to SignalTap during on-board
    -- weight-read diagnosis (address mux, write enable, and M10K output).
    attribute keep : boolean;
    attribute keep of port_b_address : signal is true;
    attribute keep of port_b_q       : signal is true;

    -- Sink write pointer (wraps naturally at 2**g_ADDR_WIDTH)
    signal wr_ptr : unsigned(g_ADDR_WIDTH - 1 downto 0);

    -- Sink backpressure (combinatorial; '1' only in IDLE)
    signal sink_ready : std_logic;

    -- Latched command registers (sampled once when rd_start_i is accepted;
    -- stable throughout PREFETCH, SEND and FLUSH; connection points for
    -- READ_START_ADDRESS and READ_LENGTH Avalon-MM registers)
    signal cmd_start_addr : unsigned(g_ADDR_WIDTH - 1 downto 0);
    signal cmd_length     : unsigned(g_ADDR_WIDTH - 1 downto 0);

    -- Source read state machine
    type rd_state_t is (IDLE, PREFETCH, SEND, FLUSH);
    signal rd_state     : rd_state_t;
    signal rd_ptr       : unsigned(g_ADDR_WIDTH - 1 downto 0);
    signal rd_remaining : unsigned(g_ADDR_WIDTH - 1 downto 0);
    signal rd_first     : std_logic;   -- '1' on the first word of a packet (SOP)

begin

    -- =========================================================================
    -- True Dual-Port RAM (altsyncram / M10K)
    -- outdata_reg_a = UNREGISTERED: q_a reflects the address registered at the
    --   previous rising edge; 1-cycle latency from address presented to data out.
    -- NEW_DATA_NO_NBE_READ: on a write, q_a = data_a (same rising edge).
    -- =========================================================================
    u_dpram : altsyncram
    generic map (
        operation_mode                => "BIDIR_DUAL_PORT",
        width_a                       => g_DATA_WIDTH,
        widthad_a                     => g_ADDR_WIDTH,
        numwords_a                    => 2**g_ADDR_WIDTH,
        width_b                       => g_DATA_WIDTH,
        widthad_b                     => g_ADDR_WIDTH,
        numwords_b                    => 2**g_ADDR_WIDTH,
        lpm_type                      => "altsyncram",
        outdata_reg_a                 => "UNREGISTERED",
        outdata_reg_b                 => "CLOCK1",
        address_reg_b                 => "CLOCK1",
        address_aclr_a                => "NONE",
        address_aclr_b                => "NONE",
        indata_aclr_a                 => "NONE",
        indata_aclr_b                 => "NONE",
        wrcontrol_aclr_a              => "NONE",
        wrcontrol_aclr_b              => "NONE",
        outdata_aclr_a                => "NONE",
        outdata_aclr_b                => "NONE",
        read_during_write_mode_port_a => "NEW_DATA_NO_NBE_READ",
        read_during_write_mode_port_b => "NEW_DATA_NO_NBE_READ",
        power_up_uninitialized        => "FALSE",
        intended_device_family        => "Cyclone V",
        clock_enable_input_a          => "BYPASS",
        clock_enable_input_b          => "BYPASS",
        clock_enable_output_a         => "BYPASS",
        clock_enable_output_b         => "BYPASS"
    )
    port map (
        clock0         => clk_i,
        clock1         => clk_i,
        address_a      => port_a_address,
        data_a         => port_a_wrdata,
        wren_a         => port_a_wren,
        q_a            => port_a_q,
        address_b      => port_b_address,
        data_b         => port_b_ram_data,
        wren_b         => port_b_ram_wren,
        q_b            => port_b_q,
        aclr0          => '0',
        aclr1          => '0',
        byteena_a      => (others => '1'),
        byteena_b      => (others => '1'),
        clocken0       => '1',
        clocken1       => '1',
        addressstall_a => '0',
        addressstall_b => '0',
        rden_a         => '1',
        rden_b         => '1'
    );

    -- =========================================================================
    -- Port B: accelerator access.
    -- Keep read and write path mutually exclusive in time. Register both the
    -- Port B address and q_b on CLOCK1, as required for a legal Cyclone V
    -- altsyncram Port B clock configuration.
    -- =========================================================================
    port_b_diag_write <= diag_trace_control_i(30) and diag_trace_control_i(14);
    port_b_ram_wren <= '1' when port_b_diag_write = '1' else ram_b_wren_i;
    port_b_ram_data <= std_logic_vector(to_unsigned(16#12345678#, g_DATA_WIDTH))
                       when port_b_diag_write = '1' else ram_b_data_i;

    ram_b_q_o      <= port_b_q;
    port_b_address <= ram_b_wraddress_i when ram_b_wren_i = '1'
                      else diag_trace_read_address when diag_trace_control_i(30) = '1'
                      else ram_b_rdaddress_i;
    diag_trace_read_address <= diag_trace_control_i(29 downto 15)
                               when diag_trace_control_i(30) = '1'
                               else ram_b_rdaddress_i;

    -- Software selects words 0..23 through REG_TEMP_2 and reads the selected
    -- word through REG_TEMP_3. Selector 31 returns capture status.
    process(diag_trace_control_i, diag_trace_mem, diag_trace_count,
            diag_trace_active, diag_trace_triggered)
        variable trace_index : natural range 0 to 31;
        variable trace_status : std_logic_vector(31 downto 0);
    begin
        trace_index := to_integer(unsigned(diag_trace_control_i(4 downto 0)));
        if trace_index < 24 then
            diag_trace_data_o <= diag_trace_mem(trace_index);
        elsif trace_index = 31 then
            trace_status := (others => '0');
            trace_status(0) := diag_trace_triggered;
            trace_status(1) := diag_trace_active;
            trace_status(7 downto 2) := std_logic_vector(to_unsigned(diag_trace_count, 6));
            diag_trace_data_o <= trace_status;
        else
            diag_trace_data_o <= (others => '0');
        end if;
    end process;

    -- A one-cycle write of bit 31 in REG_TEMP_2 clears/re-arms the capture.
    -- Capture the first 8 cycles beginning with the first requested read of
    -- word 10000, regardless of whether the M10K returns the expected value.
    process(clk_i, rst_i)
        variable sample_index : natural range 0 to 7;
    begin
        if rst_i = '1' then
            diag_trace_mem       <= (others => (others => '0'));
            diag_trace_count     <= 0;
            diag_trace_active    <= '0';
            diag_trace_triggered <= '0';
        elsif rising_edge(clk_i) then
            if diag_trace_control_i(31) = '1' then
                diag_trace_mem       <= (others => (others => '0'));
                diag_trace_count     <= 0;
                diag_trace_active    <= '0';
                diag_trace_triggered <= '0';
            elsif diag_trace_active = '1' then
                sample_index := diag_trace_count;
                diag_trace_mem(sample_index * 3) <=
                    x"000" & diag_trace_phase_i & '0' & diag_trace_read_address;
                diag_trace_mem(sample_index * 3 + 1) <=
                    x"0000" & port_b_ram_wren & port_b_address;
                diag_trace_mem(sample_index * 3 + 2) <= port_b_q;
                if diag_trace_count = 7 then
                    diag_trace_count  <= 8;
                    diag_trace_active <= '0';
                else
                    diag_trace_count <= diag_trace_count + 1;
                end if;
            elsif diag_trace_triggered = '0' and port_b_ram_wren = '0' and
                (diag_trace_control_i(30) = '1' or
                 unsigned(ram_b_rdaddress_i) = to_unsigned(16#2710#, g_ADDR_WIDTH)) then
                sample_index := 0;
                diag_trace_mem(0) <=
                  x"000" & diag_trace_phase_i & '0' & diag_trace_read_address;
                diag_trace_mem(1) <=
                    x"0000" & port_b_ram_wren & port_b_address;
                diag_trace_mem(2) <= port_b_q;
                diag_trace_count     <= 1;
                diag_trace_active    <= '1';
                diag_trace_triggered <= '1';
            end if;
        end if;
    end process;

    -- =========================================================================
    -- Sink path
    --   sink_ready gates both asi_ready_o and port_a_wren.
    --   Deasserts the moment a read transfer starts (rd_state /= IDLE), preventing
    --   Port A address collisions without any software ordering requirement.
    --   A channel-1 packet marker is treated as a reset marker for the write
    --   pointer; it is not stored into RAM as payload data.
    -- =========================================================================
    sink_ready    <= '1' when rd_state = IDLE else '0';
    asi_ready_o   <= sink_ready;
    port_a_wren   <= asi_valid_i and sink_ready and (not asi_channel_i);
    port_a_wrdata <= asi_data_i;

    -- =========================================================================
    -- Port A address mux (three combinatorial priorities):
    --
    --   1. Write active (port_a_wren = '1', only possible in IDLE):
    --        wr_ptr — routes the incoming sink word to the circular buffer.
    --
    --   2. SEND with downstream ready — look-ahead:
    --        rd_ptr + 1 — pre-fetches the next word while the current word is
    --        being accepted.  The RAM registers rd_ptr+1 on this rising edge so
    --        q_a = RAM[rd_ptr+1] is available at the very start of the next SEND
    --        cycle, maintaining 1-word/cycle throughput with zero gap.
    --
    --   3. Default (IDLE without write, PREFETCH, SEND stall, FLUSH):
    --        rd_ptr — holds the current read address stable.  In PREFETCH this
    --        causes the RAM to capture rd_ptr (= start_addr) so that
    --        RAM[start_addr] is valid at the first SEND cycle.  During a SEND
    --        stall (aso_ready_i = '0') the address is unchanged and q_a remains
    --        valid for as long as backpressure persists.
    -- =========================================================================
    port_a_address <=
        std_logic_vector(wr_ptr)          when port_a_wren = '1'
        else std_logic_vector(rd_ptr + 1) when (rd_state = SEND and aso_ready_i = '1')
        else std_logic_vector(rd_ptr);

    -- =========================================================================
    -- Source outputs (all combinatorial from state machine)
    -- =========================================================================
    aso_valid_o   <= '1' when rd_state = SEND                            else '0';
    aso_data_o    <= port_a_q;
    aso_channel_o <= '0';
    aso_error_o   <= '0';
    aso_sop_o     <= rd_first when rd_state = SEND                       else '0';
    aso_eop_o     <= '1'      when rd_state = SEND and rd_remaining = 1  else '0';

    -- Status outputs (map to CONTROL register bits in Avalon-MM block)
    rd_busy_o     <= '0' when rd_state = IDLE                            else '1';
    rd_done_o     <= '1' when rd_state = FLUSH                           else '0';

    -- =========================================================================
    -- Clocked process: sink write pointer + source read state machine
    -- =========================================================================
    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            wr_ptr         <= (others => '0');
            cmd_start_addr <= (others => '0');
            cmd_length     <= (others => '0');
            rd_state       <= IDLE;
            rd_ptr         <= (others => '0');
            rd_remaining   <= (others => '0');
            rd_first       <= '1';

        elsif rising_edge(clk_i) then

            -- ---- Sink: channel=1 resets the write pointer, channel=0 advances it
            -- on every accepted handshake.  The marker word is not stored into RAM.
            if asi_valid_i = '1' and sink_ready = '1' and asi_channel_i = '1' then
                wr_ptr <= (others => '0');
            elsif port_a_wren = '1' then
                wr_ptr <= wr_ptr + 1;
            end if;

            -- ---- Source: packet transmit state machine ----
            case rd_state is

                when IDLE =>
                    -- Sample command inputs on the accepting cycle and latch them.
                    -- rd_start_i pulses that arrive in PREFETCH, SEND, or FLUSH are
                    -- ignored because this branch is never evaluated outside IDLE.
                    if rd_start_i = '1' and unsigned(rd_length_i) /= 0 then
                        -- Latch: cmd registers hold the original command for the full
                        -- transfer duration and serve as the connection point for
                        -- READ_START_ADDRESS / READ_LENGTH Avalon-MM registers.
                        cmd_start_addr <= unsigned(rd_start_addr_i);
                        cmd_length     <= unsigned(rd_length_i);
                        -- Initialise working pointers from the latched command.
                        rd_ptr         <= unsigned(rd_start_addr_i);
                        rd_remaining   <= unsigned(rd_length_i);
                        rd_first       <= '1';
                        rd_state       <= PREFETCH;
                    end if;

                when PREFETCH =>
                    -- port_a_address = rd_ptr is already on Port A combinatorially.
                    -- The altsyncram registers that address on this rising edge;
                    -- q_a = RAM[rd_ptr] is valid from the next cycle (SEND) onward.
                    rd_state <= SEND;

                when SEND =>
                    -- Advance only on a successful Avalon-ST handshake.
                    if aso_ready_i = '1' then
                        rd_first <= '0';
                        if rd_remaining = 1 then
                            -- Last word accepted.  Move to FLUSH to assert rd_done_o
                            -- and cleanly deassert aso_valid_o before returning to IDLE.
                            rd_state <= FLUSH;
                        else
                            rd_ptr       <= rd_ptr + 1;
                            rd_remaining <= rd_remaining - 1;
                            -- Look-ahead: port_a_address = rd_ptr+1 this cycle
                            -- (combinatorial mux, priority 2), so RAM[rd_ptr+1] is
                            -- registered now and available at the next SEND cycle.
                        end if;
                    end if;

                when FLUSH =>
                    -- One-cycle cleanup state entered after the last word is accepted.
                    -- rd_done_o is '1' (combinatorial from this state).
                    -- aso_valid_o is '0' (only SEND asserts it).
                    -- Transitioning to IDLE releases sink_ready, unblocking the write DMA.
                    rd_state <= IDLE;

            end case;
        end if;
    end process;

end arch;
