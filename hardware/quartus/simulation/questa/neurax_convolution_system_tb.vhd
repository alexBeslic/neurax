-- System-level convolution test: Avalon-ST input -> shared RAM -> registers ->
-- accelerator -> shared RAM -> Avalon-ST output.
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;
use std.env.all;

entity neurax_convolution_system_tb is
    generic (
        INPUT_WIDTH  : positive := 100;
        INPUT_HEIGHT : positive := 100;
        INPUT_FILE   : string := "integration_input.txt";
        OUTPUT_FILE  : string := "integration_output.txt"
    );
end entity neurax_convolution_system_tb;

architecture sim of neurax_convolution_system_tb is
    constant INPUT_BASE_ADDR  : natural := 0;
    constant WEIGHT_BASE_ADDR: natural := 10000;
    constant BIAS_BASE_ADDR  : natural := 13000;
    constant OUTPUT_BASE_ADDR: natural := 13016;
    constant INPUT_COUNT     : natural := INPUT_WIDTH * INPUT_HEIGHT;
    constant OUTPUT_COUNT    : natural := INPUT_COUNT;
    constant MAX_WAIT_CYCLES : natural := 1000000;

    type weight_array_t is array (0 to 8) of integer;
    constant EDGE_WEIGHTS : weight_array_t :=
        (0, -256, 0, -256, 1024, -256, 0, -256, 0);
    type result_array_t is array (natural range <>) of std_logic_vector(31 downto 0);

    signal clk : std_logic := '0';
    signal rst : std_logic := '1';

    signal avs_chipselect    : std_logic := '0';
    signal avs_address       : std_logic_vector(3 downto 0) := (others => '0');
    signal avs_read          : std_logic := '0';
    signal avs_readdata      : std_logic_vector(31 downto 0);
    signal avs_readdatavalid : std_logic;
    signal avs_write         : std_logic := '0';
    signal avs_writedata     : std_logic_vector(31 downto 0) := (others => '0');
    signal avs_byteenable    : std_logic_vector(3 downto 0) := (others => '1');
    signal avs_waitrequest   : std_logic;

    signal asi_channel : std_logic := '0';
    signal asi_data    : std_logic_vector(31 downto 0) := (others => '0');
    signal asi_valid   : std_logic := '0';
    signal asi_ready   : std_logic;
    signal aso_data    : std_logic_vector(31 downto 0);
    signal aso_valid   : std_logic;
    signal aso_ready   : std_logic := '1';
    signal aso_channel : std_logic;
    signal aso_error   : std_logic;
    signal aso_sop     : std_logic;
    signal aso_eop     : std_logic;

    signal sim_done : boolean := false;
