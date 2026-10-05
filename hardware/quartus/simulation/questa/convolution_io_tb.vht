-- simple_conv_test
-- File-driven testbench for convolution_block, made to work with the Python
-- flow (PNG -> image.txt -> vsim -> output.txt -> PNG).
--
--   image.txt  : one integer per line, order = y, x, channel (R,G,B)  (HWC)
--   output.txt : one line per pixel "r g b", raster order
--
-- Image size is passed from Python:  vsim -gIMG_WIDTH=.. -gIMG_HEIGHT=..
-- Fixed configuration: 3x3 kernel, stride 1, padding 1, 3 -> 3 channels
-- (output size == input size).

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use std.textio.all;
use work.accel_types.all;

entity simple_conv_test is
    generic (
        IMG_WIDTH   : integer := 128;
        IMG_HEIGHT  : integer := 128;
        KERNEL_TYPE : integer := 4;    -- 0 identity, 1 box blur, 2 sharpen, 3 edge, 4 legacy
        INPUT_FILE  : string  := "image.txt";
        OUTPUT_FILE : string  := "output.txt"
    );
end simple_conv_test;

architecture sim of simple_conv_test is

    constant CLK_PERIOD  : time    := 10 ns;
    constant KSIZE  : integer := 3;
    constant PAD    : integer := 1;
    constant STRIDE : integer := 1;
    constant CIN    : integer := 3;
    constant COUT   : integer := 3;
    constant BATCH  : integer := 1;

    constant OUT_H  : integer := (IMG_HEIGHT + 2*PAD - KSIZE)/STRIDE + 1;
    constant OUT_W  : integer := (IMG_WIDTH  + 2*PAD - KSIZE)/STRIDE + 1;
    constant N_IN   : integer := BATCH * IMG_HEIGHT * IMG_WIDTH * CIN;
    constant N_OUT  : integer := BATCH * OUT_H * OUT_W * COUT;

    constant MEM_SIZE   : integer := IMG_HEIGHT * IMG_WIDTH * CIN;      -- 16-bit addresses
    constant UNWRITTEN  : integer := -99999999;

    type int_mem_t  is array (0 to MEM_SIZE-1) of integer;
    type weight_mem_t is array (0 to KSIZE*KSIZE*CIN*COUT-1) of integer;
    type bias_mem_t is array (0 to 255) of integer;

    ---------------------------------------------------------------------------
    -- Elaboration-time helpers
    ---------------------------------------------------------------------------
    impure function load_image return int_mem_t is
        file     f   : text;
        variable st  : file_open_status;
        variable l   : line;
        variable v   : integer;
        variable ok  : boolean;
        variable mem : int_mem_t := (others => 0);
        variable i   : integer := 0;
    begin
        file_open(st, f, INPUT_FILE, read_mode);
        if st /= open_ok then
            report "Cannot open " & INPUT_FILE severity failure;
            return mem;
        end if;
        while not endfile(f) and i < N_IN loop
            readline(f, l);
            read(l, v, ok);
            if ok then
                mem(i) := v;
                i := i + 1;
            end if;
        end loop;
        file_close(f);
        if i /= N_IN then
            report INPUT_FILE & ": expected " & integer'image(N_IN) &
                   " values, read " & integer'image(i) severity failure;
        end if;
        return mem;
    end function;

    -- Weights in fixed point (scale = 2**FRAC_WIDTH), each output channel
    -- filters only its own input channel.
    -- Layout: ((ky*K + kx)*CIN + ic)*COUT + oc   (same as the DUT address gen)
    function make_weights return weight_mem_t is
        variable mem   : weight_mem_t := (others => 0);
        variable scale : integer := 2**FRAC_WIDTH;
        variable w     : integer;
    begin
        for ky in 0 to KSIZE-1 loop
            for kx in 0 to KSIZE-1 loop
                for ic in 0 to CIN-1 loop
                    for oc in 0 to COUT-1 loop
                        w := 0;
                        if ic = oc then
                            case KERNEL_TYPE is
                                when 0 =>                       -- identity
                                    if ky = 1 and kx = 1 then w := scale; end if;
                                when 1 =>                       -- box blur
                                    w := scale / 9;
                                when 2 =>                       -- sharpen
                                    if ky = 1 and kx = 1 then
                                        w := 5 * scale;
                                    elsif ky = 1 or kx = 1 then
                                        w := -scale;
                                    end if;
                                when 3 =>                       -- edge detect
                                    if ky = 1 and kx = 1 then
                                        w := 8 * scale;
                                    else
                                        w := -scale;
                                    end if;
                                when 4 =>                       -- legacy kernel: [1, 0, -1] per row
                                    if kx = 0 then
                                        w := scale;
                                    elsif kx = 2 then
                                        w := -scale;
                                    end if;
                                when others => null;
                            end case;
                        end if;
                        mem(((ky*KSIZE + kx)*CIN + ic)*COUT + oc) := w;
                    end loop;
                end loop;
            end loop;
        end loop;
        return mem;
    end function;

    function make_config return conv_config_t is
        variable c : conv_config_t;
    begin
        c.kernel_size     := KSIZE;
        c.stride          := STRIDE;
        c.padding         := PAD;
        c.input_channels  := CIN;
        c.output_channels := COUT;
        return c;
    end function;

    ---------------------------------------------------------------------------
    -- Signals
    ---------------------------------------------------------------------------
    signal clk, rst, start : std_logic := '0';
    signal done, ready     : std_logic;
    signal config          : conv_config_t := make_config;
    signal batch_size      : integer range 1 to MAX_BATCH_SIZE := BATCH;

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

    signal in_mem   : int_mem_t  := load_image;
    signal w_mem    : weight_mem_t := make_weights;
    signal b_mem    : bias_mem_t := (others => 0);     -- bias = 0
    signal out_mem  : int_mem_t  := (others => UNWRITTEN);
    signal wr_count : integer    := 0;
    signal sim_done : boolean    := false;

