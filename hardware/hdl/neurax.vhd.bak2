library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.accel_types.all;

entity neurax is
    generic (
        g_WIDTH      : natural := 32;
        g_ADDR_WIDTH : natural := 4;
        g_PIXEL_WIDTH  : natural := 24; -- 24 bit RGB pixel
        g_FB_WIDTH     : natural := 100;
        g_FB_HEIGHT    : natural := 100;
        PARALLEL_UNITS      : integer := 4 
    );

    port (
        clk_i         : in  std_logic;
        rst_i         : in  std_logic;

        ------ Avalon MM Slave Interface ------
        avs_chipselect_i    : in  std_logic;
        avs_address_i       : in  std_logic_vector(g_ADDR_WIDTH-1 downto 0);
        avs_read_i          : in  std_logic;
        avs_readdata_o      : out std_logic_vector(g_WIDTH-1 downto 0);
        avs_readdatavalid_o : out std_logic;
        avs_write_i         : in  std_logic;
        avs_writedata_i     : in  std_logic_vector(g_WIDTH-1 downto 0);
        avs_byteenable_i    : in  std_logic_vector((g_WIDTH/8)-1 downto 0);
        avs_waitrequest_o   : out std_logic;

        ------ Avalon ST Sink Interface ------
        asi_channel_i      : in  std_logic;
        asi_data_i         : in  std_logic_vector(g_WIDTH-1 downto 0);
        asi_valid_i        : in  std_logic;
        asi_ready_o        : out std_logic;
        asi_error_i        : in  std_logic;

        ------ Avalon ST Source Interface ------
        aso_data_o     : out std_logic_vector(g_WIDTH-1 downto 0);
        aso_valid_o    : out std_logic;
        aso_ready_i    : in  std_logic;
        aso_channel_o  : out std_logic;
        aso_error_o    : out std_logic
    );
end neurax;

