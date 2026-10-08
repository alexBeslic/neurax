-- Self-checking testbench for neurax_data_interface (the mSGDMA-less variant
-- with rd_start_i/rd_start_addr_i/rd_length_i/rd_busy_o/rd_done_o, matching
-- the component instantiated in neurax.vhd).
--
-- Exercises: the Avalon-ST sink write path, direct accelerator Port B
-- read/write, a full-speed source readback (checking sop/eop placement and
-- data integrity), backpressure handling (aso_ready_i held low mid-stream),
-- and that the sink is blocked (asi_ready_o = '0') while a readback is
-- active. Accumulates a failure count and exits with it as the simulation
-- status code (std.env.stop(error_count)), matching the convention used by
-- neurax_register_block_tb.vhd.
--
-- Readback capture is driven off aso_eop_o (read beats until endofpacket)
-- rather than a hardcoded cycle count, and every wait for a status signal
-- (rd_done_o, rd_busy_o, asi_ready_o) is a bounded polling loop rather than a
-- fixed number of clock edges. This avoids baking in assumptions about the
-- exact PREFETCH/look-ahead pipeline latency, which could not be verified
-- here (no simulator was available in this environment).
--
-- NOTE: this module instantiates an Altera `altsyncram` (library altera_mf),
-- so that simulation library must already be mapped in your simulator
-- (normally provided by a Quartus/ModelSim-Altera install).

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use STD.TEXTIO.ALL;

entity neurax_data_interface_tb is
    generic (
        g_DATA_WIDTH    : natural := 32;
        g_ADDR_WIDTH    : natural := 15;
        CLK_PERIOD      : time := 10 ns;
        MAX_WAIT_CYCLES : natural := 40000  -- safety bound for full-RAM transfers
    );
end entity neurax_data_interface_tb;

architecture sim of neurax_data_interface_tb is

    type beat_t is record
        data : std_logic_vector(g_DATA_WIDTH-1 downto 0);
        sop  : std_logic;
        eop  : std_logic;
    end record;
    type beat_array_t is array (natural range <>) of beat_t;

    signal clk : std_logic := '0';
    signal rst : std_logic := '1';

    signal asi_channel : std_logic := '0';
    signal asi_data    : std_logic_vector(g_DATA_WIDTH-1 downto 0) := (others => '0');
    signal asi_valid   : std_logic := '0';
    signal asi_ready   : std_logic;
    signal asi_error   : std_logic := '0';

    signal aso_data    : std_logic_vector(g_DATA_WIDTH-1 downto 0);
    signal aso_valid   : std_logic;
    signal aso_ready   : std_logic := '0';
    signal aso_channel : std_logic;
    signal aso_error   : std_logic;
    signal aso_sop     : std_logic;
    signal aso_eop     : std_logic;

    signal rd_start      : std_logic := '0';
    signal rd_start_addr : std_logic_vector(g_ADDR_WIDTH-1 downto 0) := (others => '0');
    signal rd_length     : std_logic_vector(g_ADDR_WIDTH-1 downto 0) := (others => '0');
    signal rd_busy       : std_logic;
    signal rd_done       : std_logic;

    signal ram_b_rdaddress : std_logic_vector(g_ADDR_WIDTH-1 downto 0) := (others => '0');
    signal ram_b_q         : std_logic_vector(g_DATA_WIDTH-1 downto 0);
    signal ram_b_wraddress : std_logic_vector(g_ADDR_WIDTH-1 downto 0) := (others => '0');
    signal ram_b_data      : std_logic_vector(g_DATA_WIDTH-1 downto 0) := (others => '0');
    signal ram_b_wren      : std_logic := '0';

    signal sim_done : boolean := false;

