library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library altera;
use altera.altera_syn_attributes.all;

use work.neurax_register_pkg.all;
use work.accel_types.all;

entity neurax_register_block is
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
        neurax_debug_status_i       : in std_logic_vector(7 downto 0)
    );

end neurax_register_block;

architecture arch of neurax_register_block is
    subtype t_word is std_logic_vector((g_WIDTH - 1) downto 0);
    type memory_t is array((2 ** g_ADDR_WIDTH - 1) downto 0) of t_word; -- 2^g_ADDR_WIDTH memory locations, each word is g_WIDTH wide in total 2^g_ADDR_WIDTH * g_WIDTH bits
    signal ram : memory_t := (to_integer(c_REG_READ_ONLY) => c_MAGIC_NUMBER, others => (others => '0'));
    signal read_data_valid : std_logic;

    --- Helper functions ---
    -- Convert integer to pooling_type_t
    function int_to_pooling_type(int_val : integer) return pooling_type_t is
    begin
        case int_val is
            when c_POOL_OP_MAX     => return MAX_POOL;
            when c_POOL_OP_AVERAGE => return AVERAGE_POOL;
            when c_POOL_OP_MIN     => return MIN_POOL;
            when c_POOL_OP_SUM     => return SUM_POOL;
            when others            => return MAX_POOL; -- Safe default
        end case;
    end function;

    -- Convert integer to activation_type_t
    function int_to_activation_type(int_val : integer) return activation_type_t is
    begin
        case int_val is
            when c_ACTIVATION_RELU       => return RELU;
            when c_ACTIVATION_SIGMOID    => return SIGMOID;
            when c_ACTIVATION_TANH       => return TANH;
            when c_ACTIVATION_LINEAR     => return LINEAR;
            when c_ACTIVATION_LEAKY_RELU => return LEAKY_RELU;
            when c_ACTIVATION_ELU        => return ELU;
            when others                  => return RELU; -- Safe default
        end case;
    end function;

    -- Limit range function
    function limit_range(reg_val : std_logic_vector; min_val : integer; max_val : integer) return integer is
        variable int_val : integer;
    begin
        int_val := to_integer(unsigned(reg_val));
        if int_val < min_val or int_val = 0 then
            return min_val; -- Clamp to minimum (treat 0 as minimum)
        elsif int_val > max_val then
            return max_val; -- Clamp to maximum
        else
            return int_val;
        end if;
    end function;

