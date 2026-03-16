-- neurax.vhd: Top-level entity
-- Connects HPS interfaces to the FPGA accelerator via shared dual-port RAM.
--
-- Architecture:
--   HPS Avalon MM  -> neurax_register_block -> configuration/control signals
--   HPS Avalon ST  -> neurax_data_interface -> dual-port RAM (port A)
--   FPGA_accelerator                        -> dual-port RAM (port B)
--
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.accel_types.all;

entity neurax is
    generic (
        g_WIDTH        : natural := 32;
        g_ADDR_WIDTH   : natural := 4;
        g_PIXEL_WIDTH  : natural := 24;
        g_FB_WIDTH     : natural := 100;
        g_FB_HEIGHT    : natural := 100;
        PARALLEL_UNITS : integer := 4
    );

    port (
        g_clk_i : in  std_logic;
        g_rst_i : in  std_logic;

        ------ Avalon MM Slave Interface ------
        g_avs_chipselect_i    : in  std_logic;
        g_avs_address_i       : in  std_logic_vector(g_ADDR_WIDTH-1 downto 0);
        g_avs_read_i          : in  std_logic;
        g_avs_readdata_o      : out std_logic_vector(g_WIDTH-1 downto 0);
        g_avs_readdatavalid_o : out std_logic;
        g_avs_write_i         : in  std_logic;
        g_avs_writedata_i     : in  std_logic_vector(g_WIDTH-1 downto 0);
        g_avs_byteenable_i    : in  std_logic_vector((g_WIDTH/8)-1 downto 0);
        g_avs_waitrequest_o   : out std_logic;

        ------ Avalon ST Sink Interface ------
        g_asi_channel_i : in  std_logic;
        g_asi_data_i    : in  std_logic_vector(g_WIDTH-1 downto 0);
        g_asi_valid_i   : in  std_logic;
        g_asi_ready_o   : out std_logic;
        g_asi_error_i   : in  std_logic;

        ------ Avalon ST Source Interface ------
        g_aso_data_o    : out std_logic_vector(g_WIDTH-1 downto 0);
        g_aso_valid_o   : out std_logic;
        g_aso_ready_i   : in  std_logic;
        g_aso_channel_o : out std_logic;
        g_aso_error_o   : out std_logic
    );
end neurax;

