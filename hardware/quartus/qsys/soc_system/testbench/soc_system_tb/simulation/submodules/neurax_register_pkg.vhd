library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package neurax_register_pkg is

  constant c_REG_CMD    : unsigned(2 downto 0) := "000"; -- Address 0: Command register
  constant c_REG_TYPE   : unsigned(2 downto 0) := "001"; -- Address 1: Type register
  constant c_REG_TEMP_0 : unsigned(2 downto 0) := "010";
  constant c_REG_TEMP_1 : unsigned(2 downto 0) := "011";
  constant c_REG_TEMP_2 : unsigned(2 downto 0) := "100";
  constant c_REG_TEMP_3 : unsigned(2 downto 0) := "101";
  constant c_REG_TEMP_4 : unsigned(2 downto 0) := "110";
  constant c_REG_TEMP_5 : unsigned(2 downto 0) := "111";

   -- range
  subtype t_POOLING_TYPE         is natural range 1 downto 0; -- Bits [1:0] in register
  subtype t_ACTIVATION_TYPE      is natural range 3 downto 2; -- Bits [3:2] in register
end package neurax_register_pkg;