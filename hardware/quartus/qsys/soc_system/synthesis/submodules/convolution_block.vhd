-- Convolution Engine za FPGA
-- Implementacija 2D konvolucije sa padding, stride i bias

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use work.accel_types.all;

entity convolution_block is
    generic (
        INPUT_HEIGHT : integer := 8;
        INPUT_WIDTH : integer := 8
    );
    port (
        clk : in std_logic;
        rst : in std_logic;
        
        -- Control signals
        start : in std_logic;
        done : out std_logic;
        ready : out std_logic;
        
        -- Configuration
        config : in conv_config_t;
        batch_size : in integer range 1 to MAX_BATCH_SIZE;
        
        -- Input data interface
        input_valid : in std_logic;
        input_data : in std_logic_vector(DATA_WIDTH-1 downto 0);
        input_addr : out std_logic_vector(15 downto 0);
        input_read_en : out std_logic;
        
        -- Weight data interface
        weight_valid : in std_logic;
        weight_data : in std_logic_vector(DATA_WIDTH-1 downto 0);
        weight_addr : out std_logic_vector(15 downto 0);
        weight_read_en : out std_logic;
        
        -- Bias data interface
        bias_valid : in std_logic;
        bias_data : in std_logic_vector(DATA_WIDTH-1 downto 0);
        bias_addr : out std_logic_vector(7 downto 0);
        bias_read_en : out std_logic;
        
        -- Output data interface
        output_valid : out std_logic;
        output_data : out std_logic_vector(DATA_WIDTH-1 downto 0);
        output_addr : out std_logic_vector(15 downto 0);
        output_write_en : out std_logic
    );
end convolution_block;

architecture behavioral of convolution_block is
    
    -- State machine
    type state_t is (IDLE, CONV_COMPUTE, WRITE_OUTPUT, DONE_ST);
    signal current_state, next_state : state_t;
    
    -- Counters
    signal batch_cnt : integer range 0 to MAX_BATCH_SIZE-1;
    signal out_ch_cnt : integer range 0 to MAX_CHANNELS-1;
    signal out_h_cnt : integer range 0 to MAX_HEIGHT-1;
    signal out_w_cnt : integer range 0 to MAX_WIDTH-1;
    signal in_ch_cnt : integer range 0 to MAX_CHANNELS-1;
    signal kh_cnt : integer range 0 to MAX_KERNEL_SIZE-1;
    signal kw_cnt : integer range 0 to MAX_KERNEL_SIZE-1;
    
    -- Calculated dimensions
    signal output_height : integer range 0 to MAX_HEIGHT;
    signal output_width : integer range 0 to MAX_WIDTH;
    
    -- Accumulator za konvoluciju
    signal conv_accumulator : signed(31 downto 0);
    signal mult_result : signed(31 downto 0);
    
    -- Pipeline registri
    signal input_reg : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal weight_reg : std_logic_vector(DATA_WIDTH-1 downto 0);
    signal bias_reg : std_logic_vector(DATA_WIDTH-1 downto 0);

    -- Track whether all output positions have been written
    signal last_output : std_logic;  -- '1' when current output is the very last one
    -- Track whether the kernel window for current output position is complete
    signal kernel_done : std_logic;
    
    -- Helper funkcije
    function calculate_output_dim(input_dim, kernel_size, padding, stride : integer) return integer is
    begin
        return (input_dim + 2 * padding - kernel_size) / stride + 1;
    end function;
    
    function calculate_input_pos(out_pos, stride, k_pos, padding : integer) return integer is
    begin
        return out_pos * stride + k_pos - padding;
    end function;