architecture arch of neurax is

    -- =========================================================================
    -- Component declarations
    -- =========================================================================

    component neurax_register_block
    generic (
        g_WIDTH      : natural := 32;
        g_ADDR_WIDTH : natural := 4
    );
    port (
        clk_i                : in  std_logic;
        rst_i                : in  std_logic;
        avs_chipselect_i     : in  std_logic;
        avs_address_i        : in  std_logic_vector(g_ADDR_WIDTH-1 downto 0);
        avs_read_i           : in  std_logic;
        avs_readdata_o       : out std_logic_vector(g_WIDTH-1 downto 0);
        avs_readdatavalid_o  : out std_logic;
        avs_write_i          : in  std_logic;
        avs_writedata_i      : in  std_logic_vector(g_WIDTH-1 downto 0);
        avs_byteenable_i     : in  std_logic_vector((g_WIDTH/8)-1 downto 0);
        avs_waitrequest_o    : out std_logic;

        neurax_enable_o              : out std_logic;
        neurax_operation_select_o    : out std_logic_vector(1 downto 0);
        neurax_start_operation_o     : out std_logic;
        neurax_conv_kernel_size_o    : out integer range 1 to MAX_KERNEL_SIZE;
        neurax_conv_stride_o         : out integer range 1 to 4;
        neurax_conv_padding_o        : out integer range 0 to MAX_KERNEL_SIZE/2;
        neurax_conv_input_channels_o : out integer range 1 to MAX_CHANNELS;
        neurax_conv_output_channels_o: out integer range 1 to MAX_CHANNELS;
        neurax_pool_size_o           : out integer range 1 to MAX_POOL_SIZE;
        neurax_pool_stride_o         : out integer range 1 to MAX_POOL_SIZE;
        neurax_pool_type_o           : out pooling_type_t;
        neurax_pool_channels_o       : out integer range 1 to MAX_CHANNELS;
        neurax_activation_type_o     : out activation_type_t;
        neurax_tensor_size_o         : out integer range 1 to MAX_TENSOR_SIZE;
        neurax_alpha_o               : out std_logic_vector(DATA_WIDTH-1 downto 0);
        neurax_batch_size_o          : out integer range 1 to MAX_BATCH_SIZE;
        neurax_input_valid_o         : out std_logic;
        neurax_input_ready_i         : in  std_logic;
        neurax_weight_valid_o        : out std_logic;
        neurax_weight_ready_i        : in  std_logic;
        neurax_bias_valid_o          : out std_logic;
        neurax_bias_ready_i          : in  std_logic;
        neurax_output_valid_i        : in  std_logic;
        neurax_output_ready_o        : out std_logic;
        neurax_operation_done_i      : in  std_logic;
        neurax_operation_busy_i      : in  std_logic;
        neurax_current_operation_i   : in  std_logic_vector(1 downto 0);
        neurax_debug_cycle_i         : in  std_logic_vector(g_WIDTH-1 downto 0);
        neurax_debug_status_i        : in  std_logic_vector(7 downto 0)
    );
    end component;

    component neurax_data_interface
    generic (
        g_DATA_WIDTH : natural := 32;
        g_ADDR_WIDTH : natural := 15;
        g_RAM_SIZE   : natural := 23000
    );
    port (
        clk_i             : in  std_logic;
        rst_i             : in  std_logic;
        asi_channel_i     : in  std_logic;
        asi_data_i        : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        asi_valid_i       : in  std_logic;
        asi_ready_o       : out std_logic;
        asi_error_i       : in  std_logic;
        aso_data_o        : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        aso_valid_o       : out std_logic;
        aso_ready_i       : in  std_logic;
        aso_channel_o     : out std_logic;
        aso_error_o       : out std_logic;
        ram_b_rdaddress_i : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_b_q_o         : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        ram_b_wraddress_i : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_b_data_i      : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        ram_b_wren_i      : in  std_logic
    );
    end component;

    component FPGA_accelerator
    generic (
        INPUT_HEIGHT     : integer := 8;
        INPUT_WIDTH      : integer := 8;
        MAX_CHANNELS     : integer := 4;
        PARALLEL_UNITS   : integer := 4;
        INPUT_BASE_ADDR  : integer := 0;
        WEIGHT_BASE_ADDR : integer := 10000;
        BIAS_BASE_ADDR   : integer := 13000;
        OUTPUT_BASE_ADDR : integer := 13016
    );
    port (
        clk                  : in  std_logic;
        rst                  : in  std_logic;
        enable               : in  std_logic;
        operation_select     : in  std_logic_vector(1 downto 0);
        start_operation      : in  std_logic;
        conv_kernel_size     : in  integer range 1 to MAX_KERNEL_SIZE;
        conv_stride          : in  integer range 1 to 4;
        conv_padding         : in  integer range 0 to MAX_KERNEL_SIZE/2;
        conv_input_channels  : in  integer range 1 to MAX_CHANNELS;
        conv_output_channels : in  integer range 1 to MAX_CHANNELS;
        pool_size            : in  integer range 1 to MAX_POOL_SIZE;
        pool_stride          : in  integer range 1 to MAX_POOL_SIZE;
        pool_type            : in  pooling_type_t;
        pool_channels        : in  integer range 1 to MAX_CHANNELS;
        activation_type      : in  activation_type_t;
        tensor_size          : in  integer range 1 to MAX_TENSOR_SIZE;
        alpha                : in  std_logic_vector(DATA_WIDTH-1 downto 0);
        batch_size           : in  integer range 1 to MAX_BATCH_SIZE;
        ram_rdaddress_o      : out std_logic_vector(15 downto 0);
        ram_q_i              : in  std_logic_vector(31 downto 0);
        ram_wraddress_o      : out std_logic_vector(15 downto 0);
        ram_data_o           : out std_logic_vector(31 downto 0);
        ram_wren_o           : out std_logic;
        operation_done       : out std_logic;
        operation_busy       : out std_logic;
        current_operation    : out std_logic_vector(1 downto 0);
        debug_cycles         : out std_logic_vector(31 downto 0);
        debug_status         : out std_logic_vector(7 downto 0)
    );
    end component;

    -- =========================================================================
    -- Internal signals: register block <-> accelerator
    -- =========================================================================
    signal neurax_enable              : std_logic;
    signal neurax_operation_select    : std_logic_vector(1 downto 0);
    signal neurax_start_operation     : std_logic;

    -- Convolution parameters
    signal neurax_conv_kernel_size    : integer;
    signal neurax_conv_stride         : integer;
    signal neurax_conv_padding        : integer;
    signal neurax_conv_input_channels : integer;
    signal neurax_conv_output_channels: integer;

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

    -- Data handshake signals (from register block, kept for status feedback)
    signal neurax_input_valid  : std_logic;
    signal neurax_weight_valid : std_logic;
    signal neurax_bias_valid   : std_logic;
    signal neurax_output_ready : std_logic;

    -- Status signals
    signal neurax_operation_done    : std_logic;
    signal neurax_operation_busy    : std_logic;
    signal neurax_current_operation : std_logic_vector(1 downto 0);
    signal neurax_output_valid      : std_logic;

    -- Debug
    signal neurax_debug_cycle  : std_logic_vector(31 downto 0);
    signal neurax_debug_status : std_logic_vector(7 downto 0);

    -- =========================================================================
    -- RAM Port B signals: accelerator <-> data_interface
    -- =========================================================================
    signal accel_ram_rdaddress : std_logic_vector(15 downto 0);
    signal accel_ram_q         : std_logic_vector(31 downto 0);
    signal accel_ram_wraddress : std_logic_vector(15 downto 0);
    signal accel_ram_data      : std_logic_vector(31 downto 0);
    signal accel_ram_wren      : std_logic;

