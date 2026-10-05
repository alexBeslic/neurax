library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use std.textio.all;
use std.env.all;
use work.accel_types.all;

entity conv_activation_tb is
    generic (
        RUN_CONVOLUTION : boolean := true;
        RUN_ACTIVATION  : boolean := true;
        IMAGE_MODE      : boolean := false;
        IMG_WIDTH       : integer := 4;
        IMG_HEIGHT      : integer := 4;
        INPUT_FILE      : string := "image.txt";
        OUTPUT_FILE     : string := "output.txt"
    );
end conv_activation_tb;

architecture sim of conv_activation_tb is
    constant CLK_PERIOD     : time := 10 ns;
    constant PARALLEL_UNITS : integer := 4;

    function configured_channels return integer is
    begin
        if IMAGE_MODE then return 3; else return 1; end if;
    end function;

    function configured_kernel_size return integer is
    begin
        if IMAGE_MODE then return 3; else return 1; end if;
    end function;

    function configured_padding return integer is
    begin
        if IMAGE_MODE then return 1; else return 0; end if;
    end function;

    constant CHANNELS       : integer := configured_channels;
    constant KERNEL_SIZE    : integer := configured_kernel_size;
    constant PADDING        : integer := configured_padding;
    constant TENSOR_SIZE    : integer := IMG_WIDTH * IMG_HEIGHT * CHANNELS;
    constant UNWRITTEN      : integer := 32767;

    type sample_mem_t is array (0 to TENSOR_SIZE-1) of integer;
    type weight_mem_t is array (0 to KERNEL_SIZE*KERNEL_SIZE*CHANNELS*CHANNELS-1) of integer;

    impure function load_input return sample_mem_t is
        file input_handle : text;
        variable status : file_open_status;
        variable input_line : line;
        variable sample_value : integer;
        variable sample_ok : boolean;
        variable values : sample_mem_t := (others => 0);
    begin
        if IMAGE_MODE then
            file_open(status, input_handle, INPUT_FILE, read_mode);
            assert status = open_ok
                report "Cannot open input file: " & INPUT_FILE severity failure;
            for index in 0 to TENSOR_SIZE-1 loop
                assert not endfile(input_handle)
                    report "Input file ended before all RGB samples were read" severity failure;
                readline(input_handle, input_line);
                read(input_line, sample_value, sample_ok);
                assert sample_ok and sample_value >= 0 and sample_value <= 255
                    report "Invalid RGB value at index " & integer'image(index) severity failure;
                values(index) := sample_value;
            end loop;
            file_close(input_handle);
        else
            for index in 0 to TENSOR_SIZE-1 loop
                values(index) := index + 1;
            end loop;
        end if;
        return values;
    end function;

    constant input_mem : sample_mem_t := load_input;

    function make_weights return weight_mem_t is
        variable values : weight_mem_t := (others => 0);
        variable weight_value : integer;
    begin
        if IMAGE_MODE then
            for ky in 0 to KERNEL_SIZE-1 loop
                for kx in 0 to KERNEL_SIZE-1 loop
                    for in_ch in 0 to CHANNELS-1 loop
                        for out_ch in 0 to CHANNELS-1 loop
                            weight_value := 0;
                            if in_ch = out_ch then
                                if kx = 0 then
                                    weight_value := 2**FRAC_WIDTH;
                                elsif kx = 2 then
                                    weight_value := -(2**FRAC_WIDTH);
                                end if;
                            end if;
                            values(((ky*KERNEL_SIZE+kx)*CHANNELS+in_ch)*CHANNELS+out_ch) := weight_value;
                        end loop;
                    end loop;
                end loop;
            end loop;
        else
            values(0) := -(2**FRAC_WIDTH);
        end if;
        return values;
    end function;

    function initial_results return sample_mem_t is
        variable values : sample_mem_t := (others => UNWRITTEN);
    begin
        if not RUN_CONVOLUTION then
            for index in 0 to TENSOR_SIZE-1 loop
                if IMAGE_MODE then
                    values(index) := input_mem(index);
                else
                    values(index) := -(index + 1);
                end if;
            end loop;
        end if;
        return values;
    end function;

    constant weight_mem : weight_mem_t := make_weights;
    signal conv_results : sample_mem_t := initial_results;

    signal clk                 : std_logic := '0';
    signal rst                 : std_logic := '1';
    signal conv_enable         : std_logic := '0';
    signal activation_enable   : std_logic := '0';
    signal conv_done           : std_logic;
    signal conv_ready          : std_logic;
    signal conv_config         : conv_config_t := (
        kernel_size => KERNEL_SIZE,
        stride => 1,
        padding => PADDING,
        input_channels => CHANNELS,
        output_channels => CHANNELS
    );
    signal batch_size          : integer range 1 to MAX_BATCH_SIZE := 1;
    signal conv_input_valid    : std_logic;
    signal conv_input_data     : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal conv_input_addr     : std_logic_vector(15 downto 0);
    signal conv_input_read_en  : std_logic;
    signal weight_valid        : std_logic;
    signal weight_data         : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal weight_addr         : std_logic_vector(15 downto 0);
    signal weight_read_en      : std_logic;
    signal bias_valid          : std_logic;
    signal bias_data           : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal bias_addr           : std_logic_vector(7 downto 0);
    signal bias_read_en        : std_logic;
    signal conv_output_valid   : std_logic;
    signal conv_output_data    : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal conv_output_addr    : std_logic_vector(15 downto 0);
    signal conv_output_write   : std_logic;
    signal act_done            : std_logic;
    signal act_ready           : std_logic;
    signal act_busy            : std_logic;
    signal act_tensor_size     : integer range 1 to MAX_TENSOR_SIZE := 1;
    signal act_chunk_base      : integer range 0 to TENSOR_SIZE-1 := 0;
    signal act_input_valid     : std_logic;
    signal act_input_data      : data_array_t(0 to PARALLEL_UNITS-1);
    signal act_input_addr      : std_logic_vector(15 downto 0);
    signal act_input_read_en   : std_logic;
    signal act_output_valid    : std_logic;
    signal act_output_data     : data_array_t(0 to PARALLEL_UNITS-1);
    signal act_output_addr     : std_logic_vector(15 downto 0);
    signal act_output_write    : std_logic;
    signal act_current_element : std_logic_vector(15 downto 0);
    signal act_cycles          : std_logic_vector(31 downto 0);
    signal conv_write_count   : integer := 0;

