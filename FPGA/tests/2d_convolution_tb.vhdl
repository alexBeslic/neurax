library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity convolution_tb is
end convolution_tb;

architecture behavior of convolution_tb is

    component convolution_3x3
        Port (
            clk        : in  std_logic;
            rst        : in  std_logic;
            pixel_in   : in  std_logic_vector(7 downto 0);
            valid_in   : in  std_logic;
            pixel_out  : out std_logic_vector(15 downto 0);
            valid_out  : out std_logic
        );
    end component;

    signal clk       : std_logic := '0';
    signal rst       : std_logic := '1';
    signal pixel_in  : std_logic_vector(7 downto 0) := (others => '0');
    signal valid_in  : std_logic := '0';
    signal pixel_out : std_logic_vector(15 downto 0);
    signal valid_out : std_logic;

    -- Example 5x5 grayscale image (25 pixels)
    type image_array is array (0 to 24) of std_logic_vector(7 downto 0);
    constant image_data : image_array := (
        x"10", x"20", x"30", x"40", x"50",
        x"60", x"70", x"80", x"90", x"A0",
        x"B0", x"C0", x"D0", x"E0", x"F0",
        x"01", x"11", x"21", x"31", x"41",
        x"51", x"61", x"71", x"81", x"91"
    );

    signal pixel_index : integer := 0;

begin

    -- Clock generation
    clk_process : process
    begin
        while now < 5000 ns loop
            clk <= '0';
            wait for 10 ns;
            clk <= '1';
            wait for 10 ns;
        end loop;
        wait;
    end process;

    -- Instantiate DUT
    uut: convolution_3x3
        Port map (
            clk => clk,
            rst => rst,
            pixel_in => pixel_in,
            valid_in => valid_in,
            pixel_out => pixel_out,
            valid_out => valid_out
        );

    -- Stimulus process
    stim_proc: process
    begin
        wait for 30 ns;
        rst <= '0';
        wait for 20 ns;

        for i in 0 to 24 loop
            pixel_in <= image_data(i);
            valid_in <= '1';
            wait for 20 ns;
        end loop;

        valid_in <= '0';
        wait;
    end process;

end behavior;
