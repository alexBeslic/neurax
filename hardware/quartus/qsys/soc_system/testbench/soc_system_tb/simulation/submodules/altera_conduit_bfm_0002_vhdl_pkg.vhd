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
-- This is a Bus Functional Model (BFM) VHDL package for a Standard Conduit Master.
-- This package provides the API that will be used to get the value of the sampled
-- input/bidirection port or set the value to be driven to the output ports.
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
























package altera_conduit_bfm_0002_vhdl_pkg is

   -- output signal register
   type altera_conduit_bfm_0002_out_trans_t is record
      sig_hps_io_emac1_inst_MDIO_out       : std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_MDIO_oe        : std_logic;
      sig_hps_io_emac1_inst_RXD0_out       : std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_RXD1_out       : std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_RXD2_out       : std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_RXD3_out       : std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_RX_CLK_out     : std_logic_vector(0 downto 0);
      sig_hps_io_emac1_inst_RX_CTL_out     : std_logic_vector(0 downto 0);
      sig_hps_io_gpio_inst_GPIO35_out      : std_logic_vector(0 downto 0);
      sig_hps_io_gpio_inst_GPIO35_oe       : std_logic;
      sig_hps_io_sdio_inst_CMD_out         : std_logic_vector(0 downto 0);
      sig_hps_io_sdio_inst_CMD_oe          : std_logic;
      sig_hps_io_sdio_inst_D0_out          : std_logic_vector(0 downto 0);
      sig_hps_io_sdio_inst_D0_oe           : std_logic;
      sig_hps_io_sdio_inst_D1_out          : std_logic_vector(0 downto 0);
      sig_hps_io_sdio_inst_D1_oe           : std_logic;
      sig_hps_io_sdio_inst_D2_out          : std_logic_vector(0 downto 0);
      sig_hps_io_sdio_inst_D2_oe           : std_logic;
      sig_hps_io_sdio_inst_D3_out          : std_logic_vector(0 downto 0);
      sig_hps_io_sdio_inst_D3_oe           : std_logic;
      sig_hps_io_uart0_inst_RX_out         : std_logic_vector(0 downto 0);
   end record;
   
   shared variable out_trans        : altera_conduit_bfm_0002_out_trans_t;

   -- input signal register
   signal sig_hps_io_emac1_inst_MDC_in         : std_logic_vector(0 downto 0);
   signal sig_hps_io_emac1_inst_MDIO_in        : std_logic_vector(0 downto 0);
   signal sig_hps_io_emac1_inst_TXD0_in        : std_logic_vector(0 downto 0);
   signal sig_hps_io_emac1_inst_TXD1_in        : std_logic_vector(0 downto 0);
   signal sig_hps_io_emac1_inst_TXD2_in        : std_logic_vector(0 downto 0);
   signal sig_hps_io_emac1_inst_TXD3_in        : std_logic_vector(0 downto 0);
   signal sig_hps_io_emac1_inst_TX_CLK_in      : std_logic_vector(0 downto 0);
   signal sig_hps_io_emac1_inst_TX_CTL_in      : std_logic_vector(0 downto 0);
   signal sig_hps_io_gpio_inst_GPIO35_in       : std_logic_vector(0 downto 0);
   signal sig_hps_io_sdio_inst_CLK_in          : std_logic_vector(0 downto 0);
   signal sig_hps_io_sdio_inst_CMD_in          : std_logic_vector(0 downto 0);
   signal sig_hps_io_sdio_inst_D0_in           : std_logic_vector(0 downto 0);
   signal sig_hps_io_sdio_inst_D1_in           : std_logic_vector(0 downto 0);
   signal sig_hps_io_sdio_inst_D2_in           : std_logic_vector(0 downto 0);
   signal sig_hps_io_sdio_inst_D3_in           : std_logic_vector(0 downto 0);
   signal sig_hps_io_uart0_inst_TX_in          : std_logic_vector(0 downto 0);

   -- VHDL Procedure API
   
   -- get hps_io_emac1_inst_MDC value
   procedure get_hps_io_emac1_inst_MDC                (signal_value : out std_logic_vector(0 downto 0));
   
   -- get hps_io_emac1_inst_MDIO value
   procedure get_hps_io_emac1_inst_MDIO               (signal_value : out std_logic_vector(0 downto 0));
   
   -- set hps_io_emac1_inst_MDIO value
   procedure set_hps_io_emac1_inst_MDIO               (signal_value : in std_logic_vector(0 downto 0));
   
   -- set hps_io_emac1_inst_MDIO input / output direction
   procedure set_hps_io_emac1_inst_MDIO_oe            (signal_value : in std_logic);
   
   -- set hps_io_emac1_inst_RXD0 value
   procedure set_hps_io_emac1_inst_RXD0               (signal_value : in std_logic_vector(0 downto 0));
   
   -- set hps_io_emac1_inst_RXD1 value
   procedure set_hps_io_emac1_inst_RXD1               (signal_value : in std_logic_vector(0 downto 0));
   
   -- set hps_io_emac1_inst_RXD2 value
   procedure set_hps_io_emac1_inst_RXD2               (signal_value : in std_logic_vector(0 downto 0));
   
   -- set hps_io_emac1_inst_RXD3 value
   procedure set_hps_io_emac1_inst_RXD3               (signal_value : in std_logic_vector(0 downto 0));
   
   -- set hps_io_emac1_inst_RX_CLK value
   procedure set_hps_io_emac1_inst_RX_CLK             (signal_value : in std_logic_vector(0 downto 0));
   
   -- set hps_io_emac1_inst_RX_CTL value
   procedure set_hps_io_emac1_inst_RX_CTL             (signal_value : in std_logic_vector(0 downto 0));
   
   -- get hps_io_emac1_inst_TXD0 value
   procedure get_hps_io_emac1_inst_TXD0               (signal_value : out std_logic_vector(0 downto 0));
   
   -- get hps_io_emac1_inst_TXD1 value
   procedure get_hps_io_emac1_inst_TXD1               (signal_value : out std_logic_vector(0 downto 0));
   
   -- get hps_io_emac1_inst_TXD2 value
   procedure get_hps_io_emac1_inst_TXD2               (signal_value : out std_logic_vector(0 downto 0));
   
   -- get hps_io_emac1_inst_TXD3 value
   procedure get_hps_io_emac1_inst_TXD3               (signal_value : out std_logic_vector(0 downto 0));
   
   -- get hps_io_emac1_inst_TX_CLK value
   procedure get_hps_io_emac1_inst_TX_CLK             (signal_value : out std_logic_vector(0 downto 0));
   
   -- get hps_io_emac1_inst_TX_CTL value
   procedure get_hps_io_emac1_inst_TX_CTL             (signal_value : out std_logic_vector(0 downto 0));
   
   -- get hps_io_gpio_inst_GPIO35 value
   procedure get_hps_io_gpio_inst_GPIO35              (signal_value : out std_logic_vector(0 downto 0));
   
   -- set hps_io_gpio_inst_GPIO35 value
   procedure set_hps_io_gpio_inst_GPIO35              (signal_value : in std_logic_vector(0 downto 0));
   
   -- set hps_io_gpio_inst_GPIO35 input / output direction
   procedure set_hps_io_gpio_inst_GPIO35_oe           (signal_value : in std_logic);
   
   -- get hps_io_sdio_inst_CLK value
   procedure get_hps_io_sdio_inst_CLK                 (signal_value : out std_logic_vector(0 downto 0));
   
   -- get hps_io_sdio_inst_CMD value
   procedure get_hps_io_sdio_inst_CMD                 (signal_value : out std_logic_vector(0 downto 0));
   
   -- set hps_io_sdio_inst_CMD value
   procedure set_hps_io_sdio_inst_CMD                 (signal_value : in std_logic_vector(0 downto 0));
   
   -- set hps_io_sdio_inst_CMD input / output direction
   procedure set_hps_io_sdio_inst_CMD_oe              (signal_value : in std_logic);
   
   -- get hps_io_sdio_inst_D0 value
   procedure get_hps_io_sdio_inst_D0                  (signal_value : out std_logic_vector(0 downto 0));
   
   -- set hps_io_sdio_inst_D0 value
   procedure set_hps_io_sdio_inst_D0                  (signal_value : in std_logic_vector(0 downto 0));
   
   -- set hps_io_sdio_inst_D0 input / output direction
   procedure set_hps_io_sdio_inst_D0_oe               (signal_value : in std_logic);
   
   -- get hps_io_sdio_inst_D1 value
   procedure get_hps_io_sdio_inst_D1                  (signal_value : out std_logic_vector(0 downto 0));
   
   -- set hps_io_sdio_inst_D1 value
   procedure set_hps_io_sdio_inst_D1                  (signal_value : in std_logic_vector(0 downto 0));
   
   -- set hps_io_sdio_inst_D1 input / output direction
   procedure set_hps_io_sdio_inst_D1_oe               (signal_value : in std_logic);
   
   -- get hps_io_sdio_inst_D2 value
   procedure get_hps_io_sdio_inst_D2                  (signal_value : out std_logic_vector(0 downto 0));
   
   -- set hps_io_sdio_inst_D2 value
   procedure set_hps_io_sdio_inst_D2                  (signal_value : in std_logic_vector(0 downto 0));
   
   -- set hps_io_sdio_inst_D2 input / output direction
   procedure set_hps_io_sdio_inst_D2_oe               (signal_value : in std_logic);
   
   -- get hps_io_sdio_inst_D3 value
   procedure get_hps_io_sdio_inst_D3                  (signal_value : out std_logic_vector(0 downto 0));
   
   -- set hps_io_sdio_inst_D3 value
   procedure set_hps_io_sdio_inst_D3                  (signal_value : in std_logic_vector(0 downto 0));
   
   -- set hps_io_sdio_inst_D3 input / output direction
   procedure set_hps_io_sdio_inst_D3_oe               (signal_value : in std_logic);
   
   -- set hps_io_uart0_inst_RX value
   procedure set_hps_io_uart0_inst_RX                 (signal_value : in std_logic_vector(0 downto 0));
   
   -- get hps_io_uart0_inst_TX value
   procedure get_hps_io_uart0_inst_TX                 (signal_value : out std_logic_vector(0 downto 0));
   
   -- VHDL Event API

   procedure event_hps_io_emac1_inst_MDC_change;   

   procedure event_hps_io_emac1_inst_MDIO_change;   

   procedure event_hps_io_emac1_inst_TXD0_change;   

   procedure event_hps_io_emac1_inst_TXD1_change;   

   procedure event_hps_io_emac1_inst_TXD2_change;   

   procedure event_hps_io_emac1_inst_TXD3_change;   

   procedure event_hps_io_emac1_inst_TX_CLK_change;   

   procedure event_hps_io_emac1_inst_TX_CTL_change;   

   procedure event_hps_io_gpio_inst_GPIO35_change;   

   procedure event_hps_io_sdio_inst_CLK_change;   

   procedure event_hps_io_sdio_inst_CMD_change;   

   procedure event_hps_io_sdio_inst_D0_change;   

   procedure event_hps_io_sdio_inst_D1_change;   

   procedure event_hps_io_sdio_inst_D2_change;   

   procedure event_hps_io_sdio_inst_D3_change;   

   procedure event_hps_io_uart0_inst_TX_change;   

