library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library altera;
use altera.altera_syn_attributes.all;

entity neurax_data_interface is
    generic (
        g_PIXEL_WIDTH  : natural := 24; -- 24 bit RGB pixel
        g_FB_WIDTH     : natural := 100;
        g_FB_HEIGHT    : natural := 100    
    );
    port (
        clk_i         : in  std_logic;
        rst_i         : in  std_logic;

        ------ Avalon ST Sink Interface ------
        asi_channel_i      : in  std_logic;
        asi_data_i         : in  std_logic_vector(31 downto 0);
        asi_valid_i        : in  std_logic;
        asi_ready_o        : out std_logic;
        asi_error_i        : in  std_logic;

        ------ Avalon ST Source Interface ------
        aso_data_o     : out std_logic_vector(31 downto 0);
        aso_valid_o    : out std_logic;
        aso_ready_i    : in  std_logic;
        aso_channel_o  : out std_logic;
        aso_error_o    : out std_logic
    );
end neurax_data_interface;

architecture arch of neurax_data_interface is

    subtype t_data is std_logic_vector(g_PIXEL_WIDTH - 1 downto 0);
    type t_framebuffer is array(natural range <>) of t_data;
    signal framebuffer : t_framebuffer(0 to g_FB_WIDTH * g_FB_HEIGHT - 1);

    -- Tell Quartus to use M10K RAM blocks for the framebuffer
--    attribute ramstyle : string;
--    attribute ramstyle of framebuffer : signal is "MLAB";

    signal write_index : natural range 0 to g_FB_WIDTH * g_FB_HEIGHT - 1 := 0;
    signal read_index : natural range 0 to g_FB_WIDTH * g_FB_HEIGHT - 1 := 0;

    signal write_complete : std_logic := '0';
    signal read_complete : std_logic := '0';
    signal ready_reg : std_logic := '1';
    
    -- Buffer management
    signal buffer_full : std_logic := '0';
    signal buffer_empty : std_logic := '1';
    signal reading_active : std_logic := '0';
    
    constant c_TOTAL_PIXELS : natural := g_FB_WIDTH * g_FB_HEIGHT;

begin
    
    -- Flow control for input
    asi_ready_o <= ready_reg and not buffer_full;

    -- Write process (Avalon ST Sink)
    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            write_index <= 0;
            framebuffer <= (others => (others => '0'));
            write_complete <= '0';
            ready_reg <= '1';
            buffer_full <= '0';
            
        elsif rising_edge(clk_i) then
            write_complete <= '0';
            ready_reg <= '1';
            
            -- Handle error condition
            if asi_error_i = '1' then
                write_index <= 0;
                ready_reg <= '0';
                buffer_full <= '0';
                
            -- Handle new frame start
            elsif asi_channel_i = '1' then
                write_index <= 0;
                buffer_full <= '0';
                
            -- Handle normal data reception
            elsif asi_valid_i = '1' and ready_reg = '1' then
                -- Store pixel data
                framebuffer(write_index) <= asi_data_i(g_PIXEL_WIDTH - 1 downto 0);
                
                -- Update write index
                if write_index + 1 >= c_TOTAL_PIXELS then
                    write_index <= 0;
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
            read_index <= 0;
            read_complete <= '0';
            buffer_empty <= '1';
            reading_active <= '0';

        elsif rising_edge(clk_i) then
            -- Default outputs
            aso_valid_o <= '0';
            aso_channel_o <= '0';
            aso_error_o <= '0';
            read_complete <= '0';

            -- Start reading when buffer has data and not currently reading
            if buffer_full = '1' and reading_active = '0' then
                reading_active <= '1';
                read_index <= 0;
                buffer_empty <= '0';
            end if;

            -- Read data from framebuffer when active and downstream is ready
            if reading_active = '1' and aso_ready_i = '1' then
                -- Output pixel data (pad 24-bit to 32-bit)
                aso_data_o <= x"00" & framebuffer(read_index);
                aso_valid_o <= '1';
                
                -- Channel signal: high for first pixel of frame
                if read_index = 0 then
                    aso_channel_o <= '1';
                end if;

                -- Update read index
                if read_index + 1 >= c_TOTAL_PIXELS then
                    read_index <= 0;
                    read_complete <= '1';
                    reading_active <= '0';
                    buffer_empty <= '1';
                else
                    read_index <= read_index + 1;
                end if;
            end if;
        end if;
    end process;

end arch;