begin

    -- =========================================================================
    -- Register Block Instance
    -- =========================================================================
    u_neurax_register_block : neurax_register_block
    generic map (
        g_WIDTH      => g_WIDTH,
        g_ADDR_WIDTH => g_ADDR_WIDTH
    )
    port map (
        clk_i         => g_clk_i,
        rst_i         => g_rst_i,

        avs_chipselect_i    => g_avs_chipselect_i,
        avs_address_i       => g_avs_address_i,
        avs_read_i          => g_avs_read_i,
        avs_readdata_o      => g_avs_readdata_o,
        avs_readdatavalid_o => g_avs_readdatavalid_o,
        avs_write_i         => g_avs_write_i,
        avs_writedata_i     => g_avs_writedata_i,
        avs_byteenable_i    => g_avs_byteenable_i,
        avs_waitrequest_o   => g_avs_waitrequest_o,

        neurax_enable_o              => neurax_enable,
        neurax_operation_select_o    => neurax_operation_select,
        neurax_start_operation_o     => neurax_start_operation,

        neurax_conv_kernel_size_o    => neurax_conv_kernel_size,
        neurax_conv_stride_o         => neurax_conv_stride,
        neurax_conv_padding_o        => neurax_conv_padding,
        neurax_conv_input_channels_o => neurax_conv_input_channels,
        neurax_conv_output_channels_o=> neurax_conv_output_channels,

        neurax_pool_size_o     => neurax_pool_size,
        neurax_pool_stride_o   => neurax_pool_stride,
        neurax_pool_type_o     => neurax_pool_type,
        neurax_pool_channels_o => neurax_pool_channels,

        neurax_activation_type_o => neurax_activation_type,
        neurax_tensor_size_o     => neurax_tensor_size,
        neurax_alpha_o           => neurax_alpha,

        neurax_batch_size_o => neurax_batch_size,

        -- Handshake signals: these still exist in the register block for
        -- status feedback, but data now flows through the shared RAM.
        neurax_input_valid_o  => neurax_input_valid,
        neurax_input_ready_i  => '1',  -- Always ready (data is in RAM)
        neurax_weight_valid_o => neurax_weight_valid,
        neurax_weight_ready_i => '1',  -- Always ready (weights are in RAM)
        neurax_bias_valid_o   => neurax_bias_valid,
        neurax_bias_ready_i   => '1',  -- Always ready (bias is in RAM)

        neurax_output_valid_i      => neurax_output_valid,
        neurax_output_ready_o      => neurax_output_ready,
        neurax_operation_done_i    => neurax_operation_done,
        neurax_operation_busy_i    => neurax_operation_busy,
        neurax_current_operation_i => neurax_current_operation,

        neurax_debug_cycle_i  => neurax_debug_cycle,
        neurax_debug_status_i => neurax_debug_status
    );

    -- Output valid mirrors operation done (results are in RAM and ready to stream)
    neurax_output_valid <= neurax_operation_done;

    -- =========================================================================
    -- Data Interface Instance (contains the shared dual-port RAM)
    -- Port A: Avalon ST (HPS streaming)
    -- Port B: FPGA accelerator (address-driven access)
    -- =========================================================================
    u_neurax_data_interface : neurax_data_interface
    generic map (
        g_DATA_WIDTH => g_WIDTH,
        g_ADDR_WIDTH => 15,
        g_RAM_SIZE   => 23000  -- 23K x 32-bit words = 92 KB (one Q8.8 per word, 100x100 conv)
    )
    port map (
        clk_i         => g_clk_i,
        rst_i         => g_rst_i,

        -- Avalon ST Sink (HPS writes data into RAM)
        asi_channel_i => g_asi_channel_i,
        asi_data_i    => g_asi_data_i,
        asi_valid_i   => g_asi_valid_i,
        asi_ready_o   => g_asi_ready_o,
        asi_error_i   => g_asi_error_i,

        -- Avalon ST Source (HPS reads results from RAM)
        aso_data_o    => g_aso_data_o,
        aso_valid_o   => g_aso_valid_o,
        aso_ready_i   => g_aso_ready_i,
        aso_channel_o => g_aso_channel_o,
        aso_error_o   => g_aso_error_o,

        -- RAM Port B -> connected to FPGA accelerator
        -- Accelerator uses 16-bit addresses; truncate to 15-bit for M10K RAM
        ram_b_rdaddress_i => accel_ram_rdaddress(14 downto 0),
        ram_b_q_o         => accel_ram_q,
        ram_b_wraddress_i => accel_ram_wraddress(14 downto 0),
        ram_b_data_i      => accel_ram_data,
        ram_b_wren_i      => accel_ram_wren
    );

    -- =========================================================================
    -- FPGA Accelerator Instance
    -- Reads input/weights/bias from RAM, writes output to RAM
    -- =========================================================================
    u_fpga_accelerator : FPGA_accelerator
    generic map (
        INPUT_HEIGHT     => g_FB_HEIGHT,
        INPUT_WIDTH      => g_FB_WIDTH,
        MAX_CHANNELS     => 4,
        PARALLEL_UNITS   => 4,
        INPUT_BASE_ADDR  => 0,       -- Input data starts at word 0
        WEIGHT_BASE_ADDR => 10000,   -- Weights start at word 10000
        BIAS_BASE_ADDR   => 13000,   -- Bias starts at word 13000
        OUTPUT_BASE_ADDR => 13016    -- Output starts at word 13016
    )
    port map (
        clk => g_clk_i,
        rst => g_rst_i,

        -- Control signals (from register block)
        enable           => neurax_enable,
        operation_select => neurax_operation_select,
        start_operation  => neurax_start_operation,

        -- Convolution config
        conv_kernel_size     => neurax_conv_kernel_size,
        conv_stride          => neurax_conv_stride,
        conv_padding         => neurax_conv_padding,
        conv_input_channels  => neurax_conv_input_channels,
        conv_output_channels => neurax_conv_output_channels,

        -- Pooling config
        pool_size    => neurax_pool_size,
        pool_stride  => neurax_pool_stride,
        pool_type    => neurax_pool_type,
        pool_channels=> neurax_pool_channels,

        -- Activation config
        activation_type => neurax_activation_type,
        tensor_size     => neurax_tensor_size,
        alpha           => neurax_alpha,

        -- Common config
        batch_size => neurax_batch_size,

        -- RAM Port B: connected to data_interface's port B
        ram_rdaddress_o => accel_ram_rdaddress,
        ram_q_i         => accel_ram_q,
        ram_wraddress_o => accel_ram_wraddress,
        ram_data_o      => accel_ram_data,
        ram_wren_o      => accel_ram_wren,

        -- Status
        operation_done    => neurax_operation_done,
        operation_busy    => neurax_operation_busy,
        current_operation => neurax_current_operation,

        -- Debug
        debug_cycles => neurax_debug_cycle,
        debug_status => neurax_debug_status
    );

end arch;
