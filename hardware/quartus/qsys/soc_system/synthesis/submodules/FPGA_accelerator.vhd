-- Top-level FPGA Accelerator sa selektorom operacije
-- Rewritten to use a shared dual-port RAM interface (port B)
-- The accelerator drives read/write addresses and receives/sends data
-- through a single RAM port, with internal muxing per active operation.
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

-- Globalni paket za tipove i konstante
package accel_types is
    -- Opšti parametri
    constant DATA_WIDTH    : integer := 16;
    constant FRAC_WIDTH    : integer := 8;
    constant INT_WIDTH     : integer := DATA_WIDTH - FRAC_WIDTH;

    constant MAX_BATCH_SIZE : integer := 4;

    -- Maksimalne dimenzije
    constant MAX_HEIGHT      : integer := 128;
    constant MAX_WIDTH       : integer := 128;
    constant MAX_CHANNELS    : integer := 16;

    -- Konvolucija
    constant MAX_KERNEL_SIZE : integer := 5;

    type conv_config_t is record
        kernel_size      : integer range 1 to MAX_KERNEL_SIZE;
        stride           : integer range 1 to 4;
        padding          : integer range 0 to MAX_KERNEL_SIZE/2;
        input_channels   : integer range 1 to MAX_CHANNELS;
        output_channels  : integer range 1 to MAX_CHANNELS;
    end record;

    -- Pooling
    constant MAX_POOL_SIZE : integer := 8;

    type pooling_type_t is (MAX_POOL, AVERAGE_POOL, MIN_POOL, SUM_POOL);

    type pooling_config_t is record
        pool_size : integer range 1 to MAX_POOL_SIZE;
        stride    : integer range 1 to MAX_POOL_SIZE;
        pool_type : pooling_type_t;
        channels  : integer range 1 to MAX_CHANNELS;
    end record;

    -- Aktivacije
    constant MAX_TENSOR_SIZE   : integer := 4096;
    constant SIGMOID_LUT_SIZE  : integer := 256;
    constant TANH_LUT_SIZE     : integer := 256;
    constant EXP_LUT_SIZE      : integer := 128;

    type activation_type_t is (RELU, SIGMOID, TANH, LINEAR, LEAKY_RELU, ELU);

    -- Array tipovi (koristimo std_logic_vector radi kompatibilnosti sa portovima)
    type data_array_1d is array (natural range <>) of std_logic_vector(DATA_WIDTH-1 downto 0);
    type data_array_2d is array (natural range <>, natural range <>) of std_logic_vector(DATA_WIDTH-1 downto 0);
    type data_array_3d is array (natural range <>, natural range <>, natural range <>) of std_logic_vector(DATA_WIDTH-1 downto 0);

    type data_array_t is array (natural range <>) of std_logic_vector(DATA_WIDTH-1 downto 0);
    type lut_array_t  is array (0 to 255) of std_logic_vector(DATA_WIDTH-1 downto 0);

end package accel_types;

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

use work.accel_types.all;

