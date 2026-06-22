-- neurax_data_interface: Shared dual-port RAM with Avalon ST and accelerator access
--
-- Uses Altera altsyncram megafunction for guaranteed M10K block RAM mapping.
-- True dual-port mode: Port A for Avalon ST, Port B for FPGA accelerator.
--
-- Data flow:
--   1. HPS streams input/weights/bias via Avalon ST Sink -> Port A writes to RAM
--   2. HPS starts accelerator via register write
--   3. Accelerator reads input/weights/bias from RAM via Port B, writes output via Port B
--   4. HPS reads results from RAM via Avalon ST Source -> Port A reads from RAM
--
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library altera_mf;
use altera_mf.altera_mf_components.all;

entity neurax_data_interface is
    generic (
        g_DATA_WIDTH   : natural := 32;
        g_ADDR_WIDTH   : natural := 15;   -- ceil(log2(23000)) = 15 bits
        g_RAM_SIZE     : natural := 23000  -- Total RAM size in 32-bit words (one Q8.8 per word)
    );
    port (
        clk_i         : in  std_logic;
        rst_i         : in  std_logic;

        ------ Avalon ST Sink Interface (HPS -> RAM) ------
        asi_channel_i : in  std_logic;
        asi_data_i    : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        asi_valid_i   : in  std_logic;
        asi_ready_o   : out std_logic;
        asi_error_i   : in  std_logic;

        ------ Avalon ST Source Interface (RAM -> HPS) ------
        aso_data_o    : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        aso_valid_o   : out std_logic;
        aso_ready_i   : in  std_logic;
        aso_channel_o : out std_logic;
        aso_error_o   : out std_logic;

        ------ RAM Port B Interface (exposed to accelerator) ------
        ram_b_rdaddress_i : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_b_q_o         : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        ram_b_wraddress_i : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_b_data_i      : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        ram_b_wren_i      : in  std_logic
    );
end neurax_data_interface;

architecture arch of neurax_data_interface is

    -- Port A RAM signals (single address for altsyncram: muxed write/read)
    signal port_a_wraddress : std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
    signal port_a_wren      : std_logic;
    signal port_a_wrdata    : std_logic_vector(g_DATA_WIDTH - 1 downto 0);
    signal port_a_q         : std_logic_vector(g_DATA_WIDTH - 1 downto 0);

    -- Port B RAM signals (directly mapped to accelerator ports)
    signal port_b_q         : std_logic_vector(g_DATA_WIDTH - 1 downto 0);
    -- altsyncram has one address per port; mux read/write address for port B
    signal port_b_address   : std_logic_vector(g_ADDR_WIDTH - 1 downto 0);

    -- Write state machine
    signal write_index    : unsigned(g_ADDR_WIDTH - 1 downto 0) := (others => '0');
    signal write_complete : std_logic := '0';
    signal write_sof      : std_logic := '0';  -- pulsed for 1 cycle on channel=1 SOF
    signal ready_reg      : std_logic := '1';
    signal buffer_full    : std_logic := '0';

    -- Read state machine
    signal read_index      : unsigned(g_ADDR_WIDTH - 1 downto 0) := (others => '0');
    signal read_complete   : std_logic := '0';
    signal buffer_empty    : std_logic := '1';
    signal reading_active  : std_logic := '0';

    -- Read pipeline registers (2-stage for M10K 1-cycle read latency)
    signal read_request    : std_logic := '0';
    signal read_request_d  : std_logic := '0';
    signal sof_request     : std_logic := '0';
    signal sof_request_d   : std_logic := '0';
    signal error_request   : std_logic := '0';
    signal error_request_d : std_logic := '0';

    signal ram_data_reg    : std_logic_vector(g_DATA_WIDTH - 1 downto 0) := (others => '0');
    signal ram_valid_reg   : std_logic := '0';
    signal ram_channel_reg : std_logic := '0';
    signal ram_error_reg   : std_logic := '0';

    constant c_TOTAL_WORDS : natural := g_RAM_SIZE;

