library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity dummy is
    Port (
        clk   : in  STD_LOGIC;
        rst   : in  STD_LOGIC;
        out1  : out STD_LOGIC
    );
end dummy;

architecture Behavioral of dummy is
begin
    process(clk, rst)
    begin
        if rst = '1' then
            out1 <= '0';
        elsif rising_edge(clk) then
            out1 <= not out1;
        end if;
    end process;
end Behavioral;