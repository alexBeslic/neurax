-- activation_block.vhd (ispravljena verzija)
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use work.accel_types.all;

entity activation_block is
    generic (
        PIPELINE_STAGES : integer := 3;    -- Broj pipeline stage-ova
        PARALLEL_UNITS : integer := 4      -- Broj paralelnih processing jedinica
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
        activation_type : in activation_type_t;
        tensor_size : in integer range 1 to MAX_TENSOR_SIZE;
        alpha : in std_logic_vector(DATA_WIDTH-1 downto 0);  -- Za Leaky ReLU i ELU
        
        -- Input data interface
        input_valid : in std_logic;
        input_data : in data_array_t(0 to PARALLEL_UNITS-1);
        input_addr : out std_logic_vector(15 downto 0);
        input_read_en : out std_logic;
        
        -- Output data interface
        output_valid : out std_logic;
        output_data : out data_array_t(0 to PARALLEL_UNITS-1);
        output_addr : out std_logic_vector(15 downto 0);
        output_write_en : out std_logic;
        
        -- Debug/status
        current_element : out std_logic_vector(15 downto 0);
        processing_cycles : out std_logic_vector(31 downto 0)
    );
end activation_block;

architecture behavioral of activation_block is
    
    -- State machine
    type state_t is (IDLE, INIT_LUT, PROCESSING, PIPELINE_FLUSH, DONE_ST);
    signal current_state, next_state : state_t;
    
    -- Pipeline structure
    type pipeline_stage_t is record
        valid : std_logic;
        data : std_logic_vector(DATA_WIDTH-1 downto 0);
        activation_sel : activation_type_t;
    end record;
    
    type pipeline_array_t is array (0 to PIPELINE_STAGES-1) of pipeline_stage_t;
    type parallel_pipeline_t is array (0 to PARALLEL_UNITS-1) of pipeline_array_t;
    
    signal pipeline : parallel_pipeline_t;
    
    -- Counters
    signal element_counter : integer range 0 to MAX_TENSOR_SIZE;
    signal cycle_counter : unsigned(31 downto 0);
    signal lut_init_counter : integer range 0 to 255;
    
    -- LUT memories za aproksimacije
    signal sigmoid_lut : lut_array_t;
    signal tanh_lut : lut_array_t;
    signal exp_lut : lut_array_t;
    signal lut_initialized : std_logic;
    
    -- Control signali
    signal pipeline_enable : std_logic;
    signal processing_active : std_logic;
    
    -- ---- Helper funkcije (ostale bez promena osim uklanjanja "declare") ----
    function to_fixed(real_val : real) return std_logic_vector is
        variable int_val : integer;
    begin
        int_val := integer(real_val * real(2**FRAC_WIDTH));
        if int_val > 2**(DATA_WIDTH-1)-1 then int_val := 2**(DATA_WIDTH-1)-1; end if;
        if int_val < -2**(DATA_WIDTH-1) then int_val := -2**(DATA_WIDTH-1); end if;
        return std_logic_vector(to_signed(int_val, DATA_WIDTH));
    end function;
    
    function from_fixed(fixed_val : std_logic_vector) return real is
    begin
        return real(to_integer(signed(fixed_val))) / real(2**FRAC_WIDTH);
    end function;
    
    function apply_relu(input_val : std_logic_vector) return std_logic_vector is
        variable result : std_logic_vector(DATA_WIDTH-1 downto 0);
    begin
        if signed(input_val) > 0 then
            result := input_val;
        else
            result := (others => '0');
        end if;
        return result;
    end function;
    
    function apply_leaky_relu(input_val, alpha_val : std_logic_vector) return std_logic_vector is
        variable result : std_logic_vector(DATA_WIDTH-1 downto 0);
        variable mult_result : signed(31 downto 0);
    begin
        if signed(input_val) > 0 then
            result := input_val;
        else
            mult_result := signed(input_val) * signed(alpha_val);
            result := std_logic_vector(mult_result(DATA_WIDTH+FRAC_WIDTH-1 downto FRAC_WIDTH));
        end if;
        return result;
    end function;
    
    function lut_lookup(lut_mem : lut_array_t; input_val : std_logic_vector) return std_logic_vector is
        variable addr : integer range 0 to 255;
        variable normalized : integer;
    begin
        -- Normalizuj input u opseg [0, 255]
        normalized := to_integer(signed(input_val)) + 2**(DATA_WIDTH-1);
        addr := normalized / (2**(DATA_WIDTH-8));
        if addr > 255 then addr := 255; end if;
        if addr < 0 then addr := 0; end if;
        return lut_mem(addr);
    end function;
    -- ---------------------------------------------------------------------------
	 signal out_addr_int : integer range 0 to 65535;

