-- Testbench for convolution_block.
--
-- convolution_block addresses its input/weight memories with a 16-bit bus, so
-- at most 65536 elements (INPUT_HEIGHT*INPUT_WIDTH*INPUT_CHANNELS) can be
-- addressed. For a 256x256 test image (65536 pixels) this means only a single
-- channel "plane" can be processed per simulation run -- the driving Python
-- script (run_convolution_tb.py) runs this testbench once per R/G/B plane and
-- recombines the filtered planes into the output image.
--
-- Input image plane, kernel weights and bias are read from plain text files
-- (one signed integer per line, Q8.8 fixed point). The filtered plane is
-- written back out the same way.

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use STD.TEXTIO.ALL;
use work.accel_types.all;

entity convolution_tb is
    generic (
        INPUT_HEIGHT    : integer := 256;
        INPUT_WIDTH     : integer := 256;
        INPUT_CHANNELS  : integer := 1;
        OUTPUT_CHANNELS : integer := 1;
        KERNEL_SIZE     : integer := 3;
        STRIDE          : integer := 1;
        PADDING         : integer := 1;
        OUTPUT_HEIGHT   : integer := 256;
        OUTPUT_WIDTH    : integer := 256;
        INPUT_FILE      : string := "input.txt";
        WEIGHT_FILE     : string := "weight.txt";
        BIAS_FILE       : string := "bias.txt";
        OUTPUT_FILE     : string := "output.txt";
        CLK_PERIOD      : time := 10 ns
    );
end entity convolution_tb;

architecture sim of convolution_tb is

    signal clk   : std_logic := '0';
    signal rst   : std_logic := '1';
    signal start : std_logic := '0';
    signal done  : std_logic;
    signal ready : std_logic;

    signal config     : conv_config_t;
    signal batch_size : integer range 1 to MAX_BATCH_SIZE := 1;

    signal input_valid    : std_logic;
    signal input_data     : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal input_addr     : std_logic_vector(15 downto 0);
    signal input_read_en  : std_logic;

    signal weight_valid   : std_logic;
    signal weight_data    : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal weight_addr    : std_logic_vector(15 downto 0);
    signal weight_read_en : std_logic;

    signal bias_valid   : std_logic;
    signal bias_data    : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal bias_addr    : std_logic_vector(7 downto 0);
    signal bias_read_en : std_logic;

    signal output_valid    : std_logic;
    signal output_data     : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal output_addr     : std_logic_vector(15 downto 0);
    signal output_write_en : std_logic;

    constant INPUT_ELEMENTS  : integer := INPUT_HEIGHT * INPUT_WIDTH * INPUT_CHANNELS;
    constant WEIGHT_ELEMENTS : integer := KERNEL_SIZE * KERNEL_SIZE * INPUT_CHANNELS * OUTPUT_CHANNELS;
    constant BIAS_ELEMENTS   : integer := OUTPUT_CHANNELS;
    constant OUTPUT_ELEMENTS : integer := OUTPUT_HEIGHT * OUTPUT_WIDTH * OUTPUT_CHANNELS;

    signal input_mem  : data_array_1d(0 to INPUT_ELEMENTS-1);
    signal weight_mem : data_array_1d(0 to WEIGHT_ELEMENTS-1);
    signal bias_mem   : data_array_1d(0 to BIAS_ELEMENTS-1);
    signal output_mem : data_array_1d(0 to OUTPUT_ELEMENTS-1);

    signal sim_done : boolean := false;

begin

    assert INPUT_ELEMENTS <= 65536
        report "convolution_tb: INPUT_HEIGHT*INPUT_WIDTH*INPUT_CHANNELS exceeds the 16-bit address space of convolution_block"
        severity failure;

    config.kernel_size     <= KERNEL_SIZE;
    config.stride          <= STRIDE;
    config.padding         <= PADDING;
    config.input_channels  <= INPUT_CHANNELS;
    config.output_channels <= OUTPUT_CHANNELS;

    -- Clock generation
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

    -- Load input plane, kernel weights and bias from text files
    load_files: process
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

        file_open(f, WEIGHT_FILE, read_mode);
        for i in weight_mem'range loop
            readline(f, l);
            read(l, v);
            weight_mem(i) <= std_logic_vector(to_signed(v, DATA_WIDTH));
        end loop;
        file_close(f);

        file_open(f, BIAS_FILE, read_mode);
        for i in bias_mem'range loop
            readline(f, l);
            read(l, v);
            bias_mem(i) <= std_logic_vector(to_signed(v, DATA_WIDTH));
        end loop;
        file_close(f);

        wait;
    end process;

    -- Behavioral ROM reads (zero-latency, combinational - addresses come from
    -- registered counters so this models a synchronous memory faithfully)
    input_data <= input_mem(to_integer(unsigned(input_addr)))
                  when to_integer(unsigned(input_addr)) <= input_mem'high
                  else (others => '0');
    input_valid <= input_read_en;

    weight_data <= weight_mem(to_integer(unsigned(weight_addr)))
                   when to_integer(unsigned(weight_addr)) <= weight_mem'high
                   else (others => '0');
    weight_valid <= weight_read_en;

    bias_data <= bias_mem(to_integer(unsigned(bias_addr)))
                 when to_integer(unsigned(bias_addr)) <= bias_mem'high
                 else (others => '0');
    bias_valid <= bias_read_en;

    -- Output capture
    capture_output: process(clk)
    begin
        if rising_edge(clk) then
            if output_write_en = '1' and to_integer(unsigned(output_addr)) <= output_mem'high then
                output_mem(to_integer(unsigned(output_addr))) <= output_data;
            end if;
        end if;
    end process;

    -- Device under test
    dut: entity work.convolution_block
        generic map (
            INPUT_HEIGHT => INPUT_HEIGHT,
            INPUT_WIDTH  => INPUT_WIDTH
        )
        port map (
            clk             => clk,
            rst             => rst,
            start           => start,
            done            => done,
            ready           => ready,
            config          => config,
            batch_size      => batch_size,
            input_valid     => input_valid,
            input_data      => input_data,
            input_addr      => input_addr,
            input_read_en   => input_read_en,
            weight_valid    => weight_valid,
            weight_data     => weight_data,
            weight_addr     => weight_addr,
            weight_read_en  => weight_read_en,
            bias_valid      => bias_valid,
            bias_data       => bias_data,
            bias_addr       => bias_addr,
            bias_read_en    => bias_read_en,
            output_valid    => output_valid,
            output_data     => output_data,
            output_addr     => output_addr,
            output_write_en => output_write_en
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

    -- Wait for completion, dump results, end simulation
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

        report "convolution_tb: simulation complete, output written to " & OUTPUT_FILE;
        sim_done <= true;
        wait for CLK_PERIOD*2;
        std.env.stop(0);
        wait;
    end process;

end architecture sim;
