library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;

entity tb_conv_file is
end tb_conv_file;

architecture sim of tb_conv_file is
    constant CHANNELS : integer := 3;
    constant BPP      : integer := 8;

    -- Signals
    signal clk         : std_logic := '0';
    signal rst         : std_logic := '1';
    signal enable      : std_logic := '0';
    signal pixel_in    : std_logic_vector(CHANNELS*BPP-1 downto 0) := (others=>'0');
    signal pixel_valid : std_logic := '0';
    signal conv_out    : std_logic_vector((CHANNELS*16)-1 downto 0);
    signal out_valid   : std_logic;

    -- Input buffer
    constant MAX_PIXELS : integer := 196608;  -- podržava velike slike
    type pixel_array_t is array (0 to MAX_PIXELS-1) of integer range 0 to 255;
    signal img_pixels : pixel_array_t;

    -- Image dimensions (mora odgovarati slici)
    constant IMG_WIDTH  : integer := 256;
    constant IMG_HEIGHT : integer := 256;

begin

    -- Clock 100 MHz
    clk <= not clk after 5 ns;

    -- DUT
    uut: entity work.convolution_block
        generic map (
            IMG_WIDTH           => IMG_WIDTH,
            IMG_HEIGHT          => IMG_HEIGHT,
            CHANNELS            => CHANNELS,
            PIXEL_CHANNEL_WIDTH => BPP
        )
        port map (
            clk         => clk,
            rst         => rst,
            enable      => enable,
            pixel_in    => pixel_in,
            pixel_valid => pixel_valid,
            conv_out    => conv_out,
            out_valid   => out_valid
        );

    ----------------------------------------------------------------
    -- Stimulus: load input file
    ----------------------------------------------------------------
    stim: process
        file fin  : text open read_mode is "image.txt";
        variable line_in : line;
        variable val     : integer;
        variable i       : integer := 0;
        variable pixel_idx : integer := 0;
    begin
        -- Reset
        rst <= '1';
        wait for 20 ns;
        rst <= '0';
        enable <= '1';

        -- Read pixels
        while not endfile(fin) loop
            readline(fin, line_in);
            read(line_in, val);
            img_pixels(i) <= val;
            i := i + 1;
        end loop;

        -- Send pixels to DUT (3 channels per pixel)
        pixel_idx := 0;
        while pixel_idx < i loop
            pixel_valid <= '1';
            pixel_in <= std_logic_vector(to_unsigned(img_pixels(pixel_idx), BPP)) &
                        std_logic_vector(to_unsigned(img_pixels(pixel_idx+1), BPP)) &
                        std_logic_vector(to_unsigned(img_pixels(pixel_idx+2), BPP));
            wait until rising_edge(clk);
            pixel_valid <= '0';
            wait until rising_edge(clk);
            pixel_idx := pixel_idx + 3;
        end loop;

        wait;  -- end process
    end process;

    ----------------------------------------------------------------
    -- Monitor: write conv_out to output.txt
    ----------------------------------------------------------------
    monitor: process(clk)
        file fout : text open write_mode is "output.txt";
        variable line_out : line;
        variable r_val, g_val, b_val : integer;
    begin
        if rising_edge(clk) then
            if out_valid = '1' then
                r_val := to_integer(signed(conv_out(15 downto 0)));
                g_val := to_integer(signed(conv_out(31 downto 16)));
                b_val := to_integer(signed(conv_out(47 downto 32)));

                write(line_out, r_val);
                write(line_out, string'(" "));
                write(line_out, g_val);
                write(line_out, string'(" "));
                write(line_out, b_val);
                writeline(fout, line_out);
            end if;
        end if;
    end process;

end sim;