entity FPGA_accelerator is
    generic (
        INPUT_HEIGHT        : integer := 8;
        INPUT_WIDTH         : integer := 8;
        MAX_CHANNELS        : integer := 4;
        PARALLEL_UNITS      : integer := 4;
        -- RAM layout base addresses (in 32-bit words)
        -- HPS streams data into these regions before starting an operation.
        -- Input:   0 .. WEIGHT_BASE-1
        -- Weights: WEIGHT_BASE .. BIAS_BASE-1
        -- Bias:    BIAS_BASE .. OUTPUT_BASE-1
        -- Output:  OUTPUT_BASE .. end
        -- (One Q8.8 value per 32-bit word, in lower 16 bits)
        INPUT_BASE_ADDR     : integer := 0;
        WEIGHT_BASE_ADDR    : integer := 10000;
        BIAS_BASE_ADDR      : integer := 13000;
        OUTPUT_BASE_ADDR    : integer := 13016
    );
    port (
        clk : in std_logic;
        rst : in std_logic;

        -- Control signals
        enable : in std_logic;
        operation_select : in std_logic_vector(1 downto 0);
        start_operation : in std_logic;

        -- Configuration inputs
        conv_kernel_size     : in integer range 1 to MAX_KERNEL_SIZE;
        conv_stride          : in integer range 1 to 4;
        conv_padding         : in integer range 0 to MAX_KERNEL_SIZE/2;
        conv_input_channels  : in integer range 1 to MAX_CHANNELS;
        conv_output_channels : in integer range 1 to MAX_CHANNELS;

        pool_size    : in integer range 1 to MAX_POOL_SIZE;
        pool_stride  : in integer range 1 to MAX_POOL_SIZE;
        pool_type    : in pooling_type_t;
        pool_channels: in integer range 1 to MAX_CHANNELS;

        activation_type : in activation_type_t;
        tensor_size     : in integer range 1 to MAX_TENSOR_SIZE;
        alpha           : in std_logic_vector(DATA_WIDTH-1 downto 0);

        batch_size : in integer range 1 to MAX_BATCH_SIZE;

        -- Shared RAM Port B Interface (directly drives dual-port RAM)
        ram_rdaddress_o : out std_logic_vector(15 downto 0);
        ram_q_i         : in  std_logic_vector(31 downto 0);
        ram_wraddress_o : out std_logic_vector(15 downto 0);
        ram_data_o      : out std_logic_vector(31 downto 0);
        ram_wren_o      : out std_logic;

        -- Status signals
        operation_done    : out std_logic;
        operation_busy    : out std_logic;
        current_operation : out std_logic_vector(1 downto 0);

        -- Debug/monitoring
        debug_cycles : out std_logic_vector(31 downto 0);
        debug_status : out std_logic_vector(7 downto 0)
    );
end FPGA_accelerator;

