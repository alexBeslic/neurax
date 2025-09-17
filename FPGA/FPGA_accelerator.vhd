library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity FPGA_accelerator is
    generic (
        IMG_WIDTH           : integer := 4;
        IMG_HEIGHT          : integer := 4;
        CHANNELS            : integer := 3;  -- 3 = RGB, 4 = RGBA
        PIXEL_CHANNEL_WIDTH : integer := 8;  -- bits per channel
        ACTIVATION_TYPE     : string := "RELU" -- "RELU", "SIGMOID", "TANH"
    );
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        enable      : in  std_logic;
        pixel_in    : in  std_logic_vector(CHANNELS*PIXEL_CHANNEL_WIDTH-1 downto 0);
        pixel_valid : in  std_logic;
        result_out  : out std_logic_vector((CHANNELS*16)-1 downto 0); -- 16 bits per kanal
        out_valid   : out std_logic
    );
end FPGA_accelerator;

architecture FPGA_accelerator_arch of FPGA_accelerator is

    -- Component declaration
    component convolution_block
        generic (
            IMG_WIDTH           : integer := 4;
            IMG_HEIGHT          : integer := 4;
            CHANNELS            : integer := 3;
            PIXEL_CHANNEL_WIDTH : integer := 8
        );
        port (
            clk         : in  std_logic;
            rst         : in  std_logic;
            enable      : in  std_logic;
            pixel_in    : in  std_logic_vector(CHANNELS*PIXEL_CHANNEL_WIDTH-1 downto 0);
            pixel_valid : in  std_logic;
            conv_out    : out std_logic_vector((CHANNELS*16)-1 downto 0);
            out_valid   : out std_logic
        );
    end component;

    component activation_block
        generic (
            CHANNELS        : integer := 3;
            DATA_WIDTH      : integer := 16;
            ACTIVATION_TYPE : string := "RELU"
        );
        port (
            clk         : in  std_logic;
            rst         : in  std_logic;
            enable      : in  std_logic;
            data_in     : in  std_logic_vector(CHANNELS*DATA_WIDTH-1 downto 0);
            in_valid    : in  std_logic;
            data_out    : out std_logic_vector(CHANNELS*DATA_WIDTH-1 downto 0);
            out_valid   : out std_logic
        );
    end component;

    signal conv_out_sig      : std_logic_vector((CHANNELS*16)-1 downto 0);
    signal conv_valid_sig    : std_logic;
    signal act_out_sig       : std_logic_vector((CHANNELS*16)-1 downto 0);
    signal act_valid_sig     : std_logic;

begin

    -- Instanciranje convolution_block
    conv_inst: convolution_block
        generic map (
            IMG_WIDTH           => IMG_WIDTH,
            IMG_HEIGHT          => IMG_HEIGHT,
            CHANNELS            => CHANNELS,
            PIXEL_CHANNEL_WIDTH => PIXEL_CHANNEL_WIDTH
        )
        port map (
            clk         => clk,
            rst         => rst,
            enable      => enable,
            pixel_in    => pixel_in,
            pixel_valid => pixel_valid,
            conv_out    => conv_out_sig,
            out_valid   => conv_valid_sig
        );

    -- Instanciranje activation_block
    act_inst: activation_block
        generic map (
            CHANNELS        => CHANNELS,
            DATA_WIDTH      => 16,
            ACTIVATION_TYPE => ACTIVATION_TYPE
        )
        port map (
            clk         => clk,
            rst         => rst,
            enable      => enable,
            data_in     => conv_out_sig,
            in_valid    => conv_valid_sig,
            data_out    => act_out_sig,
            out_valid   => act_valid_sig
        );

    -- Output assignment
    result_out <= act_out_sig;
    out_valid  <= act_valid_sig;

end FPGA_accelerator_arch;
