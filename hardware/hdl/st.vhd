library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity neurax_data_interface is
    generic (
        g_DATA_WIDTH   : natural := 32;
        g_ADDR_WIDTH   : natural := 15;   
        g_RAM_SIZE     : natural := 23000  
    );
    port (
        clk_i         : in  std_logic;
        rst_i         : in  std_logic;

        ------ Avalon ST Sink Interface (HPS -> FPGA) ------
        asi_channel_i : in  std_logic;
        asi_data_i    : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        asi_valid_i   : in  std_logic;
        asi_ready_o   : out std_logic;
        asi_error_i   : in  std_logic;

        ------ Avalon ST Source Interface (FPGA -> HPS) ------
        aso_data_o    : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        aso_valid_o   : out std_logic;
        aso_ready_i   : in  std_logic;
        aso_channel_o : out std_logic;
        aso_error_o   : out std_logic;

        ------ RAM Port B Interface------
        ram_b_rdaddress_i : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_b_q_o         : out std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        ram_b_wraddress_i : in  std_logic_vector(g_ADDR_WIDTH - 1 downto 0);
        ram_b_data_i      : in  std_logic_vector(g_DATA_WIDTH - 1 downto 0);
        ram_b_wren_i      : in  std_logic
    );
end neurax_data_interface;

architecture arch of neurax_data_interface is
    -- Pipeline registri za podatke i kontrolne signale
    signal r_data       : std_logic_vector(g_DATA_WIDTH - 1 downto 0);
    signal r_valid      : std_logic;
    signal r_channel    : std_logic;
    signal r_error      : std_logic;
    
    -- Interni signal za upravljanje spremnošću (backpressure)
    signal sink_ready_int : std_logic;
begin

    -- LOGIKA ZA BACKPRESSURE (Kritično za Avalon-ST):
    -- Sink interfejs je spreman da primi novi podatak ako:
    -- 1) HPS sa Source strane javlja da je spreman da preuzme podatak (aso_ready_i = '1')
    -- OR
    -- 2) Trenutno nemamo validan podatak u našem registru (r_valid = '0'), pa imamo mesta za jedan takt
    sink_ready_int <= aso_ready_i or not r_valid;
    asi_ready_o    <= sink_ready_int;

    -- Direktno povezivanje registrovanih vrednosti na izlazne Source portove
    aso_data_o     <= r_data;
    aso_valid_o    <= r_valid;
    aso_channel_o  <= r_channel;
    aso_error_o    <= r_error;

    -- Neaktivni Port B za akcelerator u ovom testu stavljamo u bezbedno stanje
    ram_b_q_o <= (others => '0');

    -- Sinhroni proces za prenos podataka kroz petlju
    process(clk_i, rst_i)
    begin
        if rst_i = '1' then
            r_data    <= (others => '0');
            r_valid   <= '0';
            r_channel <= '0';
            r_error   <= '0';
        elsif rising_edge(clk_i) then
            -- Ako je interni protočni kanal slobodan/spreman
            if sink_ready_int = '1' then
                r_valid   <= asi_valid_i;   -- Latch-ujemo validnost (ako HPS šalje, postaje validno za slanje nazad)
                r_data    <= asi_data_i;    -- Kopiramo podatak direktno u loopback
                r_channel <= asi_channel_i; -- Prenosimo i kanal ako ga HPS koristi
                r_error   <= asi_error_i;   -- Prenosimo error bit radi provere integriteta
            end if;
        end if;
    end process;

end arch;