begin
    
    -- Izračunaj output dimenzije
    output_height <= calculate_output_dim(INPUT_HEIGHT, config.kernel_size, config.padding, config.stride);
    output_width <= calculate_output_dim(INPUT_WIDTH, config.kernel_size, config.padding, config.stride);

    -- Detect when kernel iteration for current output position is at its last element
    kernel_done <= '1' when kh_cnt = config.kernel_size - 1 and
                            kw_cnt = config.kernel_size - 1 and
                            in_ch_cnt = config.input_channels - 1
                       else '0';

    -- Detect when current output position is the very last one
    last_output <= '1' when batch_cnt = batch_size - 1 and
                            out_ch_cnt = config.output_channels - 1 and
                            out_h_cnt = output_height - 1 and
                            out_w_cnt = output_width - 1
                       else '0';

    -- State machine process
    process(clk, rst)
    begin
        if rst = '1' then
            current_state <= IDLE;
            batch_cnt <= 0;
            out_ch_cnt <= 0;
            out_h_cnt <= 0;
            out_w_cnt <= 0;
            in_ch_cnt <= 0;
            kh_cnt <= 0;
            kw_cnt <= 0;
            conv_accumulator <= (others => '0');
            mult_result <= (others => '0');
        elsif rising_edge(clk) then
            current_state <= next_state;
            
            case current_state is
                when IDLE =>
                    batch_cnt <= 0;
                    out_ch_cnt <= 0;
                    out_h_cnt <= 0;
                    out_w_cnt <= 0;
                    in_ch_cnt <= 0;
                    kh_cnt <= 0;
                    kw_cnt <= 0;
                    conv_accumulator <= (others => '0');
                    mult_result <= (others => '0');
                    
                when CONV_COMPUTE =>
                    -- Multiply-accumulate operacija
                    if input_valid = '1' and weight_valid = '1' then
                        -- Compute multiply (registered for timing)
                        mult_result <= signed(input_data) * signed(weight_data);
                        -- Accumulate: add previous mult result to accumulator
                        -- (1-cycle pipeline: we accumulate the result from the
                        --  previous cycle's multiply)
                        conv_accumulator <= conv_accumulator + mult_result;
                        
                        -- Advance kernel counters (kw -> kh -> in_ch)
                        if kw_cnt < config.kernel_size - 1 then
                            kw_cnt <= kw_cnt + 1;
                        elsif kh_cnt < config.kernel_size - 1 then
                            kw_cnt <= 0;
                            kh_cnt <= kh_cnt + 1;
                        elsif in_ch_cnt < config.input_channels - 1 then
                            kw_cnt <= 0;
                            kh_cnt <= 0;
                            in_ch_cnt <= in_ch_cnt + 1;
                        else
                            -- Kernel window complete for this output position.
                            -- Counters stay at max - they'll be reset in WRITE_OUTPUT.
                            null;
                        end if;
                    end if;
                    
                when WRITE_OUTPUT =>
                    -- Reset kernel counters and accumulator for next output position.
                    kw_cnt <= 0;
                    kh_cnt <= 0;
                    in_ch_cnt <= 0;
                    conv_accumulator <= (others => '0');
                    mult_result <= (others => '0');
                    
                    -- Advance output position counters (w -> h -> ch -> batch)
                    if not (last_output = '1') then
                        if out_w_cnt < output_width - 1 then
                            out_w_cnt <= out_w_cnt + 1;
                        elsif out_h_cnt < output_height - 1 then
                            out_w_cnt <= 0;
                            out_h_cnt <= out_h_cnt + 1;
                        elsif out_ch_cnt < config.output_channels - 1 then
                            out_w_cnt <= 0;
                            out_h_cnt <= 0;
                            out_ch_cnt <= out_ch_cnt + 1;
                        elsif batch_cnt < batch_size - 1 then
                            out_w_cnt <= 0;
                            out_h_cnt <= 0;
                            out_ch_cnt <= 0;
                            batch_cnt <= batch_cnt + 1;
                        end if;
                    end if;

                when others =>
                    null;
            end case;
        end if;
    end process;
    
    -- Next state logic
    process(current_state, start, kernel_done, last_output, input_valid, weight_valid)
    begin
        next_state <= current_state;
        
        case current_state is
            when IDLE =>
                if start = '1' then
                    next_state <= CONV_COMPUTE;
                end if;
                
            when CONV_COMPUTE =>
                -- When kernel is done and we get the last valid data,
                -- transition to write the output
                if input_valid = '1' and weight_valid = '1' and kernel_done = '1' then
                    next_state <= WRITE_OUTPUT;
                end if;
                
            when WRITE_OUTPUT =>
                if last_output = '1' then
                    -- All outputs written, we're done
                    next_state <= DONE_ST;
                else
                    -- More output positions to compute
                    next_state <= CONV_COMPUTE;
                end if;
                
            when DONE_ST =>
                next_state <= IDLE;
                
            when others =>
                next_state <= IDLE;
        end case;
    end process;
    
    -- Address generation
    process(current_state, batch_cnt, out_ch_cnt, out_h_cnt, out_w_cnt, 
            in_ch_cnt, kh_cnt, kw_cnt, config, output_height, output_width)
        variable in_h_pos, in_w_pos : integer;
    begin
        -- Input address calculation
        in_h_pos := calculate_input_pos(out_h_cnt, config.stride, kh_cnt, config.padding);
        in_w_pos := calculate_input_pos(out_w_cnt, config.stride, kw_cnt, config.padding);
        
        if in_h_pos >= 0 and in_h_pos < INPUT_HEIGHT and 
           in_w_pos >= 0 and in_w_pos < INPUT_WIDTH then
            input_addr <= std_logic_vector(to_unsigned(
                ((batch_cnt * INPUT_HEIGHT + in_h_pos) * INPUT_WIDTH + in_w_pos) * 
                config.input_channels + in_ch_cnt, 16));
        else
            input_addr <= (others => '0');
        end if;
        
        -- Weight address calculation
        weight_addr <= std_logic_vector(to_unsigned(
            ((kh_cnt * config.kernel_size + kw_cnt) * config.input_channels + in_ch_cnt) * 
            config.output_channels + out_ch_cnt, 16));
        
        -- Bias address
        bias_addr <= std_logic_vector(to_unsigned(out_ch_cnt, 8));
        
        -- Output address
        output_addr <= std_logic_vector(to_unsigned(
            ((batch_cnt * output_height + out_h_cnt) * output_width + out_w_cnt) * 
            config.output_channels + out_ch_cnt, 16));
    end process;
    
    -- Control signals
    input_read_en <= '1' when current_state = CONV_COMPUTE else '0';
    weight_read_en <= '1' when current_state = CONV_COMPUTE else '0';
    bias_read_en <= '1' when current_state = CONV_COMPUTE and 
                             in_ch_cnt = 0 and kh_cnt = 0 and kw_cnt = 0 else '0';
    
    output_write_en <= '1' when current_state = WRITE_OUTPUT else '0';
    
    -- Output data: accumulator result (with final multiply added) + bias
    -- The accumulator contains the sum of all but the last multiply.
    -- We add the last mult_result and bias in the output.
    output_data <= std_logic_vector(
        resize(
            shift_right(conv_accumulator + mult_result, FRAC_WIDTH) + 
            resize(signed(bias_reg), 32),
            DATA_WIDTH
        )
    );
    
    output_valid <= '1' when current_state = WRITE_OUTPUT else '0';
    ready <= '1' when current_state = IDLE else '0';
    done <= '1' when current_state = DONE_ST else '0';

    -- Pipeline registri for bias
    process(clk)
    begin
        if rising_edge(clk) then
            if bias_valid = '1' then
                bias_reg <= bias_data;
            end if;
        end if;
    end process;

end behavioral;