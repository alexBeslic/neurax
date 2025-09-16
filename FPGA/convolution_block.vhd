library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use IEEE.MATH_REAL.ALL;

entity convolution_block is
    generic (
        IMG_WIDTH             : integer := 4;
        IMG_HEIGHT            : integer := 4;
        CHANNELS              : integer := 3;  -- 3 = RGB, 4 = RGBA
        PIXEL_CHANNEL_WIDTH   : integer := 8   -- bits per channel
    );
    port (
        clk         : in  std_logic;
        rst         : in  std_logic;
        enable      : in  std_logic;
        pixel_in    : in  std_logic_vector(CHANNELS*PIXEL_CHANNEL_WIDTH-1 downto 0);
        pixel_valid : in  std_logic;
        conv_out    : out std_logic_vector((CHANNELS*16)-1 downto 0); -- 16 bits per channel output
        out_valid   : out std_logic
    );
end convolution_block;

architecture convolution_block_arch of convolution_block is

    -- Type for storing one pixel as a bitvector
    subtype pixel_t is std_logic_vector(CHANNELS*PIXEL_CHANNEL_WIDTH-1 downto 0);

    -- 2D image buffer (stores full pixels)
    type img_array_t is array (0 to IMG_HEIGHT-1, 0 to IMG_WIDTH-1) of pixel_t;
    signal image : img_array_t; -- no initializer; cleared on reset

    -- indices for writing incoming pixels
    signal row_idx : integer range 0 to IMG_HEIGHT-1 := 0;
    signal col_idx : integer range 0 to IMG_WIDTH-1 := 0;

    signal image_loaded : std_logic := '0';

    -- 3x3 kernel (signed integers). Promijeni vrijednosti kako želiš.
    type kernel_row_t is array (0 to 2) of integer;
    type kernel_t is array (0 to 2) of kernel_row_t;
    signal kernel : kernel_t := ((1, 0, -1), (1, 0, -1), (1, 0, -1)); -- primjer (Sobel-ish)

    -- storage for convolution results (valid output region: excluding padding)
    type result_row_t is array (0 to IMG_WIDTH-1) of integer range -32768 to 32767;
    type result_image_t is array (0 to IMG_HEIGHT-1) of result_row_t;
    -- za svaki kanal držimo rezultat
    type multi_result_t is array (0 to CHANNELS-1) of result_image_t;
    signal results : multi_result_t;

    -- kontrola izlaza: iteratori za redanje rezultata
    signal out_row : integer range 0 to IMG_HEIGHT-1 := 0;
    signal out_col : integer range 0 to IMG_WIDTH-1 := 0;
    signal output_phase : std_logic := '0';

    -- pomoćne signale
    signal conv_busy : std_logic := '0';

    -- funkcija: izvadi integer vrijednost jednog kanala iz pixel_t (pretpostavlja unsigned kanal)
    function get_channel_value(p : pixel_t; channel : integer) return integer is
        variable high_bit : integer;
        variable low_bit  : integer;
        variable slice    : std_logic_vector(PIXEL_CHANNEL_WIDTH-1 downto 0);
    begin
        -- zaštita od out-of-range kanala
        if (channel < 0) or (channel > CHANNELS-1) then
            return 0;
        end if;

        high_bit := (channel+1)*PIXEL_CHANNEL_WIDTH - 1;
        low_bit  := channel*PIXEL_CHANNEL_WIDTH;

        -- sigurnosna provjera (ne bi trebala pasti)
        if high_bit > p'high or low_bit < p'low then
            return 0;
        end if;

        slice := p(high_bit downto low_bit);
        return to_integer(unsigned(slice));
    end function;