begin
    clk_gen : process
    begin
        while not sim_done loop
            clk <= '0';
            wait for 5 ns;
            clk <= '1';
            wait for 5 ns;
        end loop;
        wait;
    end process;

    dut : entity work.neurax
        generic map (
            g_WIDTH          => 32,
            g_ADDR_WIDTH     => 4,
            g_DATA_ADDR_WIDTH=> 15,
            g_FB_WIDTH       => INPUT_WIDTH,
            g_FB_HEIGHT      => INPUT_HEIGHT,
            PARALLEL_UNITS   => 4
        )
        port map (
            g_clk_i => clk,
            g_rst_i => rst,
            g_avs_chipselect_i    => avs_chipselect,
            g_avs_address_i       => avs_address,
            g_avs_read_i          => avs_read,
            g_avs_readdata_o      => avs_readdata,
            g_avs_readdatavalid_o => avs_readdatavalid,
            g_avs_write_i         => avs_write,
            g_avs_writedata_i     => avs_writedata,
            g_avs_byteenable_i    => avs_byteenable,
            g_avs_waitrequest_o   => avs_waitrequest,
            g_asi_channel_i => asi_channel,
            g_asi_data_i    => asi_data,
            g_asi_valid_i   => asi_valid,
            g_asi_ready_o   => asi_ready,
            g_asi_error_i   => '0',
            g_aso_data_o    => aso_data,
            g_aso_valid_o   => aso_valid,
            g_aso_ready_i   => aso_ready,
            g_aso_channel_o => aso_channel,
            g_aso_error_o   => aso_error,
            g_aso_sop_o     => aso_sop,
            g_aso_eop_o     => aso_eop
        );

    stimulus : process
        variable error_count   : natural := 0;
        file input_f           : text;
        variable input_line    : line;
        variable input_value   : integer;
        variable status_value  : std_logic_vector(31 downto 0);
        variable data_command  : std_logic_vector(31 downto 0);
        file output_f          : text;
        variable output_line   : line;
        variable captured      : result_array_t(0 to OUTPUT_COUNT-1);
        variable captured_count : natural := 0;
        variable operation_done : boolean := false;
        variable packet_done    : boolean := false;

        procedure check(condition : boolean; message : string) is
        begin
            if not condition then
                report "CHECK FAILED: " & message severity error;
                error_count := error_count + 1;
            end if;
        end procedure;

        procedure avs_write_register(address : natural; value : std_logic_vector(31 downto 0)) is
        begin
            avs_address    <= std_logic_vector(to_unsigned(address, avs_address'length));
            avs_writedata  <= value;
            avs_chipselect <= '1';
            avs_write      <= '1';
            wait until rising_edge(clk);
            wait for 1 ns;
            avs_chipselect <= '0';
            avs_write      <= '0';
        end procedure;

        procedure avs_read_register(
            address : natural;
            variable value : out std_logic_vector(31 downto 0)
        ) is
        begin
            avs_address    <= std_logic_vector(to_unsigned(address, avs_address'length));
            avs_chipselect <= '1';
            avs_read       <= '1';
            wait until rising_edge(clk);
            wait for 1 ns;
            value := avs_readdata;
            avs_chipselect <= '0';
            avs_read       <= '0';
        end procedure;

        procedure sink_send(value : std_logic_vector(31 downto 0); channel : std_logic := '0') is
        begin
            asi_data    <= value;
            asi_channel <= channel;
            asi_valid   <= '1';
            loop
                wait until rising_edge(clk);
                exit when asi_ready = '1';
            end loop;
            wait for 1 ns;
            asi_valid   <= '0';
            asi_channel <= '0';
        end procedure;
    begin
        check(INPUT_COUNT <= WEIGHT_BASE_ADDR,
              "input image does not fit before the weight RAM region");
        check(OUTPUT_BASE_ADDR + OUTPUT_COUNT <= 2**15,
              "output image exceeds the data RAM capacity");
        check(INPUT_WIDTH <= 128 and INPUT_HEIGHT <= 128,
              "convolution dimensions exceed the accelerator limit");

        rst <= '1';
        wait for 40 ns;
        rst <= '0';
        wait for 20 ns;

        -- Channel 1 resets the sink write pointer; channel 0 words are stored.
        sink_send((others => '0'), '1');
        file_open(input_f, INPUT_FILE, read_mode);
        for i in 0 to INPUT_COUNT-1 loop
            readline(input_f, input_line);
            read(input_line, input_value);
            sink_send(std_logic_vector(to_signed(input_value, 32)));
        end loop;
        file_close(input_f);

        -- Populate the fixed input/weight/bias/output memory layout.
        for i in INPUT_COUNT to WEIGHT_BASE_ADDR-1 loop
            sink_send((others => '0'));
        end loop;
        for i in EDGE_WEIGHTS'range loop
            sink_send(std_logic_vector(to_signed(EDGE_WEIGHTS(i), 32)));
        end loop;
        for i in WEIGHT_BASE_ADDR + EDGE_WEIGHTS'length to BIAS_BASE_ADDR-1 loop
            sink_send((others => '0'));
        end loop;
        sink_send((others => '0')); -- zero bias at BIAS_BASE_ADDR

        -- Configure 1-channel, 3x3, stride-1, padding-1 convolution.
        avs_write_register(3, x"03000101");
        avs_write_register(4, x"00000101");
        avs_write_register(8, x"00000001");
        avs_write_register(0, x"00000001"); -- enable accelerator
        avs_write_register(0, x"00000009"); -- enable + start convolution
        avs_write_register(0, x"00000001"); -- clear the start pulse

        for i in 0 to MAX_WAIT_CYCLES-1 loop
            avs_read_register(1, status_value);
            if status_value(4) = '1' then
                operation_done := true;
                exit;
            end if;
        end loop;
        check(operation_done, "timed out waiting for convolution to finish");

        if operation_done then
            -- Avalon-MM DATA_READ: address in [14:0], transfer length in [30:16].
            data_command := std_logic_vector(
                shift_left(to_unsigned(OUTPUT_COUNT, 32), 16) or
                to_unsigned(OUTPUT_BASE_ADDR, 32)
            );
            avs_write_register(10, data_command);
            avs_write_register(9, x"80000000"); -- start output readback

            for i in 0 to MAX_WAIT_CYCLES-1 loop
                wait until rising_edge(clk);
                wait for 1 ns;
                if aso_valid = '1' and aso_ready = '1' then
                    if captured_count < OUTPUT_COUNT then
                        captured(captured_count) := aso_data;
                        captured_count := captured_count + 1;
                    else
                        check(false, "received more output words than requested");
                    end if;
                    if aso_eop = '1' then
                        packet_done := true;
                        exit;
                    end if;
                end if;
            end loop;

            check(packet_done, "timed out waiting for output packet EOP");
            check(captured_count = OUTPUT_COUNT,
                  "expected " & integer'image(OUTPUT_COUNT) & " output words, received " &
                  integer'image(captured_count));

            file_open(output_f, OUTPUT_FILE, write_mode);
            if captured_count > 0 then
                for i in 0 to captured_count-1 loop
                    write(output_line, to_integer(signed(captured(i)(15 downto 0))));
                    writeline(output_f, output_line);
                end loop;
            end if;
            file_close(output_f);
            report "neurax_convolution_system_tb: output written to " & OUTPUT_FILE;
        end if;

        if error_count = 0 then
            report "neurax_convolution_system_tb: ALL CHECKS PASSED";
        else
            report "neurax_convolution_system_tb: " & integer'image(error_count) &
                   " CHECK(S) FAILED" severity error;
        end if;
        sim_done <= true;
        wait for 20 ns;
        std.env.stop(error_count);
        wait;
    end process;
end architecture sim;