architecture arch of neurax is

    component neurax_register_block
    generic (
        g_WIDTH      : natural := 32;
        g_ADDR_WIDTH : natural := 4
    );

    port (
        clk_i         : in  std_logic;
        rst_i         : in  std_logic;

        ------ Avalon MM Slave Interface ------
        avs_chipselect_i    : in  std_logic;
        avs_address_i       : in  std_logic_vector(g_ADDR_WIDTH-1 downto 0);
        avs_read_i          : in  std_logic;
        avs_readdata_o      : out std_logic_vector(g_WIDTH-1 downto 0);
        avs_readdatavalid_o : out std_logic;
        avs_write_i         : in  std_logic;
        avs_writedata_i     : in  std_logic_vector(g_WIDTH-1 downto 0);
        avs_byteenable_i    : in  std_logic_vector((g_WIDTH/8)-1 downto 0);
        avs_waitrequest_o   : out std_logic;


        ------ Neurax Register Interface ------
        neurax_enable_o           : out std_logic;
        neurax_operation_select_o : out std_logic_vector(1 downto 0);
        neurax_start_operation_o  : out std_logic;
        -- Convolution parameters
        neurax_conv_kernel_size_o     : out integer range 1 to MAX_KERNEL_SIZE;
        neurax_conv_stride_o          : out integer range 1 to 4;
        neurax_conv_padding_o         : out integer range 0 to MAX_KERNEL_SIZE/2;
        neurax_conv_input_channels_o  : out integer range 1 to MAX_CHANNELS;
        neurax_conv_output_channels_o : out integer range 1 to MAX_CHANNELS;
        -- Pooling parameters
        neurax_pool_size_o     : out integer range 1 to MAX_POOL_SIZE;
        neurax_pool_stride_o   : out integer range 1 to MAX_POOL_SIZE;
        neurax_pool_type_o     : out pooling_type_t;
        neurax_pool_channels_o : out integer range 1 to MAX_CHANNELS;
        -- Activation parameters
        neurax_activation_type_o : out activation_type_t;
        neurax_tensor_size_o     : out integer range 1 to MAX_TENSOR_SIZE;
        neurax_alpha_o           : out std_logic_vector(DATA_WIDTH-1 downto 0);
        -- Common parameters
        neurax_batch_size_o : out integer range 1 to MAX_BATCH_SIZE;
        -- Data interfaces
        neurax_input_valid_o  : out std_logic;
        neurax_input_ready_i : in  std_logic;
        -- Weight interface
        neurax_weight_valid_o : out std_logic;
        neurax_weight_ready_i : in  std_logic;
        -- Bias interface
        neurax_bias_valid_o   : out std_logic;
        neurax_bias_ready_i   : in  std_logic;
        -- Output interface
        neurax_output_valid_i : in  std_logic;
        neurax_output_ready_o : out std_logic;
        -- Status signals
        neurax_operation_done_i : in  std_logic;
        neurax_operation_busy_i : in  std_logic;
        neurax_current_operation_i : in  std_logic_vector(1 downto 0);
        -- Debug/monitoring
        neurax_debug_cycle_i  : in std_logic_vector(g_WIDTH-1 downto 0);
        neurax_debug_status_i : in std_logic_vector(7 downto 0)
    );
    end component;

    component neurax_data_interface
    generic (
        g_DATA_WIDTH   : natural := 32; -- RAM data width
        g_ADDR_WIDTH   : natural := 16; -- RAM address width
        g_RAM_SIZE     : natural := 10000 -- Total RAM size in words
    );
    port (
        clk_i         : in  std_logic;
        rst_i         : in  std_logic;

        ------ Avalon ST Sink Interface ------
        asi_channel_i      : in  std_logic;
        asi_data_i         : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        asi_valid_i        : in  std_logic;
        asi_ready_o        : out std_logic;
        asi_error_i        : in  std_logic;

        ------ Avalon ST Source Interface ------
        aso_data_o     : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        aso_valid_o    : out std_logic;
        aso_ready_i    : in  std_logic;
        aso_channel_o  : out std_logic;
        aso_error_o    : out std_logic;

        ------ RAM 2-Port Interface ------
        ram_data_o      : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        ram_rdaddress_o : out std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_wraddress_o : out std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_wren_o      : out std_logic;
        ram_q_i         : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0)
    );
    end component;

    component FPGA_accelerator
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
    end component;

    signal neurax_enable           : std_logic;
    signal neurax_operation_select : std_logic_vector(1 downto 0);
    signal neurax_start_operation  : std_logic;
    -- Convolution parameters
    signal neurax_conv_kernel_size : integer;
    signal neurax_conv_stride      : integer;
    signal neurax_conv_padding     : integer;
    signal neurax_conv_input_channels : integer;
    signal neurax_conv_output_channels : integer;
    -- Pooling parameters
    signal neurax_pool_size     : integer;
    signal neurax_pool_stride   : integer;
    signal neurax_pool_type     : pooling_type_t;
    signal neurax_pool_channels : integer;
    -- Activation parameters
    signal neurax_activation_type : activation_type_t;
    signal neurax_tensor_size     : integer;
    signal neurax_alpha           : std_logic_vector(DATA_WIDTH-1 downto 0);
    -- Common parameters
    signal neurax_batch_size : integer;
    -- Data interfaces
    signal neurax_input_valid : std_logic;
    signal neurax_input_ready : std_logic;
    -- Weight interface
    signal neurax_weight_valid : std_logic;
    signal neurax_weight_ready : std_logic;
    -- Bias interface
    signal neurax_bias_valid : std_logic;
    signal neurax_bias_ready : std_logic;
    -- Output interface
    signal neurax_output_valid : std_logic;
    signal neurax_output_data : std_logic_vector((PARALLEL_UNITS*DATA_WIDTH)-1 downto 0);
    signal neurax_output_ready : std_logic;
    -- Status signals
    signal neurax_operation_done : std_logic;
    signal neurax_operation_busy : std_logic;
    signal neurax_current_operation : std_logic_vector(1 downto 0);
    -- Debug/monitoring
    signal neurax_debug_cycle : std_logic_vector(31 downto 0);
    signal neurax_debug_status : std_logic_vector(7 downto 0);