begin

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

    dut: entity work.neurax_data_interface
        generic map (
            g_DATA_WIDTH => g_DATA_WIDTH,
            g_ADDR_WIDTH => g_ADDR_WIDTH
        )
        port map (
            clk_i => clk,
            rst_i => rst,

            asi_channel_i => asi_channel,
            asi_data_i    => asi_data,
            asi_valid_i   => asi_valid,
            asi_ready_o   => asi_ready,
            asi_error_i   => asi_error,

            aso_data_o    => aso_data,
            aso_valid_o   => aso_valid,
            aso_ready_i   => aso_ready,
            aso_channel_o => aso_channel,
            aso_error_o   => aso_error,
            aso_sop_o     => aso_sop,
            aso_eop_o     => aso_eop,

            rd_start_i      => rd_start,
            rd_start_addr_i => rd_start_addr,
            rd_length_i     => rd_length,
            rd_busy_o       => rd_busy,
            rd_done_o       => rd_done,

            ram_b_rdaddress_i => ram_b_rdaddress,
            ram_b_q_o         => ram_b_q,
            ram_b_wraddress_i => ram_b_wraddress,
            ram_b_data_i      => ram_b_data,
            ram_b_wren_i      => ram_b_wren
        );

    stimulus: process
        variable error_count    : integer := 0;
        variable captured       : beat_array_t(0 to 32767);
        variable captured_count : natural := 0;
        variable held_data      : std_logic_vector(g_DATA_WIDTH-1 downto 0);
        variable held_sop       : std_logic;
        variable output_line    : line;
        file output_file        : text;

        procedure check(cond : boolean; msg : string) is
        begin
            if not cond then
                report "CHECK FAILED: " & msg severity error;
                error_count := error_count + 1;
            end if;
        end procedure;

        -- Avalon-ST sink handshake (blocks until asi_ready is seen high)
        procedure sink_write(data_val : std_logic_vector(g_DATA_WIDTH-1 downto 0)) is
        begin
            asi_data  <= data_val;
            asi_valid <= '1';
            loop
                wait until rising_edge(clk);
                exit when asi_ready = '1';
            end loop;
            asi_valid <= '0';
            wait for 1 ns;
        end procedure;

        -- Direct accelerator Port B write (one cycle)
        procedure portb_write(addr : natural; data_val : std_logic_vector(g_DATA_WIDTH-1 downto 0)) is
        begin
            ram_b_wraddress <= std_logic_vector(to_unsigned(addr, g_ADDR_WIDTH));
            ram_b_data      <= data_val;
            ram_b_wren      <= '1';
            wait until rising_edge(clk);
            ram_b_wren      <= '0';
            wait for 1 ns;
        end procedure;

        -- Direct accelerator Port B read; result is available on ram_b_q
        -- after this procedure returns (2-cycle margin for RAM read latency)
        procedure portb_read(addr : natural) is
        begin
            ram_b_rdaddress <= std_logic_vector(to_unsigned(addr, g_ADDR_WIDTH));
            wait until rising_edge(clk);
            wait until rising_edge(clk);
            wait for 1 ns;
        end procedure;

        -- Poll a single-bit std_logic signal until it equals 'want', up to
        -- MAX_WAIT_CYCLES clock edges. Reports a check failure on timeout.
        procedure wait_for_bit(signal sig : std_logic; want : std_logic; msg : string) is
            variable n : natural := 0;
        begin
            while sig /= want and n < MAX_WAIT_CYCLES loop
                wait until rising_edge(clk);
                wait for 1 ns;
                n := n + 1;
            end loop;
            check(sig = want, msg);
        end procedure;

        -- Pulse rd_start_i to kick off a readback; aso_ready must already be
        -- driven by the caller (so backpressure can be injected mid-stream).
        procedure start_readback(start_addr : natural; length : natural) is
        begin
            rd_start_addr <= std_logic_vector(to_unsigned(start_addr, g_ADDR_WIDTH));
            rd_length     <= std_logic_vector(to_unsigned(length, g_ADDR_WIDTH));
            rd_start      <= '1';
            wait until rising_edge(clk);
            rd_start <= '0';
        end procedure;

        -- Collect beats (one per cycle aso_valid_o and aso_ready are both
        -- high) into 'captured', stopping once a beat with eop='1' is seen
        -- (or MAX_WAIT_CYCLES elapses). Sets captured_count to the number of
        -- beats actually collected -- deliberately not assumed up front, so
        -- an unexpected pipeline latency shows up as a count mismatch rather
        -- than a hang or a corrupted capture.
        procedure collect_until_eop is
            variable n : natural := 0;
        begin
            captured_count := 0;
            loop
                wait until rising_edge(clk);
                if aso_valid = '1' and aso_ready = '1' then
                    captured(captured_count) := (data => aso_data, sop => aso_sop, eop => aso_eop);
                    captured_count := captured_count + 1;
                    if aso_eop = '1' then
                        wait for 1 ns;
                        exit;
                    end if;
                end if;
                n := n + 1;
                exit when n >= MAX_WAIT_CYCLES;
            end loop;
            check(n < MAX_WAIT_CYCLES, "collect_until_eop: timed out waiting for aso_eop_o");
        end procedure;

    begin
        -- ---------------------------------------------------------------
        -- Reset
        -- ---------------------------------------------------------------
        rst <= '1';
        wait for CLK_PERIOD*4;
        rst <= '0';
        wait for CLK_PERIOD*2;

        check(asi_ready = '1', "asi_ready_o should be 1 in IDLE");
        check(rd_busy = '0', "rd_busy_o should be 0 before any transfer");

        -- ---------------------------------------------------------------
        -- Sink write burst: 16 words at addresses 0..15
        -- ---------------------------------------------------------------
        for i in 0 to 15 loop
            report "TB INPUT[" & integer'image(i) & "] = " & integer'image(100 + i);
            sink_write(std_logic_vector(to_unsigned(100 + i, g_DATA_WIDTH)));
        end loop;

        -- Verify storage via the independent accelerator Port B path
        for i in 0 to 15 loop
            portb_read(i);
            check(unsigned(ram_b_q) = 100 + i,
                  "sink write: RAM[" & integer'image(i) & "] mismatch after sink burst");
        end loop;

        -- ---------------------------------------------------------------
        -- Accelerator Port B write/read (independent of the sink/source path)
        -- ---------------------------------------------------------------
        for i in 0 to 4 loop
            portb_write(100 + i, std_logic_vector(to_unsigned(500 + i, g_DATA_WIDTH)));
        end loop;
        for i in 0 to 4 loop
            portb_read(100 + i);
            check(unsigned(ram_b_q) = 500 + i,
                  "portb write: RAM[" & integer'image(100 + i) & "] mismatch");
        end loop;

        -- ---------------------------------------------------------------
        -- Full-speed readback of the first 16 words, sop/eop + data check
        -- ---------------------------------------------------------------
        aso_ready <= '1';
        start_readback(0, 16);
        collect_until_eop;

        check(captured_count = 16, "full-speed readback: expected 16 beats, got " & integer'image(captured_count));
        if captured_count > 0 then
            check(captured(0).sop = '1', "readback: first beat should have sop=1");
            check(captured(0).eop = '0', "readback: first beat should have eop=0");
        end if;
        if captured_count = 16 then
            check(captured(15).sop = '0', "readback: last beat should have sop=0");
            check(captured(15).eop = '1', "readback: last beat should have eop=1");
            for i in 0 to 15 loop
                report "TB OUTPUT[" & integer'image(i) & "] = " &
                       integer'image(to_integer(unsigned(captured(i).data)));
                check(unsigned(captured(i).data) = 100 + i,
                      "readback: beat " & integer'image(i) & " data mismatch");
                if i /= 0 and i /= 15 then
                    check(captured(i).sop = '0' and captured(i).eop = '0',
                          "readback: middle beat " & integer'image(i) & " should have sop=eop=0");
                end if;
            end loop;
        end if;

        wait_for_bit(rd_done, '1', "rd_done_o should pulse after the last beat is accepted");
        wait_for_bit(rd_done, '0', "rd_done_o should deassert after its one-cycle pulse");
        wait_for_bit(rd_busy, '0', "rd_busy_o should return to 0 once back in IDLE");

        aso_ready <= '0';

        -- ---------------------------------------------------------------
        -- Sink blocked during an active readback
        -- ---------------------------------------------------------------
        aso_ready <= '1';
        start_readback(0, 16);
        wait for 1 ns;
        check(asi_ready = '0', "asi_ready_o should be 0 while a readback is active");

        collect_until_eop;
        check(captured_count = 16, "sink-blocked readback: expected 16 beats, got " & integer'image(captured_count));

        wait_for_bit(asi_ready, '1', "asi_ready_o should return to 1 once the readback completes");

        -- ---------------------------------------------------------------
        -- Backpressure: hold aso_ready low mid-stream, verify the bus holds
        -- ---------------------------------------------------------------
        aso_ready <= '1';
        start_readback(100, 5);

        -- Wait for the first beat to appear, then stall for 3 cycles
        loop
            wait until rising_edge(clk);
            wait for 1 ns;
            exit when aso_valid = '1';
        end loop;
        held_data := aso_data;
        held_sop  := aso_sop;
        aso_ready <= '0';

        for i in 0 to 2 loop
            wait until rising_edge(clk);
            wait for 1 ns;
            check(aso_valid = '1', "backpressure: aso_valid_o should stay high while stalled");
            check(aso_data = held_data, "backpressure: aso_data_o should not change while stalled");
            check(aso_sop = held_sop, "backpressure: aso_sop_o should not change while stalled");
        end loop;

        -- Re-arm ready; collect_until_eop sees this still-pending first beat
        -- plus the remaining ones.
        aso_ready <= '1';
        collect_until_eop;
        check(captured_count = 5, "backpressure readback: expected 5 beats, got " & integer'image(captured_count));
        if captured_count = 5 then
            check(unsigned(captured(0).data) = 500, "backpressure: beat 0 data mismatch");
            check(unsigned(captured(4).data) = 504, "backpressure: beat 4 data mismatch");
            check(captured(4).eop = '1', "backpressure: last beat should have eop=1");
        end if;

        wait_for_bit(rd_done, '1', "backpressure: rd_done_o should still pulse after the stall");
        wait_for_bit(rd_busy, '0', "backpressure: rd_busy_o should return to 0");

        -- ---------------------------------------------------------------
        -- Fill and read the full 32768-word RAM capacity
        -- ---------------------------------------------------------------
        aso_ready <= '0';
        for i in 16 to 32767 loop
            if i = 16 or i = 32767 then
                report "FULL RAM INPUT[" & integer'image(i) & "] = " &
                       integer'image(100 + i);
            end if;
            sink_write(std_logic_vector(to_unsigned(100 + i, g_DATA_WIDTH)));
        end loop;

        -- rd_length_i is 15 bits, so the largest single transfer is 32767
        -- words. Read those words, then read the final RAM address separately.
        file_open(output_file, "data_interface_output.txt", write_mode);
        aso_ready <= '1';
        start_readback(0, 32767);
        collect_until_eop;
        check(captured_count = 32767,
              "full-RAM readback: expected 32767 beats, got " & integer'image(captured_count));
        if captured_count > 0 then
            for i in 0 to captured_count - 1 loop
                write(output_line, to_integer(unsigned(captured(i).data)));
                writeline(output_file, output_line);
            end loop;
        end if;
        if captured_count = 32767 then
            for i in 0 to 32766 loop
                check(unsigned(captured(i).data) = 100 + i,
                      "full-RAM readback: data mismatch at address " & integer'image(i));
            end loop;
            report "FULL RAM OUTPUT[0] = " &
                   integer'image(to_integer(unsigned(captured(0).data)));
            report "FULL RAM OUTPUT[32766] = " &
                   integer'image(to_integer(unsigned(captured(32766).data)));
        end if;

        wait_for_bit(rd_done, '1', "full-RAM readback: rd_done_o should pulse");
        wait_for_bit(rd_busy, '0', "full-RAM readback: rd_busy_o should return to 0");

        start_readback(32767, 1);
        collect_until_eop;
        check(captured_count = 1, "last-address readback: expected 1 beat");
        if captured_count = 1 then
            report "FULL RAM OUTPUT[32767] = " &
                   integer'image(to_integer(unsigned(captured(0).data)));
            check(unsigned(captured(0).data) = 32867,
                  "last-address readback: data mismatch at address 32767");
            write(output_line, to_integer(unsigned(captured(0).data)));
            writeline(output_file, output_line);
        end if;
        file_close(output_file);
        report "neurax_data_interface_tb: full readback written to data_interface_output.txt";

        wait_for_bit(rd_done, '1', "last-address readback: rd_done_o should pulse");

        -- ---------------------------------------------------------------
        -- Summary
        -- ---------------------------------------------------------------
        if error_count = 0 then
            report "neurax_data_interface_tb: ALL CHECKS PASSED";
        else
            report "neurax_data_interface_tb: " & integer'image(error_count) & " CHECK(S) FAILED" severity error;
        end if;

        sim_done <= true;
        wait for CLK_PERIOD*2;
        std.env.stop(error_count);
        wait;
    end process;

end architecture sim;
