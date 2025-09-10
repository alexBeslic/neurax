library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity dummy_tb is
end dummy_tb;

architecture Behavioral of dummy_tb is
    signal clk   : STD_LOGIC := '0';
    signal rst   : STD_LOGIC := '0';
    signal out1  : STD_LOGIC;

    -- Instantiate the Unit Under Test (UUT)
    component dummy
        Port (
            clk   : in  STD_LOGIC;
            rst   : in  STD_LOGIC;
            out1  : out STD_LOGIC
        );
    end component;

begin
    uut: dummy port map (
        clk => clk,
        rst => rst,
        out1 => out1
    );

    -- Clock generation
    clk_process : process
    begin
        while true loop
            clk <= '0';
            wait for 10 ns;
            clk <= '1';
            wait for 10 ns;
        end loop;
        wait;
    end process;

    -- Stimulus process
    stim_proc: process
    begin
        rst <= '1';
        wait for 25 ns;
        rst <= '0';
        wait for 100 ns;
        assert out1 = '1' report "out1 should be toggled" severity note;
        wait for 100 ns;
        assert out1 = '0' report "out1 should be toggled again" severity note;
        wait;
    end process;

end Behavioral;
