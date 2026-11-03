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
-- output_name:                  altera_conduit_bfm
-- role:width:direction:         mem_a:15:input,mem_ba:3:input,mem_cas_n:1:input,mem_ck:1:input,mem_ck_n:1:input,mem_cke:1:input,mem_cs_n:1:input,mem_dm:4:input,mem_dq:32:bidir,mem_dqs:4:bidir,mem_dqs_n:4:bidir,mem_odt:1:input,mem_ras_n:1:input,mem_reset_n:1:input,mem_we_n:1:input,oct_rzqin:1:output
-- clocked                       0
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

library work;
use work.all;

















package altera_conduit_bfm_vhdl_pkg is

   -- output signal register
   type altera_conduit_bfm_out_trans_t is record
      sig_mem_dq_out          : std_logic_vector(31 downto 0);
      sig_mem_dq_oe           : std_logic;
      sig_mem_dqs_out         : std_logic_vector(3 downto 0);
      sig_mem_dqs_oe          : std_logic;
      sig_mem_dqs_n_out       : std_logic_vector(3 downto 0);
      sig_mem_dqs_n_oe        : std_logic;
      sig_oct_rzqin_out       : std_logic_vector(0 downto 0);
   end record;
   
   shared variable out_trans        : altera_conduit_bfm_out_trans_t;

   -- input signal register
   signal sig_mem_a_in            : std_logic_vector(14 downto 0);
   signal sig_mem_ba_in           : std_logic_vector(2 downto 0);
   signal sig_mem_cas_n_in        : std_logic_vector(0 downto 0);
   signal sig_mem_ck_in           : std_logic_vector(0 downto 0);
   signal sig_mem_ck_n_in         : std_logic_vector(0 downto 0);
   signal sig_mem_cke_in          : std_logic_vector(0 downto 0);
   signal sig_mem_cs_n_in         : std_logic_vector(0 downto 0);
   signal sig_mem_dm_in           : std_logic_vector(3 downto 0);
   signal sig_mem_dq_in           : std_logic_vector(31 downto 0);
   signal sig_mem_dqs_in          : std_logic_vector(3 downto 0);
   signal sig_mem_dqs_n_in        : std_logic_vector(3 downto 0);
   signal sig_mem_odt_in          : std_logic_vector(0 downto 0);
   signal sig_mem_ras_n_in        : std_logic_vector(0 downto 0);
   signal sig_mem_reset_n_in      : std_logic_vector(0 downto 0);
   signal sig_mem_we_n_in         : std_logic_vector(0 downto 0);

   -- VHDL Procedure API
   
   -- get mem_a value
   procedure get_mem_a                   (signal_value : out std_logic_vector(14 downto 0));
   
   -- get mem_ba value
   procedure get_mem_ba                  (signal_value : out std_logic_vector(2 downto 0));
   
   -- get mem_cas_n value
   procedure get_mem_cas_n               (signal_value : out std_logic_vector(0 downto 0));
   
   -- get mem_ck value
   procedure get_mem_ck                  (signal_value : out std_logic_vector(0 downto 0));
   
   -- get mem_ck_n value
   procedure get_mem_ck_n                (signal_value : out std_logic_vector(0 downto 0));
   
   -- get mem_cke value
   procedure get_mem_cke                 (signal_value : out std_logic_vector(0 downto 0));
   
   -- get mem_cs_n value
   procedure get_mem_cs_n                (signal_value : out std_logic_vector(0 downto 0));
   
   -- get mem_dm value
   procedure get_mem_dm                  (signal_value : out std_logic_vector(3 downto 0));
   
   -- get mem_dq value
   procedure get_mem_dq                  (signal_value : out std_logic_vector(31 downto 0));
   
   -- set mem_dq value
   procedure set_mem_dq                  (signal_value : in std_logic_vector(31 downto 0));
   
   -- set mem_dq input / output direction
   procedure set_mem_dq_oe               (signal_value : in std_logic);
   
   -- get mem_dqs value
   procedure get_mem_dqs                 (signal_value : out std_logic_vector(3 downto 0));
   
   -- set mem_dqs value
   procedure set_mem_dqs                 (signal_value : in std_logic_vector(3 downto 0));
   
   -- set mem_dqs input / output direction
   procedure set_mem_dqs_oe              (signal_value : in std_logic);
   
   -- get mem_dqs_n value
   procedure get_mem_dqs_n               (signal_value : out std_logic_vector(3 downto 0));
   
   -- set mem_dqs_n value
   procedure set_mem_dqs_n               (signal_value : in std_logic_vector(3 downto 0));
   
   -- set mem_dqs_n input / output direction
   procedure set_mem_dqs_n_oe            (signal_value : in std_logic);
   
   -- get mem_odt value
   procedure get_mem_odt                 (signal_value : out std_logic_vector(0 downto 0));
   
   -- get mem_ras_n value
   procedure get_mem_ras_n               (signal_value : out std_logic_vector(0 downto 0));
   
   -- get mem_reset_n value
   procedure get_mem_reset_n             (signal_value : out std_logic_vector(0 downto 0));
   
   -- get mem_we_n value
   procedure get_mem_we_n                (signal_value : out std_logic_vector(0 downto 0));
   
   -- set oct_rzqin value
   procedure set_oct_rzqin               (signal_value : in std_logic_vector(0 downto 0));
   
   -- VHDL Event API

   procedure event_mem_a_change;   

   procedure event_mem_ba_change;   

   procedure event_mem_cas_n_change;   

   procedure event_mem_ck_change;   

   procedure event_mem_ck_n_change;   

   procedure event_mem_cke_change;   

   procedure event_mem_cs_n_change;   

   procedure event_mem_dm_change;   

   procedure event_mem_dq_change;   

   procedure event_mem_dqs_change;   

   procedure event_mem_dqs_n_change;   

   procedure event_mem_odt_change;   

   procedure event_mem_ras_n_change;   

   procedure event_mem_reset_n_change;   

   procedure event_mem_we_n_change;   

