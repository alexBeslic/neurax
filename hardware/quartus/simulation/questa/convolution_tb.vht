-- Testbench for convolution_block
-- Self-checking: builds a golden model, drives a combinational memory model
-- and compares every output word after 'done'.

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use std.textio.all;
use std.env.all;
use work.accel_types.all;

entity convolution_block_tb is
    generic (
        IO_MODE     : boolean := false; -- when true, read INPUT_FILE and write OUTPUT_FILE once
        INPUT_FILE  : string  := "image.txt";
        OUTPUT_FILE : string  := "output.txt";
        IMG_H       : integer := 4;
        IMG_W       : integer := 4;
        IMG_C       : integer := 1;
        K_G         : integer := 3;
        S_G         : integer := 1;
        P_G         : integer := 0;
        COUT_G      : integer := 1
    );
end convolution_block_tb;

architecture sim of convolution_block_tb is

    constant CLK_PERIOD   : time := 10 ns;
    -- image/input dimensions (can be overridden by generics)
    constant IN_H         : integer := IMG_H;
    constant IN_W         : integer := IMG_W;
    constant MEM_SIZE     : integer := 8192;
    constant UNWRITTEN    : integer := -99999999;

    type int_mem_t is array (0 to MEM_SIZE-1) of integer;

    -- DUT signals
    signal clk, rst, start : std_logic := '0';
    signal done, ready     : std_logic;
    signal config          : conv_config_t;
    signal batch_size      : integer range 1 to MAX_BATCH_SIZE := 1;

    signal input_valid     : std_logic;
    signal input_data      : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal input_addr      : std_logic_vector(15 downto 0);
    signal input_read_en   : std_logic;

    signal weight_valid    : std_logic;
    signal weight_data     : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal weight_addr     : std_logic_vector(15 downto 0);
    signal weight_read_en  : std_logic;

    signal bias_valid      : std_logic;
    signal bias_data       : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal bias_addr       : std_logic_vector(7 downto 0);
    signal bias_read_en    : std_logic;

    signal output_valid    : std_logic;
    signal output_data     : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal output_addr     : std_logic_vector(15 downto 0);
    signal output_write_en : std_logic;

    -- Memories (test data lives here)
    signal in_mem   : int_mem_t := (others => 0);
    signal w_mem    : int_mem_t := (others => 0);
    signal b_mem    : int_mem_t := (others => 0);
    signal out_mem  : int_mem_t := (others => UNWRITTEN);
    signal clr_out  : std_logic := '0';
    signal wr_count : integer := 0;
    file io_out : text;

