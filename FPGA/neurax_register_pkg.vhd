library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package neurax_register_pkg is

  --- READ-ONLY register values
  constant c_MAGIC_NUMBER : std_logic_vector(31 downto 0) := x"CAB00D1E"; -- Magic number caboodle in hex

  --- register addresses
  constant c_REG_CMD    : unsigned(2 downto 0) := "000"; -- Address 0: Command register
  constant c_REG_CONFIG : unsigned(2 downto 0) := "001"; -- Address 1: Config register
  constant c_REG_STATUS : unsigned(2 downto 0) := "010"; -- Address 2: Status register
  constant c_REG_TEMP_1 : unsigned(2 downto 0) := "011";
  constant c_REG_TEMP_2 : unsigned(2 downto 0) := "100";
  constant c_REG_TEMP_3 : unsigned(2 downto 0) := "101";
  constant c_REG_TEMP_4 : unsigned(2 downto 0) := "110";
  constant c_REG_READ_ONLY : unsigned(2 downto 0) := "111";

  --- Output register bit fields
  --- Command register
  constant c_CMD_START           : natural := 0; -- Bit 0 in command register

  --- Config register
  subtype t_LAYER_ENABLE         is natural range 2 downto 0; -- Bits [2:0] in register
  subtype t_POOLING_TYPE         is natural range 4 downto 3; -- Bits [4:3] in register
  subtype t_ACTIVATION_TYPE      is natural range 6 downto 5; -- Bits [6:5] in register
  constant c_16BIT_MODE          : natural := 7;              -- Bit  [7] in register

  --- Status register
  constant c_STATUS_FINISHED     : natural := 0; -- Bit 0 in status register
end package neurax_register_pkg;