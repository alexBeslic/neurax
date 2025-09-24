-- Top-level FPGA Accelerator sa selektorom operacije
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
    constant MAX_HEIGHT      : integer := 64;
    constant MAX_WIDTH       : integer := 64;
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
        PARALLEL_UNITS      : integer := 4
    );
    port (
        clk : in std_logic;
        rst : in std_logic;
        
        -- Control signals
        enable : in std_logic;
        operation_select : in std_logic_vector(1 downto 0); -- 00=Conv, 01=Pool, 10=Activation, 11=Reserved
        start_operation : in std_logic;
        
        -- Configuration inputs
        -- Convolution config
        conv_kernel_size : in integer range 1 to MAX_KERNEL_SIZE;
        conv_stride : in integer range 1 to 4;
        conv_padding : in integer range 0 to MAX_KERNEL_SIZE/2;
        conv_input_channels : in integer range 1 to MAX_CHANNELS;
        conv_output_channels : in integer range 1 to MAX_CHANNELS;
        
        -- Pooling config
        pool_size : in integer range 1 to MAX_POOL_SIZE;
        pool_stride : in integer range 1 to MAX_POOL_SIZE;
        pool_type : in pooling_type_t;
        pool_channels : in integer range 1 to MAX_CHANNELS;
        
        -- Activation config
        activation_type : in activation_type_t;
        tensor_size : in integer range 1 to MAX_TENSOR_SIZE;
        alpha : in std_logic_vector(DATA_WIDTH-1 downto 0);
        
        -- Common config
        batch_size : in integer range 1 to MAX_BATCH_SIZE;
        
        -- Data interfaces
        input_valid : in std_logic;
        input_data : in std_logic_vector((PARALLEL_UNITS*DATA_WIDTH)-1 downto 0);
        input_ready : out std_logic;
        
        -- Weight interface (samo za convolution)
        weight_valid : in std_logic;
        weight_data : in std_logic_vector(DATA_WIDTH-1 downto 0);
        weight_ready : out std_logic;
        
        -- Bias interface (samo za convolution)
        bias_valid : in std_logic;
        bias_data : in std_logic_vector(DATA_WIDTH-1 downto 0);
        bias_ready : out std_logic;
        
        -- Output interface
        output_valid : out std_logic;
        output_data : out std_logic_vector((PARALLEL_UNITS*DATA_WIDTH)-1 downto 0);
        output_ready : in std_logic;
        
        -- Status signals
        operation_done : out std_logic;
        operation_busy : out std_logic;
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
            INPUT_WIDTH : integer := 8
        );
        port (
            clk : in std_logic;
            rst : in std_logic;
            start : in std_logic;
            done : out std_logic;
            ready : out std_logic;
            config : in conv_config_t;
            batch_size : in integer range 1 to MAX_BATCH_SIZE;
            input_valid : in std_logic;
            input_data : in std_logic_vector(DATA_WIDTH-1 downto 0);
            input_addr : out std_logic_vector(15 downto 0);
            input_read_en : out std_logic;
            weight_valid : in std_logic;
            weight_data : in std_logic_vector(DATA_WIDTH-1 downto 0);
            weight_addr : out std_logic_vector(15 downto 0);
            weight_read_en : out std_logic;
            bias_valid : in std_logic;
            bias_data : in std_logic_vector(DATA_WIDTH-1 downto 0);
            bias_addr : out std_logic_vector(7 downto 0);
            bias_read_en : out std_logic;
            output_valid : out std_logic;
            output_data : out std_logic_vector(DATA_WIDTH-1 downto 0);
            output_addr : out std_logic_vector(15 downto 0);
            output_write_en : out std_logic
        );
    end component;
    
    component pooling_block
        generic (
            INPUT_HEIGHT : integer := 32;
            INPUT_WIDTH : integer := 32;
            PARALLEL_CHANNELS : integer := 4
        );
        port (
            clk : in std_logic;
            rst : in std_logic;
            start : in std_logic;
            done : out std_logic;
            ready : out std_logic;
            busy : out std_logic;
            config : in pooling_config_t;
            batch_size : in integer range 1 to MAX_BATCH_SIZE;
            input_valid : in std_logic;
            input_data : in data_array_1d(0 to PARALLEL_CHANNELS-1);
            input_addr : out std_logic_vector(17 downto 0);
            input_read_en : out std_logic;
            output_valid : out std_logic;
            output_data : out data_array_1d(0 to PARALLEL_CHANNELS-1);
            output_addr : out std_logic_vector(17 downto 0);
            output_write_en : out std_logic;
            current_position : out std_logic_vector(31 downto 0);
            processing_cycles : out std_logic_vector(31 downto 0);
            pool_window_count : out std_logic_vector(15 downto 0)
        );
    end component;
    
    component activation_block
        generic (
            PIPELINE_STAGES : integer := 3;
            PARALLEL_UNITS : integer := 4
        );
        port (
            clk : in std_logic;
            rst : in std_logic;
            start : in std_logic;
            done : out std_logic;
            ready : out std_logic;
            busy : out std_logic;
            activation_type : in activation_type_t;
            tensor_size : in integer range 1 to MAX_TENSOR_SIZE;
            alpha : in std_logic_vector(DATA_WIDTH-1 downto 0);
            input_valid : in std_logic;
            input_data : in data_array_t(0 to PARALLEL_UNITS-1);
            input_addr : out std_logic_vector(15 downto 0);
            input_read_en : out std_logic;
            output_valid : out std_logic;
            output_data : out data_array_t(0 to PARALLEL_UNITS-1);
            output_addr : out std_logic_vector(15 downto 0);
            output_write_en : out std_logic;
            current_element : out std_logic_vector(15 downto 0);
            processing_cycles : out std_logic_vector(31 downto 0)
        );
    end component;
    
    -- Internal signals
    signal conv_config : conv_config_t;
    signal pooling_config : pooling_config_t;
    
    -- Control signals for each block
    signal conv_start, conv_done, conv_ready : std_logic;
    signal pool_start, pool_done, pool_ready, pool_busy : std_logic;
    signal act_start, act_done, act_ready, act_busy : std_logic;
    
    -- Data routing signals (ispravno tipizirani)
    signal conv_input_valid  : std_logic;
    signal conv_input_data   : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal conv_output_valid : std_logic;
    signal conv_output_data  : std_logic_vector(DATA_WIDTH-1 downto 0);
    
    signal pool_input_valid  : std_logic;
    signal pool_input_data   : data_array_1d(0 to PARALLEL_UNITS-1);
    signal pool_output_valid : std_logic;
    signal pool_output_data  : data_array_1d(0 to PARALLEL_UNITS-1);
    
    signal act_input_valid  : std_logic;
    signal act_input_data   : data_array_t(0 to PARALLEL_UNITS-1);
    signal act_output_valid : std_logic;
    signal act_output_data  : data_array_t(0 to PARALLEL_UNITS-1);
    
    -- Address and control signals
    signal conv_input_addr, conv_weight_addr, conv_output_addr : std_logic_vector(15 downto 0);
    signal conv_bias_addr : std_logic_vector(7 downto 0);
    signal conv_input_read_en, conv_weight_read_en, conv_bias_read_en, conv_output_write_en : std_logic;
    
    signal pool_input_addr, pool_output_addr : std_logic_vector(17 downto 0);
    signal pool_input_read_en, pool_output_write_en : std_logic;
    signal pool_current_position, pool_cycles : std_logic_vector(31 downto 0);
    signal pool_window_count : std_logic_vector(15 downto 0);
    
    signal act_input_addr, act_output_addr : std_logic_vector(15 downto 0);
    signal act_input_read_en, act_output_write_en : std_logic;
    signal act_current_element : std_logic_vector(15 downto 0);
    signal act_cycles : std_logic_vector(31 downto 0);
    
    -- State machine za top-level control
    type top_state_t is (IDLE, CONV_OP, POOL_OP, ACT_OP, DONE_ST);
    signal current_state, next_state : top_state_t;
    
    -- Operation tracking
    signal selected_operation : std_logic_vector(1 downto 0);
    signal cycle_counter : unsigned(31 downto 0);

