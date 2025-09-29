library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package neurax_register_pkg is
  --- register addresses
  constant c_REG_CMD    : unsigned(3 downto 0) := "0000"; -- Address 0: Command register
  constant c_REG_STATUS  : unsigned(3 downto 0) := "0001"; -- Address 1: Status register
  constant c_REG_CONFIG  : unsigned(3 downto 0) := "0010"; -- Address 2: Config register
  constant c_REG_CONV_CONFIG_0 : unsigned(3 downto 0) := "0011"; -- Address 3: Convolution config register 0
  constant c_REG_CONV_CONFIG_1 : unsigned(3 downto 0) := "0100"; -- Address 4: Convolution config register 1
  constant c_REG_POOL_CONFIG   : unsigned(3 downto 0) := "0101"; -- Address 5: Pooling config register
  constant c_REG_ACTIVATION_CONFIG : unsigned(3 downto 0) := "0110"; -- Address 6: Activation config register
  constant c_REG_ACTIVATION_ALPHA : unsigned(3 downto 0) := "0111"; -- Address 7: Activation alpha register
  constant c_REG_BATCH_SIZE : unsigned(3 downto 0) := "1000"; -- Address 8: Batch size register
  constant c_REG_TEMP_0 : unsigned(3 downto 0) := "1001"; -- Address 9: Temporary register 0
  constant c_REG_TEMP_1 : unsigned(3 downto 0) := "1010"; -- Address 10: Temporary register 1
  constant c_REG_TEMP_2 : unsigned(3 downto 0) := "1011"; -- Address 11: Temporary register 2
  constant c_REG_TEMP_3 : unsigned(3 downto 0) := "1100"; -- Address 12: Temporary register 3
  constant c_REG_DEBUG_CYCLES : unsigned(3 downto 0) := "1101"; -- Address 13: Read-only register
  constant c_REG_DEBUG_STATUS : unsigned(3 downto 0) := "1110"; -- Address 14: Read-only register
  constant c_REG_READ_ONLY : unsigned(3 downto 0) := "1111"; -- Address 15: Read-only register

  --- Output register bit fields ---
  -- Command register
  constant c_CMD_ENABLE          : natural := 0; -- Bit 0
  subtype t_CMD_OPERATION_SELECT is natural range 2 downto 1; -- Bits [2:1] 
  constant c_CMD_START_OPERATION : natural := 3; -- Bit 3 (moved from 2)
  constant c_CMD_WEIGHT_VALID       : natural := 16; -- Bit 16 in status register
  constant c_CMD_BIAS_VALID       : natural := 17; -- Bit 17 in status register

  -- Status register
  constant c_STATUS_INPUT_READY    : natural := 0; -- Bit 0 in status register
  constant c_STATUS_INPUT_VALID    : natural := 1; -- Bit 0 in status register
  constant c_STATUS_OUTPUT_READY    : natural := 2; -- Bit 1 in status register
  constant c_STATUS_OUTPUT_VALID    : natural := 3; -- Bit 2 in status register
  constant c_STATUS_DONE           : natural := 4; -- Bit 3 in status register
  constant c_STATUS_BUSY           : natural := 5; -- Bit 4 in status register
  subtype t_STATUS_CURRENT_OPERATION is natural range 7 downto 6; -- Bits [6:5] in status register
  constant c_STATUS_WEIGHT_READY       : natural := 17; -- Bit 17 in status register
  constant c_STATUS_BIAS_READY       : natural := 18; -- Bit 18 in status register

  -- Convolution config register 0
  subtype t_CONV_STRIDE is natural range 7 downto 0; -- Bits [7:0] in conv config register 0
  subtype t_CONV_PADDING is natural range 15 downto 8; -- Bits [15:8] in conv config register 0
  subtype t_CONV_GROUPS is natural range 23 downto 16; -- Bits [23:16] in conv config register 0
  subtype t_CONV_KERNEL_SIZE is natural range 31 downto 24; -- Bits [31:24] in conv config register 0

  -- Convolution config register 1
  subtype t_CONV_INPUT_CHANNELS is natural range 7 downto 0; -- Bits [7:0] in conv config register 1
  subtype t_CONV_OUTPUT_CHANNELS is natural range 15 downto 8; -- Bits [15:8] in conv config register 1

  -- Pooling config register
  subtype t_POOL_SIZE is natural range 7 downto 0; -- Bits [7:0] in pooling config register
  subtype t_POOL_STRIDE is natural range 15 downto 8; -- Bits [15:8] in pooling config register
  subtype t_POOL_TYPE is natural range 23 downto 16; -- Bits [23:16] in pooling config register
  subtype t_POOL_CHANNELS is natural range 31 downto 24; -- Bits [31:24] in pooling config register

  -- Activation config register
  subtype t_ACTIVATION_TYPE is natural range 7 downto 0; -- Bits [7:0] in activation config register
  subtype t_ACTIVATION_TENSOR_SIZE is natural range 23 downto 8; -- Bits [23:8] in activation config register

  -- Common parameters
  subtype t_BATCH_SIZE is natural range 7 downto 0; -- Bit [7:0] in batch size register

  -- Debug registers
  subtype t_DEBUG_STATUS is natural range 7 downto 0; -- Bits [7:0] in debug status register


  
  -- READ-ONLY register values
  constant c_MAGIC_NUMBER : std_logic_vector(31 downto 0) := x"CAB00D1E"; -- Magic number caboodle in hex
  
  -- Helper constants
  constant INVALID_READ_DATA    : std_logic_vector(31 downto 0) := (others => '1');

  constant c_POOL_OP_MAX     : integer := 0;
  constant c_POOL_OP_AVERAGE : integer := 1;
  constant c_POOL_OP_MIN     : integer := 2;
  constant c_POOL_OP_SUM     : integer := 3;
  
  constant c_ACTIVATION_RELU       : integer := 0;
  constant c_ACTIVATION_SIGMOID    : integer := 1;
  constant c_ACTIVATION_TANH       : integer := 2;
  constant c_ACTIVATION_LINEAR     : integer := 3;
  constant c_ACTIVATION_LEAKY_RELU : integer := 4;
  constant c_ACTIVATION_ELU        : integer := 5;
end package neurax_register_pkg;