begin

    ---------------------------------------------------------------------------
    -- Clock
    ---------------------------------------------------------------------------
    clk <= not clk after CLK_PERIOD/2;

    ---------------------------------------------------------------------------
    -- DUT
    ---------------------------------------------------------------------------
    dut : entity work.convolution_block
        generic map (
            INPUT_HEIGHT => IN_H,
            INPUT_WIDTH  => IN_W
        )
        port map (
            clk => clk, rst => rst,
            start => start, done => done, ready => ready,
            config => config, batch_size => batch_size,
            input_valid => input_valid, input_data => input_data,
            input_addr => input_addr, input_read_en => input_read_en,
            weight_valid => weight_valid, weight_data => weight_data,
            weight_addr => weight_addr, weight_read_en => weight_read_en,
            bias_valid => bias_valid, bias_data => bias_data,
            bias_addr => bias_addr, bias_read_en => bias_read_en,
            output_valid => output_valid, output_data => output_data,
            output_addr => output_addr, output_write_en => output_write_en
        );

    ---------------------------------------------------------------------------
    -- Combinational memory models (DUT expects data in the same cycle
    -- the address is presented)
    ---------------------------------------------------------------------------
    input_valid <= input_read_en;
    input_data  <= std_logic_vector(to_signed(in_mem(to_integer(unsigned(input_addr))), DATA_WIDTH))
                   when input_read_en = '1' else (others => '0');

    weight_valid <= weight_read_en;
    weight_data  <= std_logic_vector(to_signed(w_mem(to_integer(unsigned(weight_addr))), DATA_WIDTH))
                    when weight_read_en = '1' else (others => '0');

    bias_valid <= bias_read_en;
    bias_data  <= std_logic_vector(to_signed(b_mem(to_integer(unsigned(bias_addr))), DATA_WIDTH))
                  when bias_read_en = '1' else (others => '0');

    ---------------------------------------------------------------------------
    -- Output monitor: captures every written word
    ---------------------------------------------------------------------------
    monitor : process(clk)
    begin
        if rising_edge(clk) then
            if clr_out = '1' then
                out_mem  <= (others => UNWRITTEN);
                wr_count <= 0;
            elsif output_write_en = '1' and output_valid = '1' then
                out_mem(to_integer(unsigned(output_addr))) <= to_integer(signed(output_data));
                wr_count <= wr_count + 1;
            end if;
        end if;
    end process;

    ---------------------------------------------------------------------------
    -- Stimulus + checker
    ---------------------------------------------------------------------------
    stim : process
        variable seed   : integer := 12345;
        variable errors : integer := 0;
        variable total_errors : integer := 0;
        -- IO-mode local declarations
        file fin : text;
        variable ln : line;
        variable v  : integer;
        variable idx : integer := 0;
        variable oh : integer := 0;
        variable ow : integer := 0;
        variable n_outputs : integer := 0;
        variable L : line;
        variable cycles : integer := 0;

        -- small LCG random generator, result in [-2**(bits-1), 2**(bits-1)-1]
        impure function rnd(bits : integer) return integer is
        begin
            seed := (seed * 1103515245 + 12345) mod 2147483647;
            return ((seed / 65536) mod (2**bits)) - 2**(bits-1);
        end function;

        procedure run_test(
            k, s, p, cin, cout, batch : in integer;
            name : in string
        ) is
            variable oh, ow       : integer;
            variable acc          : signed(31 downto 0);
            variable prod         : integer;
            variable ih, iw       : integer;
            variable exp_val      : signed(31 downto 0);
            variable exp_word     : integer;
            variable exp_addr     : integer;
            variable expected     : int_mem_t := (others => UNWRITTEN);
            variable n_outputs    : integer;
            variable cycles       : integer;
            variable rand_bits    : integer := DATA_WIDTH/2;
        begin
            report "=== " & name & " ===" severity note;
            errors := 0;

            oh := (IN_H + 2*p - k)/s + 1;
            ow := (IN_W + 2*p - k)/s + 1;
            n_outputs := batch * oh * ow * cout;

            -- Fill memories with random data
            for i in 0 to batch * IN_H * IN_W * cin - 1 loop
                in_mem(i) <= rnd(rand_bits);
            end loop;
            for i in 0 to k*k*cin*cout - 1 loop
                w_mem(i) <= rnd(rand_bits);
            end loop;
            for i in 0 to cout - 1 loop
                b_mem(i) <= rnd(rand_bits);
            end loop;

            -- Configuration
            config.kernel_size     <= k;
            config.stride          <= s;
            config.padding         <= p;
            config.input_channels  <= cin;
            config.output_channels <= cout;
            batch_size             <= batch;

            -- Clear output capture
            clr_out <= '1';
            wait until rising_edge(clk);
            clr_out <= '0';
            wait for 1 ns;   -- let memory signals settle

            -- Golden model (layout matches DUT address generation)
            for b in 0 to batch-1 loop
              for y in 0 to oh-1 loop
                for x in 0 to ow-1 loop
                  for oc in 0 to cout-1 loop
                    acc := (others => '0');
                    for ky in 0 to k-1 loop
                      for kx in 0 to k-1 loop
                        for ic in 0 to cin-1 loop
                          ih := y*s + ky - p;
                          iw := x*s + kx - p;
                          if ih >= 0 and ih < IN_H and iw >= 0 and iw < IN_W then
                            prod := in_mem(((b*IN_H + ih)*IN_W + iw)*cin + ic) *
                                    w_mem(((ky*k + kx)*cin + ic)*cout + oc);
                            acc := acc + to_signed(prod, 32);
                          end if;
                        end loop;
                      end loop;
                    end loop;
                    exp_val  := shift_right(acc, FRAC_WIDTH) + to_signed(b_mem(oc), 32);
                    exp_word := to_integer(resize(exp_val, DATA_WIDTH));
                    exp_addr := ((b*oh + y)*ow + x)*cout + oc;
                    expected(exp_addr) := exp_word;
                  end loop;
                end loop;
              end loop;
            end loop;

            -- Wait for DUT to be ready, then pulse start
            wait until rising_edge(clk) and ready = '1';
            start <= '1';
            wait until rising_edge(clk);
            start <= '0';

            -- Wait for done with timeout
            cycles := 0;
            while done /= '1' loop
                wait until rising_edge(clk);
                cycles := cycles + 1;
                assert cycles < 200000
                    report name & ": TIMEOUT waiting for done" severity failure;
            end loop;
            wait until rising_edge(clk);   -- make sure last write is captured
            wait for 1 ns;

            -- Compare
            assert wr_count = n_outputs
                report name & ": expected " & integer'image(n_outputs) &
                       " output writes, got " & integer'image(wr_count)
                severity error;
            if wr_count /= n_outputs then
                errors := errors + 1;
            end if;

            for i in 0 to n_outputs - 1 loop
                if out_mem(i) /= expected(i) then
                    errors := errors + 1;
                    if errors <= 20 then
                        report name & ": MISMATCH at addr " & integer'image(i) &
                               " expected " & integer'image(expected(i)) &
                               " got " & integer'image(out_mem(i))
                            severity error;
                    end if;
                end if;
            end loop;

            if errors = 0 then
                report name & ": PASSED (" & integer'image(n_outputs) &
                       " outputs, " & integer'image(cycles) & " cycles)" severity note;
            else
                report name & ": FAILED with " & integer'image(errors) & " error(s)"
                    severity error;
            end if;
            total_errors := total_errors + errors;

            -- Let DUT return to IDLE
            wait until rising_edge(clk);
            wait until rising_edge(clk);
        end procedure;

    begin
        -- Reset
        rst <= '1';
        wait for 5 * CLK_PERIOD;
        wait until rising_edge(clk);
        rst <= '0';
        wait until rising_edge(clk);

        assert ready = '1' report "DUT not ready after reset" severity error;

        if IO_MODE then
            -- Read input pixels from INPUT_FILE into in_mem
            file_open(fin, INPUT_FILE, read_mode);
            idx := 0;
            while not endfile(fin) loop
                readline(fin, ln);
                read(ln, v);
                if idx < MEM_SIZE then
                    in_mem(idx) <= v;
                end if;
                idx := idx + 1;
            end loop;
            file_close(fin);
            report "IO_MODE: Loaded " & integer'image(idx) & " pixels from " & INPUT_FILE;

            -- initialize weights and biases (simple defaults)
            for i in 0 to K_G*K_G*IMG_C*COUT_G - 1 loop
                w_mem(i) <= 2**FRAC_WIDTH; -- Q-format '1'
            end loop;
            for i in 0 to COUT_G - 1 loop
                b_mem(i) <= 0;
            end loop;

            -- configure and run single test using provided generics
            config.kernel_size     <= K_G;
            config.stride          <= S_G;
            config.padding         <= P_G;
            config.input_channels  <= IMG_C;
            config.output_channels <= COUT_G;
            batch_size             <= 1;

            -- compute output dimensions and expected writes
            oh := (IN_H + 2*P_G - K_G)/S_G + 1;
            ow := (IN_W + 2*P_G - K_G)/S_G + 1;
            n_outputs := 1 * oh * ow * COUT_G;

            -- clear output capture
            clr_out <= '1';
            wait until rising_edge(clk);
            clr_out <= '0';
            wait for 1 ns;

            -- start DUT
            wait until rising_edge(clk) and ready = '1';
            start <= '1';
            wait until rising_edge(clk);
            start <= '0';

            -- wait for done
            cycles := 0;
            while done /= '1' loop
                wait until rising_edge(clk);
                cycles := cycles + 1;
                assert cycles < 200000 report "IO_MODE: TIMEOUT waiting for done" severity failure;
            end loop;
            wait until rising_edge(clk);

            -- dump outputs to OUTPUT_FILE
            file_open(io_out, OUTPUT_FILE, write_mode);
            for i in 0 to n_outputs-1 loop
                if out_mem(i) = UNWRITTEN then
                    write(L, 0);
                else
                    write(L, out_mem(i));
                end if;
                writeline(io_out, L);
            end loop;
            file_close(io_out);
            report "IO_MODE: Wrote " & integer'image(n_outputs) & " outputs to " & OUTPUT_FILE;

        else
            --          k  s  p  cin cout batch
            run_test(   3, 1, 0,  1,  1,   1,  "T1: 3x3, s1, p0, 1->1 ch, batch 1");
            run_test(   3, 1, 0,  2,  2,   1,  "T2: 3x3, s1, p0, 2->2 ch, batch 1");
            run_test(   3, 2, 0,  3,  4,   2,  "T3: 3x3, s2, p0, 3->4 ch, batch 2");
            run_test(   1, 1, 0,  2,  3,   1,  "T4: 1x1, s1, p0, 2->3 ch (pointwise)");
            run_test(   5, 1, 1,  1,  2,   1,  "T5: 5x5, s1, p1, 1->2 ch");
            run_test(   3, 1, 1,  1,  1,   1,  "T6: 3x3, s1, p1 (zero padding)");

            if total_errors = 0 then
                report "*** ALL TESTS PASSED ***" severity note;
            else
                report "*** TESTS FAILED: " & integer'image(total_errors) & " total error(s) ***"
                    severity error;
                assert total_errors = 0 report "Convolution regression failed" severity failure;
            end if;

            std.env.stop;   -- VHDL-2008; replace with 'wait;' for older tools
            wait;
        end if;
    end process;

end sim;