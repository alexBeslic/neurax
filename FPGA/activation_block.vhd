library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity activation_block is
    generic (
        CHANNELS            : integer := 3;
        DATA_WIDTH          : integer := 16; -- input width per channel
        ACTIVATION_TYPE     : string := "RELU" -- "RELU", "SIGMOID", "TANH"
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
end activation_block;

architecture activation_block_arch of activation_block is
    subtype channel_t is std_logic_vector(DATA_WIDTH-1 downto 0);
    type multi_channel_t is array (0 to CHANNELS-1) of channel_t;
    signal input_channels  : multi_channel_t;
    signal output_channels : multi_channel_t;
    -- Removed unused valid_reg signal

    -- Helper function: ReLU
    function relu(x : signed) return signed is
    begin
        if x < 0 then
            return to_signed(0, x'length);
        else
            return x;
        end if;
    end function;

    -- Helper function: Tanh (approximation)
    function tanh_approx(x : signed) return signed is
        variable result : signed(x'length-1 downto 0);
    begin
        if x > to_signed(20480, x'length) then -- ~1.25
            result := to_signed(32767, x'length); -- max
        elsif x < to_signed(-20480, x'length) then
            result := to_signed(-32768, x'length); -- min
        else
            result := x / 2; -- crude tanh approx
        end if;
        return result;
    end function;

    -- Helper function: Sigmoid (approximation)
    function sigmoid_approx(x : signed) return signed is
        variable result : signed(x'length-1 downto 0);
    begin
        if x > to_signed(20480, x'length) then
            result := to_signed(32767, x'length); -- max
        elsif x < to_signed(-20480, x'length) then
            result := to_signed(0, x'length); -- min
        else
            result := to_signed(16384, x'length) + (x / 4); -- crude sigmoid approx
        end if;
        return result;
    end function;

begin
    process(clk, rst)
        variable ch : integer;
        variable x_signed : signed(DATA_WIDTH-1 downto 0);
        variable y_signed : signed(DATA_WIDTH-1 downto 0);
        variable out_vec : std_logic_vector(CHANNELS*DATA_WIDTH-1 downto 0);
    begin
        if rst = '1' then
            data_out <= (others => '0');
            out_valid <= '0';
        elsif rising_edge(clk) then
            if enable = '1' and in_valid = '1' then
                for ch in 0 to CHANNELS-1 loop
                    x_signed := signed(data_in((ch*DATA_WIDTH+DATA_WIDTH-1) downto (ch*DATA_WIDTH)));
                    if ACTIVATION_TYPE = "RELU" then
                        y_signed := relu(x_signed);
                    elsif ACTIVATION_TYPE = "TANH" then
                        y_signed := tanh_approx(x_signed);
                    elsif ACTIVATION_TYPE = "SIGMOID" then
                        y_signed := sigmoid_approx(x_signed);
                    else
                        y_signed := x_signed;
                    end if;
                    output_channels(ch) <= std_logic_vector(y_signed);
                    out_vec((ch*DATA_WIDTH+DATA_WIDTH-1) downto (ch*DATA_WIDTH)) := std_logic_vector(y_signed);
                end loop;
                data_out <= out_vec;
                out_valid <= '1';
            else
                data_out <= (others => '0');
                out_valid <= '0';
            end if;
        end if;
    end process;
end activation_block_arch;
