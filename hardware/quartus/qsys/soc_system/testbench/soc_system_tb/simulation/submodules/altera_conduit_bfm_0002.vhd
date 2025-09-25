-- (C) 2001-2025 Altera Corporation. All rights reserved.
-- Your use of Altera Corporation's design tools, logic functions and other 
-- software and tools, and its AMPP partner logic functions, and any output 
-- files from any of the foregoing (including device programming or simulation 
-- files), and any associated documentation or information are expressly subject 
-- to the terms and conditions of the Altera Program License Subscription 
-- Agreement, Altera IP License Agreement, or other applicable 
-- license agreement, including, without limitation, that your use is for the 
-- sole purpose of programming logic devices manufactured by Altera and sold by 
-- Altera or its authorized distributors.  Please refer to the applicable 
-- agreement for further details.


-- $Id: //acds/main/ip/sopc/components/verification/altera_tristate_conduit_bfm/altera_tristate_conduit_bfm.sv.terp#7 $
-- $Revision: #7 $
-- $Date: 2010/08/05 $
-- $Author: klong $
-------------------------------------------------------------------------------
-- =head1 NAME
-- altera_conduit_bfm
-- =head1 SYNOPSIS
-- Bus Functional Model (BFM) for a Standard Conduit BFM
-------------------------------------------------------------------------------
-- =head1 DESCRIPTION
-- This is a Bus Functional Model (BFM) for a Standard Conduit Master.
-- This BFM sampled the input/bidirection port value or driving user's value to 
-- output ports when user call the API.  
-- This BFM's HDL is been generated through terp file in Qsys/SOPC Builder.
-- Generation parameters:
-- output_name:                  altera_conduit_bfm_0002
-- role:width:direction:         hps_io_emac1_inst_MDC:1:input,hps_io_emac1_inst_MDIO:1:bidir,hps_io_emac1_inst_RXD0:1:output,hps_io_emac1_inst_RXD1:1:output,hps_io_emac1_inst_RXD2:1:output,hps_io_emac1_inst_RXD3:1:output,hps_io_emac1_inst_RX_CLK:1:output,hps_io_emac1_inst_RX_CTL:1:output,hps_io_emac1_inst_TXD0:1:input,hps_io_emac1_inst_TXD1:1:input,hps_io_emac1_inst_TXD2:1:input,hps_io_emac1_inst_TXD3:1:input,hps_io_emac1_inst_TX_CLK:1:input,hps_io_emac1_inst_TX_CTL:1:input,hps_io_gpio_inst_GPIO35:1:bidir,hps_io_sdio_inst_CLK:1:input,hps_io_sdio_inst_CMD:1:bidir,hps_io_sdio_inst_D0:1:bidir,hps_io_sdio_inst_D1:1:bidir,hps_io_sdio_inst_D2:1:bidir,hps_io_sdio_inst_D3:1:bidir,hps_io_uart0_inst_RX:1:output,hps_io_uart0_inst_TX:1:input
-- clocked                       0
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

library work;
use work.all;
use work.altera_conduit_bfm_0002_vhdl_pkg.all;

entity altera_conduit_bfm_0002 is
   port (
      sig_hps_io_emac1_inst_MDC      : in    std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_MDIO     : inout std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_RXD0     : out   std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_RXD1     : out   std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_RXD2     : out   std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_RXD3     : out   std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_RX_CLK   : out   std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_RX_CTL   : out   std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_TXD0     : in    std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_TXD1     : in    std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_TXD2     : in    std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_TXD3     : in    std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_TX_CLK   : in    std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_TX_CTL   : in    std_logic_vector(0 downto 0);
      sig_hps_io_gpio_inst_GPIO35    : inout std_logic_vector(0 downto 0);
      sig_hps_io_sdio_inst_CLK       : in    std_logic_vector(0 downto 0);
      sig_hps_io_sdio_inst_CMD       : inout std_logic_vector(0 downto 0);
      sig_hps_io_sdio_inst_D0        : inout std_logic_vector(0 downto 0);
      sig_hps_io_sdio_inst_D1        : inout std_logic_vector(0 downto 0);
      sig_hps_io_sdio_inst_D2        : inout std_logic_vector(0 downto 0);
      sig_hps_io_sdio_inst_D3        : inout std_logic_vector(0 downto 0);
      sig_hps_io_uart0_inst_RX       : out   std_logic_vector(0 downto 0);
      sig_hps_io_uart0_inst_TX       : in    std_logic_vector(0 downto 0)
   );
end altera_conduit_bfm_0002;