end altera_conduit_bfm_vhdl_pkg;

package body altera_conduit_bfm_vhdl_pkg is
   
   procedure get_mem_a                   (signal_value : out std_logic_vector(14 downto 0)) is
   begin

      signal_value := sig_mem_a_in;
   
   end procedure get_mem_a;
   
   procedure get_mem_ba                  (signal_value : out std_logic_vector(2 downto 0)) is
   begin

      signal_value := sig_mem_ba_in;
   
   end procedure get_mem_ba;
   
   procedure get_mem_cas_n               (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_mem_cas_n_in;
   
   end procedure get_mem_cas_n;
   
   procedure get_mem_ck                  (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_mem_ck_in;
   
   end procedure get_mem_ck;
   
   procedure get_mem_ck_n                (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_mem_ck_n_in;
   
   end procedure get_mem_ck_n;
   
   procedure get_mem_cke                 (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_mem_cke_in;
   
   end procedure get_mem_cke;
   
   procedure get_mem_cs_n                (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_mem_cs_n_in;
   
   end procedure get_mem_cs_n;
   
   procedure get_mem_dm                  (signal_value : out std_logic_vector(3 downto 0)) is
   begin

      signal_value := sig_mem_dm_in;
   
   end procedure get_mem_dm;
   
   procedure get_mem_dq                  (signal_value : out std_logic_vector(31 downto 0)) is
   begin

      signal_value := sig_mem_dq_in;
   
   end procedure get_mem_dq;
   
   procedure set_mem_dq                  (signal_value : in std_logic_vector(31 downto 0)) is
   begin
      
      out_trans.sig_mem_dq_out := signal_value;
      
   end procedure set_mem_dq;
   
   procedure set_mem_dq_oe               (signal_value : in std_logic) is
   begin
   
      out_trans.sig_mem_dq_oe := signal_value;
   
   end procedure set_mem_dq_oe;
   
   procedure get_mem_dqs                 (signal_value : out std_logic_vector(3 downto 0)) is
   begin

      signal_value := sig_mem_dqs_in;
   
   end procedure get_mem_dqs;
   
   procedure set_mem_dqs                 (signal_value : in std_logic_vector(3 downto 0)) is
   begin
      
      out_trans.sig_mem_dqs_out := signal_value;
      
   end procedure set_mem_dqs;
   
   procedure set_mem_dqs_oe              (signal_value : in std_logic) is
   begin
   
      out_trans.sig_mem_dqs_oe := signal_value;
   
   end procedure set_mem_dqs_oe;
   
   procedure get_mem_dqs_n               (signal_value : out std_logic_vector(3 downto 0)) is
   begin

      signal_value := sig_mem_dqs_n_in;
   
   end procedure get_mem_dqs_n;
   
   procedure set_mem_dqs_n               (signal_value : in std_logic_vector(3 downto 0)) is
   begin
      
      out_trans.sig_mem_dqs_n_out := signal_value;
      
   end procedure set_mem_dqs_n;
   
   procedure set_mem_dqs_n_oe            (signal_value : in std_logic) is
   begin
   
      out_trans.sig_mem_dqs_n_oe := signal_value;
   
   end procedure set_mem_dqs_n_oe;
   
   procedure get_mem_odt                 (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_mem_odt_in;
   
   end procedure get_mem_odt;
   
   procedure get_mem_ras_n               (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_mem_ras_n_in;
   
   end procedure get_mem_ras_n;
   
   procedure get_mem_reset_n             (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_mem_reset_n_in;
   
   end procedure get_mem_reset_n;
   
   procedure get_mem_we_n                (signal_value : out std_logic_vector(0 downto 0)) is
   begin

      signal_value := sig_mem_we_n_in;
   
   end procedure get_mem_we_n;
   
   procedure set_oct_rzqin               (signal_value : in std_logic_vector(0 downto 0)) is
   begin
      
      out_trans.sig_oct_rzqin_out := signal_value;
      
   end procedure set_oct_rzqin;
   
   procedure event_mem_a_change is
   begin

      wait until (sig_mem_a_in'event);

   end event_mem_a_change;
   procedure event_mem_ba_change is
   begin

      wait until (sig_mem_ba_in'event);

   end event_mem_ba_change;
   procedure event_mem_cas_n_change is
   begin

      wait until (sig_mem_cas_n_in'event);

   end event_mem_cas_n_change;
   procedure event_mem_ck_change is
   begin

      wait until (sig_mem_ck_in'event);

   end event_mem_ck_change;
   procedure event_mem_ck_n_change is
   begin

      wait until (sig_mem_ck_n_in'event);

   end event_mem_ck_n_change;
   procedure event_mem_cke_change is
   begin

      wait until (sig_mem_cke_in'event);

   end event_mem_cke_change;
   procedure event_mem_cs_n_change is
   begin

      wait until (sig_mem_cs_n_in'event);

   end event_mem_cs_n_change;
   procedure event_mem_dm_change is
   begin

      wait until (sig_mem_dm_in'event);

   end event_mem_dm_change;
   procedure event_mem_dq_change is
   begin

      wait until (sig_mem_dq_in'event and out_trans.sig_mem_dq_oe = '0');

   end event_mem_dq_change;
   procedure event_mem_dqs_change is
   begin

      wait until (sig_mem_dqs_in'event and out_trans.sig_mem_dqs_oe = '0');

   end event_mem_dqs_change;
   procedure event_mem_dqs_n_change is
   begin

      wait until (sig_mem_dqs_n_in'event and out_trans.sig_mem_dqs_n_oe = '0');

   end event_mem_dqs_n_change;
   procedure event_mem_odt_change is
   begin

      wait until (sig_mem_odt_in'event);

   end event_mem_odt_change;
   procedure event_mem_ras_n_change is
   begin

      wait until (sig_mem_ras_n_in'event);

   end event_mem_ras_n_change;
   procedure event_mem_reset_n_change is
   begin

      wait until (sig_mem_reset_n_in'event);

   end event_mem_reset_n_change;
   procedure event_mem_we_n_change is
   begin

      wait until (sig_mem_we_n_in'event);

   end event_mem_we_n_change;

end altera_conduit_bfm_vhdl_pkg;

