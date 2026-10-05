-- Copyright (C) 2025  Altera Corporation. All rights reserved.
-- Your use of Altera Corporation's design tools, logic functions 
-- and other software and tools, and any partner logic 
-- functions, and any output files from any of the foregoing 
-- (including device programming or simulation files), and any 
-- associated documentation or information are expressly subject 
-- to the terms and conditions of the Altera Program License 
-- Subscription Agreement, the Altera Quartus Prime License Agreement,
-- the Altera IP License Agreement, or other applicable license
-- agreement, including, without limitation, that your use is for
-- the sole purpose of programming logic devices manufactured by
-- Altera and sold by Altera or its authorized distributors.  Please
-- refer to the Altera Software License Subscription Agreements 
-- on the Quartus Prime software download page.

-- ***************************************************************************
-- This file contains a Vhdl test bench template that is freely editable to   
-- suit user's needs .Comments are provided in each section to help the user  
-- fill out necessary details.                                                
-- ***************************************************************************
-- Generated on "09/28/2026 19:06:21"
                                                            
-- Vhdl Test Bench template for design  :  de1_soc_top
-- 
-- Simulation tool : Questa Intel FPGA (VHDL)
-- 

LIBRARY ieee;                                               
USE ieee.std_logic_1164.all;                                

ENTITY de1_soc_top_vhd_tst IS
END de1_soc_top_vhd_tst;
ARCHITECTURE de1_soc_top_arch OF de1_soc_top_vhd_tst IS
-- constants                                                 
-- signals                                                   
SIGNAL CLOCK_50 : STD_LOGIC;
SIGNAL HPS_DDR3_ADDR : STD_LOGIC_VECTOR(14 DOWNTO 0);
SIGNAL HPS_DDR3_BA : STD_LOGIC_VECTOR(2 DOWNTO 0);
SIGNAL HPS_DDR3_CAS_N : STD_LOGIC;
SIGNAL HPS_DDR3_CK_N : STD_LOGIC;
SIGNAL HPS_DDR3_CK_P : STD_LOGIC;
SIGNAL HPS_DDR3_CKE : STD_LOGIC;
SIGNAL HPS_DDR3_CS_N : STD_LOGIC;
SIGNAL HPS_DDR3_DM : STD_LOGIC_VECTOR(3 DOWNTO 0);
SIGNAL HPS_DDR3_DQ : STD_LOGIC_VECTOR(31 DOWNTO 0);
SIGNAL HPS_DDR3_DQS_N : STD_LOGIC_VECTOR(3 DOWNTO 0);
SIGNAL HPS_DDR3_DQS_P : STD_LOGIC_VECTOR(3 DOWNTO 0);
SIGNAL HPS_DDR3_ODT : STD_LOGIC;
SIGNAL HPS_DDR3_RAS_N : STD_LOGIC;
SIGNAL HPS_DDR3_RESET_N : STD_LOGIC;
SIGNAL HPS_DDR3_RZQ : STD_LOGIC;
SIGNAL HPS_DDR3_WE_N : STD_LOGIC;
SIGNAL HPS_ENET_GTX_CLK : STD_LOGIC;
SIGNAL HPS_ENET_INT_N : STD_LOGIC;
SIGNAL HPS_ENET_MDC : STD_LOGIC;
SIGNAL HPS_ENET_MDIO : STD_LOGIC;
SIGNAL HPS_ENET_RX_CLK : STD_LOGIC;
SIGNAL HPS_ENET_RX_DATA : STD_LOGIC_VECTOR(3 DOWNTO 0);
SIGNAL HPS_ENET_RX_DV : STD_LOGIC;
SIGNAL HPS_ENET_TX_DATA : STD_LOGIC_VECTOR(3 DOWNTO 0);
SIGNAL HPS_ENET_TX_EN : STD_LOGIC;
SIGNAL HPS_SD_CLK : STD_LOGIC;
SIGNAL HPS_SD_CMD : STD_LOGIC;
SIGNAL HPS_SD_DATA : STD_LOGIC_VECTOR(3 DOWNTO 0);
SIGNAL HPS_UART_RX : STD_LOGIC;
SIGNAL HPS_UART_TX : STD_LOGIC;
COMPONENT de1_soc_top
	PORT (
	CLOCK_50 : IN STD_LOGIC;
	HPS_DDR3_ADDR : BUFFER STD_LOGIC_VECTOR(14 DOWNTO 0);
	HPS_DDR3_BA : BUFFER STD_LOGIC_VECTOR(2 DOWNTO 0);
	HPS_DDR3_CAS_N : BUFFER STD_LOGIC;
	HPS_DDR3_CK_N : BUFFER STD_LOGIC;
	HPS_DDR3_CK_P : BUFFER STD_LOGIC;
	HPS_DDR3_CKE : BUFFER STD_LOGIC;
	HPS_DDR3_CS_N : BUFFER STD_LOGIC;
	HPS_DDR3_DM : BUFFER STD_LOGIC_VECTOR(3 DOWNTO 0);
	HPS_DDR3_DQ : BUFFER STD_LOGIC_VECTOR(31 DOWNTO 0);
	HPS_DDR3_DQS_N : BUFFER STD_LOGIC_VECTOR(3 DOWNTO 0);
	HPS_DDR3_DQS_P : BUFFER STD_LOGIC_VECTOR(3 DOWNTO 0);
	HPS_DDR3_ODT : BUFFER STD_LOGIC;
	HPS_DDR3_RAS_N : BUFFER STD_LOGIC;
	HPS_DDR3_RESET_N : BUFFER STD_LOGIC;
	HPS_DDR3_RZQ : IN STD_LOGIC;
	HPS_DDR3_WE_N : BUFFER STD_LOGIC;
	HPS_ENET_GTX_CLK : BUFFER STD_LOGIC;
	HPS_ENET_INT_N : BUFFER STD_LOGIC;
	HPS_ENET_MDC : BUFFER STD_LOGIC;
	HPS_ENET_MDIO : BUFFER STD_LOGIC;
	HPS_ENET_RX_CLK : IN STD_LOGIC;
	HPS_ENET_RX_DATA : IN STD_LOGIC_VECTOR(3 DOWNTO 0);
	HPS_ENET_RX_DV : IN STD_LOGIC;
	HPS_ENET_TX_DATA : BUFFER STD_LOGIC_VECTOR(3 DOWNTO 0);
	HPS_ENET_TX_EN : BUFFER STD_LOGIC;
	HPS_SD_CLK : BUFFER STD_LOGIC;
	HPS_SD_CMD : BUFFER STD_LOGIC;
	HPS_SD_DATA : BUFFER STD_LOGIC_VECTOR(3 DOWNTO 0);
	HPS_UART_RX : IN STD_LOGIC;
	HPS_UART_TX : BUFFER STD_LOGIC
	);
