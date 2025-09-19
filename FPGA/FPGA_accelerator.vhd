library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity FPGA_accelerator is
    generic (
        IMG_WIDTH           : integer := 4;
        IMG_HEIGHT          : integer := 4;
        CHANNELS            : integer := 3;  -- 3 = RGB, 4 = RGBA
        PIXEL_CHANNEL_WIDTH : integer := 8;  -- bits per input channel
        ACTIVATION_TYPE     : string := "RELU" -- "RELU", "SIGMOID", "TANH"
    );
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        enable      : in  std_logic;
        pixel_in    : in  std_logic_vector(CHANNELS*PIXEL_CHANNEL_WIDTH-1 downto 0);
        pixel_valid : in  std_logic;
        result_out  : out std_logic_vector((CHANNELS*16)-1 downto 0); -- 16 bits per channel
        out_valid   : out std_logic
    );
end FPGA_accelerator;

architecture FPGA_accelerator_arch of FPGA_accelerator is

    -- === Convolution block ===
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

    -- === Activation block ===
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

    -- === Max pool block (RGB) ===
    component max_pool
        generic (
            DATA_WIDTH : integer := 8;
            IMG_WIDTH  : integer := 4
        );
        port (
            clk        : in  std_logic;
            reset      : in  std_logic;
            valid_in   : in  std_logic;
            pixel_r_in : in  unsigned(DATA_WIDTH-1 downto 0);
            pixel_g_in : in  unsigned(DATA_WIDTH-1 downto 0);
            pixel_b_in : in  unsigned(DATA_WIDTH-1 downto 0);
            pooled_r_out : out unsigned(DATA_WIDTH-1 downto 0);
            pooled_g_out : out unsigned(DATA_WIDTH-1 downto 0);
            pooled_b_out : out unsigned(DATA_WIDTH-1 downto 0);
            valid_out    : out std_logic
        );
    end component;

    -- === Internal signals ===
    signal conv_out_sig  : std_logic_vector((CHANNELS*16)-1 downto 0);
    signal conv_valid_sig: std_logic;
    signal act_out_sig   : std_logic_vector((CHANNELS*16)-1 downto 0);
    signal act_valid_sig : std_logic;

    -- Split channels for pooling (taking lower 8 bits of each channel)
    signal act_r, act_g, act_b : unsigned(7 downto 0);
    signal pooled_r, pooled_g, pooled_b : unsigned(7 downto 0);
    signal pool_valid : std_logic;

begin

    -- === Convolution ===
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

    -- === Activation ===
    act_inst: activation_block
        generic map (
            CHANNELS        => CHANNELS,
            DATA_WIDTH      => 16,
            ACTIVATION_TYPE => ACTIVATION_TYPE
        )
        port map (
            clk      => clk,
            rst      => rst,
            enable   => enable,
            data_in  => conv_out_sig,
            in_valid => conv_valid_sig,
            data_out => act_out_sig,
            out_valid=> act_valid_sig
        );

    -- === Extract lower 8 bits of each channel for pooling ===
    act_r <= unsigned(act_out_sig(7 downto 0));
    act_g <= unsigned(act_out_sig(23 downto 16));
    act_b <= unsigned(act_out_sig(39 downto 32));

    -- === Max Pool RGB ===
    maxpool_inst: max_pool
        generic map (
            DATA_WIDTH => 8,
            IMG_WIDTH  => IMG_WIDTH
        )
        port map (
            clk          => clk,
            reset        => rst,
            valid_in     => act_valid_sig,
            pixel_r_in   => act_r,
            pixel_g_in   => act_g,
            pixel_b_in   => act_b,
            pooled_r_out => pooled_r,
            pooled_g_out => pooled_g,
            pooled_b_out => pooled_b,
            valid_out    => pool_valid
        );

    -- === Output assignment ===
    result_out(7 downto 0)    <= std_logic_vector(pooled_r);
    result_out(15 downto 8)   <= std_logic_vector(pooled_g);
    result_out(23 downto 16)  <= std_logic_vector(pooled_b);
    out_valid <= pool_valid;

end FPGA_accelerator_arch;
