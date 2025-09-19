library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;

entity tb_max_pool is
end tb_max_pool;

architecture sim of tb_max_pool is
    constant DATA_WIDTH : integer := 8;
    constant IMG_WIDTH  : integer := 256;
    constant IMG_HEIGHT : integer := 256;

    signal clk        : std_logic := '0';
    signal reset      : std_logic := '1';
    signal valid_in   : std_logic := '0';
    signal pixel_r_in : unsigned(DATA_WIDTH-1 downto 0) := (others=>'0');
    signal pixel_g_in : unsigned(DATA_WIDTH-1 downto 0) := (others=>'0');
    signal pixel_b_in : unsigned(DATA_WIDTH-1 downto 0) := (others=>'0');

    signal pooled_r_out : unsigned(DATA_WIDTH-1 downto 0);
    signal pooled_g_out : unsigned(DATA_WIDTH-1 downto 0);
    signal pooled_b_out : unsigned(DATA_WIDTH-1 downto 0);
    signal valid_out    : std_logic;

    -- Input and output files
    file fin  : text open read_mode is "image.txt";
    file fout : text open write_mode is "output.txt";
begin
    clk <= not clk after 5 ns;

    -- DUT instantiation
    uut: entity work.max_pool
        generic map (
            DATA_WIDTH => DATA_WIDTH,
            IMG_WIDTH  => IMG_WIDTH
        )
        port map (
            clk        => clk,
            reset      => reset,
            valid_in   => valid_in,
            pixel_r_in => pixel_r_in,
            pixel_g_in => pixel_g_in,
            pixel_b_in => pixel_b_in,
            pooled_r_out => pooled_r_out,
            pooled_g_out => pooled_g_out,
            pooled_b_out => pooled_b_out,
            valid_out    => valid_out
        );

    -- Stimulus process for planar RGB input
    stim: process
        variable line_in : line;
        variable r_val, g_val, b_val : integer;
    begin
        reset <= '1';
        wait for 20 ns;
        reset <= '0';
        wait for 10 ns;

        while not endfile(fin) loop
            -- Read R
            readline(fin, line_in);
            read(line_in, r_val);
            pixel_r_in <= to_unsigned(r_val, DATA_WIDTH);
            -- Read G
            readline(fin, line_in);
            read(line_in, g_val);
            pixel_g_in <= to_unsigned(g_val, DATA_WIDTH);
            -- Read B
            readline(fin, line_in);
            read(line_in, b_val);
            pixel_b_in <= to_unsigned(b_val, DATA_WIDTH);

            valid_in <= '1';
            wait for 10 ns;
        end loop;

        valid_in <= '0';
        wait for 500 ns;
        std.env.stop;
        wait;
    end process;

    -- Monitor process (writes pooled RGB output)
    monitor: process(clk)
    variable line_out : line;
    begin
        if rising_edge(clk) then
            if valid_out = '1' then
                -- Write one line per pixel: R G B
                write(line_out, to_integer(pooled_r_out), right, 4);
                write(line_out, ' ');
                write(line_out, to_integer(pooled_g_out), right, 4);
                write(line_out, ' ');
                write(line_out, to_integer(pooled_b_out), right, 4);
                writeline(fout, line_out);
            end if;
        end if;
    end process;

end sim;
