library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use std.textio.all;
use std.env.all;
use work.accel_types.all;

entity activation_image_tb is
    generic (
        IMG_WIDTH   : integer := 256;
        IMG_HEIGHT  : integer := 256;
        INPUT_FILE  : string := "image.txt";
        OUTPUT_FILE : string := "activation_output.txt"
    );
end activation_image_tb;

architecture sim of activation_image_tb is
    constant CLK_PERIOD     : time := 10 ns;
    constant PARALLEL_UNITS : integer := 4;
    constant INPUT_COUNT    : integer := IMG_WIDTH * IMG_HEIGHT * 3;

    type sample_array_t is array (natural range <>) of integer range 0 to 255;

    impure function load_samples return sample_array_t is
        file input_file_handle : text;
        variable status        : file_open_status;
        variable input_line    : line;
        variable sample_value  : integer;
        variable sample_ok     : boolean;
        variable samples       : sample_array_t(0 to INPUT_COUNT-1);
        variable sample_index  : integer := 0;
    begin
        file_open(status, input_file_handle, INPUT_FILE, read_mode);
        assert status = open_ok
            report "Cannot open activation input file: " & INPUT_FILE severity failure;

        while not endfile(input_file_handle) and sample_index < INPUT_COUNT loop
            readline(input_file_handle, input_line);
            read(input_line, sample_value, sample_ok);
            assert sample_ok and sample_value >= 0 and sample_value <= 255
                report "Invalid RGB sample at index " & integer'image(sample_index)
                severity failure;
            samples(sample_index) := sample_value;
            sample_index := sample_index + 1;
        end loop;
        file_close(input_file_handle);

        assert sample_index = INPUT_COUNT
            report "Expected " & integer'image(INPUT_COUNT) & " image samples, read " &
                   integer'image(sample_index) severity failure;
        return samples;
    end function;

    constant samples : sample_array_t(0 to INPUT_COUNT-1) := load_samples;

    signal clk               : std_logic := '0';
    signal rst               : std_logic := '1';
    signal start             : std_logic := '0';
    signal done              : std_logic;
    signal ready             : std_logic;
    signal busy              : std_logic;
    signal tensor_size       : integer range 1 to MAX_TENSOR_SIZE := 1;
    signal chunk_base        : integer range 0 to INPUT_COUNT-1 := 0;
    signal input_valid       : std_logic;
    signal input_data        : data_array_t(0 to PARALLEL_UNITS-1);
    signal input_addr        : std_logic_vector(15 downto 0);
    signal input_read_en     : std_logic;
    signal output_valid      : std_logic;
    signal output_data       : data_array_t(0 to PARALLEL_UNITS-1);
    signal output_addr       : std_logic_vector(15 downto 0);
    signal output_write_en   : std_logic;
    signal current_element   : std_logic_vector(15 downto 0);
    signal processing_cycles : std_logic_vector(31 downto 0);
begin
    clk <= not clk after CLK_PERIOD/2;
    input_valid <= input_read_en;

    input_memory : process(input_addr, chunk_base)
        variable base_addr : integer;
        variable sample_idx : integer;
    begin
        if is_x(input_addr) then
            for lane in 0 to PARALLEL_UNITS-1 loop
                input_data(lane) <= (others => '0');
            end loop;
        else
            base_addr := chunk_base + to_integer(unsigned(input_addr));
            for lane in 0 to PARALLEL_UNITS-1 loop
                sample_idx := base_addr + lane;
                if sample_idx < INPUT_COUNT then
                    input_data(lane) <= std_logic_vector(to_unsigned(samples(sample_idx), DATA_WIDTH));
                else
                    input_data(lane) <= (others => '0');
                end if;
            end loop;
        end if;
    end process;

    dut : entity work.activation_block
        generic map (
            PIPELINE_STAGES => 3,
            PARALLEL_UNITS  => PARALLEL_UNITS
        )
        port map (
            clk               => clk,
            rst               => rst,
            start             => start,
            done              => done,
            ready             => ready,
            busy              => busy,
            activation_type   => RELU,
            tensor_size       => tensor_size,
            alpha             => (others => '0'),
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

    stimulus : process
        file output_file_handle : text;
        variable output_line    : line;
        variable base_index     : integer := 0;
        variable chunk_size     : integer;
        variable output_count   : integer;
        variable cycle_count    : integer;
        variable sample_valid   : std_logic;
    begin
        rst <= '1';
        wait for 5 * CLK_PERIOD;
        wait until rising_edge(clk);
        rst <= '0';
        file_open(output_file_handle, OUTPUT_FILE, write_mode);

        while base_index < INPUT_COUNT loop
            chunk_size := INPUT_COUNT - base_index;
            if chunk_size > MAX_TENSOR_SIZE then
                chunk_size := MAX_TENSOR_SIZE;
            end if;
            chunk_base <= base_index;
            tensor_size <= chunk_size;

            wait until rising_edge(clk) and ready = '1';
            start <= '1';
            wait until rising_edge(clk);
            start <= '0';
            output_count := 0;
            cycle_count := 0;

            while done /= '1' loop
                wait until rising_edge(clk);
                sample_valid := output_valid;
                wait for 1 ns;
                if sample_valid = '1' then
                    for lane in 0 to PARALLEL_UNITS-1 loop
                        if output_count < chunk_size then
                            write(output_line, to_integer(signed(output_data(lane))));
                            writeline(output_file_handle, output_line);
                            output_count := output_count + 1;
                        end if;
                    end loop;
                end if;
                cycle_count := cycle_count + 1;
                assert cycle_count < MAX_TENSOR_SIZE + 2000
                    report "Timeout during image activation" severity failure;
            end loop;

            assert output_count = chunk_size
                report "Activation chunk produced " & integer'image(output_count) &
                       " values; expected " & integer'image(chunk_size)
                severity failure;
            base_index := base_index + chunk_size;
        end loop;

        file_close(output_file_handle);
        report "Image RELU passed for " & integer'image(INPUT_COUNT) & " values" severity note;
        stop;
        wait;
    end process;
end sim;