architecture altera_conduit_bfm_0002_arch of altera_conduit_bfm_0002 is 

      signal update : std_logic := '0';

   begin
      process begin
         wait for 1 ps;
         update <= not update;
      end process;

      process (update) begin
         sig_hps_io_emac1_inst_MDC_in   <= sig_hps_io_emac1_inst_MDC;

         if out_trans.sig_hps_io_emac1_inst_MDIO_oe = '1' then
            sig_hps_io_emac1_inst_MDIO     <= out_trans.sig_hps_io_emac1_inst_MDIO_out after 1 ps;
            sig_hps_io_emac1_inst_MDIO_in  <= (others => 'Z');
         else
            sig_hps_io_emac1_inst_MDIO     <= (others => 'Z');
            sig_hps_io_emac1_inst_MDIO_in  <= sig_hps_io_emac1_inst_MDIO;
         end if;
         sig_hps_io_emac1_inst_RXD0     <= out_trans.sig_hps_io_emac1_inst_RXD0_out after 1 ps;
         sig_hps_io_emac1_inst_RXD1     <= out_trans.sig_hps_io_emac1_inst_RXD1_out after 1 ps;
         sig_hps_io_emac1_inst_RXD2     <= out_trans.sig_hps_io_emac1_inst_RXD2_out after 1 ps;
         sig_hps_io_emac1_inst_RXD3     <= out_trans.sig_hps_io_emac1_inst_RXD3_out after 1 ps;
         sig_hps_io_emac1_inst_RX_CLK   <= out_trans.sig_hps_io_emac1_inst_RX_CLK_out after 1 ps;
         sig_hps_io_emac1_inst_RX_CTL   <= out_trans.sig_hps_io_emac1_inst_RX_CTL_out after 1 ps;
         sig_hps_io_emac1_inst_TXD0_in  <= sig_hps_io_emac1_inst_TXD0;
         sig_hps_io_emac1_inst_TXD1_in  <= sig_hps_io_emac1_inst_TXD1;
         sig_hps_io_emac1_inst_TXD2_in  <= sig_hps_io_emac1_inst_TXD2;
         sig_hps_io_emac1_inst_TXD3_in  <= sig_hps_io_emac1_inst_TXD3;
         sig_hps_io_emac1_inst_TX_CLK_in <= sig_hps_io_emac1_inst_TX_CLK;
         sig_hps_io_emac1_inst_TX_CTL_in <= sig_hps_io_emac1_inst_TX_CTL;

         if out_trans.sig_hps_io_gpio_inst_GPIO35_oe = '1' then
            sig_hps_io_gpio_inst_GPIO35    <= out_trans.sig_hps_io_gpio_inst_GPIO35_out after 1 ps;
            sig_hps_io_gpio_inst_GPIO35_in <= (others => 'Z');
         else
            sig_hps_io_gpio_inst_GPIO35    <= (others => 'Z');
            sig_hps_io_gpio_inst_GPIO35_in <= sig_hps_io_gpio_inst_GPIO35;
         end if;
         sig_hps_io_sdio_inst_CLK_in    <= sig_hps_io_sdio_inst_CLK;

         if out_trans.sig_hps_io_sdio_inst_CMD_oe = '1' then
            sig_hps_io_sdio_inst_CMD       <= out_trans.sig_hps_io_sdio_inst_CMD_out after 1 ps;
            sig_hps_io_sdio_inst_CMD_in    <= (others => 'Z');
         else
            sig_hps_io_sdio_inst_CMD       <= (others => 'Z');
            sig_hps_io_sdio_inst_CMD_in    <= sig_hps_io_sdio_inst_CMD;
         end if;

         if out_trans.sig_hps_io_sdio_inst_D0_oe = '1' then
            sig_hps_io_sdio_inst_D0        <= out_trans.sig_hps_io_sdio_inst_D0_out after 1 ps;
            sig_hps_io_sdio_inst_D0_in     <= (others => 'Z');
         else
            sig_hps_io_sdio_inst_D0        <= (others => 'Z');
            sig_hps_io_sdio_inst_D0_in     <= sig_hps_io_sdio_inst_D0;
         end if;

         if out_trans.sig_hps_io_sdio_inst_D1_oe = '1' then
            sig_hps_io_sdio_inst_D1        <= out_trans.sig_hps_io_sdio_inst_D1_out after 1 ps;
            sig_hps_io_sdio_inst_D1_in     <= (others => 'Z');
         else
            sig_hps_io_sdio_inst_D1        <= (others => 'Z');
            sig_hps_io_sdio_inst_D1_in     <= sig_hps_io_sdio_inst_D1;
         end if;

         if out_trans.sig_hps_io_sdio_inst_D2_oe = '1' then
            sig_hps_io_sdio_inst_D2        <= out_trans.sig_hps_io_sdio_inst_D2_out after 1 ps;
            sig_hps_io_sdio_inst_D2_in     <= (others => 'Z');
         else
            sig_hps_io_sdio_inst_D2        <= (others => 'Z');
            sig_hps_io_sdio_inst_D2_in     <= sig_hps_io_sdio_inst_D2;
         end if;

         if out_trans.sig_hps_io_sdio_inst_D3_oe = '1' then
            sig_hps_io_sdio_inst_D3        <= out_trans.sig_hps_io_sdio_inst_D3_out after 1 ps;
            sig_hps_io_sdio_inst_D3_in     <= (others => 'Z');
         else
            sig_hps_io_sdio_inst_D3        <= (others => 'Z');
            sig_hps_io_sdio_inst_D3_in     <= sig_hps_io_sdio_inst_D3;
         end if;
         sig_hps_io_uart0_inst_RX       <= out_trans.sig_hps_io_uart0_inst_RX_out after 1 ps;
         sig_hps_io_uart0_inst_TX_in    <= sig_hps_io_uart0_inst_TX;
      end process;

end altera_conduit_bfm_0002_arch;