begin

    assert N_IN <= MEM_SIZE and N_OUT <= MEM_SIZE
        report "Image too large for 16-bit addressing (HW3 must be <= 65536)"
        severity failure;
    assert IMG_HEIGHT <= MAX_HEIGHT and IMG_WIDTH <= MAX_WIDTH
        report "Image larger than MAX_HEIGHT/MAX_WIDTH in accel_types"
        severity failure;

    ---------------------------------------------------------------------------
    -- Clock (stops when the test is finished so 'run -all' returns)
    ---------------------------------------------------------------------------
    clk_gen : process
    begin
        while not sim_done loop
            clk <= '0';
            wait for CLK_PERIOD/2;
            clk <= '1';
            wait for CLK_PERIOD/2;
        end loop;
        wait;
    end process;

    ---------------------------------------------------------------------------
    -- DUT
    ---------------------------------------------------------------------------
    dut : entity work.convolution_block
        generic map (
            INPUT_HEIGHT => IMG_HEIGHT,
            INPUT_WIDTH  => IMG_WIDTH
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
    -- Combinational memory models
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
    -- Output monitor
    ---------------------------------------------------------------------------
    monitor : process(clk)
    begin
        if rising_edge(clk) then
            if output_write_en = '1' and output_valid = '1' then
                out_mem(to_integer(unsigned(output_addr))) <= to_integer(signed(output_data));
                wr_count <= wr_count + 1;
            end if;
        end if;
    end process;

    ---------------------------------------------------------------------------
    -- Stimulus, checker, file writer
    ---------------------------------------------------------------------------
    stim : process
        variable acc      : signed(31 downto 0);
        variable exp_val  : signed(31 downto 0);
        variable expected : int_mem_t := (others => UNWRITTEN);
        variable ih, iw   : integer;
        variable cycles   : integer := 0;
        variable limit    : integer;
        variable mism     : integer := 0;
        file     fout     : text;
        variable l        : line;
    begin
        report "Image " & integer'image(IMG_WIDTH) & "x" & integer'image(IMG_HEIGHT) &
               ", kernel type " & integer'image(KERNEL_TYPE) severity note;

        -- Reset
        rst <= '1';
        wait for 5 * CLK_PERIOD;
        wait until rising_edge(clk);
        rst <= '0';
        wait until rising_edge(clk);

        -- Start
        wait until rising_edge(clk) and ready = '1';
        start <= '1';
        wait until rising_edge(clk);
        start <= '0';

        -- Wait for done
        limit := N_OUT * (KSIZE * KSIZE * CIN + 4) + 1000;
        while done /= '1' loop
            wait until rising_edge(clk);
            cycles := cycles + 1;
            assert cycles < limit report "TIMEOUT waiting for done" severity failure;
        end loop;
        wait until rising_edge(clk);
        wait for 1 ns;

        report "DUT finished after " & integer'image(cycles) & " cycles, " &
               integer'image(wr_count) & " outputs written" severity note;
        assert wr_count = N_OUT
            report "Expected " & integer'image(N_OUT) & " output writes" severity failure;

        -- Golden model (true zero padding) and comparison
        for bt in 0 to BATCH-1 loop
          for y in 0 to OUT_H-1 loop
            for x in 0 to OUT_W-1 loop
              for oc in 0 to COUT-1 loop
                acc := (others => '0');
                for ky in 0 to KSIZE-1 loop
                  for kx in 0 to KSIZE-1 loop
                    for ic in 0 to CIN-1 loop
                      ih := y*STRIDE + ky - PAD;
                      iw := x*STRIDE + kx - PAD;
                      if ih >= 0 and ih < IMG_HEIGHT and iw >= 0 and iw < IMG_WIDTH then
                        acc := acc + to_signed(
                                 in_mem(((bt*IMG_HEIGHT + ih)*IMG_WIDTH + iw)*CIN + ic) *
                                 w_mem(((ky*KSIZE + kx)*CIN + ic)*COUT + oc), 32);
                      end if;
                    end loop;
                  end loop;
                end loop;
                exp_val := shift_right(acc, FRAC_WIDTH) + to_signed(b_mem(oc), 32);
                expected(((bt*OUT_H + y)*OUT_W + x)*COUT + oc) :=
                    to_integer(resize(exp_val, DATA_WIDTH));
              end loop;
            end loop;
          end loop;
        end loop;

        for i in 0 to N_OUT-1 loop
            if out_mem(i) /= expected(i) then
                mism := mism + 1;
                if mism <= 10 then
                    report "MISMATCH addr " & integer'image(i) &
                           " expected " & integer'image(expected(i)) &
                           " got " & integer'image(out_mem(i)) severity warning;
                end if;
            end if;
        end loop;

        if mism = 0 then
            report "Golden check PASSED (" & integer'image(N_OUT) & " values)" severity note;
        else
            report "Golden check FAILED: " & integer'image(mism) & " of " &
                   integer'image(N_OUT) & " values differ" severity failure;
        end if;

        -- Write output.txt : one line per pixel "r g b"
        file_open(fout, OUTPUT_FILE, write_mode);
        for p in 0 to BATCH * OUT_H * OUT_W - 1 loop
            for c in 0 to COUT-1 loop
                write(l, out_mem(p*COUT + c));
                if c < COUT-1 then
                    write(l, ' ');
                end if;
            end loop;
            writeline(fout, l);
        end loop;
        file_close(fout);
        report "Wrote " & OUTPUT_FILE severity note;

        sim_done <= true;
        wait;
    end process;

end sim;