end altera_conduit_bfm_0002_vhdl_pkg;

package body altera_conduit_bfm_0002_vhdl_pkg is
   
   procedure get_hps_io_emac1_inst_MDC                (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_emac1_inst_MDC_in;
   
   end procedure get_hps_io_emac1_inst_MDC;
   
   procedure get_hps_io_emac1_inst_MDIO               (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_emac1_inst_MDIO_in;
   
   end procedure get_hps_io_emac1_inst_MDIO;
   
   procedure set_hps_io_emac1_inst_MDIO               (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_emac1_inst_MDIO_out := signal_value;
      
   end procedure set_hps_io_emac1_inst_MDIO;
   
   procedure set_hps_io_emac1_inst_MDIO_oe            (signal_value : in std_logic) is
   begin
   
      out_trans.sig_hps_io_emac1_inst_MDIO_oe := signal_value;
   
   end procedure set_hps_io_emac1_inst_MDIO_oe;
   
   procedure set_hps_io_emac1_inst_RXD0               (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_emac1_inst_RXD0_out := signal_value;
      
   end procedure set_hps_io_emac1_inst_RXD0;
   
   procedure set_hps_io_emac1_inst_RXD1               (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_emac1_inst_RXD1_out := signal_value;
      
   end procedure set_hps_io_emac1_inst_RXD1;
   
   procedure set_hps_io_emac1_inst_RXD2               (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_emac1_inst_RXD2_out := signal_value;
      
   end procedure set_hps_io_emac1_inst_RXD2;
   
   procedure set_hps_io_emac1_inst_RXD3               (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_emac1_inst_RXD3_out := signal_value;
      
   end procedure set_hps_io_emac1_inst_RXD3;
   
   procedure set_hps_io_emac1_inst_RX_CLK             (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_emac1_inst_RX_CLK_out := signal_value;
      
   end procedure set_hps_io_emac1_inst_RX_CLK;
   
   procedure set_hps_io_emac1_inst_RX_CTL             (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_emac1_inst_RX_CTL_out := signal_value;
      
   end procedure set_hps_io_emac1_inst_RX_CTL;
   
   procedure get_hps_io_emac1_inst_TXD0               (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_emac1_inst_TXD0_in;
   
   end procedure get_hps_io_emac1_inst_TXD0;
   
   procedure get_hps_io_emac1_inst_TXD1               (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_emac1_inst_TXD1_in;
   
   end procedure get_hps_io_emac1_inst_TXD1;
   
   procedure get_hps_io_emac1_inst_TXD2               (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_emac1_inst_TXD2_in;
   
   end procedure get_hps_io_emac1_inst_TXD2;
   
   procedure get_hps_io_emac1_inst_TXD3               (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_emac1_inst_TXD3_in;
   
   end procedure get_hps_io_emac1_inst_TXD3;
   
   procedure get_hps_io_emac1_inst_TX_CLK             (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_emac1_inst_TX_CLK_in;
   
   end procedure get_hps_io_emac1_inst_TX_CLK;
   
   procedure get_hps_io_emac1_inst_TX_CTL             (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_emac1_inst_TX_CTL_in;
   
   end procedure get_hps_io_emac1_inst_TX_CTL;
   
   procedure get_hps_io_gpio_inst_GPIO35              (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_gpio_inst_GPIO35_in;
   
   end procedure get_hps_io_gpio_inst_GPIO35;
   
   procedure set_hps_io_gpio_inst_GPIO35              (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_gpio_inst_GPIO35_out := signal_value;
      
   end procedure set_hps_io_gpio_inst_GPIO35;
   
   procedure set_hps_io_gpio_inst_GPIO35_oe           (signal_value : in std_logic) is
   begin
   
      out_trans.sig_hps_io_gpio_inst_GPIO35_oe := signal_value;
   
   end procedure set_hps_io_gpio_inst_GPIO35_oe;
   
   procedure get_hps_io_sdio_inst_CLK                 (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_sdio_inst_CLK_in;
   
   end procedure get_hps_io_sdio_inst_CLK;
   
   procedure get_hps_io_sdio_inst_CMD                 (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_sdio_inst_CMD_in;
   
   end procedure get_hps_io_sdio_inst_CMD;
   
   procedure set_hps_io_sdio_inst_CMD                 (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_sdio_inst_CMD_out := signal_value;
      
   end procedure set_hps_io_sdio_inst_CMD;
   
   procedure set_hps_io_sdio_inst_CMD_oe              (signal_value : in std_logic) is
   begin
   
      out_trans.sig_hps_io_sdio_inst_CMD_oe := signal_value;
   
   end procedure set_hps_io_sdio_inst_CMD_oe;
   
   procedure get_hps_io_sdio_inst_D0                  (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_sdio_inst_D0_in;
   
   end procedure get_hps_io_sdio_inst_D0;
   
   procedure set_hps_io_sdio_inst_D0                  (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_sdio_inst_D0_out := signal_value;
      
   end procedure set_hps_io_sdio_inst_D0;
   
   procedure set_hps_io_sdio_inst_D0_oe               (signal_value : in std_logic) is
   begin
   
      out_trans.sig_hps_io_sdio_inst_D0_oe := signal_value;
   
   end procedure set_hps_io_sdio_inst_D0_oe;
   
   procedure get_hps_io_sdio_inst_D1                  (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_sdio_inst_D1_in;
   
   end procedure get_hps_io_sdio_inst_D1;
   
   procedure set_hps_io_sdio_inst_D1                  (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_sdio_inst_D1_out := signal_value;
      
   end procedure set_hps_io_sdio_inst_D1;
   
   procedure set_hps_io_sdio_inst_D1_oe               (signal_value : in std_logic) is
   begin
   
      out_trans.sig_hps_io_sdio_inst_D1_oe := signal_value;
   
   end procedure set_hps_io_sdio_inst_D1_oe;
   
   procedure get_hps_io_sdio_inst_D2                  (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_sdio_inst_D2_in;
   
   end procedure get_hps_io_sdio_inst_D2;
   
   procedure set_hps_io_sdio_inst_D2                  (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_sdio_inst_D2_out := signal_value;
      
   end procedure set_hps_io_sdio_inst_D2;
   
   procedure set_hps_io_sdio_inst_D2_oe               (signal_value : in std_logic) is
   begin
   
      out_trans.sig_hps_io_sdio_inst_D2_oe := signal_value;
   
   end procedure set_hps_io_sdio_inst_D2_oe;
   
   procedure get_hps_io_sdio_inst_D3                  (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_sdio_inst_D3_in;
   
   end procedure get_hps_io_sdio_inst_D3;
   
   procedure set_hps_io_sdio_inst_D3                  (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_sdio_inst_D3_out := signal_value;
      
   end procedure set_hps_io_sdio_inst_D3;
   
   procedure set_hps_io_sdio_inst_D3_oe               (signal_value : in std_logic) is
   begin
   
      out_trans.sig_hps_io_sdio_inst_D3_oe := signal_value;
   
   end procedure set_hps_io_sdio_inst_D3_oe;
   
   procedure set_hps_io_uart0_inst_RX                 (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_hps_io_uart0_inst_RX_out := signal_value;
      
   end procedure set_hps_io_uart0_inst_RX;
   
   procedure get_hps_io_uart0_inst_TX                 (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_hps_io_uart0_inst_TX_in;
   
   end procedure get_hps_io_uart0_inst_TX;
   
   procedure event_hps_io_emac1_inst_MDC_change is
   begin

      wait until (sig_hps_io_emac1_inst_MDC_in'event);

   end event_hps_io_emac1_inst_MDC_change;
   procedure event_hps_io_emac1_inst_MDIO_change is
   begin

      wait until (sig_hps_io_emac1_inst_MDIO_in'event and out_trans.sig_hps_io_emac1_inst_MDIO_oe = '0');

   end event_hps_io_emac1_inst_MDIO_change;
   procedure event_hps_io_emac1_inst_TXD0_change is
   begin

      wait until (sig_hps_io_emac1_inst_TXD0_in'event);

   end event_hps_io_emac1_inst_TXD0_change;
   procedure event_hps_io_emac1_inst_TXD1_change is
   begin

      wait until (sig_hps_io_emac1_inst_TXD1_in'event);

   end event_hps_io_emac1_inst_TXD1_change;
   procedure event_hps_io_emac1_inst_TXD2_change is
   begin

      wait until (sig_hps_io_emac1_inst_TXD2_in'event);

   end event_hps_io_emac1_inst_TXD2_change;
   procedure event_hps_io_emac1_inst_TXD3_change is
   begin

      wait until (sig_hps_io_emac1_inst_TXD3_in'event);

   end event_hps_io_emac1_inst_TXD3_change;
   procedure event_hps_io_emac1_inst_TX_CLK_change is
   begin

      wait until (sig_hps_io_emac1_inst_TX_CLK_in'event);

   end event_hps_io_emac1_inst_TX_CLK_change;
   procedure event_hps_io_emac1_inst_TX_CTL_change is
   begin

      wait until (sig_hps_io_emac1_inst_TX_CTL_in'event);

   end event_hps_io_emac1_inst_TX_CTL_change;
   procedure event_hps_io_gpio_inst_GPIO35_change is
   begin

      wait until (sig_hps_io_gpio_inst_GPIO35_in'event and out_trans.sig_hps_io_gpio_inst_GPIO35_oe = '0');

   end event_hps_io_gpio_inst_GPIO35_change;
   procedure event_hps_io_sdio_inst_CLK_change is
   begin

      wait until (sig_hps_io_sdio_inst_CLK_in'event);

   end event_hps_io_sdio_inst_CLK_change;
   procedure event_hps_io_sdio_inst_CMD_change is
   begin

      wait until (sig_hps_io_sdio_inst_CMD_in'event and out_trans.sig_hps_io_sdio_inst_CMD_oe = '0');

   end event_hps_io_sdio_inst_CMD_change;
   procedure event_hps_io_sdio_inst_D0_change is
   begin

      wait until (sig_hps_io_sdio_inst_D0_in'event and out_trans.sig_hps_io_sdio_inst_D0_oe = '0');

   end event_hps_io_sdio_inst_D0_change;
   procedure event_hps_io_sdio_inst_D1_change is
   begin

      wait until (sig_hps_io_sdio_inst_D1_in'event and out_trans.sig_hps_io_sdio_inst_D1_oe = '0');

   end event_hps_io_sdio_inst_D1_change;
   procedure event_hps_io_sdio_inst_D2_change is
   begin

      wait until (sig_hps_io_sdio_inst_D2_in'event and out_trans.sig_hps_io_sdio_inst_D2_oe = '0');

   end event_hps_io_sdio_inst_D2_change;
   procedure event_hps_io_sdio_inst_D3_change is
   begin

      wait until (sig_hps_io_sdio_inst_D3_in'event and out_trans.sig_hps_io_sdio_inst_D3_oe = '0');

   end event_hps_io_sdio_inst_D3_change;
   procedure event_hps_io_uart0_inst_TX_change is
   begin

      wait until (sig_hps_io_uart0_inst_TX_in'event);

   end event_hps_io_uart0_inst_TX_change;

end altera_conduit_bfm_0002_vhdl_pkg;

