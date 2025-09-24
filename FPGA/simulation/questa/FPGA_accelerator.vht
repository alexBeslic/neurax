-- Testbench za FPGA Accelerator Top Module
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use work.conv_types.all;
use work.pooling_types.all;
use work.activation_types.all;

entity FPGA_accelerator_tb is
end FPGA_accelerator_tb;

architecture testbench of FPGA_accelerator_tb is
    
    -- Konstante
    constant CLK_PERIOD : time := 10 ns;
    constant INPUT_HEIGHT_TB : integer := 8;
    constant INPUT_WIDTH_TB : integer := 8;
    constant MAX_CHANNELS_TB : integer := 4;
    constant DATA_WIDTH_TB : integer := 16;
    constant PARALLEL_UNITS_TB : integer := 4;
    
    -- Component declaration
    component FPGA_accelerator
        generic (
            INPUT_HEIGHT        : integer := 8;
            INPUT_WIDTH         : integer := 8;
            MAX_CHANNELS        : integer := 4;
            DATA_WIDTH          : integer := 16;
            PARALLEL_UNITS      : integer := 4
        );
        port (
            clk : in std_logic;
            rst : in std_logic;
            enable : in std_logic;
            operation_select : in std_logic_vector(1 downto 0);
            start_operation : in std_logic;
            
            -- Configuration inputs
            conv_kernel_size : in integer range 1 to MAX_KERNEL_SIZE;
            conv_stride : in integer range 1 to 4;
            conv_padding : in integer range 0 to MAX_KERNEL_SIZE/2;
            conv_input_channels : in integer range 1 to MAX_CHANNELS_TB;
            conv_output_channels : in integer range 1 to MAX_CHANNELS_TB;
            
            pool_size : in integer range 1 to MAX_POOL_SIZE;
            pool_stride : in integer range 1 to MAX_POOL_SIZE;
            pool_type : in pooling_type_t;
            pool_channels : in integer range 1 to MAX_CHANNELS_TB;
            
            activation_type : in activation_type_t;
            tensor_size : in integer range 1 to MAX_TENSOR_SIZE;
            alpha : in std_logic_vector(DATA_WIDTH_TB-1 downto 0);
            
            batch_size : in integer range 1 to MAX_BATCH_SIZE;
            
            input_valid : in std_logic;
            input_data : in std_logic_vector((PARALLEL_UNITS_TB*DATA_WIDTH_TB)-1 downto 0);
            input_ready : out std_logic;
            
            weight_valid : in std_logic;
            weight_data : in std_logic_vector(DATA_WIDTH_TB-1 downto 0);
            weight_ready : out std_logic;
            
            bias_valid : in std_logic;
            bias_data : in std_logic_vector(DATA_WIDTH_TB-1 downto 0);
            bias_ready : out std_logic;
            
            output_valid : out std_logic;
            output_data : out std_logic_vector((PARALLEL_UNITS_TB*DATA_WIDTH_TB)-1 downto 0);
            output_ready : in std_logic;
            
            operation_done : out std_logic;
            operation_busy : out std_logic;
            current_operation : out std_logic_vector(1 downto 0);
            
            debug_cycles : out std_logic_vector(31 downto 0);
            debug_status : out std_logic_vector(7 downto 0)
        );
    end component;
    
    -- Test signals
    signal clk : std_logic := '0';
    signal rst : std_logic := '1';
    signal enable : std_logic := '0';
    signal operation_select : std_logic_vector(1 downto 0) := "00";
    signal start_operation : std_logic := '0';
    
    -- Configuration signals
    signal conv_kernel_size : integer range 1 to MAX_KERNEL_SIZE := 3;
    signal conv_stride : integer range 1 to 4 := 1;
    signal conv_padding : integer range 0 to MAX_KERNEL_SIZE/2 := 0;
    signal conv_input_channels : integer range 1 to MAX_CHANNELS_TB := 1;
    signal conv_output_channels : integer range 1 to MAX_CHANNELS_TB := 1;
    
    signal pool_size : integer range 1 to MAX_POOL_SIZE := 2;
    signal pool_stride : integer range 1 to MAX_POOL_SIZE := 2;
    signal pool_type : pooling_type_t := MAX_POOL;
    signal pool_channels : integer range 1 to MAX_CHANNELS_TB := 4;
    
    signal activation_type : activation_type_t := RELU;
    signal tensor_size : integer range 1 to MAX_TENSOR_SIZE := 64;
    signal alpha : std_logic_vector(DATA_WIDTH_TB-1 downto 0);
    
    signal batch_size : integer range 1 to MAX_BATCH_SIZE := 1;
    
    -- Data signals
    signal input_valid : std_logic := '0';
    signal input_data : std_logic_vector((PARALLEL_UNITS_TB*DATA_WIDTH_TB)-1 downto 0);
    signal input_ready : std_logic;
    
    signal weight_valid : std_logic := '0';
    signal weight_data : std_logic_vector(DATA_WIDTH_TB-1 downto 0);
    signal weight_ready : std_logic;
    
    signal bias_valid : std_logic := '0';
    signal bias_data : std_logic_vector(DATA_WIDTH_TB-1 downto 0);
    signal bias_ready : std_logic;
    
    signal output_valid : std_logic;
    signal output_data : std_logic_vector((PARALLEL_UNITS_TB*DATA_WIDTH_TB)-1 downto 0);
    signal output_ready : std_logic := '1';
    
    signal operation_done : std_logic;
    signal operation_busy : std_logic;
    signal current_operation : std_logic_vector(1 downto 0);
    
    signal debug_cycles : std_logic_vector(31 downto 0);
    signal debug_status : std_logic_vector(7 downto 0);
    
    -- Helper functions
    function to_fixed(real_val : real) return std_logic_vector is
        variable int_val : integer;
    begin
        int_val := integer(real_val * real(2**8));  -- 8 fractional bits
        if int_val > 2**(DATA_WIDTH_TB-1)-1 then int_val := 2**(DATA_WIDTH_TB-1)-1; end if;
        if int_val < -2**(DATA_WIDTH_TB-1) then int_val := -2**(DATA_WIDTH_TB-1); end if;
        return std_logic_vector(to_signed(int_val, DATA_WIDTH_TB));
    end function;
    
    function from_fixed(fixed_val : std_logic_vector) return real is
    begin
        return real(to_integer(signed(fixed_val))) / real(2**8);
    end function;
    
    -- Test procedure
    procedure run_operation_test(
        signal clk : in std_logic;
        signal rst : out std_logic;
        signal enable : out std_logic;
        signal operation_select : out std_logic_vector;
        signal start_operation : out std_logic;
        signal operation_done : in std_logic;
        signal operation_busy : in std_logic;
        op_code : std_logic_vector(1 downto 0);
        test_name : string
    ) is
    begin
        report "=== Running Test: " & test_name & " ===" severity note;
        
        -- Reset system
        rst <= '1';
        enable <= '0';
        wait for 5 * CLK_PERIOD;
        rst <= '0';
        wait for CLK_PERIOD;
        
        -- Configure and start operation
        operation_select <= op_code;
        enable <= '1';
        wait for CLK_PERIOD;
        
        start_operation <= '1';
        wait for CLK_PERIOD;
        start_operation <= '0';
        
        -- Wait for operation to complete
        wait until operation_done = '1' or operation_busy = '0';
        
        report "=== Test " & test_name & " Completed ===" severity note;
        wait for 5 * CLK_PERIOD;
    end procedure;

