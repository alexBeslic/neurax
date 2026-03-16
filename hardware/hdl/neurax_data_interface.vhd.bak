library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library altera;
use altera.altera_syn_attributes.all;

entity neurax_data_interface is
    generic (
        g_DATA_WIDTH   : natural := 32; -- RAM data width
        g_ADDR_WIDTH   : natural := 16; -- RAM address width
        g_RAM_SIZE     : natural := 10000 -- Total RAM size in words
    );
    port (
        clk_i         : in  std_logic;
        rst_i         : in  std_logic;

        ------ Avalon ST Sink Interface ------
        asi_channel_i      : in  std_logic;
        asi_data_i         : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        asi_valid_i        : in  std_logic;
        asi_ready_o        : out std_logic;
        asi_error_i        : in  std_logic;

        ------ Avalon ST Source Interface ------
        aso_data_o     : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        aso_valid_o    : out std_logic;
        aso_ready_i    : in  std_logic;
        aso_channel_o  : out std_logic;
        aso_error_o    : out std_logic;

        ------ RAM 2-Port Interface ------
        ram_data_o      : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        ram_rdaddress_o : out std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_wraddress_o : out std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_wren_o      : out std_logic;
        ram_q_i         : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0)
    );
end neurax_data_interface;

architecture arch of neurax_data_interface is

    signal write_index : unsigned(g_ADDR_WIDTH-1 downto 0) := (others => '0');
    signal read_index  : unsigned(g_ADDR_WIDTH-1 downto 0) := (others => '0');

    signal write_complete : std_logic := '0';
    signal read_complete  : std_logic := '0';
    signal ready_reg : std_logic := '1';
    
    -- Buffer management
    signal buffer_full    : std_logic := '0';
    signal buffer_empty   : std_logic := '1';
    signal reading_active : std_logic := '0';

     -- pipeline / request signals for correct read alignment
    signal ram_data_reg     : std_logic_vector(g_DATA_WIDTH-1 downto 0) := (others => '0');
    signal ram_valid_reg    : std_logic := '0';
    signal ram_channel_reg  : std_logic := '0';
    signal ram_error_reg    : std_logic := '0';

    signal read_request     : std_logic := '0';
    signal read_request_d   : std_logic := '0';
    signal sof_request      : std_logic := '0';
    signal sof_request_d    : std_logic := '0';
    signal error_request    : std_logic := '0';
    signal error_request_d  : std_logic := '0';

    constant c_TOTAL_WORDS : natural := g_RAM_SIZE;

begin
    
    -- Flow control for input
    asi_ready_o <= ready_reg and not buffer_full;

    -- RAM write control
    ram_wren_o      <= asi_valid_i and (ready_reg and not buffer_full);
    ram_wraddress_o <= std_logic_vector(write_index);
    ram_data_o      <= asi_data_i;

    -- RAM read address
    ram_rdaddress_o <= std_logic_vector(read_index);

    -- Write process (Avalon ST Sink)
    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            write_index    <= (others => '0');
            write_complete <= '0';
            ready_reg <= '1';
            buffer_full <= '0';

        elsif rising_edge(clk_i) then
            write_complete <= '0';
            ready_reg <= '1';

            -- Handle error condition
            if asi_error_i = '1' then
                write_index <= (others => '0');
                ready_reg <= '0';
                buffer_full <= '0';
                
            -- Handle new frame start
            elsif asi_channel_i = '1' then
                write_index <= (others => '0');
                buffer_full <= '0';
                
            -- Normal write
            elsif asi_valid_i = '1' and ready_reg = '1' then
                if to_integer(write_index) + 1 >= c_TOTAL_WORDS then
                    write_index <= (others => '0');
                    write_complete <= '1';
                    buffer_full <= '1';
                    ready_reg <= '0';  -- Stop accepting data
                else
                    write_index <= write_index + 1;
                end if;
            end if;

            -- Release buffer_full when reading starts
            if reading_active = '1' and read_complete = '1' then
                buffer_full <= '0';
            end if;
        end if;
    end process;

    -- Read process (Avalon ST Source)
    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            aso_data_o <= (others => '0');
            aso_valid_o <= '0';
            aso_channel_o <= '0';
            aso_error_o <= '0';
            read_index <= (others => '0');
            read_complete <= '0';
            buffer_empty <= '1';
            reading_active <= '0';

            ram_data_reg <= (others => '0');
            ram_valid_reg <= '0';
            ram_channel_reg <= '0';
            ram_error_reg <= '0';

            read_request <= '0';
            read_request_d <= '0';
            sof_request <= '0';
            sof_request_d <= '0';
            error_request <= '0';
            error_request_d <= '0';

        elsif rising_edge(clk_i) then
            -- Capture RAM output into pipeline register (data arriving this cycle
            -- corresponds to the address presented in the previous cycle)
            ram_data_reg <= ram_q_i;

            -- Present the delayed request as valid/channel/error
            ram_valid_reg <= read_request_d;
            ram_channel_reg <= sof_request_d;
            ram_error_reg <= error_request_d;

            -- Drive outputs from the pipeline registers
            aso_data_o <= ram_data_reg;
            aso_valid_o <= ram_valid_reg;
            aso_channel_o <= ram_channel_reg;
            aso_error_o <= ram_error_reg;

            -- default (deassert) ephemeral request signals; they'll be set below if we issue a read
            read_request <= '0';
            sof_request <= '0';
            error_request <= '0';

            read_complete <= '0';

            -- Start reading when buffer is full and not already reading
            if buffer_full = '1' and reading_active = '0' then
                reading_active <= '1';
                read_index <= (others => '0'); -- prepare first address (present immediately)
                buffer_empty <= '0';
            end if;

            -- When active and downstream ready -> request a read at current read_index
            if reading_active = '1' and aso_ready_i = '1' then
                -- assert a one-cycle read request (this requests RAM with the current read_index)
                read_request <= '1';

                -- mark start-of-frame for this request (so it will appear next cycle alongside data)
                if read_index = 0 then
                    sof_request <= '1';
                end if;

                -- optionally propagate input error that corresponds to this data
                if asi_error_i = '1' then
                    error_request <= '1';
                end if;

                -- update index for the *next* request (read_index is the address presented this cycle)
                if to_integer(read_index) + 1 >= c_TOTAL_WORDS then
                    read_index <= (others => '0');
                    read_complete <= '1';
                    reading_active <= '0';  -- stop requesting further reads
                    buffer_empty <= '1';
                else
                    read_index <= read_index + 1;
                end if;
            end if;

            -- shift the request delay registers (these capture the request made in the previous cycle)
            read_request_d <= read_request;
            sof_request_d <= sof_request;
            error_request_d <= error_request;
        end if;
    end process;

end arch;