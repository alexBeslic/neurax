library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use work.accel_types.all;

entity activation_block_tb is
end activation_block_tb;

architecture sim of activation_block_tb is
    constant CLK_PERIOD     : time := 10 ns;
    constant PARALLEL_UNITS : integer := 4;
    constant TENSOR_SIZE    : integer := 8;

    type test_values_t is array (natural range <>) of integer;
    constant TEST_VALUES : test_values_t(0 to TENSOR_SIZE-1) :=
        (-512, -1, 0, 1, 256, -256, 32767, -32768);

    signal clk               : std_logic := '0';
    signal rst               : std_logic := '1';
    signal start             : std_logic := '0';
    signal done              : std_logic;
    signal ready             : std_logic;
    signal busy              : std_logic;
    signal activation_type   : activation_type_t := RELU;
    signal alpha             : std_logic_vector(DATA_WIDTH-1 downto 0) :=
                                   std_logic_vector(to_signed(26, DATA_WIDTH));
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

    input_memory : process(input_addr)
        variable base_addr : integer;
        variable value_idx : integer;
    begin
        if is_x(input_addr) then
            for lane in 0 to PARALLEL_UNITS-1 loop
                input_data(lane) <= (others => '0');
            end loop;
        else
            base_addr := to_integer(unsigned(input_addr));
            for lane in 0 to PARALLEL_UNITS-1 loop
                value_idx := base_addr + lane;
                if value_idx < TENSOR_SIZE then
                    input_data(lane) <= std_logic_vector(to_signed(TEST_VALUES(value_idx), DATA_WIDTH));
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
            activation_type   => activation_type,
            tensor_size       => TENSOR_SIZE,
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

    stimulus : process
        variable result_count : integer;
        variable cycle_count  : integer;
        variable sample_valid : std_logic;
        variable expected     : integer;
    begin
        rst <= '1';
        wait for 5 * CLK_PERIOD;
        wait until rising_edge(clk);
        rst <= '0';

        for test_case in 0 to 1 loop
            if test_case = 0 then
                activation_type <= RELU;
                report "Checking RELU" severity note;
            else
                activation_type <= LINEAR;
                report "Checking LINEAR" severity note;
            end if;

            wait until rising_edge(clk) and ready = '1';
            start <= '1';
            wait until rising_edge(clk);
            start <= '0';

            result_count := 0;
            cycle_count := 0;
            while done /= '1' loop
                wait until rising_edge(clk);
                sample_valid := output_valid;
                wait for 1 ns;

                if sample_valid = '1' then
                    for lane in 0 to PARALLEL_UNITS-1 loop
                        if result_count < TENSOR_SIZE then
                            if test_case = 0 and TEST_VALUES(result_count) < 0 then
                                expected := 0;
                            else
                                expected := TEST_VALUES(result_count);
                            end if;
                            assert to_integer(signed(output_data(lane))) = expected
                                report "Activation mismatch at element " &
                                       integer'image(result_count) & ", expected " &
                                       integer'image(expected) & ", received " &
                                       integer'image(to_integer(signed(output_data(lane))))
                                severity failure;
                            result_count := result_count + 1;
                        end if;
                    end loop;
                end if;

                cycle_count := cycle_count + 1;
                assert cycle_count < 2000
                    report "Timeout waiting for activation completion" severity failure;
            end loop;

            assert result_count = TENSOR_SIZE
                report "Expected " & integer'image(TENSOR_SIZE) & " outputs, got " &
                       integer'image(result_count) severity failure;
            report "Activation test passed with " & integer'image(result_count) &
                   " values" severity note;
            wait until rising_edge(clk);
        end loop;

        report "*** ALL ACTIVATION TESTS PASSED ***" severity note;
        std.env.stop;
        wait;
    end process;
end sim;