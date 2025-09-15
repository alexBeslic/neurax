library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity convolution_block is
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
end convolution_block;

architecture convolution_block_arch of convolution_block is
    type img_array is array (0 to IMG_HEIGHT-1, 0 to IMG_WIDTH-1) of integer range 0 to 255;
    signal image : img_array := (others => (others => 0));
    signal row, col : integer range 0 to IMG_HEIGHT-1 := 0;
	type result_array is array (0 to 1, 0 to 1) of integer;
	signal results : result_array := (others => (others => 0));
	signal result_idx_i : integer range 0 to 1 := 0;
	signal result_idx_j : integer range 0 to 1 := 0;
	signal output_phase : std_logic := '0';
	signal image_loaded : std_logic := '0';
    type kernel_array is array (0 to 2, 0 to 2) of integer;
	signal kernel : kernel_array := ((1, 0, -1), (1, 0, -1), (1, 0, -1)); -- Example Sobel kernel
begin
		process(clk, rst)
			variable conv_acc : integer;
		begin
			if rst = '1' then
				row <= 0;
				col <= 0;
				results <= (others => (others => 0));
				result_idx_i <= 0;
				result_idx_j <= 0;
				output_phase <= '0';
				conv_out <= (others => '0');
				out_valid <= '0';
				image_loaded <= '0';
			elsif rising_edge(clk) then
				if enable = '1' and pixel_valid = '1' then
					image(row, col) <= to_integer(unsigned(pixel_in));
					if col = IMG_WIDTH-1 then
						col <= 0;
						if row = IMG_HEIGHT-1 then
							row <= 0;
							image_loaded <= '1'; -- Set flag after last pixel
						else
							row <= row + 1;
						end if;
					else
						col <= col + 1;
					end if;
				end if;

				-- Calculate all convolution results after image is loaded
				if image_loaded = '1' then
					for i in 0 to 1 loop
						for j in 0 to 1 loop
							conv_acc := 0;
							for ki in 0 to 2 loop
								for kj in 0 to 2 loop
									conv_acc := conv_acc + image(i+ki, j+kj) * kernel(ki, kj);
								end loop;
							end loop;
							results(i, j) <= conv_acc;
						end loop;
					end loop;
					result_idx_i <= 0;
					result_idx_j <= 0;
					output_phase <= '1';
					image_loaded <= '0';
				end if;

				-- Output each result one by one
				if output_phase = '1' then
					conv_out <= std_logic_vector(to_signed(results(result_idx_i, result_idx_j), 16));
					out_valid <= '1';
					if result_idx_j < 1 then
						result_idx_j <= result_idx_j + 1;
					else
						result_idx_j <= 0;
						if result_idx_i < 1 then
							result_idx_i <= result_idx_i + 1;
						else
							result_idx_i <= 0;
							output_phase <= '0'; -- Done outputting all results
						end if;
					end if;
				else
					out_valid <= '0';
				end if;
			end if;
		end process;
end convolution_block_arch;
