-- Package za definiranje pooling operacija
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use IEEE.STD_LOGIC_SIGNED.ALL;

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use work.accel_types.all;
entity pooling_block is
    generic (
        INPUT_HEIGHT : integer := 32;
        INPUT_WIDTH : integer := 32;
        PARALLEL_CHANNELS : integer := 4  -- Broj kanala koji se procesiraju paralelno
    );
    port (
        clk : in std_logic;
        rst : in std_logic;
        
        -- Control signals
        start : in std_logic;
        done : out std_logic;
        ready : out std_logic;
        busy : out std_logic;
        
        -- Configuration
        config : in pooling_config_t;
        batch_size : in integer range 1 to MAX_BATCH_SIZE;
        
        -- Input data interface
        input_valid : in std_logic;
        input_data : in data_array_1d(0 to PARALLEL_CHANNELS-1);
        input_addr : out std_logic_vector(17 downto 0);  -- Veća adresa za 4D tensor
        input_read_en : out std_logic;
        
        -- Output data interface
        output_valid : out std_logic;
        output_data : out data_array_1d(0 to PARALLEL_CHANNELS-1);
        output_addr : out std_logic_vector(17 downto 0);
        output_write_en : out std_logic;
        
        -- Debug/monitoring
        current_position : out std_logic_vector(31 downto 0);
        processing_cycles : out std_logic_vector(31 downto 0);
        pool_window_count : out std_logic_vector(15 downto 0)
    );
end pooling_block;

architecture behavioral of pooling_block is
    
    -- State machine
    type state_t is (IDLE, SETUP_DIMS, POOL_COMPUTE, WRITE_OUTPUT, DONE_ST);
    signal current_state, next_state : state_t;
    
    -- Calculated dimensions
    signal output_height : integer range 0 to MAX_HEIGHT;
    signal output_width : integer range 0 to MAX_WIDTH;
    
    -- Counters
    signal batch_cnt : integer range 0 to MAX_BATCH_SIZE-1;
    signal channel_cnt : integer range 0 to MAX_CHANNELS-1;
    signal out_h_cnt : integer range 0 to MAX_HEIGHT-1;
    signal out_w_cnt : integer range 0 to MAX_WIDTH-1;
    signal pool_h_cnt : integer range 0 to MAX_POOL_SIZE-1;
    signal pool_w_cnt : integer range 0 to MAX_POOL_SIZE-1;
    
    -- Pooling accumulatori za paralelne kanale
    type pool_accumulator_t is record
        value : signed(31 downto 0);
        count : integer range 0 to MAX_POOL_SIZE*MAX_POOL_SIZE;
        initialized : std_logic;
    end record;
    
    type pool_acc_array_t is array (0 to PARALLEL_CHANNELS-1) of pool_accumulator_t;
    signal pool_accumulators : pool_acc_array_t;
    
    -- Buffer za window podatke
    type window_buffer_t is array (0 to MAX_POOL_SIZE-1, 0 to MAX_POOL_SIZE-1) of data_array_1d(0 to PARALLEL_CHANNELS-1);
    signal window_buffer : window_buffer_t;
    signal window_valid : std_logic_vector(0 to MAX_POOL_SIZE*MAX_POOL_SIZE-1);
    
    -- Control signali
    signal pool_operation_active : std_logic;
    signal window_complete : std_logic;
    signal cycle_counter : unsigned(31 downto 0);
    signal window_counter : unsigned(15 downto 0);
    
    -- Helper funkcije
    function calculate_output_dim(input_dim, pool_size, stride : integer) return integer is
    begin
        return (input_dim - pool_size) / stride + 1;
    end function;
    
    function max_of_two(a, b : signed) return signed is
    begin
        if a > b then
            return a;
        else
            return b;
        end if;
    end function;
    
    function min_of_two(a, b : signed) return signed is
    begin
        if a < b then
            return a;
        else
            return b;
        end if;
    end function;

