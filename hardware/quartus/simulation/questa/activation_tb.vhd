-- Testbench for activation_block.
--
-- activation_block only accepts tensor_size <= MAX_TENSOR_SIZE (4096 elements),
-- so a flattened 256x256x3 image (196608 elements) cannot be processed in a
-- single run. This testbench loops internally over TOTAL_ELEMENTS/CHUNK_SIZE
-- chunks, re-pulsing 'start' for each chunk (the LUTs used by SIGMOID/TANH/ELU
-- are only initialized once, on the first chunk).
--
-- The flattened image is read from a text file (one signed integer per line,
-- in [height][width][channel] order). The activated output is written back
-- out the same way.

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use STD.TEXTIO.ALL;
use work.accel_types.all;

entity activation_tb is
    generic (
        TOTAL_ELEMENTS  : integer := 196608;  -- 256*256*3
        CHUNK_SIZE      : integer := 4096;     -- <= MAX_TENSOR_SIZE, must divide TOTAL_ELEMENTS
        PIPELINE_STAGES : integer := 3;
        PARALLEL_UNITS  : integer := 4;
        ACT_TYPE_STR    : string := "RELU";    -- RELU | SIGMOID | TANH | LINEAR | LEAKY_RELU | ELU
        ALPHA_RAW       : integer := 26;       -- ~0.1 in Q8.8, used by LEAKY_RELU/ELU
        INPUT_FILE      : string := "input.txt";
        OUTPUT_FILE     : string := "output.txt";
        CLK_PERIOD      : time := 10 ns
    );
end entity activation_tb;

architecture sim of activation_tb is

    function to_activation_type(s : string) return activation_type_t is
    begin
        if s = "RELU" then
            return RELU;
        elsif s = "SIGMOID" then
            return SIGMOID;
        elsif s = "TANH" then
            return TANH;
        elsif s = "LINEAR" then
            return LINEAR;
        elsif s = "LEAKY_RELU" then
            return LEAKY_RELU;
        elsif s = "ELU" then
            return ELU;
        else
            report "activation_tb: unknown ACT_TYPE_STR '" & s & "'" severity failure;
            return RELU;
        end if;
    end function;

    constant NUM_CHUNKS : integer := TOTAL_ELEMENTS / CHUNK_SIZE;

    signal clk   : std_logic := '0';
    signal rst   : std_logic := '1';
    signal start : std_logic := '0';
    signal done  : std_logic;
    signal ready : std_logic;
    signal busy  : std_logic;

    signal tensor_size : integer range 1 to MAX_TENSOR_SIZE := CHUNK_SIZE;
    signal alpha       : std_logic_vector(DATA_WIDTH-1 downto 0);

    signal input_valid   : std_logic;
    signal input_data    : data_array_t(0 to PARALLEL_UNITS-1);
    signal input_addr    : std_logic_vector(15 downto 0);
    signal input_read_en : std_logic;

    signal output_valid    : std_logic;
    signal output_data     : data_array_t(0 to PARALLEL_UNITS-1);
    signal output_addr     : std_logic_vector(15 downto 0);
    signal output_write_en : std_logic;

    signal current_element   : std_logic_vector(15 downto 0);
    signal processing_cycles : std_logic_vector(31 downto 0);

    signal input_mem  : data_array_1d(0 to TOTAL_ELEMENTS-1);
    signal output_mem : data_array_1d(0 to TOTAL_ELEMENTS-1);

    signal chunk_offset    : integer := 0;
    signal all_chunks_done : boolean := false;
    signal sim_done        : boolean := false;

begin

    assert TOTAL_ELEMENTS mod CHUNK_SIZE = 0
        report "activation_tb: CHUNK_SIZE must evenly divide TOTAL_ELEMENTS"
        severity failure;

    alpha <= std_logic_vector(to_signed(ALPHA_RAW, DATA_WIDTH));

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

    -- ROM read: parallel units map to consecutive absolute addresses
    -- (offset by the current chunk's base position in the full image)
    rom_read: process(input_addr, chunk_offset)
        variable base : integer;
    begin
        base := chunk_offset + to_integer(unsigned(input_addr));
        for i in input_data'range loop
            if base + i <= input_mem'high then
                input_data(i) <= input_mem(base + i);
            else
                input_data(i) <= (others => '0');
            end if;
        end loop;
    end process;
    input_valid <= input_read_en;

    capture_output: process(clk)
        variable base : integer;
    begin
        if rising_edge(clk) then
            if output_write_en = '1' then
                base := chunk_offset + to_integer(unsigned(output_addr));
                for i in output_data'range loop
                    if base + i <= output_mem'high then
                        output_mem(base + i) <= output_data(i);
                    end if;
                end loop;
            end if;
        end if;
    end process;

    dut: entity work.activation_block
        generic map (
            PIPELINE_STAGES => PIPELINE_STAGES,
            PARALLEL_UNITS  => PARALLEL_UNITS
        )
        port map (
            clk               => clk,
            rst               => rst,
            start             => start,
            done              => done,
            ready             => ready,
            busy              => busy,
            activation_type   => to_activation_type(ACT_TYPE_STR),
            tensor_size       => tensor_size,
            alpha             => alpha,
            input_valid       => input_valid,
            input_data        => input_data,
            input_addr        => input_addr,
            input_read_en     => input_read_en,
            output_valid      => output_valid,
            output_data       => output_data,
            output_addr       => output_addr,
            output_write_en   => output_write_en,
            current_element   => current_element,
            processing_cycles => processing_cycles
        );

    -- Reset once, then re-pulse start for each chunk
    stimulus: process
    begin
        rst <= '1';
        start <= '0';
        wait for CLK_PERIOD*4;
        rst <= '0';
        wait for CLK_PERIOD*2;

        for chunk in 0 to NUM_CHUNKS-1 loop
            chunk_offset <= chunk * CHUNK_SIZE;
            wait until rising_edge(clk);
            start <= '1';
            wait until rising_edge(clk);
            start <= '0';
            wait until done = '1';
            wait until rising_edge(clk);
        end loop;

        all_chunks_done <= true;
        wait;
    end process;

    finish: process
        variable l : line;
        file f : text;
    begin
        wait until all_chunks_done;
        wait for CLK_PERIOD*2;

        file_open(f, OUTPUT_FILE, write_mode);
        for i in output_mem'range loop
            write(l, to_integer(signed(output_mem(i))));
            writeline(f, l);
        end loop;
        file_close(f);

        report "activation_tb: simulation complete, output written to " & OUTPUT_FILE;
        sim_done <= true;
        wait for CLK_PERIOD*2;
        std.env.stop(0);
        wait;
    end process;

end architecture sim;