begin

    process(clk, rst)
        variable i, j, kh, kw, ch : integer;
        variable acc : integer;
        variable pix_val : integer;
        variable tmp_out_vec : std_logic_vector((CHANNELS*16)-1 downto 0);
        variable signed_val : signed(15 downto 0);
    begin
        if rst = '1' then
            row_idx <= 0;
            col_idx <= 0;
            image_loaded <= '0';
            out_row <= 0;
            out_col <= 0;
            output_phase <= '0';
            conv_out <= (others => '0');
            out_valid <= '0';
            conv_busy <= '0';
            -- clear image (reset RAM contents)
            for i in 0 to IMG_HEIGHT-1 loop
                for j in 0 to IMG_WIDTH-1 loop
                    image(i,j) <= (others => '0');
                end loop;
            end loop;
            -- clear results as well
            for ch in 0 to CHANNELS-1 loop
                for i in 0 to IMG_HEIGHT-1 loop
                    for j in 0 to IMG_WIDTH-1 loop
                        results(ch)(i)(j) <= 0;
                    end loop;
                end loop;
            end loop;

        elsif rising_edge(clk) then
            -- write incoming pixel into image buffer
            if enable = '1' and pixel_valid = '1' and conv_busy = '0' then
                image(row_idx, col_idx) <= pixel_in;
                if col_idx = IMG_WIDTH-1 then
                    col_idx <= 0;
                    if row_idx = IMG_HEIGHT-1 then
                        row_idx <= 0;
                        image_loaded <= '1';
                    else
                        row_idx <= row_idx + 1;
                    end if;
                else
                    col_idx <= col_idx + 1;
                end if;
            end if;

            -- kada je slika učitana, pokreni konvoluciju (računamo cijelu sliku u jednom taktu serije)
            if image_loaded = '1' and conv_busy = '0' then
                conv_busy <= '1';
                -- compute convolution for each pixel and each channel (zero padding at borders)
                for ch in 0 to CHANNELS-1 loop
                    for i in 0 to IMG_HEIGHT-1 loop
                        for j in 0 to IMG_WIDTH-1 loop
                            acc := 0;
                            for kh in 0 to 2 loop
                                for kw in 0 to 2 loop
                                    if (i + kh - 1) >= 0 and (i + kh - 1) <= IMG_HEIGHT-1 and
                                       (j + kw - 1) >= 0 and (j + kw - 1) <= IMG_WIDTH-1 then
                                        pix_val := get_channel_value(image(i + kh - 1, j + kw - 1), ch);
                                    else
                                        -- zero padding
                                        pix_val := 0;
                                    end if;
                                    acc := acc + pix_val * kernel(kh)(kw);
                                end loop;
                            end loop;
                            -- clamp to 16-bit signed range
                            if acc > 32767 then
                                acc := 32767;
                            elsif acc < -32768 then
                                acc := -32768;
                            end if;
                            results(ch)(i)(j) <= acc;
                        end loop;
                    end loop;
                end loop;
                -- pripremi izlazne indekse
                out_row <= 0;
                out_col <= 0;
                output_phase <= '1';
                image_loaded <= '0';
            end if;

            -- Emit outputs jedan po jedan (konkatenirani po kanalima)
            if output_phase = '1' then
                -- formiraj izlazni vektor (po kanalima stave 16-bit signed vrijednosti)
                tmp_out_vec := (others => '0');
                for ch in 0 to CHANNELS-1 loop
                    signed_val := to_signed(results(ch)(out_row)(out_col), 16);
                    -- place in correct slice: kanal 0 LSB ... kanal N MSB
                    tmp_out_vec((ch*16+15) downto (ch*16)) := std_logic_vector(signed_val);
                end loop;
                conv_out <= tmp_out_vec;
                out_valid <= '1';

                -- advance indices
                if out_col = IMG_WIDTH-1 then
                    out_col <= 0;
                    if out_row = IMG_HEIGHT-1 then
                        out_row <= 0;
                        output_phase <= '0';
                        conv_busy <= '0';
                    else
                        out_row <= out_row + 1;
                    end if;
                else
                    out_col <= out_col + 1;
                end if;
            else
                out_valid <= '0';
            end if;
        end if;
    end process;

end convolution_block_arch;