begin
    --- Write process
    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            ram <= (to_integer(c_REG_READ_ONLY) => c_MAGIC_NUMBER, others => (others => '0'));
        elsif rising_edge(clk_i) then
            -- Update register values
            ram(to_integer(c_REG_STATUS))(c_STATUS_DONE) <= neurax_operation_done_i;
            ram(to_integer(c_REG_STATUS))(c_STATUS_BUSY) <= neurax_operation_busy_i;
            ram(to_integer(c_REG_STATUS))(t_STATUS_CURRENT_OPERATION) <= neurax_current_operation_i;
            ram(to_integer(c_REG_STATUS))(c_STATUS_WEIGHT_READY) <= neurax_weight_ready_i;
            ram(to_integer(c_REG_STATUS))(c_STATUS_BIAS_READY) <= neurax_bias_ready_i;
            ram(to_integer(c_REG_STATUS))(c_STATUS_OUTPUT_VALID) <= neurax_output_valid_i;
            ram(to_integer(c_REG_STATUS))(c_STATUS_INPUT_READY) <= neurax_input_ready_i;

            ram(to_integer(c_REG_DEBUG_CYCLES)) <= neurax_debug_cycle_i;
            ram(to_integer(c_REG_DEBUG_STATUS))(t_DEBUG_STATUS) <= neurax_debug_status_i;

            if avs_chipselect_i = '1' and avs_write_i = '1' then
                if unsigned(avs_address_i) < 2**g_ADDR_WIDTH and 
                   unsigned(avs_address_i) /= c_REG_READ_ONLY and
                   unsigned(avs_address_i) /= c_REG_STATUS and
                   unsigned(avs_address_i) /= c_REG_DEBUG_CYCLES and
                   unsigned(avs_address_i) /= c_REG_DEBUG_STATUS then
                    -- Valid address range
                    for i in 0 to (g_WIDTH/8 - 1) loop
                        if avs_byteenable_i(i) = '1' then
                            -- Write only the enabled bytes
                            ram(to_integer(unsigned(avs_address_i)))(i*8 + 7 downto i*8) <= avs_writedata_i(i*8 + 7 downto i*8);
                        end if;
                    end loop;
                end if;
            end if;
        end if;
    end process;
    
    --- Read process
    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            avs_readdata_o <= (others => '0');
            read_data_valid <= '0';
        elsif rising_edge(clk_i) then
            if avs_chipselect_i = '1' and avs_read_i = '1' then
                if unsigned(avs_address_i) < 2**g_ADDR_WIDTH then
                    -- Valid address range
                    avs_readdata_o <= ram(to_integer(unsigned(avs_address_i)));
                    read_data_valid <= '1';
                else
                    -- Address out of range
                    avs_readdata_o <= INVALID_READ_DATA;
                    read_data_valid <= '0';
                end if;
            else
                read_data_valid <= '0';  -- Clear when not reading
            end if;
        end if;
    end process;
                    
                    
    --- Avalon MM Slave Interface ---
    avs_waitrequest_o <= '0'; -- Always ready
    avs_readdatavalid_o <= read_data_valid;

    --- Neurax Register Interface ---
    -- Command signals
    neurax_enable_o <= ram(to_integer(c_REG_CMD))(c_CMD_ENABLE);
    neurax_operation_select_o <= ram(to_integer(c_REG_CMD))(t_CMD_OPERATION_SELECT);
    neurax_start_operation_o <= ram(to_integer(c_REG_CMD))(c_CMD_START_OPERATION);
    neurax_weight_valid_o <= ram(to_integer(c_REG_CMD))(c_CMD_WEIGHT_VALID);
    neurax_bias_valid_o <= ram(to_integer(c_REG_CMD))(c_CMD_BIAS_VALID);

    -- Status signals
    neurax_input_valid_o <= ram(to_integer(c_REG_STATUS))(c_STATUS_INPUT_VALID);
    neurax_output_ready_o <= ram(to_integer(c_REG_STATUS))(c_STATUS_OUTPUT_READY);

    neurax_conv_kernel_size_o <= limit_range(ram(to_integer(c_REG_CONV_CONFIG_0))(t_CONV_KERNEL_SIZE), 1, MAX_KERNEL_SIZE);
    neurax_conv_stride_o <= limit_range(ram(to_integer(c_REG_CONV_CONFIG_0))(t_CONV_STRIDE), 1, 4);
    neurax_conv_padding_o <= limit_range(ram(to_integer(c_REG_CONV_CONFIG_0))(t_CONV_PADDING), 0, MAX_KERNEL_SIZE/2);

    -- Add missing convolution parameters
    neurax_conv_input_channels_o <= limit_range(ram(to_integer(c_REG_CONV_CONFIG_1))(t_CONV_INPUT_CHANNELS), 1, MAX_CHANNELS);
    neurax_conv_output_channels_o <= limit_range(ram(to_integer(c_REG_CONV_CONFIG_1))(t_CONV_OUTPUT_CHANNELS), 1, MAX_CHANNELS);

    -- Pooling parameters
    neurax_pool_size_o <= limit_range(ram(to_integer(c_REG_POOL_CONFIG))(t_POOL_SIZE), 1, MAX_POOL_SIZE);
    neurax_pool_stride_o <= limit_range(ram(to_integer(c_REG_POOL_CONFIG))(t_POOL_STRIDE), 1, MAX_POOL_SIZE);
    neurax_pool_type_o <= int_to_pooling_type(to_integer(unsigned(ram(to_integer(c_REG_POOL_CONFIG))(t_POOL_TYPE))));
    neurax_pool_channels_o <= limit_range(ram(to_integer(c_REG_POOL_CONFIG))(t_POOL_CHANNELS), 1, MAX_CHANNELS);

    -- Activation parameters
    neurax_activation_type_o <= int_to_activation_type(to_integer(unsigned(ram(to_integer(c_REG_ACTIVATION_CONFIG))(t_ACTIVATION_TYPE))));
    neurax_tensor_size_o <= limit_range(ram(to_integer(c_REG_ACTIVATION_CONFIG))(t_ACTIVATION_TENSOR_SIZE), 1, MAX_TENSOR_SIZE);
    neurax_alpha_o <= ram(to_integer(c_REG_ACTIVATION_ALPHA))(DATA_WIDTH-1 downto 0);

    -- Batch size
    neurax_batch_size_o <= limit_range(ram(to_integer(c_REG_BATCH_SIZE))(t_BATCH_SIZE), 1, MAX_BATCH_SIZE);

end arch;