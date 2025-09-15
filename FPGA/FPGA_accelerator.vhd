library ieee;
use ieee.std_logic_1164.all;

entity FPGA_accelerator is
	generic (
		IMG_WIDTH  : integer := 4;
		IMG_HEIGHT : integer := 4;
		PIXEL_WIDTH: integer := 8
	);
	port (
		clk         : in  std_logic;
		rst         : in  std_logic;
		enable      : in  std_logic;
		pixel_in    : in  std_logic_vector(PIXEL_WIDTH-1 downto 0);
		pixel_valid : in  std_logic;
		result_out  : out std_logic_vector(15 downto 0);
		out_valid   : out std_logic
	);
end FPGA_accelerator;

architecture FPGA_accelerator_arch of FPGA_accelerator is
	-- Component declaration for 2D convolution block
	component convolution_block
		generic (
			IMG_WIDTH  : integer := 4;
			IMG_HEIGHT : integer := 4;
			PIXEL_WIDTH: integer := 8
		);
		port (
			clk         : in  std_logic;
			rst         : in  std_logic;
			enable      : in  std_logic;
			pixel_in    : in  std_logic_vector(PIXEL_WIDTH-1 downto 0);
			pixel_valid : in  std_logic;
			conv_out    : out std_logic_vector(15 downto 0);
			out_valid   : out std_logic
		);
	end component;

	signal conv_out_sig  : std_logic_vector(15 downto 0);
	signal out_valid_sig : std_logic;
begin
	-- Instantiation of 2D convolution block
	conv_inst: convolution_block
		generic map (
			IMG_WIDTH => IMG_WIDTH,
			IMG_HEIGHT => IMG_HEIGHT,
			PIXEL_WIDTH => PIXEL_WIDTH
		)
		port map (
			clk         => clk,
			rst         => rst,
			enable      => enable,
			pixel_in    => pixel_in,
			pixel_valid => pixel_valid,
			conv_out    => conv_out_sig,
			out_valid   => out_valid_sig
		);

	-- Output assignment
	result_out <= conv_out_sig;
	out_valid  <= out_valid_sig;
end FPGA_accelerator_arch;
