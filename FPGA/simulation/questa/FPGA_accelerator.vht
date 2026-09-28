library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use work.accel_types.all;

entity tb_convolution_block is
end tb_convolution_block;

architecture sim of tb_convolution_block is

    -- Parametri slike i kernel
    constant INPUT_HEIGHT : integer := 256;
    constant INPUT_WIDTH  : integer := 256;
    constant INPUT_CHANNELS : integer := 3; -- RGB
    constant KERNEL_SIZE : integer := 3;
    constant DATA_WID : integer := DATA_WIDTH; -- 16-bit fixed point
    constant OUTPUT_HEIGHT : integer := INPUT_HEIGHT - KERNEL_SIZE + 1;
    constant OUTPUT_WIDTH  : integer := INPUT_WIDTH  - KERNEL_SIZE + 1;
    
    -- Signali
    signal clk : std_logic := '0';
    signal rst : std_logic := '0';
    signal start : std_logic := '0';
    signal done  : std_logic;
    signal ready : std_logic;
    
    -- Config
    signal config : conv_config_t;
    signal batch_size : integer := 1;
    
    -- Input
    signal input_valid : std_logic := '0';
    signal input_data  : std_logic_vector(DATA_WID-1 downto 0) := (others=>'0');
    signal input_addr  : std_logic_vector(15 downto 0);
    signal input_read_en : std_logic;
    
    -- Weight
    signal weight_valid : std_logic := '0';
    signal weight_data  : std_logic_vector(DATA_WID-1 downto 0) := (others=>'0');
    signal weight_addr  : std_logic_vector(15 downto 0);
    signal weight_read_en : std_logic;
    
    -- Bias
    signal bias_valid : std_logic := '0';
    signal bias_data  : std_logic_vector(DATA_WID-1 downto 0) := (others=>'0');
    signal bias_addr  : std_logic_vector(7 downto 0);
    signal bias_read_en : std_logic;
    
    -- Output
    signal output_valid : std_logic;
    signal output_data  : std_logic_vector(DATA_WID-1 downto 0);
    signal output_addr  : std_logic_vector(15 downto 0);
    signal output_write_en : std_logic;

    -- Counteri
    signal pixel_cnt : integer := 0;
    signal channel_cnt : integer := 0;

begin

    -- DUT
    dut: entity work.convolution_block
        generic map (
            INPUT_HEIGHT => INPUT_HEIGHT,
            INPUT_WIDTH  => INPUT_WIDTH
        )
        port map (
            clk => clk, rst => rst,
            start => start, done => done, ready => ready,
            config => config,
            batch_size => batch_size,
            input_valid => input_valid,
            input_data  => input_data,
            input_addr  => input_addr,
            input_read_en => input_read_en,
            weight_valid => weight_valid,
            weight_data  => weight_data,
            weight_addr  => weight_addr,
            weight_read_en => weight_read_en,
            bias_valid => bias_valid,
            bias_data  => bias_data,
            bias_addr  => bias_addr,
            bias_read_en => bias_read_en,
            output_valid => output_valid,
            output_data  => output_data,
            output_addr  => output_addr,
            output_write_en => output_write_en
        );

    -- Clock 50 MHz
    clk <= not clk after 10 ns;

    -- Reset i start
    process
    begin
        rst <= '1';
        wait for 100 ns;
        rst <= '0';
        wait for 50 ns;
        
        -- Config
        config.input_channels <= INPUT_CHANNELS;
        config.output_channels <= 1; -- test 1 filter
        config.kernel_size <= KERNEL_SIZE;
        config.stride <= 1;
        config.padding <= 0;

        -- Start convolution
        start <= '1';
        wait for 20 ns;
        start <= '0';
        
        wait until done = '1';
        report "Convolution finished!" severity note;
        wait;
    end process;

    -- Input feeder (RGB)
    process(clk)
        variable r_val, g_val, b_val : integer := 0;
    begin
        if rising_edge(clk) then
            input_valid <= '0';
            
            if input_read_en = '1' then
                input_valid <= '1';
                
                -- Simple test pattern: R, G, B sequential
                case channel_cnt is
                    when 0 =>
                        r_val := pixel_cnt mod 256;
                        input_data <= std_logic_vector(to_signed(r_val*256, DATA_WID));
                        channel_cnt <= 1;
                    when 1 =>
                        g_val := pixel_cnt mod 256;
                        input_data <= std_logic_vector(to_signed(g_val*256, DATA_WID));
                        channel_cnt <= 2;
                    when 2 =>
                        b_val := pixel_cnt mod 256;
                        input_data <= std_logic_vector(to_signed(b_val*256, DATA_WID));
                        channel_cnt <= 0;
                        pixel_cnt <= pixel_cnt + 1; -- next pixel
                    when others =>
                        channel_cnt <= 0;
                end case;
            end if;
        end if;
    end process;

    -- Weight feeder (identity kernel)
    process(clk)
    begin
        if rising_edge(clk) then
            weight_valid <= '0';
            if weight_read_en = '1' then
                weight_valid <= '1';
                if weight_addr = x"04" then
                    weight_data <= std_logic_vector(to_signed(256, DATA_WID)); -- center = 1.0
                else
                    weight_data <= (others=>'0');
                end if;
            end if;
        end if;
    end process;

    -- Bias feeder
    process(clk)
    begin
        if rising_edge(clk) then
            bias_valid <= '0';
            if bias_read_en = '1' then
                bias_valid <= '1';
                bias_data <= (others=>'0'); -- zero bias
            end if;
        end if;
    end process;

    -- Output monitor
    process(clk)
    begin
        if rising_edge(clk) then
            if output_valid = '1' then
                report "OUTPUT addr=" & integer'image(to_integer(unsigned(output_addr))) &
                       " value=" & integer'image(to_integer(signed(output_data))) severity note;
            end if;
        end if;
    end process;

end sim;
