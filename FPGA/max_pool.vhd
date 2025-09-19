library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity max_pool is
    generic (
        DATA_WIDTH : integer := 8;
        IMG_WIDTH  : integer := 256  -- input image width
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
end entity;

architecture rtl of max_pool is
    -- Line buffers per channel
    type row_array is array (0 to IMG_WIDTH-1) of unsigned(DATA_WIDTH-1 downto 0);
    signal line_r, line_g, line_b : row_array := (others => (others => '0'));

    -- Counters
    signal col_count : integer range 0 to IMG_WIDTH-1 := 0;
    signal row_count : integer range 0 to IMG_WIDTH-1 := 0;

    -- 2x2 window pixels per channel
    signal p1_r, p2_r, p3_r, p4_r : unsigned(DATA_WIDTH-1 downto 0) := (others=>'0');
    signal p1_g, p2_g, p3_g, p4_g : unsigned(DATA_WIDTH-1 downto 0) := (others=>'0');
    signal p1_b, p2_b, p3_b, p4_b : unsigned(DATA_WIDTH-1 downto 0) := (others=>'0');

    -- Internal outputs
    signal v_out      : std_logic := '0';
    signal pooled_r, pooled_g, pooled_b : unsigned(DATA_WIDTH-1 downto 0) := (others=>'0');
begin

    process(clk)
    begin
        if rising_edge(clk) then
            if reset = '1' then
                col_count <= 0;
                row_count <= 0;
                line_r <= (others => (others=>'0'));
                line_g <= (others => (others=>'0'));
                line_b <= (others => (others=>'0'));
                v_out <= '0';
                pooled_r <= (others=>'0');
                pooled_g <= (others=>'0');
                pooled_b <= (others=>'0');
            else
                if valid_in = '1' then
                    -- store current pixel in line buffers
                    line_r(col_count) <= pixel_r_in;
                    line_g(col_count) <= pixel_g_in;
                    line_b(col_count) <= pixel_b_in;

                    if (row_count > 0) and (col_count > 0) then
                        -- window for R
                        p1_r <= line_r(col_count-1);
                        p2_r <= line_r(col_count);
                        p3_r <= p4_r;
                        p4_r <= pixel_r_in;
                        -- window for G
                        p1_g <= line_g(col_count-1);
                        p2_g <= line_g(col_count);
                        p3_g <= p4_g;
                        p4_g <= pixel_g_in;
                        -- window for B
                        p1_b <= line_b(col_count-1);
                        p2_b <= line_b(col_count);
                        p3_b <= p4_b;
                        p4_b <= pixel_b_in;

                        -- stride = 2: output only at odd rows/cols
                        if (row_count mod 2 = 1) and (col_count mod 2 = 1) then
                            -- max pooling per channel
                            pooled_r <= p1_r; if p2_r>pooled_r then pooled_r<=p2_r; end if;
                            if p3_r>pooled_r then pooled_r<=p3_r; end if;
                            if p4_r>pooled_r then pooled_r<=p4_r; end if;

                            pooled_g <= p1_g; if p2_g>pooled_g then pooled_g<=p2_g; end if;
                            if p3_g>pooled_g then pooled_g<=p3_g; end if;
                            if p4_g>pooled_g then pooled_g<=p4_g; end if;

                            pooled_b <= p1_b; if p2_b>pooled_b then pooled_b<=p2_b; end if;
                            if p3_b>pooled_b then pooled_b<=p3_b; end if;
                            if p4_b>pooled_b then pooled_b<=p4_b; end if;

                            v_out <= '1';
                        else
                            v_out <= '0';
                        end if;
                    else
                        v_out <= '0';
                        p4_r <= pixel_r_in;
                        p4_g <= pixel_g_in;
                        p4_b <= pixel_b_in;
                    end if;

                    -- update coordinates
                    if col_count = IMG_WIDTH-1 then
                        col_count <= 0;
                        if row_count = IMG_WIDTH-1 then
                            row_count <= 0;
                        else
                            row_count <= row_count + 1;
                        end if;
                    else
                        col_count <= col_count + 1;
                    end if;
                else
                    v_out <= '0';
                end if;
            end if;
        end if;
    end process;

    pooled_r_out <= pooled_r;
    pooled_g_out <= pooled_g;
    pooled_b_out <= pooled_b;
    valid_out    <= v_out;

end rtl;