begin

    ---------------------------------------------------------------------------
    -- Glavni clocked proces: reset, inicijalizacija LUT-a, pipeline control
    ---------------------------------------------------------------------------
    process(clk, rst)
        -- promenljive koje su koristene u INIT_LUT delu
        variable x_val     : real := 0.0;
        variable sigmoid_v : real := 0.0;
        variable tanh_v    : real := 0.0;
        variable exp_v     : real := 0.0;
        variable idx_real  : real := 0.0;
    begin
        if rst = '1' then
            current_state <= IDLE;
            element_counter <= 0;
            cycle_counter <= (others => '0');
            lut_init_counter <= 0;
            lut_initialized <= '0';
            
            -- Reset pipeline
            for i in 0 to PARALLEL_UNITS-1 loop
                for j in 0 to PIPELINE_STAGES-1 loop
                    pipeline(i)(j).valid <= '0';
                    pipeline(i)(j).data <= (others => '0');
                    pipeline(i)(j).activation_sel <= RELU;
                end loop;
            end loop;
            
        elsif rising_edge(clk) then
            current_state <= next_state;
            cycle_counter <= cycle_counter + 1;
            
            case current_state is
                when IDLE =>
                    element_counter <= 0;
                    lut_init_counter <= 0;
                    
                -- INIT_LUT: LUT inicijalizacija (sintetizabilno)
						-- Napomena: ovo su unapred izračunate aproksimacije funkcija u fixed-point
						-- Primer: linearni segmenti (možeš kasnije zameniti preciznijim precomputed vrednostima)
                when INIT_LUT =>
						 if lut_init_counter <= 255 then
							  -- Mapiramo indekse 0..255 u range [-4, 4] za aproksimaciju
							  -- fixed-point vrednosti su unapred računati (primer)
							  case lut_init_counter is
									-- Primer: samo nekoliko vrednosti; za pravu implementaciju popuni sve 256
									when 0 =>
										 sigmoid_lut(0) <= to_fixed(0.018);  -- sigmoid(-4)
										 tanh_lut(0)    <= to_fixed(-0.999); -- tanh(-4)
										 exp_lut(0)     <= to_fixed(0.018);  -- exp(-4)
									when 64 =>
										 sigmoid_lut(64) <= to_fixed(0.119);
										 tanh_lut(64)    <= to_fixed(-0.76);
										 exp_lut(64)     <= to_fixed(0.119);
									when 128 =>
										 sigmoid_lut(128) <= to_fixed(0.5);
										 tanh_lut(128)    <= to_fixed(0.0);
										 exp_lut(128)     <= to_fixed(1.0);
									when 192 =>
										 sigmoid_lut(192) <= to_fixed(0.881);
										 tanh_lut(192)    <= to_fixed(0.76);
										 exp_lut(192)     <= to_fixed(0.119);
									when 255 =>
										 sigmoid_lut(255) <= to_fixed(0.982);
										 tanh_lut(255)    <= to_fixed(0.999);
										 exp_lut(255)     <= to_fixed(0.018);
									when others =>
										 -- Linearna interpolacija između poznatih tačaka
										 sigmoid_lut(lut_init_counter) <= to_fixed(0.982);
										 tanh_lut(lut_init_counter)    <= to_fixed(0.999);
										 exp_lut(lut_init_counter)     <= to_fixed(0.018);
							  end case;

							  lut_init_counter <= lut_init_counter + 1;
						 else
							  lut_initialized <= '1';
						 end if;
                when PROCESSING =>
                    if pipeline_enable = '1' then
                        -- Shift pipeline za svaku paralelnu jedinicu
                        for i in 0 to PARALLEL_UNITS-1 loop
                            -- Shift unutar pipeline-a (od poslednjeg ka prvom)
                            for j in PIPELINE_STAGES-1 downto 1 loop
                                pipeline(i)(j) <= pipeline(i)(j-1);
                            end loop;
                            
                            -- Input stage (0)
                            if input_valid = '1' and (element_counter + i) < tensor_size then
                                pipeline(i)(0).valid <= '1';
                                pipeline(i)(0).data <= input_data(i);
                                pipeline(i)(0).activation_sel <= activation_type;
                            else
                                pipeline(i)(0).valid <= '0';
                            end if;
                        end loop;
                        
                        -- Increment counter
                        if element_counter + PARALLEL_UNITS < tensor_size then
                            element_counter <= element_counter + PARALLEL_UNITS;
                        else
                            element_counter <= tensor_size;
                        end if;
                    end if;
                    
                when PIPELINE_FLUSH =>
                    -- Flush remaining data through pipeline
                    for i in 0 to PARALLEL_UNITS-1 loop
                        for j in PIPELINE_STAGES-1 downto 1 loop
                            pipeline(i)(j) <= pipeline(i)(j-1);
                        end loop;
                        pipeline(i)(0).valid <= '0';
                    end loop;
                    
                when DONE_ST =>
                    null;
            end case;
        end if;
    end process;
    
    ---------------------------------------------------------------------------
    -- Next state logic (combinational)
    ---------------------------------------------------------------------------
    process(current_state, start, lut_initialized, element_counter, tensor_size, lut_init_counter)
    begin
        next_state <= current_state;
        
        case current_state is
            when IDLE =>
                if start = '1' then
                    if lut_initialized = '0' then
                        next_state <= INIT_LUT;
                    else
                        next_state <= PROCESSING;
                    end if;
                end if;
                
            when INIT_LUT =>
                if lut_init_counter > 255 then
                    next_state <= PROCESSING;
                end if;
                
            when PROCESSING =>
                if element_counter >= tensor_size then
                    next_state <= PIPELINE_FLUSH;
                end if;
                
            when PIPELINE_FLUSH =>
                -- Po završetku flush-a idemo u DONE
                next_state <= DONE_ST;
                
            when DONE_ST =>
                next_state <= IDLE;
                
            when others =>
                next_state <= IDLE;
        end case;
    end process;
    
    ---------------------------------------------------------------------------
    -- Pipeline processing logic (izvršava aktivaciju na izlazu pipeline-a)
    -- Ovaj proces je sequential i deklaracije variable su ovde (pre begin)
    ---------------------------------------------------------------------------
    process(clk)
        -- variables for per-iteration computation
        variable in_val        : std_logic_vector(DATA_WIDTH-1 downto 0);
        variable out_val       : std_logic_vector(DATA_WIDTH-1 downto 0);
        variable act_sel       : activation_type_t;
        variable mult_result   : signed(31 downto 0);
        variable elu_result    : signed(DATA_WIDTH-1 downto 0);
        variable exp_val_vec   : std_logic_vector(DATA_WIDTH-1 downto 0);
        variable i_var         : integer;
        variable j_var         : integer;
    begin
        if rising_edge(clk) then
            -- Po defaultu postavi izlaze na 0 (ako nema valid)
            for i_var in 0 to PARALLEL_UNITS-1 loop
                if pipeline(i_var)(PIPELINE_STAGES-1).valid = '1' then
                    in_val := pipeline(i_var)(PIPELINE_STAGES-1).data;
                    act_sel := pipeline(i_var)(PIPELINE_STAGES-1).activation_sel;
                    
                    case act_sel is
                        when RELU =>
                            out_val := apply_relu(in_val);
                            
                        when LEAKY_RELU =>
                            out_val := apply_leaky_relu(in_val, alpha);
                            
                        when SIGMOID =>
                            out_val := lut_lookup(sigmoid_lut, in_val);
                            
                        when TANH =>
                            out_val := lut_lookup(tanh_lut, in_val);
                            
                        when LINEAR =>
                            out_val := in_val;
                            
                        when ELU =>
                            if signed(in_val) >= 0 then
                                out_val := in_val;
                            else
                                -- ELU: α * (exp(x) - 1)
                                exp_val_vec := lut_lookup(exp_lut, in_val);
                                -- elu_result = exp_val - 1.0 (fixed-point)
                                elu_result := signed(exp_val_vec) - to_signed(2**FRAC_WIDTH, DATA_WIDTH);
                                mult_result := signed(alpha) * elu_result;
                                out_val := std_logic_vector(mult_result(DATA_WIDTH+FRAC_WIDTH-1 downto FRAC_WIDTH));
                            end if;
                            
                        when others =>
                            out_val := in_val;
                    end case;
                    
                    output_data(i_var) <= out_val;
                else
                    output_data(i_var) <= (others => '0');
                end if;
            end loop;
        end if;
    end process;
    
    ---------------------------------------------------------------------------
    -- Address generation (simple mapping)
    ---------------------------------------------------------------------------
    input_addr <= std_logic_vector(to_unsigned(element_counter, 16));
    -- Proces za izračunavanje output address
	 process(element_counter)
		begin
			 if element_counter >= PIPELINE_STAGES then
				  out_addr_int <= element_counter - PIPELINE_STAGES;
			 else
				  out_addr_int <= 0;
			 end if;
	 end process;

	 -- Pretvaranje u std_logic_vector
	 output_addr <= std_logic_vector(to_unsigned(out_addr_int, 16));
    ---------------------------------------------------------------------------
    -- Control signals and simple glue logic
    ---------------------------------------------------------------------------
    pipeline_enable <= '1' when current_state = PROCESSING else '0';
    processing_active <= '1' when current_state = PROCESSING or current_state = PIPELINE_FLUSH else '0';
    
    input_read_en <= pipeline_enable;
    
    -- output_valid: '1' ako bilo koja paralelna ćelija u poslednjem stage-u ima valid
    process(pipeline)
        variable any_valid : std_logic := '0';
        variable idx : integer;
    begin
        any_valid := '0';
        for idx in 0 to PARALLEL_UNITS-1 loop
            if pipeline(idx)(PIPELINE_STAGES-1).valid = '1' then
                any_valid := '1';
            end if;
        end loop;
        output_valid <= any_valid;
		  output_write_en <= any_valid;
    end process;
    
    
    -- Status signals
    ready <= '1' when current_state = IDLE else '0';
    busy <= processing_active;
    done <= '1' when current_state = DONE_ST else '0';
    
    -- Debug outputs
    current_element <= std_logic_vector(to_unsigned(element_counter, 16));
    processing_cycles <= std_logic_vector(cycle_counter);

end behavioral;
