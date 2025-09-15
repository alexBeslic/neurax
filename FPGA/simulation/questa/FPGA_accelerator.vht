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
-- Generated on "09/15/2025 18:47:33"
                                                            
-- Vhdl Test Bench template for design  :  FPGA_accelerator
-- 
-- Simulation tool : Questa Intel FPGA (VHDL)
-- 

LIBRARY ieee;                                               
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

ENTITY FPGA_accelerator_vhd_tst IS
END FPGA_accelerator_vhd_tst;
ARCHITECTURE FPGA_accelerator_arch OF FPGA_accelerator_vhd_tst IS
-- constants                                                 
SIGNAL clk : STD_LOGIC;
SIGNAL enable : STD_LOGIC;
SIGNAL out_valid : STD_LOGIC;
SIGNAL pixel_in : STD_LOGIC_VECTOR(7 DOWNTO 0);
SIGNAL pixel_valid : STD_LOGIC;
SIGNAL result_out : STD_LOGIC_VECTOR(15 DOWNTO 0);
SIGNAL rst : STD_LOGIC;
COMPONENT FPGA_accelerator
	PORT (
	clk : IN STD_LOGIC;
	enable : IN STD_LOGIC;
	out_valid : BUFFER STD_LOGIC;
	pixel_in : IN STD_LOGIC_VECTOR(7 DOWNTO 0);
	pixel_valid : IN STD_LOGIC;
	result_out : BUFFER STD_LOGIC_VECTOR(15 DOWNTO 0);
	rst : IN STD_LOGIC
	);
END COMPONENT;
BEGIN
	i1 : FPGA_accelerator
		PORT MAP (
			clk => clk,
			enable => enable,
			out_valid => out_valid,
			pixel_in => pixel_in,
			pixel_valid => pixel_valid,
			result_out => result_out,
			rst => rst
		);
	-- Clock generation
	clk_process : process
	begin
		clk <= '0';
		wait for 10 ns;
		clk <= '1';
		wait for 10 ns;
	end process;

	-- Stimulus process
	stim_proc: process
		use std.textio.all;
		type int_array is array (0 to 15) of integer;
		variable img : int_array;
		file img_file : text open read_mode is "image.txt";
		file out_file : text open write_mode is "output.txt";
		variable linebuf : line;
		variable val : integer;
		variable result_count : integer;
	begin
		-- Read image from file
		for i in 0 to 15 loop
			if endfile(img_file) then
        report "Not enough pixel values in image.txt at index " & integer'image(i) severity error;
		  end if;
		  readline(img_file, linebuf);
		  read(linebuf, val);
		  img(i) := val;
		end loop;

		rst <= '1';
		enable <= '0';
		pixel_valid <= '0';
		wait for 25 ns;
		rst <= '0';
		enable <= '1';
		-- Feed image pixels
		for i in 0 to 15 loop
		pixel_in <= std_logic_vector(to_unsigned(img(i), pixel_in'length));
			pixel_valid <= '1';
			wait for 20 ns;
		end loop;
		pixel_valid <= '0';
		-- Wait for and print all 4 convolution results
		result_count := 0;
		while result_count < 4 loop
			wait for 20 ns;
			if out_valid = '1' then
				report "Convolution result " & integer'image(result_count+1) & ": " & integer'image(to_integer(signed(result_out)));
				linebuf := null;
				write(linebuf, to_integer(signed(result_out)));
				writeline(out_file, linebuf);
				result_count := result_count + 1;
			end if;
		end loop;
		-- Simulation end
      report "Simulation complete" severity note;
      wait for 50 ns;
      std.env.stop;  -- Stop simulation cleanly
end process;
END FPGA_accelerator_arch;
