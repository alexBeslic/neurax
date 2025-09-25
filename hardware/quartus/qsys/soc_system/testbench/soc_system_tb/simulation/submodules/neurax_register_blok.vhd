library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library altera;
use altera.altera_syn_attributes.all;

use work.neurax_register_pkg.all;

entity neurax_register_blok is
    generic (
        g_WIDTH      : natural := 32;
        g_ADDR_WIDTH : natural := 3
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


        ------ Neurax Register Interface ------
        neurax_pooling_type_o    : out std_logic_vector(2 downto 0);
        neurax_activation_type_o : out std_logic_vector(2 downto 0)
    );

end neurax_register_blok;

architecture arch of neurax_register_blok is
  subtype t_word is std_logic_vector((g_WIDTH - 1) downto 0);
  type memory_t is array((2 ** g_ADDR_WIDTH - 1) downto 0) of t_word; -- 2^g_ADDR_WIDTH memory locations, each word is g_WIDTH wide in total 2^g_ADDR_WIDTH * g_WIDTH bits
  signal ram : memory_t := (others => (others => '0'));
  signal read_data_valid : std_logic;
begin

    --- Write process
    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            ram <= (others => (others => '0'));
        elsif rising_edge(clk_i) then
            if avs_chipselect_i = '1' and avs_write_i = '1' then
                if unsigned(avs_address_i) < 2**g_ADDR_WIDTH then
                    --- Valid address range
                    for i in 0 to (g_WIDTH/8 - 1) loop
                        if avs_byteenable_i(i) = '1' then
                            --- Write only the enabled bytes
                            ram(to_integer(unsigned(avs_address_i)))(i*8 + 7 downto i*8) <= avs_writedata_i(i*8 + 7 downto i*8);
                        end if;
                    end loop;
                end if;
            end if;
        end if;
    end process;
    
    --- Read process
    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            avs_readdata_o <= (others => '0');
            read_data_valid <= '0';
        elsif rising_edge(clk_i) then
            if avs_chipselect_i = '1' and avs_read_i = '1' then
                if unsigned(avs_address_i) < 2**g_ADDR_WIDTH then
                    --- Valid address range
                    avs_readdata_o <= ram(to_integer(unsigned(avs_address_i)));
                    read_data_valid <= '1';
                else
                    --- Address out of range
                    avs_readdata_o <= (others => '0');
                    read_data_valid <= '0';
                end if;
            else
                read_data_valid <= '0';  -- Clear when not reading
            end if;
        end if;
    end process;


    --- Avalon MM Slave Interface
    avs_waitrequest_o <= '0'; -- Always ready
    avs_readdatavalid_o <= read_data_valid;


    --- Neurax Register Interface
    neurax_pooling_type_o <= ram(to_integer(c_REG_TYPE))(t_POOLING_TYPE);
    neurax_activation_type_o <= ram(to_integer(c_REG_TYPE))(t_ACTIVATION_TYPE);
end arch;