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
-- output_name:                  altera_conduit_bfm
-- role:width:direction:         mem_a:15:input,mem_ba:3:input,mem_cas_n:1:input,mem_ck:1:input,mem_ck_n:1:input,mem_cke:1:input,mem_cs_n:1:input,mem_dm:4:input,mem_dq:32:bidir,mem_dqs:4:bidir,mem_dqs_n:4:bidir,mem_odt:1:input,mem_ras_n:1:input,mem_reset_n:1:input,mem_we_n:1:input,oct_rzqin:1:output
-- clocked                       0
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

library work;
use work.all;
use work.altera_conduit_bfm_vhdl_pkg.all;

entity altera_conduit_bfm is
   port (
      sig_mem_a         : in    std_logic_vector(14 downto 0);
      sig_mem_ba        : in    std_logic_vector(2 downto 0);
      sig_mem_cas_n     : in    std_logic_vector(0 downto 0);
      sig_mem_ck        : in    std_logic_vector(0 downto 0);
      sig_mem_ck_n      : in    std_logic_vector(0 downto 0);
      sig_mem_cke       : in    std_logic_vector(0 downto 0);
      sig_mem_cs_n      : in    std_logic_vector(0 downto 0);
      sig_mem_dm        : in    std_logic_vector(3 downto 0);
      sig_mem_dq        : inout std_logic_vector(31 downto 0);
      sig_mem_dqs       : inout std_logic_vector(3 downto 0);
      sig_mem_dqs_n     : inout std_logic_vector(3 downto 0);
      sig_mem_odt       : in    std_logic_vector(0 downto 0);
      sig_mem_ras_n     : in    std_logic_vector(0 downto 0);
      sig_mem_reset_n   : in    std_logic_vector(0 downto 0);
      sig_mem_we_n      : in    std_logic_vector(0 downto 0);
      sig_oct_rzqin     : out   std_logic_vector(0 downto 0)
   );
end altera_conduit_bfm;

architecture altera_conduit_bfm_arch of altera_conduit_bfm is 

      signal update : std_logic := '0';

   begin
      process begin
         wait for 1 ps;
         update <= not update;
      end process;

      process (update) begin
         sig_mem_a_in      <= sig_mem_a;
         sig_mem_ba_in     <= sig_mem_ba;
         sig_mem_cas_n_in  <= sig_mem_cas_n;
         sig_mem_ck_in     <= sig_mem_ck;
         sig_mem_ck_n_in   <= sig_mem_ck_n;
         sig_mem_cke_in    <= sig_mem_cke;
         sig_mem_cs_n_in   <= sig_mem_cs_n;
         sig_mem_dm_in     <= sig_mem_dm;

         if out_trans.sig_mem_dq_oe = '1' then
            sig_mem_dq        <= out_trans.sig_mem_dq_out after 1 ps;
            sig_mem_dq_in     <= (others => 'Z');
         else
            sig_mem_dq        <= (others => 'Z');
            sig_mem_dq_in     <= sig_mem_dq;
         end if;

         if out_trans.sig_mem_dqs_oe = '1' then
            sig_mem_dqs       <= out_trans.sig_mem_dqs_out after 1 ps;
            sig_mem_dqs_in    <= (others => 'Z');
         else
            sig_mem_dqs       <= (others => 'Z');
            sig_mem_dqs_in    <= sig_mem_dqs;
         end if;

         if out_trans.sig_mem_dqs_n_oe = '1' then
            sig_mem_dqs_n     <= out_trans.sig_mem_dqs_n_out after 1 ps;
            sig_mem_dqs_n_in  <= (others => 'Z');
         else
            sig_mem_dqs_n     <= (others => 'Z');
            sig_mem_dqs_n_in  <= sig_mem_dqs_n;
         end if;
         sig_mem_odt_in    <= sig_mem_odt;
         sig_mem_ras_n_in  <= sig_mem_ras_n;
         sig_mem_reset_n_in <= sig_mem_reset_n;
         sig_mem_we_n_in   <= sig_mem_we_n;
         sig_oct_rzqin     <= out_trans.sig_oct_rzqin_out after 1 ps;
      end process;

end altera_conduit_bfm_arch;

