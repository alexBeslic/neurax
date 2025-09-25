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

        ------ Avalon ST Sink Interface (32-bit for HPS DMA) ------
        asi_channel_i      : in  std_logic;
        asi_data_i         : in  std_logic_vector(31 downto 0);
        asi_valid_i        : in  std_logic;
        asi_ready_o        : out std_logic;
        asi_error_i        : in  std_logic
    );
end neurax_data_interface;

architecture arch of neurax_data_interface is

    subtype t_data is std_logic_vector(g_PIXEL_WIDTH - 1 downto 0);
    type t_framebuffer is array(0 to g_FB_WIDTH * g_FB_HEIGHT - 1) of t_data;
    signal framebuffer : t_framebuffer := (others => (others => '0'));
    
    signal pixel_index : natural range 0 to g_FB_WIDTH * g_FB_HEIGHT - 1 := 0;
    signal frame_complete : std_logic := '0';
    signal ready_reg : std_logic := '1';
    
    constant c_TOTAL_PIXELS : natural := g_FB_WIDTH * g_FB_HEIGHT;

begin
    
    -- Flow control for DMA
    asi_ready_o <= ready_reg;

    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            pixel_index <= 0;
            framebuffer <= (others => (others => '0'));
            frame_complete <= '0';
            ready_reg <= '1';
            
        elsif rising_edge(clk_i) then
            frame_complete <= '0';  -- Default
            ready_reg <= '1';       -- Default: ready for data
            
            -- Handle error condition (highest priority)
            if asi_error_i = '1' then
                pixel_index <= 0;
                ready_reg <= '0';  -- Pause on error
                
            -- Handle new frame start (medium priority)
            elsif asi_channel_i = '1' then
                pixel_index <= 0;
                
            -- Handle normal data reception from HPS DMA (lowest priority)  
            elsif asi_valid_i = '1' and asi_ready_o = '1' then
                -- 32-bit word contains 1 complete 24-bit pixel + 8 bits padding
                -- Store pixel (lower 24 bits, ignore upper 8 bits)
                framebuffer(pixel_index) <= asi_data_i(g_PIXEL_WIDTH - 1 downto 0);
                
                -- Update pixel index and check for frame completion
                if pixel_index + 1 >= c_TOTAL_PIXELS then
                    pixel_index <= 0;  -- Wrap to start
                    frame_complete <= '1';
                    ready_reg <= '0';  -- Brief pause after frame completion
                else
                    pixel_index <= pixel_index + 1;
                end if;
            end if;
        end if;
    end process;

end arch;