begin
    -- Register Block Instance
    u_neurax_register_block : neurax_register_block
    generic map (
        g_WIDTH      => g_WIDTH,
        g_ADDR_WIDTH => 4
    )
    port map (
        clk_i         => clk_i,
        rst_i         => rst_i,

        ------ Avalon MM Slave Interface ------
        avs_chipselect_i    => avs_chipselect_i,
        avs_address_i       => avs_address_i,
        avs_read_i          => avs_read_i,
        avs_readdata_o      => avs_readdata_o,
        avs_readdatavalid_o => avs_readdatavalid_o,
        avs_write_i         => avs_write_i,
        avs_writedata_i     => avs_writedata_i,
        avs_byteenable_i    => avs_byteenable_i,
        avs_waitrequest_o   => avs_waitrequest_o,

        ------ Neurax Register Interface ------
        neurax_enable_o           => neurax_enable,
        neurax_operation_select_o => neurax_operation_select,
        neurax_start_operation_o  => neurax_start_operation,
        -- Convolution parameters
        neurax_conv_kernel_size_o     => neurax_conv_kernel_size,
        neurax_conv_stride_o          => neurax_conv_stride,
        neurax_conv_padding_o         => neurax_conv_padding,
        neurax_conv_input_channels_o  => neurax_conv_input_channels,
        neurax_conv_output_channels_o => neurax_conv_output_channels,
        -- Pooling parameters
        neurax_pool_size_o     => neurax_pool_size,
        neurax_pool_stride_o   => neurax_pool_stride,
        neurax_pool_type_o     => neurax_pool_type,
        neurax_pool_channels_o => neurax_pool_channels,
        -- Activation parameters
        neurax_activation_type_o => neurax_activation_type,
        neurax_tensor_size_o     => neurax_tensor_size,
        neurax_alpha_o           => neurax_alpha,
        -- Common parameters
        neurax_batch_size_o => neurax_batch_size,
        -- Data interfaces
        neurax_input_valid_o  => neurax_input_valid,
        neurax_input_ready_i  => neurax_input_ready,
        -- Weight interface
        neurax_weight_valid_o => neurax_weight_valid,
        neurax_weight_ready_i => neurax_weight_ready,
        -- Bias interface
        neurax_bias_valid_o   => neurax_bias_valid,
        neurax_bias_ready_i   => neurax_bias_ready,
        -- Output interface
        neurax_output_valid_i => neurax_output_valid,
        neurax_output_ready_o => neurax_output_ready,
        -- Status signals
        neurax_operation_done_i => neurax_operation_done,
        neurax_operation_busy_i => neurax_operation_busy,
        neurax_current_operation_i => neurax_current_operation,
        -- Debug/monitoring
        neurax_debug_cycle_i => neurax_debug_cycle,
        neurax_debug_status_i => neurax_debug_status
    );

    -- Data Interface Instance
    u_neurax_data_interface : neurax_data_interface
    generic map (
        g_DATA_WIDTH => g_WIDTH,
        g_ADDR_WIDTH => 16,
        g_RAM_SIZE   => 10000
    )
    port map (
        clk_i         => clk_i,
        rst_i         => rst_i,

        ------ Avalon ST Sink Interface ------
        asi_channel_i      => asi_channel_i,
        asi_data_i         => asi_data_i,
        asi_valid_i        => asi_valid_i,
        asi_ready_o        => asi_ready_o,
        asi_error_i        => asi_error_i,

        ------ Avalon ST Source Interface ------
        aso_data_o     => aso_data_o,
        aso_valid_o    => aso_valid_o,
        aso_ready_i    => aso_ready_i,
        aso_channel_o  => aso_channel_o,
        aso_error_o    => aso_error_o,

        ------ RAM 2-Port Interface ------
        ram_data_o      => open, -- Not connected in this top-level
        ram_rdaddress_o => open, -- Not connected in this top-level
        ram_wraddress_o => open, -- Not connected in this top-level
        ram_wren_o      => open, -- Not connected in this top-level
        ram_q_i         => (others => '0') -- Not connected in this top-level
    );

    -- FPGA Accelerator Instance
    u_fpga_accelerator : FPGA_accelerator
    generic map (
        INPUT_HEIGHT   => g_FB_HEIGHT,
        INPUT_WIDTH    => g_FB_WIDTH,
        MAX_CHANNELS   => 4,
        PARALLEL_UNITS => 4
    )
    port map (
        clk => clk_i,
        rst => rst_i,

        -- Control signals
        enable => neurax_enable,
        operation_select => neurax_operation_select,
        start_operation => neurax_start_operation,

        -- Configuration inputs
        -- Convolution config
        conv_kernel_size => neurax_conv_kernel_size,
        conv_stride => neurax_conv_stride,
        conv_padding => neurax_conv_padding,
        conv_input_channels => neurax_conv_input_channels,
        conv_output_channels => neurax_conv_output_channels,

        -- Pooling config
        pool_size => neurax_pool_size,
        pool_stride => neurax_pool_stride,
        pool_type => neurax_pool_type,
        pool_channels => neurax_pool_channels,

        -- Activation config
        activation_type => neurax_activation_type,
        tensor_size => neurax_tensor_size,
        alpha => neurax_alpha,

        -- Common config
        batch_size => neurax_batch_size,

        -- Data interfaces
        input_valid => neurax_input_valid,
        input_data => (others => '0'), -- Connect to actual data source
        input_ready => neurax_input_ready,

        -- Weight interface (samo za convolution)
        weight_valid => neurax_weight_valid,
        weight_data => (others => '0'), -- Connect to actual weight source
        weight_ready => neurax_weight_ready,

        -- Bias interface (samo za convolution)
        bias_valid => neurax_bias_valid,
        bias_data => (others => '0'), -- Connect to actual bias source
        bias_ready => neurax_bias_ready,

        -- Output interface
        output_valid => neurax_output_valid,
        output_data => neurax_output_data,
        output_ready => neurax_output_ready,

        -- Status signals
        operation_done => neurax_operation_done,
        operation_busy => neurax_operation_busy,
        current_operation => neurax_current_operation,

        -- Debug/monitoring
        debug_cycles => neurax_debug_cycle,
        debug_status => neurax_debug_status
    );

end arch;