begin
    
    -- Clock generation
    clk_process: process
    begin
        clk <= '0';
        wait for CLK_PERIOD/2;
        clk <= '1';
        wait for CLK_PERIOD/2;
    end process;
    
    -- DUT instantiation
    DUT: FPGA_accelerator
        generic map (
            INPUT_HEIGHT => INPUT_HEIGHT_TB,
            INPUT_WIDTH => INPUT_WIDTH_TB,
            MAX_CHANNELS => MAX_CHANNELS_TB,
            DATA_WIDTH => DATA_WIDTH_TB,
            PARALLEL_UNITS => PARALLEL_UNITS_TB
        )
        port map (
            clk => clk,
            rst => rst,
            enable => enable,
            operation_select => operation_select,
            start_operation => start_operation,
            
            conv_kernel_size => conv_kernel_size,
            conv_stride => conv_stride,
            conv_padding => conv_padding,
            conv_input_channels => conv_input_channels,
            conv_output_channels => conv_output_channels,
            
            pool_size => pool_size,
            pool_stride => pool_stride,
            pool_type => pool_type,
            pool_channels => pool_channels,
            
            activation_type => activation_type,
            tensor_size => tensor_size,
            alpha => alpha,
            
            batch_size => batch_size,
            
            input_valid => input_valid,
            input_data => input_data,
            input_ready => input_ready,
            
            weight_valid => weight_valid,
            weight_data => weight_data,
            weight_ready => weight_ready,
            
            bias_vali