begin
    
    -- Izračunaj output dimenzije
    output_height <= calculate_output_dim(INPUT_HEIGHT, config.pool_size, config.stride);
    output_width <= calculate_output_dim(INPUT_WIDTH, config.pool_size, config.stride);
    
    -- Main state machine
    process(clk, rst)
    begin
        if rst = '1' then
            current_state <= IDLE;
            batch_cnt <= 0;
            channel_cnt <= 0;
            out_h_cnt <= 0;
            out_w_cnt <= 0;
            pool_h_cnt <= 0;
            pool_w_cnt <= 0;
            cycle_counter <= (others => '0');
            window_counter <= (others => '0');
            
            -- Reset pool accumulators
            for i in 0 to PARALLEL_CHANNELS-1 loop
                pool_accumulators(i).value <= (others => '0');
                pool_accumulators(i).count <= 0;
                pool_accumulators(i).initialized <= '0';
            end loop;
            
            -- Reset window buffer
            for h in 0 to MAX_POOL_SIZE-1 loop
                for w in 0 to MAX_POOL_SIZE-1 loop
                    for c in 0 to PARALLEL_CHANNELS-1 loop
                        window_buffer(h, w)(c) <= (others => '0');
                    end loop;
                end loop;
            end loop;
            
            window_valid <= (others => '0');
            
        elsif rising_edge(clk) then
            current_state <= next_state;
            cycle_counter <= cycle_counter + 1;
            
            case current_state is
                when IDLE =>
                    batch_cnt <= 0;
                    channel_cnt <= 0;
                    out_h_cnt <= 0;
                    out_w_cnt <= 0;
                    pool_h_cnt <= 0;
                    pool_w_cnt <= 0;
                    window_counter <= (others => '0');
                    
                    for i in 0 to PARALLEL_CHANNELS-1 loop
                        pool_accumulators(i).value <= (others => '0');
                        pool_accumulators(i).count <= 0;
                        pool_accumulators(i).initialized <= '0';
                    end loop;
                    
                when SETUP_DIMS =>
                    -- Pripremi za pooling operaciju
                    for i in 0 to PARALLEL_CHANNELS-1 loop
                        pool_accumulators(i).value <= (others => '0');
                        pool_accumulators(i).count <= 0;
                        pool_accumulators(i).initialized <= '0';
                    end loop;
                    
                when POOL_COMPUTE =>
                    if input_valid = '1' and pool_operation_active = '1' then
                        window_counter <= window_counter + 1;
                        
                        -- Load data into window buffer
                        for c in 0 to PARALLEL_CHANNELS-1 loop
                            window_buffer(pool_h_cnt, pool_w_cnt)(c) <= input_data(c);
                        end loop;
                        
                        -- Process pooling za svaki kanal
                        for c in 0 to PARALLEL_CHANNELS-1 loop
                            case config.pool_type is
                                when MAX_POOL =>
                                    if pool_accumulators(c).initialized = '0' then
                                        pool_accumulators(c).value <= resize(signed(input_data(c)), 32);
                                        pool_accumulators(c).initialized <= '1';
                                    else
                                        if signed(input_data(c)) > pool_accumulators(c).value(DATA_WIDTH-1 downto 0) then
                                            pool_accumulators(c).value <= resize(signed(input_data(c)), 32);
                                        end if;
                                    end if;
                                    
                                when MIN_POOL =>
                                    if pool_accumulators(c).initialized = '0' then
                                        pool_accumulators(c).value <= resize(signed(input_data(c)), 32);
                                        pool_accumulators(c).initialized <= '1';
                                    else
                                        if signed(input_data(c)) < pool_accumulators(c).value(DATA_WIDTH-1 downto 0) then
                                            pool_accumulators(c).value <= resize(signed(input_data(c)), 32);
                                        end if;
                                    end if;
                                    
                                when AVERAGE_POOL | SUM_POOL =>
                                    pool_accumulators(c).value <= pool_accumulators(c).value + resize(signed(input_data(c)), 32);
                                    
                                when others =>
                                    pool_accumulators(c).value <= resize(signed(input_data(c)), 32);
                            end case;
                            
                            pool_accumulators(c).count <= pool_accumulators(c).count + 1;
                        end loop;
                        
                        -- Increment pool window counters
                        if pool_w_cnt < config.pool_size - 1 then
                            pool_w_cnt <= pool_w_cnt + 1;
                        elsif pool_h_cnt < config.pool_size - 1 then
                            pool_w_cnt <= 0;
                            pool_h_cnt <= pool_h_cnt + 1;
                        else
                            -- Window complete, move to next output position
                            pool_w_cnt <= 0;
                            pool_h_cnt <= 0;
                            
                            -- Finalize pooling results
                            for c in 0 to PARALLEL_CHANNELS-1 loop
                                if config.pool_type = AVERAGE_POOL and pool_accumulators(c).count > 0 then
                                    pool_accumulators(c).value <= 
                                        pool_accumulators(c).value / pool_accumulators(c).count;
                                end if;
                            end loop;
                            
                            -- Move to next output position
                            if out_w_cnt < output_width - 1 then
                                out_w_cnt <= out_w_cnt + 1;
                            elsif out_h_cnt < output_height - 1 then
                                out_w_cnt <= 0;
                                out_h_cnt <= out_h_cnt + 1;
                            elsif channel_cnt + PARALLEL_CHANNELS < config.channels then
                                out_w_cnt <= 0;
                                out_h_cnt <= 0;
                                channel_cnt <= channel_cnt + PARALLEL_CHANNELS;
                            elsif batch_cnt < batch_size - 1 then
                                out_w_cnt <= 0;
                                out_h_cnt <= 0;
                                channel_cnt <= 0;
                                batch_cnt <= batch_cnt + 1;
                            end if;
                        end if;
                    end if;
                    
                when WRITE_OUTPUT =>
                    -- Output results
                    null;
                    
                when DONE_ST =>
                    null;
            end case;
        end if;
    end process;
    
    -- Next state logic
    process(current_state, start, batch_cnt, channel_cnt, out_h_cnt, out_w_cnt,
            pool_h_cnt, pool_w_cnt, batch_size, config, output_height, output_width)
    begin
        next_state <= current_state;
        
        case current_state is
            when IDLE =>
                if start = '1' then
                    next_state <= SETUP_DIMS;
                end if;
                
            when SETUP_DIMS =>
                next_state <= POOL_COMPUTE;
                
            when POOL_COMPUTE =>
                if batch_cnt = batch_size - 1 and
                   channel_cnt + PARALLEL_CHANNELS >= config.channels and
                   out_h_cnt = output_height - 1 and
                   out_w_cnt = output_width - 1 and
                   pool_h_cnt = config.pool_size - 1 and
                   pool_w_cnt = config.pool_size - 1 then
                    next_state <= WRITE_OUTPUT;
                end if;
                
            when WRITE_OUTPUT =>
                next_state <= DONE_ST;
                
            when DONE_ST =>
                next_state <= IDLE;
                
            when others =>
                next_state <= IDLE;
        end case;
    end process;
    
    -- Input address generation
    input_address_gen: process(batch_cnt, out_h_cnt, out_w_cnt, pool_h_cnt, pool_w_cnt, 
            channel_cnt, config)
        variable in_h, in_w : integer;
        variable base_addr : integer;
    begin
        in_h := out_h_cnt * config.stride + pool_h_cnt;
        in_w := out_w_cnt * config.stride + pool_w_cnt;
        
        -- 4D tensor indexing: [batch][height][width][channel]
        base_addr := ((batch_cnt * INPUT_HEIGHT + in_h) * INPUT_WIDTH + in_w) * 
                    config.channels + channel_cnt;
        
        input_addr <= std_logic_vector(to_unsigned(base_addr, 18));
    end process;
    
    -- Output address generation
    output_address_gen: process(batch_cnt, out_h_cnt, out_w_cnt, channel_cnt, config, output_height, output_width)
        variable out_addr : integer;
    begin
        out_addr := ((batch_cnt * output_height + out_h_cnt) * output_width + out_w_cnt) * 
                   config.channels + channel_cnt;
        
        output_addr <= std_logic_vector(to_unsigned(out_addr, 18));
    end process;
    
    -- Output data generation
    output_data_gen: process(clk)
    begin
        if rising_edge(clk) then
            if current_state = WRITE_OUTPUT or 
               (current_state = POOL_COMPUTE and 
                pool_h_cnt = config.pool_size - 1 and 
                pool_w_cnt = config.pool_size - 1) then
                
                for c in 0 to PARALLEL_CHANNELS-1 loop
                    output_data(c) <= std_logic_vector(pool_accumulators(c).value(DATA_WIDTH-1 downto 0));
                end loop;
            else
                for c in 0 to PARALLEL_CHANNELS-1 loop
                    output_data(c) <= (others => '0');
                end loop;
            end if;
        end if;
    end process;
    
    -- Control signals
    pool_operation_active <= '1' when current_state = POOL_COMPUTE else '0';
    window_complete <= '1' when pool_h_cnt = config.pool_size - 1 and 
                              pool_w_cnt = config.pool_size - 1 else '0';
    
    input_read_en <= pool_operation_active;
    
    output_valid <= '1' when current_state = WRITE_OUTPUT or 
                            (current_state = POOL_COMPUTE and window_complete = '1') else '0';
    
    output_write_en <= '1' when current_state = WRITE_OUTPUT or 
                            (current_state = POOL_COMPUTE and window_complete = '1') else '0';
    
    -- Status signals
    ready <= '1' when current_state = IDLE else '0';
    busy <= '1' when current_state /= IDLE and current_state /= DONE_ST else '0';
    done <= '1' when current_state = DONE_ST else '0';
    
    -- Debug outputs
    current_position <= std_logic_vector(to_unsigned(batch_cnt, 8)) &
                       std_logic_vector(to_unsigned(out_h_cnt, 8)) &
                       std_logic_vector(to_unsigned(out_w_cnt, 8)) &
                       std_logic_vector(to_unsigned(channel_cnt, 8));
    
    processing_cycles <= std_logic_vector(cycle_counter);
    pool_window_count <= std_logic_vector(window_counter);

end behavioral;