begin

    -- =========================================================================
    -- True Dual-Port RAM using altsyncram (maps to M10K blocks)
    -- Port A: Avalon ST read/write
    -- Port B: Accelerator read/write
    -- =========================================================================
    u_dpram : altsyncram
    generic map (
        operation_mode          => "BIDIR_DUAL_PORT",
        width_a                 => g_DATA_WIDTH,
        widthad_a               => g_ADDR_WIDTH,
        numwords_a              => 2**g_ADDR_WIDTH,  -- 32768 words (covers 23000)
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
        -- Port A (Avalon ST side)
        address_a => port_a_wraddress,  -- Write address during writes, read address during reads
        data_a    => port_a_wrdata,
        wren_a    => port_a_wren,
        q_a       => port_a_q,
        -- Port B (Accelerator side) - muxed address: write takes priority
        address_b => port_b_address,
        data_b    => ram_b_data_i,
        wren_b    => ram_b_wren_i,
        q_b       => port_b_q,
        -- Unused
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

    ram_b_q_o <= port_b_q;

    -- Port B address mux: write address when writing, read address when reading
    port_b_address <= ram_b_wraddress_i when ram_b_wren_i = '1'
                      else ram_b_rdaddress_i;

    -- =========================================================================
    -- Port A: Avalon ST Sink -> RAM write logic
    -- =========================================================================
    asi_ready_o <= ready_reg and not buffer_full;

    port_a_wren   <= asi_valid_i and ready_reg and not buffer_full;
    port_a_wrdata <= asi_data_i;

    -- Port A address mux: write_index during writes, read_index during reads
    port_a_wraddress <= std_logic_vector(write_index) when (reading_active = '0')
                        else std_logic_vector(read_index);

    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            write_index    <= (others => '0');
            write_complete <= '0';
            ready_reg      <= '1';
            buffer_full    <= '0';
        elsif rising_edge(clk_i) then
            write_complete <= '0';
            write_sof      <= '0';
            ready_reg      <= '1';

            -- Handle error: reset write pointer
            if asi_error_i = '1' then
                write_index <= (others => '0');
                ready_reg   <= '0';
                buffer_full <= '0';

            -- Handle start-of-frame: reset write pointer and signal read FSM
            elsif asi_channel_i = '1' then
                write_index <= (others => '0');
                buffer_full <= '0';
                write_sof   <= '1';

            -- Normal write
            elsif asi_valid_i = '1' and ready_reg = '1' and buffer_full = '0' then
                if to_integer(write_index) + 1 >= c_TOTAL_WORDS then
                    write_index    <= (others => '0');
                    write_complete <= '1';
                    buffer_full    <= '1';
                    ready_reg      <= '0';
                else
                    write_index <= write_index + 1;
                end if;
            end if;

            -- Release buffer_full when reading completes
            -- (read_complete pulses for 1 cycle at the end of readback)
            if read_complete = '1' then
                buffer_full <= '0';
            end if;
        end if;
    end process;

    -- =========================================================================
    -- Port A: RAM -> Avalon ST Source read logic
    -- =========================================================================
    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            aso_data_o     <= (others => '0');
            aso_valid_o    <= '0';
            aso_channel_o  <= '0';
            aso_error_o    <= '0';
            read_index     <= (others => '0');
            read_complete  <= '0';
            buffer_empty   <= '1';
            reading_active <= '0';

            ram_data_reg    <= (others => '0');
            ram_valid_reg   <= '0';
            ram_channel_reg <= '0';
            ram_error_reg   <= '0';

            read_request    <= '0';
            read_request_d  <= '0';
            sof_request     <= '0';
            sof_request_d   <= '0';
            error_request   <= '0';
            error_request_d <= '0';

        elsif rising_edge(clk_i) then
            -- Capture RAM output (1-cycle latency from M10K)
            ram_data_reg    <= port_a_q;
            ram_valid_reg   <= read_request_d;
            ram_channel_reg <= sof_request_d;
            ram_error_reg   <= error_request_d;

            -- Drive Avalon ST Source from pipeline registers
            aso_data_o    <= ram_data_reg;
            aso_valid_o   <= ram_valid_reg;
            aso_channel_o <= ram_channel_reg;
            aso_error_o   <= ram_error_reg;

            -- Deassert request signals by default
            read_request  <= '0';
            sof_request   <= '0';
            error_request <= '0';
            read_complete <= '0';

            -- Start reading when buffer is full
            if buffer_full = '1' and reading_active = '0' then
                reading_active <= '1';
                read_index     <= (others => '0');
                buffer_empty   <= '0';
            end if;

            -- Issue reads when active and downstream is ready
            if reading_active = '1' and aso_ready_i = '1' then
                read_request <= '1';

                if read_index = 0 then
                    sof_request <= '1';
                end if;

                if asi_error_i = '1' then
                    error_request <= '1';
                end if;

                if to_integer(read_index) + 1 >= c_TOTAL_WORDS then
                    read_index     <= (others => '0');
                    read_complete  <= '1';
                    reading_active <= '0';
                    buffer_empty   <= '1';
                else
                    read_index <= read_index + 1;
                end if;
            end if;

            -- Shift delay registers
            read_request_d  <= read_request;
            sof_request_d   <= sof_request;
            error_request_d <= error_request;

            -- SOF from write side: abort any in-progress read and reset read pointer.
            -- Placed last so it overrides any read advancement on the same clock edge.
            if write_sof = '1' then
                reading_active <= '0';
                read_index     <= (others => '0');
            end if;
        end if;
    end process;

end arch;
