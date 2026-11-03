library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.accel_types.all;

entity activation_ram_path_tb is
end entity;

architecture sim of activation_ram_path_tb is
    constant CLK_PERIOD : time := 10 ns;
    constant TENSOR_SIZE : integer := 8;
    constant OUTPUT_BASE : integer := 13016;

    type ram_t is array (0 to 32767) of std_logic_vector(DATA_WIDTH-1 downto 0);
    type expected_t is array (0 to TENSOR_SIZE-1) of integer;
    constant EXPECTED : expected_t := (0, 256, 0, 512, 0, 1024, 0, 0);

    function init_ram return ram_t is
        variable contents : ram_t := (others => (others => '0'));
    begin
        contents(0) := std_logic_vector(to_signed(0, DATA_WIDTH));
        contents(1) := std_logic_vector(to_signed(256, DATA_WIDTH));
        contents(2) := std_logic_vector(to_signed(-256, DATA_WIDTH));
        contents(3) := std_logic_vector(to_signed(512, DATA_WIDTH));
        contents(4) := std_logic_vector(to_signed(-512, DATA_WIDTH));
        contents(5) := std_logic_vector(to_signed(1024, DATA_WIDTH));
        contents(6) := std_logic_vector(to_signed(-1, DATA_WIDTH));
        contents(7) := std_logic_vector(to_signed(0, DATA_WIDTH));
        return contents;
    end function;

    signal clk : std_logic := '0';
    signal rst : std_logic := '1';
    signal finished : boolean := false;
    signal start : std_logic := '0';
    signal done : std_logic;
    signal ready : std_logic;
    signal busy : std_logic;
    signal input_valid : std_logic := '0';
    signal input_data : data_array_t(0 to 0) := (others => (others => '0'));
    signal input_addr : std_logic_vector(15 downto 0);
    signal input_read_en : std_logic;
    signal output_valid : std_logic;
    signal output_data : data_array_t(0 to 0);
    signal output_addr : std_logic_vector(15 downto 0);
    signal output_write_en : std_logic;
    signal read_pending : std_logic := '0';
    signal ram : ram_t := init_ram;
    signal ram_q : std_logic_vector(DATA_WIDTH-1 downto 0) := (others => '0');
begin
    clock_gen : process
    begin
        while not finished loop
            clk <= '0';
            wait for CLK_PERIOD / 2;
            clk <= '1';
            wait for CLK_PERIOD / 2;
        end loop;
        wait;
    end process;

    dut : entity work.activation_block
        generic map (
            PIPELINE_STAGES => 3,
            PARALLEL_UNITS => 1
        )
        port map (
            clk => clk,
            rst => rst,
            start => start,
            done => done,
            ready => ready,
            busy => busy,
            activation_type => RELU,
            tensor_size => TENSOR_SIZE,
            alpha => std_logic_vector(to_signed(26, DATA_WIDTH)),
            input_valid => input_valid,
            input_data => input_data,
            input_addr => input_addr,
            input_read_en => input_read_en,
            output_valid => output_valid,
            output_data => output_data,
            output_addr => output_addr,
            output_write_en => output_write_en
        );

    -- Match the accelerator's synchronous Port B read and registered write path.
    ram_port : process(clk, rst)
        variable read_address : integer;
        variable write_address : integer;
    begin
        if rst = '1' then
            read_pending <= '0';
            input_valid <= '0';
            input_data(0) <= (others => '0');
            ram_q <= (others => '0');
        elsif rising_edge(clk) then
            read_address := to_integer(unsigned(input_addr));
            ram_q <= ram(read_address);

            input_valid <= read_pending;
            read_pending <= input_read_en;
            if read_pending = '1' then
                input_data(0) <= ram_q;
            end if;

            if output_write_en = '1' then
                write_address := OUTPUT_BASE + to_integer(unsigned(output_addr));
                ram(write_address) <= output_data(0);
            end if;
        end if;
    end process;

    stimulus : process
        variable completed : boolean := false;
    begin
        wait for 4 * CLK_PERIOD;
        wait until rising_edge(clk);
        rst <= '0';
        wait until rising_edge(clk) and ready = '1';
        start <= '1';
        wait until rising_edge(clk);
        start <= '0';

        for cycle in 0 to 1000 loop
            wait until rising_edge(clk);
            if done = '1' then
                completed := true;
                exit;
            end if;
        end loop;
        assert completed
            report "Timeout waiting for activation operation_done"
            severity failure;

        wait for 1 ns;
        for i in 0 to TENSOR_SIZE-1 loop
            assert signed(ram(OUTPUT_BASE + i)) = to_signed(EXPECTED(i), DATA_WIDTH)
                report "RAM activation output mismatch at index " & integer'image(i)
                severity failure;
        end loop;

        report "*** Activation block synchronous RAM-path test passed ***"
            severity note;
        finished <= true;
        wait;
    end process;
end architecture;