END COMPONENT;
BEGIN
	i1 : de1_soc_top
	PORT MAP (
-- list connections between master ports and signals
	CLOCK_50 => CLOCK_50,
	HPS_DDR3_ADDR => HPS_DDR3_ADDR,
	HPS_DDR3_BA => HPS_DDR3_BA,
	HPS_DDR3_CAS_N => HPS_DDR3_CAS_N,
	HPS_DDR3_CK_N => HPS_DDR3_CK_N,
	HPS_DDR3_CK_P => HPS_DDR3_CK_P,
	HPS_DDR3_CKE => HPS_DDR3_CKE,
	HPS_DDR3_CS_N => HPS_DDR3_CS_N,
	HPS_DDR3_DM => HPS_DDR3_DM,
	HPS_DDR3_DQ => HPS_DDR3_DQ,
	HPS_DDR3_DQS_N => HPS_DDR3_DQS_N,
	HPS_DDR3_DQS_P => HPS_DDR3_DQS_P,
	HPS_DDR3_ODT => HPS_DDR3_ODT,
	HPS_DDR3_RAS_N => HPS_DDR3_RAS_N,
	HPS_DDR3_RESET_N => HPS_DDR3_RESET_N,
	HPS_DDR3_RZQ => HPS_DDR3_RZQ,
	HPS_DDR3_WE_N => HPS_DDR3_WE_N,
	HPS_ENET_GTX_CLK => HPS_ENET_GTX_CLK,
	HPS_ENET_INT_N => HPS_ENET_INT_N,
	HPS_ENET_MDC => HPS_ENET_MDC,
	HPS_ENET_MDIO => HPS_ENET_MDIO,
	HPS_ENET_RX_CLK => HPS_ENET_RX_CLK,
	HPS_ENET_RX_DATA => HPS_ENET_RX_DATA,
	HPS_ENET_RX_DV => HPS_ENET_RX_DV,
	HPS_ENET_TX_DATA => HPS_ENET_TX_DATA,
	HPS_ENET_TX_EN => HPS_ENET_TX_EN,
	HPS_SD_CLK => HPS_SD_CLK,
	HPS_SD_CMD => HPS_SD_CMD,
	HPS_SD_DATA => HPS_SD_DATA,
	HPS_UART_RX => HPS_UART_RX,
	HPS_UART_TX => HPS_UART_TX
	);
init : PROCESS                                               
-- variable declarations                                     
BEGIN                                                        
        -- code that executes only once                      
WAIT;                                                       
END PROCESS init;                                           
always : PROCESS                                              
-- optional sensitivity list                                  
-- (        )                                                 
-- variable declarations                                      
BEGIN                                                         
        -- code executes for every event on sensitivity list  
WAIT;                                                        
END PROCESS always;                                          
END de1_soc_top_arch;