begin
    
    -- Configuration setup
    conv_config.kernel_size <= conv_kernel_size;
    conv_config.stride <= conv_stride;
    conv_config.padding <= conv_padding;
    conv_config.input_channels <= conv_input_channels;
    conv_config.output_channels <= conv_output_channels;
    
    pooling_config.pool_size <= pool_size;
    pooling_config.stride <= pool_stride;
    pooling_config.pool_type <= pool_type;
    pooling_config.channels <= pool_channels;
    
    -- Component instantiations
    conv_inst: convolution_block
        generic map (
            INPUT_HEIGHT => INPUT_HEIGHT,
            INPUT_WIDTH => INPUT_WIDTH
        )
        port map (
            clk => clk,
            rst => rst,
            start => conv_start,
            done => conv_done,
            ready => conv_ready,
            config => conv_config,
            batch_size => batch_size,
            input_valid => conv_input_valid,
            input_data => conv_input_data,
            input_addr => conv_input_addr,
            input_read_en => conv_input_read_en,
            weight_valid => weight_valid,
            weight_data => weight_data,
            weight_addr => conv_weight_addr,
            weight_read_en => conv_weight_read_en,
            bias_valid => bias_valid,
            bias_data => bias_data,
            bias_addr => conv_bias_addr,
            bias_read_en => conv_bias_read_en,
            output_valid => conv_output_valid,
            output_data => conv_output_data,
            output_addr => conv_output_addr,
            output_write_en => conv_output_write_en
        );
    
    pool_inst: pooling_block
        generic map (
            INPUT_HEIGHT => INPUT_HEIGHT,
            INPUT_WIDTH => INPUT_WIDTH,
            PARALLEL_CHANNELS => PARALLEL_UNITS
        )
        port map (
            clk => clk,
            rst => rst,
            start => pool_start,
            done => pool_done,
            ready => pool_ready,
            busy => pool_busy,
            config => pooling_config,
            batch_size => batch_size,
            input_valid => pool_input_valid,
            input_data => pool_input_data,
            input_addr => pool_input_addr,
            input_read_en => pool_input_read_en,
            output_valid => pool_output_valid,
            output_data => pool_output_data,
            output_addr => pool_output_addr,
            output_write_en => pool_output_write_en,
            current_position => pool_current_position,
            processing_cycles => pool_cycles,
            pool_window_count => pool_window_count
        );
    
    act_inst: activation_block
        generic map (
            PIPELINE_STAGES => 3,
            PARALLEL_UNITS => PARALLEL_UNITS
        )
        port map (
            clk => clk,
            rst => rst,
            start => act_start,
            done => act_done,
            ready => act_ready,
            busy => act_busy,
            activation_type => activation_type,
            tensor_size => tensor_size,
            alpha => alpha,
            input_valid => act_input_valid,
            input_data => act_input_data,
            input_addr => act_input_addr,
            input_read_en => act_input_read_en,
            output_valid => act_output_valid,
            output_data => act_output_data,
            output_addr => act_output_addr,
            output_write_en => act_output_write_en,
            current_element => act_current_element,
            processing_cycles => act_cycles
        );
    
    -- Top-level state machine
    process(clk, rst)
    begin
        if rst = '1' then
            current_state <= IDLE;
            selected_operation <= "00";
            cycle_counter <= (others => '0');
        elsif rising_edge(clk) then
            current_state <= next_state;
            cycle_counter <= cycle_counter + 1;
            
            if current_state = IDLE and enable = '1' and start_operation = '1' then
                selected_operation <= operation_select;
            end if;
        end if;
    end process;
    
    -- Next state logic
    process(current_state, enable, start_operation, operation_select,
            conv_done, pool_done, act_done, conv_ready, pool_ready, act_ready)
    begin
        next_state <= current_state;
        
        case current_state is
            when IDLE =>
                if enable = '1' and start_operation = '1' then
                    case operation_select is
                        when "00" => next_state <= CONV_OP;
                        when "01" => next_state <= POOL_OP;
                        when "10" => next_state <= ACT_OP;
                        when others => next_state <= IDLE;
                    end case;
                end if;
                
            when CONV_OP =>
                if conv_done = '1' then
                    next_state <= DONE_ST;
                end if;
                
            when POOL_OP =>
                if pool_done = '1' then
                    next_state <= DONE_ST;
                end if;
                
            when ACT_OP =>
                if act_done = '1' then
                    next_state <= DONE_ST;
                end if;
                
            when DONE_ST =>
                next_state <= IDLE;
                
            when others =>
                next_state <= IDLE;
        end case;
    end process;
    
    -- Control signal routing
    conv_start <= '1' when current_state = CONV_OP and selected_operation = "00" else '0';
    pool_start <= '1' when current_state = POOL_OP and selected_operation = "01" else '0';
    act_start <= '1' when current_state = ACT_OP and selected_operation = "10" else '0';
    
    -- Input data routing
    process(selected_operation, input_valid, input_data)
		begin
			 -- Default assignments
			 conv_input_valid <= '0';
			 pool_input_valid <= '0';
			 act_input_valid  <= '0';
			 
			 conv_input_data <= (others => '0');
			 for i in 0 to PARALLEL_UNITS-1 loop
				  pool_input_data(i) <= (others => '0');
				  act_input_data(i)  <= (others => '0');
			 end loop;
			 
			 case selected_operation is
				  when "00" => -- Convolution
						conv_input_valid <= input_valid;
						conv_input_data  <= input_data(DATA_WIDTH-1 downto 0);
						
				  when "01" => -- Pooling
						pool_input_valid <= input_valid;
						for i in 0 to PARALLEL_UNITS-1 loop
							 pool_input_data(i) <= input_data((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH);
						end loop;
						
				  when "10" => -- Activation
						act_input_valid <= input_valid;
						for i in 0 to PARALLEL_UNITS-1 loop
							 act_input_data(i) <= input_data((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH);
						end loop;
						
				  when others =>
						null;
			 end case;
		end process;
    
    -- Output data routing
    process(selected_operation, conv_output_valid, conv_output_data,
            pool_output_valid, pool_output_data, act_output_valid, act_output_data)
    begin
        -- Default outputs
        output_valid <= '0';
        output_data <= (others => '0');
        
        case selected_operation is
            when "00" => -- Convolution
                output_valid <= conv_output_valid;
                output_data(DATA_WIDTH-1 downto 0) <= conv_output_data;
                
            when "01" => -- Pooling
                output_valid <= pool_output_valid;
                for i in 0 to PARALLEL_UNITS-1 loop
                    output_data((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH) <= pool_output_data(i);
                end loop;
                
            when "10" => -- Activation
                output_valid <= act_output_valid;
                for i in 0 to PARALLEL_UNITS-1 loop
                    output_data((i+1)*DATA_WIDTH-1 downto i*DATA_WIDTH) <= act_output_data(i);
                end loop;
                
            when others =>
                null;
        end case;
    end process;
    
    -- Status and ready signals
    input_ready <= conv_ready when selected_operation = "00" else
                   pool_ready when selected_operation = "01" else
                   act_ready when selected_operation = "10" else
                   '1';
    
    weight_ready <= '1' when selected_operation = "00" else '0';
    bias_ready <= '1' when selected_operation = "00" else '0';
    
    operation_done <= conv_done when selected_operation = "00" else
                      pool_done when selected_operation = "01" else
                      act_done when selected_operation = "10" else
                      '0';
    
    operation_busy <= '1' when current_state /= IDLE and current_state /= DONE_ST else '0';
    
    current_operation <= selected_operation;
    
    -- Debug outputs
    debug_cycles <= std_logic_vector(cycle_counter);
    debug_status <= "000000" & selected_operation;

end behavioral;
