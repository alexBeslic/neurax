library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package neurax_accel_types_pkg is
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

end package neurax_accel_types_pkg;