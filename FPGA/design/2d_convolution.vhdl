library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity convolution_3x3 is
    Port (
        clk     : in  std_logic;
        rst     : in  std_logic;
        pixel_in : in  std_logic_vector(7 downto 0);
        valid_in : in  std_logic;
        pixel_out : out std_logic_vector(15 downto 0);
        valid_out : out std_logic
    );
end convolution_3x3;

architecture Behavioral of convolution_3x3 is

    -- Internal 3x3 window buffer (sliding window)
    type pixel_array is array (0 to 2, 0 to 2) of std_logic_vector(7 downto 0);
    signal window : pixel_array := (others => (others => '0'));

    -- Line buffers
    signal line1, line2 : std_logic_vector(7 downto 0) := (others => '0');

    -- Control
    signal count : integer := 0;

    -- Kernel constants
    type kernel_array is array (0 to 2, 0 to 2) of integer;
    constant kernel : kernel_array := (
        ( -1, -1, -1 ),
        ( -1,  8, -1 ),
        ( -1, -1, -1 )
    );

begin

    process(clk)
        variable conv_sum : integer := 0;
    begin
        if rising_edge(clk) then
            if rst = '1' then
                count <= 0;
                valid_out <= '0';
                window <= (others => (others => (others => '0')));
            elsif valid_in = '1' then
                -- Shift window pixels
                window(0)(0) <= window(0)(1);
                window(0)(1) <= window(0)(2);
                window(0)(2) <= line2;

                window(1)(0) <= window(1)(1);
                window(1)(1) <= window(1)(2);
                window(1)(2) <= line1;

                window(2)(0) <= window(2)(1);
                window(2)(1) <= window(2)(2);
                window(2)(2) <= pixel_in;

                -- Shift line buffers
                line2 <= line1;
                line1 <= pixel_in;

                count <= count + 1;

                -- Wait until at least 9 pixels received
                if count > 4 then
                    conv_sum := 0;
                    for i in 0 to 2 loop
                        for j in 0 to 2 loop
                            conv_sum := conv_sum + to_integer(signed(('0' & window(i)(j)))) * kernel(i)(j);
                        end loop;
                    end loop;
                    pixel_out <= std_logic_vector(to_signed(conv_sum, 16));
                    valid_out <= '1';
                else
                    valid_out <= '0';
                end if;
            else
                valid_out <= '0';
            end if;
        end if;
    end process;

end Behavioral;