architecture behavioral of FPGA_accelerator is

    -- Component declarations
    component convolution_block
        generic (
            INPUT_HEIGHT : integer := 8;
            INPUT_WIDTH  : integer := 8
        );
        port (
            clk            : in  std_logic;
            rst            : in  std_logic;
            start          : in  std_logic;
            done           : out std_logic;
            ready          : out std_logic;
            config         : in  conv_config_t;
            batch_size     : in  integer range 1 to MAX_BATCH_SIZE;
            input_valid    : in  std_logic;
            input_data     : in  std_logic_vector(DATA_WIDTH-1 downto 0);
            input_addr     : out std_logic_vector(15 downto 0);
            input_read_en  : out std_logic;
            weight_valid   : in  std_logic;
            weight_data    : in  std_logic_vector(DATA_WIDTH-1 downto 0);
            weight_addr    : out std_logic_vector(15 downto 0);
            weight_read_en : out std_logic;
            bias_valid     : in  std_logic;
            bias_data      : in  std_logic_vector(DATA_WIDTH-1 downto 0);
            bias_addr      : out std_logic_vector(7 downto 0);
            bias_read_en   : out std_logic;
            output_valid   : out std_logic;
            output_data    : out std_logic_vector(DATA_WIDTH-1 downto 0);
            output_addr    : out std_logic_vector(15 downto 0);
            output_write_en: out std_logic
        );
    end component;

    component pooling_block
        generic (
            INPUT_HEIGHT      : integer := 32;
            INPUT_WIDTH       : integer := 32;
            PARALLEL_CHANNELS : integer := 4
        );
        port (
            clk              : in  std_logic;
            rst              : in  std_logic;
            start            : in  std_logic;
            done             : out std_logic;
            ready            : out std_logic;
            busy             : out std_logic;
            config           : in  pooling_config_t;
            batch_size       : in  integer range 1 to MAX_BATCH_SIZE;
            input_valid      : in  std_logic;
            input_data       : in  data_array_1d(0 to PARALLEL_CHANNELS-1);
            input_addr       : out std_logic_vector(17 downto 0);
            input_read_en    : out std_logic;
            output_valid     : out std_logic;
            output_data      : out data_array_1d(0 to PARALLEL_CHANNELS-1);
            output_addr      : out std_logic_vector(17 downto 0);
            output_write_en  : out std_logic;
            current_position : out std_logic_vector(31 downto 0);
            processing_cycles: out std_logic_vector(31 downto 0);
            pool_window_count: out std_logic_vector(15 downto 0)
        );
    end component;

    component activation_block
        generic (
            PIPELINE_STAGES : integer := 3;
            PARALLEL_UNITS  : integer := 4
        );
        port (
            clk              : in  std_logic;
            rst              : in  std_logic;
            start            : in  std_logic;
            done             : out std_logic;
            ready            : out std_logic;
            busy             : out std_logic;
            activation_type  : in  activation_type_t;
            tensor_size      : in  integer range 1 to MAX_TENSOR_SIZE;
            alpha            : in  std_logic_vector(DATA_WIDTH-1 downto 0);
            input_valid      : in  std_logic;
            input_data       : in  data_array_t(0 to PARALLEL_UNITS-1);
            input_addr       : out std_logic_vector(15 downto 0);
            input_read_en    : out std_logic;
            output_valid     : out std_logic;
            output_data      : out data_array_t(0 to PARALLEL_UNITS-1);
            output_addr      : out std_logic_vector(15 downto 0);
            output_write_en  : out std_logic;
            current_element  : out std_logic_vector(15 downto 0);
            processing_cycles: out std_logic_vector(31 downto 0)
        );
    end component;

    -- Internal config records
    signal conv_config    : conv_config_t;
    signal pooling_config : pooling_config_t;

    -- Control signals per block
    signal conv_start, conv_done, conv_ready : std_logic;
    signal pool_start, pool_done, pool_ready, pool_busy : std_logic;
    signal act_start,  act_done,  act_ready,  act_busy  : std_logic;

    -- Convolution block wires
    signal conv_input_valid   : std_logic;
    signal conv_input_data    : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal conv_weight_valid  : std_logic;
    signal conv_weight_data   : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal conv_bias_valid    : std_logic;
    signal conv_bias_data     : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal conv_output_valid  : std_logic;
    signal conv_output_data   : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal conv_input_addr    : std_logic_vector(15 downto 0);
    signal conv_weight_addr   : std_logic_vector(15 downto 0);
    signal conv_output_addr   : std_logic_vector(15 downto 0);
    signal conv_bias_addr     : std_logic_vector(7 downto 0);
    signal conv_input_read_en : std_logic;
    signal conv_weight_read_en: std_logic;
    signal conv_bias_read_en  : std_logic;
    signal conv_output_write_en : std_logic;

    -- Pooling block wires
    signal pool_input_valid   : std_logic;
    signal pool_input_data    : data_array_1d(0 to PARALLEL_UNITS-1);
    signal pool_output_valid  : std_logic;
    signal pool_output_data   : data_array_1d(0 to PARALLEL_UNITS-1);
    signal pool_input_addr    : std_logic_vector(17 downto 0);
    signal pool_output_addr   : std_logic_vector(17 downto 0);
    signal pool_input_read_en : std_logic;
    signal pool_output_write_en : std_logic;
    signal pool_current_position : std_logic_vector(31 downto 0);
    signal pool_cycles        : std_logic_vector(31 downto 0);
    signal pool_window_count  : std_logic_vector(15 downto 0);

    -- Activation block wires
    signal act_input_valid    : std_logic;
    signal act_input_data     : data_array_t(0 to PARALLEL_UNITS-1);
    signal act_output_valid   : std_logic;
    signal act_output_data    : data_array_t(0 to PARALLEL_UNITS-1);
    signal act_input_addr     : std_logic_vector(15 downto 0);
    signal act_output_addr    : std_logic_vector(15 downto 0);
    signal act_input_read_en  : std_logic;
    signal act_output_write_en: std_logic;
    signal act_current_element: std_logic_vector(15 downto 0);
    signal act_cycles         : std_logic_vector(31 downto 0);

    -- Top-level state machine
    type top_state_t is (IDLE, CONV_OP, POOL_OP, ACT_OP, DONE_ST);
    signal current_state, next_state : top_state_t;

    signal selected_operation : std_logic_vector(1 downto 0);
    signal cycle_counter      : unsigned(31 downto 0);

    -- Sticky done flag: latched high when operation completes, cleared on new start
    signal done_latch : std_logic;

    -- Convolution RAM read sequencer
    -- Conv needs input + weight + (optionally) bias per MAC iteration.
    -- We time-multiplex the single read port across 3 phases.
    type conv_read_phase_t is (PHASE_INPUT, PHASE_WEIGHT, PHASE_BIAS, PHASE_DONE);
    signal conv_read_phase    : conv_read_phase_t;
    signal conv_read_phase_d1 : conv_read_phase_t;
    signal conv_data_ready    : std_logic;  -- pulses when all reads captured
    signal conv_need_bias     : std_logic;

    -- Registered RAM read data per conv channel
    signal conv_input_data_reg  : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal conv_weight_data_reg : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal conv_bias_data_reg   : std_logic_vector(DATA_WIDTH-1 downto 0);

    -- Pooling/Activation RAM read pipeline (1 cycle latency)
    signal pool_read_pending : std_logic;
    signal act_read_pending  : std_logic;

    -- 16-bit extract from 32-bit RAM word
    signal ram_q_16 : std_logic_vector(DATA_WIDTH-1 downto 0);

