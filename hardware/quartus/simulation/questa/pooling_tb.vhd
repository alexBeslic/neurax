-- Testbench for pooling_block.
--
-- pooling_block addresses its input/output memories with an 18-bit bus, so a
-- 256x256x3 image (196608 elements) fits in a single run with all channels
-- processed in parallel (PARALLEL_CHANNELS = CHANNELS).
--
-- The input image is read from a text file (one signed integer per line, in
-- flattened [height][width][channel] order, matching a raw RGB-interleaved
-- pixel dump). The pooled output is written back out the same way.

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use STD.TEXTIO.ALL;
use work.accel_types.all;

entity pooling_tb is
    generic (
        INPUT_HEIGHT  : integer := 256;
        INPUT_WIDTH   : integer := 256;
        CHANNELS      : integer := 3;
        POOL_SIZE     : integer := 2;
        STRIDE        : integer := 2;
        POOL_TYPE_STR : string := "MAX_POOL";  -- MAX_POOL | AVERAGE_POOL | MIN_POOL | SUM_POOL
        OUTPUT_HEIGHT : integer := 128;
        OUTPUT_WIDTH  : integer := 128;
        INPUT_FILE    : string := "input.txt";
        OUTPUT_FILE   : string := "output.txt";
        CLK_PERIOD    : time := 10 ns
    );
end entity pooling_tb;

architecture sim of pooling_tb is

    function to_pool_type(s : string) return pooling_type_t is
    begin
        if s = "MAX_POOL" then
            return MAX_POOL;
        elsif s = "AVERAGE_POOL" then
            return AVERAGE_POOL;
        elsif s = "MIN_POOL" then
            return MIN_POOL;
        elsif s = "SUM_POOL" then
            return SUM_POOL;
        else
            report "pooling_tb: unknown POOL_TYPE_STR '" & s & "'" severity failure;
            return MAX_POOL;
        end if;
    end function;

    signal clk   : std_logic := '0';
    signal rst   : std_logic := '1';
    signal start : std_logic := '0';
    signal done  : std_logic;
    signal ready : std_logic;
    signal busy  : std_logic;

    signal config     : pooling_config_t;
    signal batch_size : integer range 1 to MAX_BATCH_SIZE := 1;

    signal input_valid   : std_logic;
    signal input_data    : data_array_1d(0 to CHANNELS-1);
    signal input_addr    : std_logic_vector(17 downto 0);
    signal input_read_en : std_logic;

    signal output_valid    : std_logic;
    signal output_data     : data_array_1d(0 to CHANNELS-1);
    signal output_addr     : std_logic_vector(17 downto 0);
    signal output_write_en : std_logic;

    signal current_position  : std_logic_vector(31 downto 0);
    signal processing_cycles : std_logic_vector(31 downto 0);
    signal pool_window_count : std_logic_vector(15 downto 0);

    constant INPUT_ELEMENTS  : integer := INPUT_HEIGHT * INPUT_WIDTH * CHANNELS;
    constant OUTPUT_ELEMENTS : integer := OUTPUT_HEIGHT * OUTPUT_WIDTH * CHANNELS;

    signal input_mem  : data_array_1d(0 to INPUT_ELEMENTS-1);
    signal output_mem : data_array_1d(0 to OUTPUT_ELEMENTS-1);

    signal sim_done : boolean := false;

begin

    assert INPUT_ELEMENTS <= 262144
        report "pooling_tb: INPUT_HEIGHT*INPUT_WIDTH*CHANNELS exceeds the 18-bit address space of pooling_block"
        severity failure;

    config.pool_size <= POOL_SIZE;
    config.stride    <= STRIDE;
    config.pool_type <= to_pool_type(POOL_TYPE_STR);
    config.channels  <= CHANNELS;

    clk_gen: process
    begin
        while not sim_done loop
            clk <= '0';
            wait for CLK_PERIOD/2;
            clk <= '1';
            wait for CLK_PERIOD/2;
        end loop;
        wait;
    end process;

    load_input: process
        variable l : line;
        variable v : integer;
        file f : text;
    begin
        file_open(f, INPUT_FILE, read_mode);
        for i in input_mem'range loop
            readline(f, l);
            read(l, v);
            input_mem(i) <= std_logic_vector(to_signed(v, DATA_WIDTH));
        end loop;
        file_close(f);
        wait;
    end process;

    -- ROM read: PARALLEL_CHANNELS worth of data supplied simultaneously,
    -- starting at the address the DUT provides (base = pixel's first channel)
    rom_read: process(input_addr)
        variable base : integer;
    begin
        base := to_integer(unsigned(input_addr));
        for c in input_data'range loop
            if base + c <= input_mem'high then
                input_data(c) <= input_mem(base + c);
            else
                input_data(c) <= (others => '0');
            end if;
        end loop;
    end process;
    input_valid <= input_read_en;

    capture_output: process(clk)
        variable base : integer;
    begin
        if rising_edge(clk) then
            if output_write_en = '1' then
                base := to_integer(unsigned(output_addr));
                for c in output_data'range loop
                    if base + c <= output_mem'high then
                        output_mem(base + c) <= output_data(c);
                    end if;
                end loop;
            end if;
        end if;
    end process;

    dut: entity work.pooling_block
        generic map (
            INPUT_HEIGHT      => INPUT_HEIGHT,
            INPUT_WIDTH       => INPUT_WIDTH,
            PARALLEL_CHANNELS => CHANNELS
        )
        port map (
            clk               => clk,
            rst               => rst,
            start             => start,
            done              => done,
            ready             => ready,
            busy              => busy,
            config            => config,
            batch_size        => batch_size,
            input_valid       => input_valid,
            input_data        => input_data,
            input_addr        => input_addr,
            input_read_en     => input_read_en,
            output_valid      => output_valid,
            output_data       => output_data,
            output_addr       => output_addr,
            output_write_en   => output_write_en,
            current_position  => current_position,
            processing_cycles => processing_cycles,
            pool_window_count => pool_window_count
        );

    -- Reset then pulse start once
    stimulus: process
    begin
        rst <= '1';
        start <= '0';
        wait for CLK_PERIOD*4;
        rst <= '0';
        wait for CLK_PERIOD*2;
        wait until rising_edge(clk);
        start <= '1';
        wait until rising_edge(clk);
        start <= '0';
        wait;
    end process;

    finish: process
        variable l : line;
        file f : text;
    begin
        wait until done = '1';
        wait for CLK_PERIOD*2;

        file_open(f, OUTPUT_FILE, write_mode);
        for i in output_mem'range loop
            write(l, to_integer(signed(output_mem(i))));
            writeline(f, l);
        end loop;
        file_close(f);

        report "pooling_tb: simulation complete, output written to " & OUTPUT_FILE;
        sim_done <= true;
        wait for CLK_PERIOD*2;
        std.env.stop(0);
        wait;
    end process;

end architecture sim;
