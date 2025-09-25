library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity neurax is
    generic (
        g_WIDTH      : natural := 32;
        g_ADDR_WIDTH : natural := 3;
        g_PIXEL_WIDTH  : natural := 24; -- 24 bit RGB pixel
        g_FB_WIDTH     : natural := 100;
        g_FB_HEIGHT    : natural := 100   
    );

    port (
        clk_i         : in  std_logic;
        rst_i         : in  std_logic;

        ------ Avalon MM Slave Interface ------
        avs_chipselect_i    : in  std_logic;
        avs_address_i       : in  std_logic_vector(g_ADDR_WIDTH-1 downto 0);
        avs_read_i          : in  std_logic;
        avs_readdata_o      : out std_logic_vector(g_WIDTH-1 downto 0);
        avs_readdatavalid_o : out std_logic;
        avs_write_i         : in  std_logic;
        avs_writedata_i     : in  std_logic_vector(g_WIDTH-1 downto 0);
        avs_byteenable_i    : in  std_logic_vector((g_WIDTH/8)-1 downto 0);
        avs_waitrequest_o   : out std_logic;

        ------ Avalon ST Sink Interface ------
        asi_channel_i      : in  std_logic;
        asi_data_i         : in  std_logic_vector(g_WIDTH-1 downto 0);
        asi_valid_i        : in  std_logic;
        asi_ready_o        : out std_logic;
        asi_error_i        : in  std_logic;

        ------ Avalon ST Source Interface ------
        aso_data_o     : out std_logic_vector(g_WIDTH-1 downto 0);
        aso_valid_o    : out std_logic;
        aso_ready_i    : in  std_logic;
        aso_channel_o  : out std_logic;
        aso_error_o    : out std_logic
    );
end entity neurax;