begin

    ram_q_16 <= ram_q_i(DATA_WIDTH-1 downto 0);

    -- =========================================================================
    -- Configuration record setup
    -- =========================================================================
    conv_config.kernel_size     <= conv_kernel_size;
    conv_config.stride          <= conv_stride;
    conv_config.padding         <= conv_padding;
    conv_config.input_channels  <= conv_input_channels;
    conv_config.output_channels <= conv_output_channels;

    pooling_config.pool_size <= pool_size;
    pooling_config.stride    <= pool_stride;
    pooling_config.pool_type <= pool_type;
    pooling_config.channels  <= pool_channels;

    -- =========================================================================
    -- Component instantiations
    -- =========================================================================
    conv_inst: convolution_block
        generic map (
            INPUT_HEIGHT => INPUT_HEIGHT,
            INPUT_WIDTH  => INPUT_WIDTH
        )
        port map (
            clk            => clk,
            rst            => rst,
            start          => conv_start,
            done           => conv_done,
            ready          => conv_ready,
            config         => conv_config,
            batch_size     => batch_size,
            input_valid    => conv_input_valid,
            input_data     => conv_input_data,
            input_addr     => conv_input_addr,
            input_read_en  => conv_input_read_en,
            weight_valid   => conv_weight_valid,
            weight_data    => conv_weight_data,
            weight_addr    => conv_weight_addr,
            weight_read_en => conv_weight_read_en,
            bias_valid     => conv_bias_valid,
            bias_data      => conv_bias_data,
            bias_addr      => conv_bias_addr,
            bias_read_en   => conv_bias_read_en,
            output_valid   => conv_output_valid,
            output_data    => conv_output_data,
            output_addr    => conv_output_addr,
            output_write_en=> conv_output_write_en
        );

    pool_inst: pooling_block
        generic map (
            INPUT_HEIGHT      => INPUT_HEIGHT,
            INPUT_WIDTH       => INPUT_WIDTH,
            PARALLEL_CHANNELS => PARALLEL_UNITS
        )
        port map (
            clk              => clk,
            rst              => rst,
            start            => pool_start,
            done             => pool_done,
            ready            => pool_ready,
            busy             => pool_busy,
            config           => pooling_config,
            batch_size       => batch_size,
            input_valid      => pool_input_valid,
            input_data       => pool_input_data,
            input_addr       => pool_input_addr,
            input_read_en    => pool_input_read_en,
            output_valid     => pool_output_valid,
            output_data      => pool_output_data,
            output_addr      => pool_output_addr,
            output_write_en  => pool_output_write_en,
            current_position => pool_current_position,
            processing_cycles=> pool_cycles,
            pool_window_count=> pool_window_count
        );

    act_inst: activation_block
        generic map (
            PIPELINE_STAGES => 3,
            PARALLEL_UNITS  => PARALLEL_UNITS
        )
        port map (
            clk              => clk,
            rst              => rst,
            start            => act_start,
            done             => act_done,
            ready            => act_ready,
            busy             => act_busy,
            activation_type  => activation_type,
            tensor_size      => tensor_size,
            alpha            => alpha,
            input_valid      => act_input_valid,
            input_data       => act_input_data,
            input_addr       => act_input_addr,
            input_read_en    => act_input_read_en,
            output_valid     => act_output_valid,
            output_data      => act_output_data,
            output_addr      => act_output_addr,
            output_write_en  => act_output_write_en,
            current_element  => act_current_element,
            processing_cycles=> act_cycles
        );

    -- =========================================================================
    -- Top-level state machine
    -- =========================================================================
    process(clk, rst)
    begin
        if rst = '1' then
            current_state      <= IDLE;
            selected_operation <= "00";
            cycle_counter      <= (others => '0');
            done_latch         <= '0';
        elsif rising_edge(clk) then
            current_state <= next_state;

            if current_state /= IDLE and current_state /= DONE_ST then
                cycle_counter <= cycle_counter + 1;
            end if;

            if current_state = IDLE and enable = '1' and start_operation = '1' then
                selected_operation <= operation_select;
                cycle_counter      <= (others => '0');
                done_latch         <= '0';  -- Clear done on new operation start
            end if;

            -- Latch done when entering DONE_ST
            if next_state = DONE_ST and current_state /= DONE_ST then
                done_latch <= '1';
            end if;
        end if;
    end process;

    process(current_state, enable, start_operation, operation_select,
            conv_done, pool_done, act_done)
    begin
        next_state <= current_state;
        case current_state is
            when IDLE =>
                if enable = '1' and start_operation = '1' then
                    case operation_select is
                        when "00"   => next_state <= CONV_OP;
                        when "01"   => next_state <= POOL_OP;
                        when "10"   => next_state <= ACT_OP;
                        when others => next_state <= IDLE;
                    end case;
                end if;
            when CONV_OP =>
                if conv_done = '1' then next_state <= DONE_ST; end if;
            when POOL_OP =>
                if pool_done = '1' then next_state <= DONE_ST; end if;
            when ACT_OP =>
                if act_done = '1' then next_state <= DONE_ST; end if;
            when DONE_ST =>
                next_state <= IDLE;
            when others =>
                next_state <= IDLE;
        end case;
    end process;

    -- Start pulses: fire once on state entry
    conv_start <= '1' when current_state = IDLE and next_state = CONV_OP else '0';
    pool_start <= '1' when current_state = IDLE and next_state = POOL_OP else '0';
    act_start  <= '1' when current_state = IDLE and next_state = ACT_OP  else '0';

    -- =========================================================================
    -- Convolution RAM read sequencer
    -- Cycles through PHASE_INPUT -> PHASE_WEIGHT -> (PHASE_BIAS) -> PHASE_DONE
    -- Each phase presents an address; data arrives 1 clk later (captured in d1).
    -- =========================================================================
    conv_need_bias <= conv_bias_read_en;

    process(clk, rst)
    begin
        if rst = '1' then
            conv_read_phase    <= PHASE_DONE;
            conv_read_phase_d1 <= PHASE_DONE;
        elsif rising_edge(clk) then
            conv_read_phase_d1 <= conv_read_phase;

            if current_state = CONV_OP then
                case conv_read_phase is
                    when PHASE_DONE =>
                        -- Start a new read cycle when the conv block is requesting
                        if conv_input_read_en = '1' then
                            conv_read_phase <= PHASE_INPUT;
                        end if;
                    when PHASE_INPUT =>
                        conv_read_phase <= PHASE_WEIGHT;
                    when PHASE_WEIGHT =>
                        if conv_need_bias = '1' then
                            conv_read_phase <= PHASE_BIAS;
                        else
                            conv_read_phase <= PHASE_DONE;
                        end if;
                    when PHASE_BIAS =>
                        conv_read_phase <= PHASE_DONE;
                end case;
            else
                conv_read_phase    <= PHASE_DONE;
                conv_read_phase_d1 <= PHASE_DONE;
            end if;
        end if;
    end process;

    -- Capture returned RAM data into the correct register (delayed by 1 clk)
    process(clk, rst)
    begin
        if rst = '1' then
            conv_input_data_reg  <= (others => '0');
            conv_weight_data_reg <= (others => '0');
            conv_bias_data_reg   <= (others => '0');
            conv_data_ready      <= '0';
        elsif rising_edge(clk) then
            conv_data_ready <= '0';

            case conv_read_phase_d1 is
                when PHASE_INPUT =>
                    conv_input_data_reg <= ram_q_16;
                when PHASE_WEIGHT =>
                    conv_weight_data_reg <= ram_q_16;
                    -- If no bias was needed, data is ready now
                    if conv_read_phase = PHASE_DONE then
                        conv_data_ready <= '1';
                    end if;
                when PHASE_BIAS =>
                    conv_bias_data_reg <= ram_q_16;
                    conv_data_ready <= '1';
                when PHASE_DONE =>
                    null;
            end case;
        end if;
    end process;

    -- Feed registered data to convolution block
    conv_input_data  <= conv_input_data_reg;
    conv_weight_data <= conv_weight_data_reg;
    conv_bias_data   <= conv_bias_data_reg;
    conv_input_valid  <= conv_data_ready;
    conv_weight_valid <= conv_data_ready;
    conv_bias_valid   <= conv_data_ready and conv_need_bias;

    -- =========================================================================
    -- Pooling RAM data pipeline (1 cycle read latency)
    -- =========================================================================
    process(clk, rst)
    begin
        if rst = '1' then
            pool_read_pending <= '0';
            pool_input_valid  <= '0';
            for i in 0 to PARALLEL_UNITS-1 loop
                pool_input_data(i) <= (others => '0');
            end loop;
        elsif rising_edge(clk) then
            pool_input_valid  <= '0';
            pool_read_pending <= '0';

            if current_state = POOL_OP and pool_input_read_en = '1' then
                pool_read_pending <= '1';
            end if;

            if pool_read_pending = '1' then
                pool_input_valid <= '1';
                pool_input_data(0) <= ram_q_i(DATA_WIDTH-1 downto 0);
                if PARALLEL_UNITS > 1 then
                    pool_input_data(1) <= ram_q_i(2*DATA_WIDTH-1 downto DATA_WIDTH);
                end if;
                for i in 2 to PARALLEL_UNITS-1 loop
                    pool_input_data(i) <= (others => '0');
                end loop;
            end if;
        end if;
    end process;

    -- =========================================================================
    -- Activation RAM data pipeline (1 cycle read latency)
    -- =========================================================================
    process(clk, rst)
    begin
        if rst = '1' then
            act_read_pending <= '0';
            act_input_valid  <= '0';
            for i in 0 to PARALLEL_UNITS-1 loop
                act_input_data(i) <= (others => '0');
            end loop;
        elsif rising_edge(clk) then
            act_input_valid  <= '0';
            act_read_pending <= '0';

            if current_state = ACT_OP and act_input_read_en = '1' then
                act_read_pending <= '1';
            end if;

            if act_read_pending = '1' then
                act_input_valid <= '1';
                act_input_data(0) <= ram_q_i(DATA_WIDTH-1 downto 0);
                if PARALLEL_UNITS > 1 then
                    act_input_data(1) <= ram_q_i(2*DATA_WIDTH-1 downto DATA_WIDTH);
                end if;
                for i in 2 to PARALLEL_UNITS-1 loop
                    act_input_data(i) <= (others => '0');
                end loop;
            end if;
        end if;
    end process;

    -- =========================================================================
    -- RAM Address/Data Mux: drive port B read and write
    -- =========================================================================
    process(current_state,
            conv_read_phase, conv_input_addr, conv_weight_addr, conv_bias_addr,
            conv_input_read_en,
            conv_output_write_en, conv_output_addr, conv_output_data,
            pool_input_addr, pool_input_read_en,
            pool_output_write_en, pool_output_addr, pool_output_data,
            act_input_addr, act_input_read_en,
            act_output_write_en, act_output_addr, act_output_data)
    begin
        -- Defaults: no read, no write
        ram_rdaddress_o <= (others => '0');
        ram_wraddress_o <= (others => '0');
        ram_data_o      <= (others => '0');
        ram_wren_o      <= '0';

        case current_state is
            -- =============================================================
            when CONV_OP =>
                -- Read address mux based on sequencer phase
                case conv_read_phase is
                    when PHASE_INPUT =>
                        ram_rdaddress_o <= std_logic_vector(
                            unsigned(conv_input_addr) + to_unsigned(INPUT_BASE_ADDR, 16));
                    when PHASE_WEIGHT =>
                        ram_rdaddress_o <= std_logic_vector(
                            unsigned(conv_weight_addr) + to_unsigned(WEIGHT_BASE_ADDR, 16));
                    when PHASE_BIAS =>
                        ram_rdaddress_o <= std_logic_vector(
                            resize(unsigned(conv_bias_addr), 16) + to_unsigned(BIAS_BASE_ADDR, 16));
                    when others =>
                        null;
                end case;

                -- Write: conv output result
                if conv_output_write_en = '1' then
                    ram_wren_o <= '1';
                    ram_wraddress_o <= std_logic_vector(
                        unsigned(conv_output_addr) + to_unsigned(OUTPUT_BASE_ADDR, 16));
                    ram_data_o(DATA_WIDTH-1 downto 0)  <= conv_output_data;
                    ram_data_o(31 downto DATA_WIDTH)    <= (others => '0');
                end if;

            -- =============================================================
            when POOL_OP =>
                if pool_input_read_en = '1' then
                    ram_rdaddress_o <= std_logic_vector(
                        resize(unsigned(pool_input_addr), 16) + to_unsigned(INPUT_BASE_ADDR, 16));
                end if;

                if pool_output_write_en = '1' then
                    ram_wren_o <= '1';
                    ram_wraddress_o <= std_logic_vector(
                        resize(unsigned(pool_output_addr), 16) + to_unsigned(OUTPUT_BASE_ADDR, 16));
                    ram_data_o(DATA_WIDTH-1 downto 0) <= pool_output_data(0);
                    if PARALLEL_UNITS > 1 then
                        ram_data_o(2*DATA_WIDTH-1 downto DATA_WIDTH) <= pool_output_data(1);
                    end if;
                end if;

            -- =============================================================
            when ACT_OP =>
                if act_input_read_en = '1' then
                    ram_rdaddress_o <= std_logic_vector(
                        unsigned(act_input_addr) + to_unsigned(INPUT_BASE_ADDR, 16));
                end if;

                if act_output_write_en = '1' then
                    ram_wren_o <= '1';
                    ram_wraddress_o <= std_logic_vector(
                        unsigned(act_output_addr) + to_unsigned(OUTPUT_BASE_ADDR, 16));
                    ram_data_o(DATA_WIDTH-1 downto 0) <= act_output_data(0);
                    if PARALLEL_UNITS > 1 then
                        ram_data_o(2*DATA_WIDTH-1 downto DATA_WIDTH) <= act_output_data(1);
                    end if;
                end if;

            -- =============================================================
            when others =>
                null;
        end case;
    end process;

    -- =========================================================================
    -- Status & Debug
    -- =========================================================================
    -- Use sticky done_latch so software can observe it (the raw conv_done/
    -- pool_done/act_done signals are only 1-clock pulses)
    operation_done <= done_latch;

    operation_busy <= '1' when current_state /= IDLE and current_state /= DONE_ST else '0';
    current_operation <= selected_operation;

    debug_cycles <= std_logic_vector(cycle_counter);
    debug_status <= "000000" & selected_operation;

end behavioral;