begin
    assert RUN_CONVOLUTION or RUN_ACTIVATION
        report "Enable at least one test stage" severity failure;
    assert IMG_WIDTH <= MAX_WIDTH and IMG_HEIGHT <= MAX_HEIGHT
        report "Image tile exceeds convolution dimensions" severity failure;
    assert IMAGE_MODE or TENSOR_SIZE <= MAX_TENSOR_SIZE
        report "Synthetic tensor exceeds activation capacity" severity failure;

    clk <= not clk after CLK_PERIOD/2;
    conv_input_valid <= conv_input_read_en;
    weight_valid <= weight_read_en;
    bias_valid <= bias_read_en;
    act_input_valid <= act_input_read_en;

    conv_input_memory : process(conv_input_addr, conv_input_read_en)
        variable address : integer;
    begin
        conv_input_data <= (others => '0');
        if conv_input_read_en = '1' and not is_x(conv_input_addr) then
            address := to_integer(unsigned(conv_input_addr));
            if address < TENSOR_SIZE then
                conv_input_data <= std_logic_vector(to_signed(input_mem(address), DATA_WIDTH));
            end if;
        end if;
    end process;

    weight_memory : process(weight_addr, weight_read_en)
        variable address : integer;
    begin
        weight_data <= (others => '0');
        if weight_read_en = '1' and not is_x(weight_addr) then
            address := to_integer(unsigned(weight_addr));
            if address < weight_mem'length then
                weight_data <= std_logic_vector(to_signed(weight_mem(address), DATA_WIDTH));
            end if;
        end if;
    end process;

    bias_data <= (others => '0');

    activation_input_memory : process(act_input_addr, act_chunk_base, conv_results)
        variable base_addr : integer;
        variable value : integer;
    begin
        if is_x(act_input_addr) then
            for lane in 0 to PARALLEL_UNITS-1 loop
                act_input_data(lane) <= (others => '0');
            end loop;
        else
            base_addr := act_chunk_base + to_integer(unsigned(act_input_addr));
            for lane in 0 to PARALLEL_UNITS-1 loop
                if base_addr + lane < TENSOR_SIZE then
                    value := conv_results(base_addr + lane);
                    act_input_data(lane) <= std_logic_vector(to_signed(value, DATA_WIDTH));
                else
                    act_input_data(lane) <= (others => '0');
                end if;
            end loop;
        end if;
    end process;

    convolution : entity work.convolution_block
        generic map (
            INPUT_HEIGHT => IMG_HEIGHT,
            INPUT_WIDTH  => IMG_WIDTH
        )
        port map (
            clk => clk, rst => rst, start => conv_enable,
            done => conv_done, ready => conv_ready,
            config => conv_config, batch_size => batch_size,
            input_valid => conv_input_valid, input_data => conv_input_data,
            input_addr => conv_input_addr, input_read_en => conv_input_read_en,
            weight_valid => weight_valid, weight_data => weight_data,
            weight_addr => weight_addr, weight_read_en => weight_read_en,
            bias_valid => bias_valid, bias_data => bias_data,
            bias_addr => bias_addr, bias_read_en => bias_read_en,
            output_valid => conv_output_valid, output_data => conv_output_data,
            output_addr => conv_output_addr, output_write_en => conv_output_write
        );

    activation : entity work.activation_block
        generic map (
            PIPELINE_STAGES => 3,
            PARALLEL_UNITS => PARALLEL_UNITS
        )
        port map (
            clk => clk, rst => rst, start => activation_enable,
            done => act_done, ready => act_ready, busy => act_busy,
            activation_type => RELU, tensor_size => act_tensor_size,
            alpha => (others => '0'), input_valid => act_input_valid,
            input_data => act_input_data, input_addr => act_input_addr,
            input_read_en => act_input_read_en, output_valid => act_output_valid,
            output_data => act_output_data, output_addr => act_output_addr,
            output_write_en => act_output_write,
            current_element => act_current_element, processing_cycles => act_cycles
        );

    conv_monitor : process(clk)
        variable address : integer;
    begin
        if rising_edge(clk) and conv_output_write = '1' and conv_output_valid = '1' then
            address := to_integer(unsigned(conv_output_addr));
            if address < TENSOR_SIZE then
                conv_results(address) <= to_integer(signed(conv_output_data));
                conv_write_count <= conv_write_count + 1;
            end if;
        end if;
    end process;

    stimulus : process
        file output_handle : text;
        variable output_line : line;
        variable cycle_count : integer;
        variable result_count : integer;
        variable sample_valid : std_logic;
        variable expected : integer;
        variable y_pos, x_pos, out_channel, in_channel, kernel_y, kernel_x : integer;
        variable accumulated : integer;
        variable input_y, input_x, input_index, weight_index, output_index : integer;
        variable chunk_base, chunk_size : integer;
    begin
        if IMAGE_MODE then
            file_open(output_handle, OUTPUT_FILE, write_mode);
        end if;
        rst <= '1';
        wait for 5 * CLK_PERIOD;
        wait until rising_edge(clk);
        rst <= '0';

        if RUN_CONVOLUTION then
            wait until rising_edge(clk) and conv_ready = '1';
            conv_enable <= '1';
            wait until rising_edge(clk);
            conv_enable <= '0';
            cycle_count := 0;
            while conv_done /= '1' loop
                wait until rising_edge(clk);
                cycle_count := cycle_count + 1;
                assert cycle_count < TENSOR_SIZE * 100
                    report "Timeout waiting for convolution" severity failure;
            end loop;
            wait until rising_edge(clk);
            wait for 1 ns;
            assert conv_write_count = TENSOR_SIZE
                report "Convolution wrote " & integer'image(conv_write_count) &
                       " outputs; expected " & integer'image(TENSOR_SIZE) severity failure;

            if IMAGE_MODE then
                for y in 0 to IMG_HEIGHT-1 loop
                    for x in 0 to IMG_WIDTH-1 loop
                        for out_channel in 0 to CHANNELS-1 loop
                            accumulated := 0;
                            for kernel_y in 0 to KERNEL_SIZE-1 loop
                                for kernel_x in 0 to KERNEL_SIZE-1 loop
                                    input_y := y + kernel_y - PADDING;
                                    input_x := x + kernel_x - PADDING;
                                    if input_y >= 0 and input_y < IMG_HEIGHT and
                                       input_x >= 0 and input_x < IMG_WIDTH then
                                        for in_channel in 0 to CHANNELS-1 loop
                                            input_index := (input_y*IMG_WIDTH+input_x)*CHANNELS+in_channel;
                                            weight_index := ((kernel_y*KERNEL_SIZE+kernel_x)*CHANNELS+in_channel)*CHANNELS+out_channel;
                                            accumulated := accumulated + input_mem(input_index)*weight_mem(weight_index);
                                        end loop;
                                    end if;
                                end loop;
                            end loop;
                            output_index := (y*IMG_WIDTH+x)*CHANNELS+out_channel;
                            expected := to_integer(resize(shift_right(to_signed(accumulated, 32), FRAC_WIDTH), DATA_WIDTH));
                            assert conv_results(output_index) = expected
                                report "Convolution mismatch at value " & integer'image(output_index) severity failure;
                        end loop;
                    end loop;
                end loop;
            else
                for index in 0 to TENSOR_SIZE-1 loop
                    expected := -(index + 1);
                    assert conv_results(index) = expected
                        report "Synthetic convolution mismatch at " & integer'image(index) severity failure;
                end loop;
            end if;

            report "Convolution passed" severity note;
            if IMAGE_MODE and not RUN_ACTIVATION then
                for index in 0 to TENSOR_SIZE-1 loop
                    write(output_line, conv_results(index));
                    writeline(output_handle, output_line);
                end loop;
            end if;
        end if;

        if RUN_ACTIVATION then
            chunk_base := 0;
            while chunk_base < TENSOR_SIZE loop
                chunk_size := TENSOR_SIZE - chunk_base;
                if chunk_size > MAX_TENSOR_SIZE then
                    chunk_size := MAX_TENSOR_SIZE;
                end if;
                act_chunk_base <= chunk_base;
                act_tensor_size <= chunk_size;

                wait until rising_edge(clk) and act_ready = '1';
                activation_enable <= '1';
                wait until rising_edge(clk);
                activation_enable <= '0';
                result_count := 0;
                cycle_count := 0;
                while act_done /= '1' loop
                    wait until rising_edge(clk);
                    sample_valid := act_output_valid;
                    wait for 1 ns;
                    if sample_valid = '1' then
                        for lane in 0 to PARALLEL_UNITS-1 loop
                            if result_count < chunk_size then
                                expected := conv_results(chunk_base + result_count);
                                if expected < 0 then expected := 0; end if;
                                assert to_integer(signed(act_output_data(lane))) = expected
                                    report "RELU mismatch at value " &
                                           integer'image(chunk_base + result_count)
                                    severity failure;
                                if IMAGE_MODE then
                                    write(output_line, to_integer(signed(act_output_data(lane))));
                                    writeline(output_handle, output_line);
                                end if;
                                result_count := result_count + 1;
                            end if;
                        end loop;
                    end if;
                    cycle_count := cycle_count + 1;
                    assert cycle_count < MAX_TENSOR_SIZE + 2000
                        report "Timeout waiting for activation" severity failure;
                end loop;
                assert result_count = chunk_size
                    report "Activation chunk produced the wrong number of outputs"
                    severity failure;
                chunk_base := chunk_base + chunk_size;
            end loop;
            report "Activation RELU test passed" severity note;
        end if;

        if IMAGE_MODE then
            file_close(output_handle);
        end if;
        report "*** SELECTED TEST STAGES PASSED ***" severity note;
        stop;
        wait;
    end process;
end sim;
