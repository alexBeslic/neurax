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

-- VENDOR "Altera"
-- PROGRAM "Quartus Prime"
-- VERSION "Version 24.1std.0 Build 1077 03/04/2025 SC Lite Edition"

-- DATE "09/15/2025 18:37:07"

-- 
-- Device: Altera 5CSEMA5F31C6 Package FBGA896
-- 

-- 
-- This VHDL file should be used for Questa Intel FPGA (VHDL) only
-- 

LIBRARY ALTERA;
LIBRARY ALTERA_LNSIM;
LIBRARY CYCLONEV;
LIBRARY IEEE;
USE ALTERA.ALTERA_PRIMITIVES_COMPONENTS.ALL;
USE ALTERA_LNSIM.ALTERA_LNSIM_COMPONENTS.ALL;
USE CYCLONEV.CYCLONEV_COMPONENTS.ALL;
USE IEEE.STD_LOGIC_1164.ALL;

ENTITY 	FPGA_accelerator IS
    PORT (
	clk : IN std_logic;
	rst : IN std_logic;
	enable : IN std_logic;
	pixel_in : IN std_logic_vector(7 DOWNTO 0);
	pixel_valid : IN std_logic;
	param_a : IN std_logic_vector(7 DOWNTO 0);
	param_b : IN std_logic_vector(7 DOWNTO 0);
	result_out : BUFFER std_logic_vector(15 DOWNTO 0);
	out_valid : BUFFER std_logic
	);
END FPGA_accelerator;

-- Design Ports Information
-- param_a[0]	=>  Location: PIN_AG8,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_a[1]	=>  Location: PIN_AC30,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_a[2]	=>  Location: PIN_D6,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_a[3]	=>  Location: PIN_AA15,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_a[4]	=>  Location: PIN_AF20,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_a[5]	=>  Location: PIN_K8,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_a[6]	=>  Location: PIN_AF24,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_a[7]	=>  Location: PIN_AJ6,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_b[0]	=>  Location: PIN_AD30,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_b[1]	=>  Location: PIN_AA21,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_b[2]	=>  Location: PIN_D7,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_b[3]	=>  Location: PIN_AF6,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_b[4]	=>  Location: PIN_AG23,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_b[5]	=>  Location: PIN_C5,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_b[6]	=>  Location: PIN_AH5,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- param_b[7]	=>  Location: PIN_AJ14,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[0]	=>  Location: PIN_AG26,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[1]	=>  Location: PIN_W19,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[2]	=>  Location: PIN_Y21,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[3]	=>  Location: PIN_Y19,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[4]	=>  Location: PIN_AA20,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[5]	=>  Location: PIN_V18,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[6]	=>  Location: PIN_AK29,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[7]	=>  Location: PIN_AJ27,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[8]	=>  Location: PIN_AD20,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[9]	=>  Location: PIN_AE26,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[10]	=>  Location: PIN_AD21,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[11]	=>  Location: PIN_AH27,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[12]	=>  Location: PIN_AH29,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[13]	=>  Location: PIN_AK27,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[14]	=>  Location: PIN_AK28,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- result_out[15]	=>  Location: PIN_AJ29,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- out_valid	=>  Location: PIN_AC25,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- clk	=>  Location: PIN_AB27,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- rst	=>  Location: PIN_AD25,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- enable	=>  Location: PIN_AH28,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- pixel_valid	=>  Location: PIN_AG27,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- pixel_in[0]	=>  Location: PIN_W20,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- pixel_in[1]	=>  Location: PIN_AF26,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- pixel_in[2]	=>  Location: PIN_AF25,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- pixel_in[3]	=>  Location: PIN_W22,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- pixel_in[4]	=>  Location: PIN_AB21,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- pixel_in[5]	=>  Location: PIN_AC23,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- pixel_in[6]	=>  Location: PIN_AD27,	 I/O Standard: 2.5 V,	 Current Strength: Default
-- pixel_in[7]	=>  Location: PIN_AC22,	 I/O Standard: 2.5 V,	 Current Strength: Default


ARCHITECTURE structure OF FPGA_accelerator IS
SIGNAL gnd : std_logic := '0';
SIGNAL vcc : std_logic := '1';
SIGNAL unknown : std_logic := 'X';
SIGNAL devoe : std_logic := '1';
SIGNAL devclrn : std_logic := '1';
SIGNAL devpor : std_logic := '1';
SIGNAL ww_devoe : std_logic;
SIGNAL ww_devclrn : std_logic;
SIGNAL ww_devpor : std_logic;
SIGNAL ww_clk : std_logic;
SIGNAL ww_rst : std_logic;
SIGNAL ww_enable : std_logic;
SIGNAL ww_pixel_in : std_logic_vector(7 DOWNTO 0);
SIGNAL ww_pixel_valid : std_logic;
SIGNAL ww_param_a : std_logic_vector(7 DOWNTO 0);
SIGNAL ww_param_b : std_logic_vector(7 DOWNTO 0);
SIGNAL ww_result_out : std_logic_vector(15 DOWNTO 0);
SIGNAL ww_out_valid : std_logic;
SIGNAL \param_a[0]~input_o\ : std_logic;
SIGNAL \param_a[1]~input_o\ : std_logic;
SIGNAL \param_a[2]~input_o\ : std_logic;
SIGNAL \param_a[3]~input_o\ : std_logic;
SIGNAL \param_a[4]~input_o\ : std_logic;
SIGNAL \param_a[5]~input_o\ : std_logic;
SIGNAL \param_a[6]~input_o\ : std_logic;
SIGNAL \param_a[7]~input_o\ : std_logic;
SIGNAL \param_b[0]~input_o\ : std_logic;
SIGNAL \param_b[1]~input_o\ : std_logic;
SIGNAL \param_b[2]~input_o\ : std_logic;
SIGNAL \param_b[3]~input_o\ : std_logic;
SIGNAL \param_b[4]~input_o\ : std_logic;
SIGNAL \param_b[5]~input_o\ : std_logic;
SIGNAL \param_b[6]~input_o\ : std_logic;
SIGNAL \param_b[7]~input_o\ : std_logic;
SIGNAL \~QUARTUS_CREATED_GND~I_combout\ : std_logic;
SIGNAL \clk~input_o\ : std_logic;
SIGNAL \clk~inputCLKENA0_outclk\ : std_logic;
SIGNAL \pixel_in[0]~input_o\ : std_logic;
SIGNAL \enable~input_o\ : std_logic;
SIGNAL \pixel_valid~input_o\ : std_logic;
SIGNAL \rst~input_o\ : std_logic;
SIGNAL \conv_inst|col[0]~1_combout\ : std_logic;
SIGNAL \conv_inst|col[1]~0_combout\ : std_logic;
SIGNAL \conv_inst|row[0]~1_combout\ : std_logic;
SIGNAL \conv_inst|row[1]~0_combout\ : std_logic;
SIGNAL \conv_inst|image[3][3][0]~0_combout\ : std_logic;
SIGNAL \conv_inst|image[3][3][0]~1_combout\ : std_logic;
SIGNAL \conv_inst|image[3][3][0]~q\ : std_logic;
SIGNAL \conv_inst|Add2~1_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~1_sumout\ : std_logic;
SIGNAL \conv_inst|process_0~0_combout\ : std_logic;
SIGNAL \pixel_in[1]~input_o\ : std_logic;
SIGNAL \conv_inst|image[3][3][1]~feeder_combout\ : std_logic;
SIGNAL \conv_inst|image[3][3][1]~q\ : std_logic;
SIGNAL \conv_inst|Add2~2\ : std_logic;
SIGNAL \conv_inst|Add2~5_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~2\ : std_logic;
SIGNAL \conv_inst|Add3~5_sumout\ : std_logic;
SIGNAL \pixel_in[2]~input_o\ : std_logic;
SIGNAL \conv_inst|image[3][3][2]~q\ : std_logic;
SIGNAL \conv_inst|Add2~6\ : std_logic;
SIGNAL \conv_inst|Add2~9_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~6\ : std_logic;
SIGNAL \conv_inst|Add3~9_sumout\ : std_logic;
SIGNAL \pixel_in[3]~input_o\ : std_logic;
SIGNAL \conv_inst|image[3][3][3]~feeder_combout\ : std_logic;
SIGNAL \conv_inst|image[3][3][3]~q\ : std_logic;
SIGNAL \conv_inst|Add2~10\ : std_logic;
SIGNAL \conv_inst|Add2~13_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~10\ : std_logic;
SIGNAL \conv_inst|Add3~13_sumout\ : std_logic;
SIGNAL \pixel_in[4]~input_o\ : std_logic;
SIGNAL \conv_inst|image[3][3][4]~feeder_combout\ : std_logic;
SIGNAL \conv_inst|image[3][3][4]~q\ : std_logic;
SIGNAL \conv_inst|Add2~14\ : std_logic;
SIGNAL \conv_inst|Add2~17_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~14\ : std_logic;
SIGNAL \conv_inst|Add3~17_sumout\ : std_logic;
SIGNAL \conv_inst|conv_out[4]~feeder_combout\ : std_logic;
SIGNAL \pixel_in[5]~input_o\ : std_logic;
SIGNAL \conv_inst|image[3][3][5]~feeder_combout\ : std_logic;
SIGNAL \conv_inst|image[3][3][5]~q\ : std_logic;
SIGNAL \conv_inst|Add2~18\ : std_logic;
SIGNAL \conv_inst|Add2~21_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~18\ : std_logic;
SIGNAL \conv_inst|Add3~21_sumout\ : std_logic;
SIGNAL \pixel_in[6]~input_o\ : std_logic;
SIGNAL \conv_inst|image[3][3][6]~feeder_combout\ : std_logic;
SIGNAL \conv_inst|image[3][3][6]~q\ : std_logic;
SIGNAL \conv_inst|Add2~22\ : std_logic;
SIGNAL \conv_inst|Add2~25_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~22\ : std_logic;
SIGNAL \conv_inst|Add3~25_sumout\ : std_logic;
SIGNAL \conv_inst|conv_out[6]~feeder_combout\ : std_logic;
SIGNAL \pixel_in[7]~input_o\ : std_logic;
SIGNAL \conv_inst|image[3][3][7]~feeder_combout\ : std_logic;
SIGNAL \conv_inst|image[3][3][7]~q\ : std_logic;
SIGNAL \conv_inst|Add2~26\ : std_logic;
SIGNAL \conv_inst|Add2~29_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~26\ : std_logic;
SIGNAL \conv_inst|Add3~29_sumout\ : std_logic;
SIGNAL \conv_inst|Add2~30\ : std_logic;
SIGNAL \conv_inst|Add2~33_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~30\ : std_logic;
SIGNAL \conv_inst|Add3~33_sumout\ : std_logic;
SIGNAL \conv_inst|conv_out[8]~feeder_combout\ : std_logic;
SIGNAL \conv_inst|Add3~34\ : std_logic;
SIGNAL \conv_inst|Add3~37_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~38\ : std_logic;
SIGNAL \conv_inst|Add3~41_sumout\ : std_logic;
SIGNAL \conv_inst|conv_out[10]~feeder_combout\ : std_logic;
SIGNAL \conv_inst|Add3~42\ : std_logic;
SIGNAL \conv_inst|Add3~45_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~46\ : std_logic;
SIGNAL \conv_inst|Add3~49_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~50\ : std_logic;
SIGNAL \conv_inst|Add3~53_sumout\ : std_logic;
SIGNAL \conv_inst|conv_out[13]~feeder_combout\ : std_logic;
SIGNAL \conv_inst|Add3~54\ : std_logic;
SIGNAL \conv_inst|Add3~57_sumout\ : std_logic;
SIGNAL \conv_inst|Add3~58\ : std_logic;
SIGNAL \conv_inst|Add3~61_sumout\ : std_logic;
SIGNAL \conv_inst|out_valid~q\ : std_logic;
SIGNAL \conv_inst|conv_out\ : std_logic_vector(15 DOWNTO 0);
SIGNAL \conv_inst|conv_result\ : std_logic_vector(31 DOWNTO 0);
SIGNAL \conv_inst|row\ : std_logic_vector(1 DOWNTO 0);
SIGNAL \conv_inst|col\ : std_logic_vector(1 DOWNTO 0);
SIGNAL \ALT_INV_pixel_in[7]~input_o\ : std_logic;
SIGNAL \ALT_INV_pixel_in[6]~input_o\ : std_logic;
SIGNAL \ALT_INV_pixel_in[5]~input_o\ : std_logic;
SIGNAL \ALT_INV_pixel_in[4]~input_o\ : std_logic;
SIGNAL \ALT_INV_pixel_in[3]~input_o\ : std_logic;
SIGNAL \ALT_INV_pixel_in[1]~input_o\ : std_logic;
SIGNAL \ALT_INV_pixel_valid~input_o\ : std_logic;
SIGNAL \ALT_INV_enable~input_o\ : std_logic;
SIGNAL \ALT_INV_rst~input_o\ : std_logic;
SIGNAL \conv_inst|ALT_INV_image[3][3][0]~0_combout\ : std_logic;
SIGNAL \conv_inst|ALT_INV_image[3][3][7]~q\ : std_logic;
SIGNAL \conv_inst|ALT_INV_image[3][3][6]~q\ : std_logic;
SIGNAL \conv_inst|ALT_INV_image[3][3][5]~q\ : std_logic;
SIGNAL \conv_inst|ALT_INV_image[3][3][4]~q\ : std_logic;
SIGNAL \conv_inst|ALT_INV_image[3][3][3]~q\ : std_logic;
SIGNAL \conv_inst|ALT_INV_image[3][3][2]~q\ : std_logic;
SIGNAL \conv_inst|ALT_INV_image[3][3][1]~q\ : std_logic;
SIGNAL \conv_inst|ALT_INV_image[3][3][0]~q\ : std_logic;
SIGNAL \conv_inst|ALT_INV_conv_result\ : std_logic_vector(15 DOWNTO 0);
SIGNAL \conv_inst|ALT_INV_col\ : std_logic_vector(1 DOWNTO 0);
SIGNAL \conv_inst|ALT_INV_row\ : std_logic_vector(1 DOWNTO 0);
SIGNAL \conv_inst|ALT_INV_Add2~33_sumout\ : std_logic;
SIGNAL \conv_inst|ALT_INV_Add2~29_sumout\ : std_logic;
SIGNAL \conv_inst|ALT_INV_Add2~25_sumout\ : std_logic;
SIGNAL \conv_inst|ALT_INV_Add2~21_sumout\ : std_logic;
SIGNAL \conv_inst|ALT_INV_Add2~17_sumout\ : std_logic;
SIGNAL \conv_inst|ALT_INV_Add2~13_sumout\ : std_logic;
SIGNAL \conv_inst|ALT_INV_Add2~9_sumout\ : std_logic;
SIGNAL \conv_inst|ALT_INV_Add2~5_sumout\ : std_logic;
SIGNAL \conv_inst|ALT_INV_Add2~1_sumout\ : std_logic;

BEGIN

ww_clk <= clk;
ww_rst <= rst;
ww_enable <= enable;
ww_pixel_in <= pixel_in;
ww_pixel_valid <= pixel_valid;
ww_param_a <= param_a;
ww_param_b <= param_b;
result_out <= ww_result_out;
out_valid <= ww_out_valid;
ww_devoe <= devoe;
ww_devclrn <= devclrn;
ww_devpor <= devpor;
\ALT_INV_pixel_in[7]~input_o\ <= NOT \pixel_in[7]~input_o\;
\ALT_INV_pixel_in[6]~input_o\ <= NOT \pixel_in[6]~input_o\;
\ALT_INV_pixel_in[5]~input_o\ <= NOT \pixel_in[5]~input_o\;
\ALT_INV_pixel_in[4]~input_o\ <= NOT \pixel_in[4]~input_o\;
\ALT_INV_pixel_in[3]~input_o\ <= NOT \pixel_in[3]~input_o\;
\ALT_INV_pixel_in[1]~input_o\ <= NOT \pixel_in[1]~input_o\;
\ALT_INV_pixel_valid~input_o\ <= NOT \pixel_valid~input_o\;
\ALT_INV_enable~input_o\ <= NOT \enable~input_o\;
\ALT_INV_rst~input_o\ <= NOT \rst~input_o\;
\conv_inst|ALT_INV_image[3][3][0]~0_combout\ <= NOT \conv_inst|image[3][3][0]~0_combout\;
\conv_inst|ALT_INV_image[3][3][7]~q\ <= NOT \conv_inst|image[3][3][7]~q\;
\conv_inst|ALT_INV_image[3][3][6]~q\ <= NOT \conv_inst|image[3][3][6]~q\;
\conv_inst|ALT_INV_image[3][3][5]~q\ <= NOT \conv_inst|image[3][3][5]~q\;
\conv_inst|ALT_INV_image[3][3][4]~q\ <= NOT \conv_inst|image[3][3][4]~q\;
\conv_inst|ALT_INV_image[3][3][3]~q\ <= NOT \conv_inst|image[3][3][3]~q\;
\conv_inst|ALT_INV_image[3][3][2]~q\ <= NOT \conv_inst|image[3][3][2]~q\;
\conv_inst|ALT_INV_image[3][3][1]~q\ <= NOT \conv_inst|image[3][3][1]~q\;
\conv_inst|ALT_INV_image[3][3][0]~q\ <= NOT \conv_inst|image[3][3][0]~q\;
\conv_inst|ALT_INV_conv_result\(15) <= NOT \conv_inst|conv_result\(15);
\conv_inst|ALT_INV_conv_result\(14) <= NOT \conv_inst|conv_result\(14);
\conv_inst|ALT_INV_conv_result\(13) <= NOT \conv_inst|conv_result\(13);
\conv_inst|ALT_INV_conv_result\(12) <= NOT \conv_inst|conv_result\(12);
\conv_inst|ALT_INV_conv_result\(11) <= NOT \conv_inst|conv_result\(11);
\conv_inst|ALT_INV_conv_result\(10) <= NOT \conv_inst|conv_result\(10);
\conv_inst|ALT_INV_conv_result\(9) <= NOT \conv_inst|conv_result\(9);
\conv_inst|ALT_INV_conv_result\(8) <= NOT \conv_inst|conv_result\(8);
\conv_inst|ALT_INV_conv_result\(7) <= NOT \conv_inst|conv_result\(7);
\conv_inst|ALT_INV_conv_result\(6) <= NOT \conv_inst|conv_result\(6);
\conv_inst|ALT_INV_conv_result\(5) <= NOT \conv_inst|conv_result\(5);
\conv_inst|ALT_INV_conv_result\(4) <= NOT \conv_inst|conv_result\(4);
\conv_inst|ALT_INV_conv_result\(3) <= NOT \conv_inst|conv_result\(3);
\conv_inst|ALT_INV_conv_result\(2) <= NOT \conv_inst|conv_result\(2);
\conv_inst|ALT_INV_conv_result\(1) <= NOT \conv_inst|conv_result\(1);
\conv_inst|ALT_INV_col\(0) <= NOT \conv_inst|col\(0);
\conv_inst|ALT_INV_col\(1) <= NOT \conv_inst|col\(1);
\conv_inst|ALT_INV_row\(0) <= NOT \conv_inst|row\(0);
\conv_inst|ALT_INV_row\(1) <= NOT \conv_inst|row\(1);
\conv_inst|ALT_INV_conv_result\(0) <= NOT \conv_inst|conv_result\(0);
\conv_inst|ALT_INV_Add2~33_sumout\ <= NOT \conv_inst|Add2~33_sumout\;
\conv_inst|ALT_INV_Add2~29_sumout\ <= NOT \conv_inst|Add2~29_sumout\;
\conv_inst|ALT_INV_Add2~25_sumout\ <= NOT \conv_inst|Add2~25_sumout\;
\conv_inst|ALT_INV_Add2~21_sumout\ <= NOT \conv_inst|Add2~21_sumout\;
\conv_inst|ALT_INV_Add2~17_sumout\ <= NOT \conv_inst|Add2~17_sumout\;
\conv_inst|ALT_INV_Add2~13_sumout\ <= NOT \conv_inst|Add2~13_sumout\;
\conv_inst|ALT_INV_Add2~9_sumout\ <= NOT \conv_inst|Add2~9_sumout\;
\conv_inst|ALT_INV_Add2~5_sumout\ <= NOT \conv_inst|Add2~5_sumout\;
\conv_inst|ALT_INV_Add2~1_sumout\ <= NOT \conv_inst|Add2~1_sumout\;

-- Location: IOOBUF_X84_Y0_N36
\result_out[0]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(0),
	devoe => ww_devoe,
	o => ww_result_out(0));

-- Location: IOOBUF_X80_Y0_N19
\result_out[1]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(1),
	devoe => ww_devoe,
	o => ww_result_out(1));

-- Location: IOOBUF_X89_Y6_N22
\result_out[2]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(2),
	devoe => ww_devoe,
	o => ww_result_out(2));

-- Location: IOOBUF_X84_Y0_N2
\result_out[3]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(3),
	devoe => ww_devoe,
	o => ww_result_out(3));

-- Location: IOOBUF_X84_Y0_N19
\result_out[4]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(4),
	devoe => ww_devoe,
	o => ww_result_out(4));

-- Location: IOOBUF_X80_Y0_N2
\result_out[5]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(5),
	devoe => ww_devoe,
	o => ww_result_out(5));

-- Location: IOOBUF_X82_Y0_N93
\result_out[6]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(6),
	devoe => ww_devoe,
	o => ww_result_out(6));

-- Location: IOOBUF_X80_Y0_N36
\result_out[7]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(7),
	devoe => ww_devoe,
	o => ww_result_out(7));

-- Location: IOOBUF_X82_Y0_N42
\result_out[8]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(8),
	devoe => ww_devoe,
	o => ww_result_out(8));

-- Location: IOOBUF_X89_Y8_N39
\result_out[9]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(9),
	devoe => ww_devoe,
	o => ww_result_out(9));

-- Location: IOOBUF_X82_Y0_N59
\result_out[10]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(10),
	devoe => ww_devoe,
	o => ww_result_out(10));

-- Location: IOOBUF_X84_Y0_N53
\result_out[11]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(11),
	devoe => ww_devoe,
	o => ww_result_out(11));

-- Location: IOOBUF_X89_Y6_N56
\result_out[12]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(12),
	devoe => ww_devoe,
	o => ww_result_out(12));

-- Location: IOOBUF_X80_Y0_N53
\result_out[13]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(13),
	devoe => ww_devoe,
	o => ww_result_out(13));

-- Location: IOOBUF_X82_Y0_N76
\result_out[14]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(14),
	devoe => ww_devoe,
	o => ww_result_out(14));

-- Location: IOOBUF_X89_Y6_N39
\result_out[15]~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|conv_out\(15),
	devoe => ww_devoe,
	o => ww_result_out(15));

-- Location: IOOBUF_X89_Y4_N62
\out_valid~output\ : cyclonev_io_obuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	open_drain_output => "false",
	shift_series_termination_control => "false")
-- pragma translate_on
PORT MAP (
	i => \conv_inst|out_valid~q\,
	devoe => ww_devoe,
	o => ww_out_valid);

-- Location: IOIBUF_X89_Y23_N21
\clk~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_clk,
	o => \clk~input_o\);

-- Location: CLKCTRL_G10
\clk~inputCLKENA0\ : cyclonev_clkena
-- pragma translate_off
GENERIC MAP (
	clock_type => "global clock",
	disable_mode => "low",
	ena_register_mode => "always enabled",
	ena_register_power_up => "high",
	test_syn => "high")
-- pragma translate_on
PORT MAP (
	inclk => \clk~input_o\,
	outclk => \clk~inputCLKENA0_outclk\);

-- Location: IOIBUF_X89_Y6_N4
\pixel_in[0]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_pixel_in(0),
	o => \pixel_in[0]~input_o\);

-- Location: IOIBUF_X89_Y4_N95
\enable~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_enable,
	o => \enable~input_o\);

-- Location: IOIBUF_X89_Y4_N78
\pixel_valid~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_pixel_valid,
	o => \pixel_valid~input_o\);

-- Location: IOIBUF_X89_Y4_N44
\rst~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_rst,
	o => \rst~input_o\);

-- Location: MLABCELL_X87_Y4_N33
\conv_inst|col[0]~1\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|col[0]~1_combout\ = !\conv_inst|col\(0) $ (((!\enable~input_o\) # (!\pixel_valid~input_o\)))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000001111111100000000111111110000000011111111000000001111111100",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datab => \ALT_INV_enable~input_o\,
	datac => \ALT_INV_pixel_valid~input_o\,
	datad => \conv_inst|ALT_INV_col\(0),
	combout => \conv_inst|col[0]~1_combout\);

-- Location: FF_X87_Y4_N35
\conv_inst|col[0]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|col[0]~1_combout\,
	clrn => \ALT_INV_rst~input_o\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|col\(0));

-- Location: MLABCELL_X87_Y4_N39
\conv_inst|col[1]~0\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|col[1]~0_combout\ = ( \conv_inst|col\(1) & ( \conv_inst|col\(0) & ( (!\enable~input_o\) # (!\pixel_valid~input_o\) ) ) ) # ( !\conv_inst|col\(1) & ( \conv_inst|col\(0) & ( (\enable~input_o\ & \pixel_valid~input_o\) ) ) ) # ( \conv_inst|col\(1) 
-- & ( !\conv_inst|col\(0) ) )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111111111111111100000011000000111111110011111100",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datab => \ALT_INV_enable~input_o\,
	datac => \ALT_INV_pixel_valid~input_o\,
	datae => \conv_inst|ALT_INV_col\(1),
	dataf => \conv_inst|ALT_INV_col\(0),
	combout => \conv_inst|col[1]~0_combout\);

-- Location: FF_X87_Y4_N41
\conv_inst|col[1]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|col[1]~0_combout\,
	clrn => \ALT_INV_rst~input_o\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|col\(1));

-- Location: MLABCELL_X87_Y4_N51
\conv_inst|row[0]~1\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|row[0]~1_combout\ = ( \conv_inst|row\(0) & ( \conv_inst|col\(1) & ( (!\enable~input_o\) # ((!\pixel_valid~input_o\) # (!\conv_inst|col\(0))) ) ) ) # ( !\conv_inst|row\(0) & ( \conv_inst|col\(1) & ( (\enable~input_o\ & (\pixel_valid~input_o\ & 
-- \conv_inst|col\(0))) ) ) ) # ( \conv_inst|row\(0) & ( !\conv_inst|col\(1) ) )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111111111111111100000000000000111111111111111100",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datab => \ALT_INV_enable~input_o\,
	datac => \ALT_INV_pixel_valid~input_o\,
	datad => \conv_inst|ALT_INV_col\(0),
	datae => \conv_inst|ALT_INV_row\(0),
	dataf => \conv_inst|ALT_INV_col\(1),
	combout => \conv_inst|row[0]~1_combout\);

-- Location: FF_X87_Y4_N53
\conv_inst|row[0]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|row[0]~1_combout\,
	clrn => \ALT_INV_rst~input_o\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|row\(0));

-- Location: MLABCELL_X87_Y4_N6
\conv_inst|row[1]~0\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|row[1]~0_combout\ = ( \conv_inst|row\(1) & ( \conv_inst|col\(1) & ( (!\conv_inst|col\(0)) # ((!\enable~input_o\) # ((!\conv_inst|row\(0)) # (!\pixel_valid~input_o\))) ) ) ) # ( !\conv_inst|row\(1) & ( \conv_inst|col\(1) & ( (\conv_inst|col\(0) 
-- & (\enable~input_o\ & (\conv_inst|row\(0) & \pixel_valid~input_o\))) ) ) ) # ( \conv_inst|row\(1) & ( !\conv_inst|col\(1) ) )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111111111111111100000000000000011111111111111110",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataa => \conv_inst|ALT_INV_col\(0),
	datab => \ALT_INV_enable~input_o\,
	datac => \conv_inst|ALT_INV_row\(0),
	datad => \ALT_INV_pixel_valid~input_o\,
	datae => \conv_inst|ALT_INV_row\(1),
	dataf => \conv_inst|ALT_INV_col\(1),
	combout => \conv_inst|row[1]~0_combout\);

-- Location: FF_X87_Y4_N8
\conv_inst|row[1]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|row[1]~0_combout\,
	clrn => \ALT_INV_rst~input_o\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|row\(1));

-- Location: MLABCELL_X87_Y4_N24
\conv_inst|image[3][3][0]~0\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|image[3][3][0]~0_combout\ = ( \conv_inst|row\(1) & ( (\conv_inst|col\(0) & (\conv_inst|col\(1) & \conv_inst|row\(0))) ) )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000000000001000000010000000100000001",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataa => \conv_inst|ALT_INV_col\(0),
	datab => \conv_inst|ALT_INV_col\(1),
	datac => \conv_inst|ALT_INV_row\(0),
	dataf => \conv_inst|ALT_INV_row\(1),
	combout => \conv_inst|image[3][3][0]~0_combout\);

-- Location: MLABCELL_X87_Y4_N45
\conv_inst|image[3][3][0]~1\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|image[3][3][0]~1_combout\ = ( !\rst~input_o\ & ( \conv_inst|image[3][3][0]~0_combout\ & ( (\enable~input_o\ & \pixel_valid~input_o\) ) ) )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000000000011000000110000000000000000",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datab => \ALT_INV_enable~input_o\,
	datac => \ALT_INV_pixel_valid~input_o\,
	datae => \ALT_INV_rst~input_o\,
	dataf => \conv_inst|ALT_INV_image[3][3][0]~0_combout\,
	combout => \conv_inst|image[3][3][0]~1_combout\);

-- Location: FF_X85_Y4_N2
\conv_inst|image[3][3][0]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \pixel_in[0]~input_o\,
	sload => VCC,
	ena => \conv_inst|image[3][3][0]~1_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|image[3][3][0]~q\);

-- Location: LABCELL_X85_Y4_N30
\conv_inst|Add2~1\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add2~1_sumout\ = SUM(( !\conv_inst|image[3][3][0]~q\ ) + ( VCC ) + ( !VCC ))
-- \conv_inst|Add2~2\ = CARRY(( !\conv_inst|image[3][3][0]~q\ ) + ( VCC ) + ( !VCC ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000000000000000000001111111100000000",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datad => \conv_inst|ALT_INV_image[3][3][0]~q\,
	cin => GND,
	sumout => \conv_inst|Add2~1_sumout\,
	cout => \conv_inst|Add2~2\);

-- Location: MLABCELL_X84_Y4_N0
\conv_inst|Add3~1\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~1_sumout\ = SUM(( \conv_inst|conv_result\(0) ) + ( \conv_inst|Add2~1_sumout\ ) + ( !VCC ))
-- \conv_inst|Add3~2\ = CARRY(( \conv_inst|conv_result\(0) ) + ( \conv_inst|Add2~1_sumout\ ) + ( !VCC ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111100001111000000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_Add2~1_sumout\,
	datad => \conv_inst|ALT_INV_conv_result\(0),
	cin => GND,
	sumout => \conv_inst|Add3~1_sumout\,
	cout => \conv_inst|Add3~2\);

-- Location: MLABCELL_X87_Y4_N30
\conv_inst|process_0~0\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|process_0~0_combout\ = ( \conv_inst|row\(1) & ( (\conv_inst|col\(1) & (\enable~input_o\ & (\conv_inst|row\(0) & \conv_inst|col\(0)))) ) )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000000000000000000010000000000000001",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataa => \conv_inst|ALT_INV_col\(1),
	datab => \ALT_INV_enable~input_o\,
	datac => \conv_inst|ALT_INV_row\(0),
	datad => \conv_inst|ALT_INV_col\(0),
	dataf => \conv_inst|ALT_INV_row\(1),
	combout => \conv_inst|process_0~0_combout\);

-- Location: FF_X84_Y4_N2
\conv_inst|conv_result[0]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~1_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(0));

-- Location: FF_X83_Y4_N5
\conv_inst|conv_out[0]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \conv_inst|conv_result\(0),
	clrn => \ALT_INV_rst~input_o\,
	sload => VCC,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(0));

-- Location: IOIBUF_X86_Y0_N52
\pixel_in[1]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_pixel_in(1),
	o => \pixel_in[1]~input_o\);

-- Location: LABCELL_X85_Y4_N12
\conv_inst|image[3][3][1]~feeder\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|image[3][3][1]~feeder_combout\ = ( \pixel_in[1]~input_o\ )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000011111111111111111111111111111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataf => \ALT_INV_pixel_in[1]~input_o\,
	combout => \conv_inst|image[3][3][1]~feeder_combout\);

-- Location: FF_X85_Y4_N14
\conv_inst|image[3][3][1]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|image[3][3][1]~feeder_combout\,
	ena => \conv_inst|image[3][3][0]~1_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|image[3][3][1]~q\);

-- Location: LABCELL_X85_Y4_N33
\conv_inst|Add2~5\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add2~5_sumout\ = SUM(( !\conv_inst|image[3][3][1]~q\ ) + ( GND ) + ( \conv_inst|Add2~2\ ))
-- \conv_inst|Add2~6\ = CARRY(( !\conv_inst|image[3][3][1]~q\ ) + ( GND ) + ( \conv_inst|Add2~2\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111111111111111100000000000000001111000011110000",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_image[3][3][1]~q\,
	cin => \conv_inst|Add2~2\,
	sumout => \conv_inst|Add2~5_sumout\,
	cout => \conv_inst|Add2~6\);

-- Location: MLABCELL_X84_Y4_N3
\conv_inst|Add3~5\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~5_sumout\ = SUM(( \conv_inst|conv_result\(1) ) + ( \conv_inst|Add2~5_sumout\ ) + ( \conv_inst|Add3~2\ ))
-- \conv_inst|Add3~6\ = CARRY(( \conv_inst|conv_result\(1) ) + ( \conv_inst|Add2~5_sumout\ ) + ( \conv_inst|Add3~2\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000101010101010101000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataa => \conv_inst|ALT_INV_Add2~5_sumout\,
	datad => \conv_inst|ALT_INV_conv_result\(1),
	cin => \conv_inst|Add3~2\,
	sumout => \conv_inst|Add3~5_sumout\,
	cout => \conv_inst|Add3~6\);

-- Location: FF_X84_Y4_N5
\conv_inst|conv_result[1]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~5_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(1));

-- Location: FF_X83_Y4_N32
\conv_inst|conv_out[1]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \conv_inst|conv_result\(1),
	clrn => \ALT_INV_rst~input_o\,
	sload => VCC,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(1));

-- Location: IOIBUF_X86_Y0_N35
\pixel_in[2]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_pixel_in(2),
	o => \pixel_in[2]~input_o\);

-- Location: FF_X85_Y4_N23
\conv_inst|image[3][3][2]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \pixel_in[2]~input_o\,
	sload => VCC,
	ena => \conv_inst|image[3][3][0]~1_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|image[3][3][2]~q\);

-- Location: LABCELL_X85_Y4_N36
\conv_inst|Add2~9\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add2~9_sumout\ = SUM(( !\conv_inst|image[3][3][2]~q\ ) + ( GND ) + ( \conv_inst|Add2~6\ ))
-- \conv_inst|Add2~10\ = CARRY(( !\conv_inst|image[3][3][2]~q\ ) + ( GND ) + ( \conv_inst|Add2~6\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111111111111111100000000000000001111111100000000",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datad => \conv_inst|ALT_INV_image[3][3][2]~q\,
	cin => \conv_inst|Add2~6\,
	sumout => \conv_inst|Add2~9_sumout\,
	cout => \conv_inst|Add2~10\);

-- Location: MLABCELL_X84_Y4_N6
\conv_inst|Add3~9\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~9_sumout\ = SUM(( \conv_inst|Add2~9_sumout\ ) + ( \conv_inst|conv_result\(2) ) + ( \conv_inst|Add3~6\ ))
-- \conv_inst|Add3~10\ = CARRY(( \conv_inst|Add2~9_sumout\ ) + ( \conv_inst|conv_result\(2) ) + ( \conv_inst|Add3~6\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000110011001100110000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datab => \conv_inst|ALT_INV_conv_result\(2),
	datad => \conv_inst|ALT_INV_Add2~9_sumout\,
	cin => \conv_inst|Add3~6\,
	sumout => \conv_inst|Add3~9_sumout\,
	cout => \conv_inst|Add3~10\);

-- Location: FF_X84_Y4_N8
\conv_inst|conv_result[2]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~9_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(2));

-- Location: FF_X84_Y4_N49
\conv_inst|conv_out[2]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \conv_inst|conv_result\(2),
	clrn => \ALT_INV_rst~input_o\,
	sload => VCC,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(2));

-- Location: IOIBUF_X89_Y8_N21
\pixel_in[3]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_pixel_in(3),
	o => \pixel_in[3]~input_o\);

-- Location: LABCELL_X85_Y4_N9
\conv_inst|image[3][3][3]~feeder\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|image[3][3][3]~feeder_combout\ = ( \pixel_in[3]~input_o\ )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000011111111111111111111111111111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataf => \ALT_INV_pixel_in[3]~input_o\,
	combout => \conv_inst|image[3][3][3]~feeder_combout\);

-- Location: FF_X85_Y4_N11
\conv_inst|image[3][3][3]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|image[3][3][3]~feeder_combout\,
	ena => \conv_inst|image[3][3][0]~1_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|image[3][3][3]~q\);

-- Location: LABCELL_X85_Y4_N39
\conv_inst|Add2~13\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add2~13_sumout\ = SUM(( !\conv_inst|image[3][3][3]~q\ ) + ( GND ) + ( \conv_inst|Add2~10\ ))
-- \conv_inst|Add2~14\ = CARRY(( !\conv_inst|image[3][3][3]~q\ ) + ( GND ) + ( \conv_inst|Add2~10\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111111111111111100000000000000001111000011110000",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_image[3][3][3]~q\,
	cin => \conv_inst|Add2~10\,
	sumout => \conv_inst|Add2~13_sumout\,
	cout => \conv_inst|Add2~14\);

-- Location: MLABCELL_X84_Y4_N9
\conv_inst|Add3~13\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~13_sumout\ = SUM(( \conv_inst|Add2~13_sumout\ ) + ( \conv_inst|conv_result\(3) ) + ( \conv_inst|Add3~10\ ))
-- \conv_inst|Add3~14\ = CARRY(( \conv_inst|Add2~13_sumout\ ) + ( \conv_inst|conv_result\(3) ) + ( \conv_inst|Add3~10\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111100001111000000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_conv_result\(3),
	datad => \conv_inst|ALT_INV_Add2~13_sumout\,
	cin => \conv_inst|Add3~10\,
	sumout => \conv_inst|Add3~13_sumout\,
	cout => \conv_inst|Add3~14\);

-- Location: FF_X84_Y4_N11
\conv_inst|conv_result[3]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~13_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(3));

-- Location: FF_X83_Y4_N49
\conv_inst|conv_out[3]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \conv_inst|conv_result\(3),
	clrn => \ALT_INV_rst~input_o\,
	sload => VCC,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(3));

-- Location: IOIBUF_X88_Y0_N19
\pixel_in[4]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_pixel_in(4),
	o => \pixel_in[4]~input_o\);

-- Location: LABCELL_X85_Y4_N24
\conv_inst|image[3][3][4]~feeder\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|image[3][3][4]~feeder_combout\ = ( \pixel_in[4]~input_o\ )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000011111111111111111111111111111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataf => \ALT_INV_pixel_in[4]~input_o\,
	combout => \conv_inst|image[3][3][4]~feeder_combout\);

-- Location: FF_X85_Y4_N26
\conv_inst|image[3][3][4]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|image[3][3][4]~feeder_combout\,
	ena => \conv_inst|image[3][3][0]~1_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|image[3][3][4]~q\);

-- Location: LABCELL_X85_Y4_N42
\conv_inst|Add2~17\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add2~17_sumout\ = SUM(( !\conv_inst|image[3][3][4]~q\ ) + ( GND ) + ( \conv_inst|Add2~14\ ))
-- \conv_inst|Add2~18\ = CARRY(( !\conv_inst|image[3][3][4]~q\ ) + ( GND ) + ( \conv_inst|Add2~14\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111111111111111100000000000000001111000011110000",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_image[3][3][4]~q\,
	cin => \conv_inst|Add2~14\,
	sumout => \conv_inst|Add2~17_sumout\,
	cout => \conv_inst|Add2~18\);

-- Location: MLABCELL_X84_Y4_N12
\conv_inst|Add3~17\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~17_sumout\ = SUM(( \conv_inst|Add2~17_sumout\ ) + ( \conv_inst|conv_result\(4) ) + ( \conv_inst|Add3~14\ ))
-- \conv_inst|Add3~18\ = CARRY(( \conv_inst|Add2~17_sumout\ ) + ( \conv_inst|conv_result\(4) ) + ( \conv_inst|Add3~14\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000110011001100110000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datab => \conv_inst|ALT_INV_conv_result\(4),
	datad => \conv_inst|ALT_INV_Add2~17_sumout\,
	cin => \conv_inst|Add3~14\,
	sumout => \conv_inst|Add3~17_sumout\,
	cout => \conv_inst|Add3~18\);

-- Location: FF_X84_Y4_N14
\conv_inst|conv_result[4]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~17_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(4));

-- Location: LABCELL_X83_Y4_N42
\conv_inst|conv_out[4]~feeder\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|conv_out[4]~feeder_combout\ = ( \conv_inst|conv_result\(4) )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000011111111111111111111111111111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataf => \conv_inst|ALT_INV_conv_result\(4),
	combout => \conv_inst|conv_out[4]~feeder_combout\);

-- Location: FF_X83_Y4_N43
\conv_inst|conv_out[4]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|conv_out[4]~feeder_combout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(4));

-- Location: IOIBUF_X86_Y0_N18
\pixel_in[5]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_pixel_in(5),
	o => \pixel_in[5]~input_o\);

-- Location: LABCELL_X85_Y4_N15
\conv_inst|image[3][3][5]~feeder\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|image[3][3][5]~feeder_combout\ = ( \pixel_in[5]~input_o\ )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000011111111111111111111111111111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataf => \ALT_INV_pixel_in[5]~input_o\,
	combout => \conv_inst|image[3][3][5]~feeder_combout\);

-- Location: FF_X85_Y4_N17
\conv_inst|image[3][3][5]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|image[3][3][5]~feeder_combout\,
	ena => \conv_inst|image[3][3][0]~1_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|image[3][3][5]~q\);

-- Location: LABCELL_X85_Y4_N45
\conv_inst|Add2~21\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add2~21_sumout\ = SUM(( !\conv_inst|image[3][3][5]~q\ ) + ( GND ) + ( \conv_inst|Add2~18\ ))
-- \conv_inst|Add2~22\ = CARRY(( !\conv_inst|image[3][3][5]~q\ ) + ( GND ) + ( \conv_inst|Add2~18\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111111111111111100000000000000001111000011110000",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_image[3][3][5]~q\,
	cin => \conv_inst|Add2~18\,
	sumout => \conv_inst|Add2~21_sumout\,
	cout => \conv_inst|Add2~22\);

-- Location: MLABCELL_X84_Y4_N15
\conv_inst|Add3~21\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~21_sumout\ = SUM(( \conv_inst|Add2~21_sumout\ ) + ( \conv_inst|conv_result\(5) ) + ( \conv_inst|Add3~18\ ))
-- \conv_inst|Add3~22\ = CARRY(( \conv_inst|Add2~21_sumout\ ) + ( \conv_inst|conv_result\(5) ) + ( \conv_inst|Add3~18\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111100001111000000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_conv_result\(5),
	datad => \conv_inst|ALT_INV_Add2~21_sumout\,
	cin => \conv_inst|Add3~18\,
	sumout => \conv_inst|Add3~21_sumout\,
	cout => \conv_inst|Add3~22\);

-- Location: FF_X84_Y4_N17
\conv_inst|conv_result[5]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~21_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(5));

-- Location: FF_X83_Y4_N40
\conv_inst|conv_out[5]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \conv_inst|conv_result\(5),
	clrn => \ALT_INV_rst~input_o\,
	sload => VCC,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(5));

-- Location: IOIBUF_X89_Y8_N55
\pixel_in[6]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_pixel_in(6),
	o => \pixel_in[6]~input_o\);

-- Location: LABCELL_X85_Y4_N6
\conv_inst|image[3][3][6]~feeder\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|image[3][3][6]~feeder_combout\ = ( \pixel_in[6]~input_o\ )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000011111111111111111111111111111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataf => \ALT_INV_pixel_in[6]~input_o\,
	combout => \conv_inst|image[3][3][6]~feeder_combout\);

-- Location: FF_X85_Y4_N7
\conv_inst|image[3][3][6]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|image[3][3][6]~feeder_combout\,
	ena => \conv_inst|image[3][3][0]~1_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|image[3][3][6]~q\);

-- Location: LABCELL_X85_Y4_N48
\conv_inst|Add2~25\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add2~25_sumout\ = SUM(( !\conv_inst|image[3][3][6]~q\ ) + ( GND ) + ( \conv_inst|Add2~22\ ))
-- \conv_inst|Add2~26\ = CARRY(( !\conv_inst|image[3][3][6]~q\ ) + ( GND ) + ( \conv_inst|Add2~22\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111111111111111100000000000000001111111100000000",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datad => \conv_inst|ALT_INV_image[3][3][6]~q\,
	cin => \conv_inst|Add2~22\,
	sumout => \conv_inst|Add2~25_sumout\,
	cout => \conv_inst|Add2~26\);

-- Location: MLABCELL_X84_Y4_N18
\conv_inst|Add3~25\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~25_sumout\ = SUM(( \conv_inst|Add2~25_sumout\ ) + ( \conv_inst|conv_result\(6) ) + ( \conv_inst|Add3~22\ ))
-- \conv_inst|Add3~26\ = CARRY(( \conv_inst|Add2~25_sumout\ ) + ( \conv_inst|conv_result\(6) ) + ( \conv_inst|Add3~22\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111100001111000000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_conv_result\(6),
	datad => \conv_inst|ALT_INV_Add2~25_sumout\,
	cin => \conv_inst|Add3~22\,
	sumout => \conv_inst|Add3~25_sumout\,
	cout => \conv_inst|Add3~26\);

-- Location: FF_X84_Y4_N20
\conv_inst|conv_result[6]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~25_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(6));

-- Location: LABCELL_X83_Y4_N21
\conv_inst|conv_out[6]~feeder\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|conv_out[6]~feeder_combout\ = ( \conv_inst|conv_result\(6) )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000011111111111111111111111111111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataf => \conv_inst|ALT_INV_conv_result\(6),
	combout => \conv_inst|conv_out[6]~feeder_combout\);

-- Location: FF_X83_Y4_N22
\conv_inst|conv_out[6]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|conv_out[6]~feeder_combout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(6));

-- Location: IOIBUF_X86_Y0_N1
\pixel_in[7]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_pixel_in(7),
	o => \pixel_in[7]~input_o\);

-- Location: LABCELL_X85_Y4_N18
\conv_inst|image[3][3][7]~feeder\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|image[3][3][7]~feeder_combout\ = ( \pixel_in[7]~input_o\ )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000011111111111111111111111111111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataf => \ALT_INV_pixel_in[7]~input_o\,
	combout => \conv_inst|image[3][3][7]~feeder_combout\);

-- Location: FF_X85_Y4_N19
\conv_inst|image[3][3][7]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|image[3][3][7]~feeder_combout\,
	ena => \conv_inst|image[3][3][0]~1_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|image[3][3][7]~q\);

-- Location: LABCELL_X85_Y4_N51
\conv_inst|Add2~29\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add2~29_sumout\ = SUM(( !\conv_inst|image[3][3][7]~q\ ) + ( GND ) + ( \conv_inst|Add2~26\ ))
-- \conv_inst|Add2~30\ = CARRY(( !\conv_inst|image[3][3][7]~q\ ) + ( GND ) + ( \conv_inst|Add2~26\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111111111111111100000000000000001111000011110000",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_image[3][3][7]~q\,
	cin => \conv_inst|Add2~26\,
	sumout => \conv_inst|Add2~29_sumout\,
	cout => \conv_inst|Add2~30\);

-- Location: MLABCELL_X84_Y4_N21
\conv_inst|Add3~29\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~29_sumout\ = SUM(( \conv_inst|Add2~29_sumout\ ) + ( \conv_inst|conv_result\(7) ) + ( \conv_inst|Add3~26\ ))
-- \conv_inst|Add3~30\ = CARRY(( \conv_inst|Add2~29_sumout\ ) + ( \conv_inst|conv_result\(7) ) + ( \conv_inst|Add3~26\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000101010101010101000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataa => \conv_inst|ALT_INV_conv_result\(7),
	datad => \conv_inst|ALT_INV_Add2~29_sumout\,
	cin => \conv_inst|Add3~26\,
	sumout => \conv_inst|Add3~29_sumout\,
	cout => \conv_inst|Add3~30\);

-- Location: FF_X84_Y4_N23
\conv_inst|conv_result[7]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~29_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(7));

-- Location: FF_X83_Y4_N13
\conv_inst|conv_out[7]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \conv_inst|conv_result\(7),
	clrn => \ALT_INV_rst~input_o\,
	sload => VCC,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(7));

-- Location: LABCELL_X85_Y4_N54
\conv_inst|Add2~33\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add2~33_sumout\ = SUM(( VCC ) + ( GND ) + ( \conv_inst|Add2~30\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111111111111111100000000000000001111111111111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	cin => \conv_inst|Add2~30\,
	sumout => \conv_inst|Add2~33_sumout\);

-- Location: MLABCELL_X84_Y4_N24
\conv_inst|Add3~33\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~33_sumout\ = SUM(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(8) ) + ( \conv_inst|Add3~30\ ))
-- \conv_inst|Add3~34\ = CARRY(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(8) ) + ( \conv_inst|Add3~30\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111100001111000000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_conv_result\(8),
	datad => \conv_inst|ALT_INV_Add2~33_sumout\,
	cin => \conv_inst|Add3~30\,
	sumout => \conv_inst|Add3~33_sumout\,
	cout => \conv_inst|Add3~34\);

-- Location: FF_X84_Y4_N26
\conv_inst|conv_result[8]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~33_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(8));

-- Location: LABCELL_X83_Y4_N54
\conv_inst|conv_out[8]~feeder\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|conv_out[8]~feeder_combout\ = ( \conv_inst|conv_result\(8) )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000011111111111111111111111111111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataf => \conv_inst|ALT_INV_conv_result\(8),
	combout => \conv_inst|conv_out[8]~feeder_combout\);

-- Location: FF_X83_Y4_N56
\conv_inst|conv_out[8]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|conv_out[8]~feeder_combout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(8));

-- Location: MLABCELL_X84_Y4_N27
\conv_inst|Add3~37\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~37_sumout\ = SUM(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(9) ) + ( \conv_inst|Add3~34\ ))
-- \conv_inst|Add3~38\ = CARRY(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(9) ) + ( \conv_inst|Add3~34\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000101010101010101000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataa => \conv_inst|ALT_INV_conv_result\(9),
	datad => \conv_inst|ALT_INV_Add2~33_sumout\,
	cin => \conv_inst|Add3~34\,
	sumout => \conv_inst|Add3~37_sumout\,
	cout => \conv_inst|Add3~38\);

-- Location: FF_X84_Y4_N29
\conv_inst|conv_result[9]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~37_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(9));

-- Location: FF_X84_Y4_N52
\conv_inst|conv_out[9]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \conv_inst|conv_result\(9),
	clrn => \ALT_INV_rst~input_o\,
	sload => VCC,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(9));

-- Location: MLABCELL_X84_Y4_N30
\conv_inst|Add3~41\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~41_sumout\ = SUM(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(10) ) + ( \conv_inst|Add3~38\ ))
-- \conv_inst|Add3~42\ = CARRY(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(10) ) + ( \conv_inst|Add3~38\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000110011001100110000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datab => \conv_inst|ALT_INV_conv_result\(10),
	datad => \conv_inst|ALT_INV_Add2~33_sumout\,
	cin => \conv_inst|Add3~38\,
	sumout => \conv_inst|Add3~41_sumout\,
	cout => \conv_inst|Add3~42\);

-- Location: FF_X84_Y4_N32
\conv_inst|conv_result[10]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~41_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(10));

-- Location: LABCELL_X83_Y4_N27
\conv_inst|conv_out[10]~feeder\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|conv_out[10]~feeder_combout\ = ( \conv_inst|conv_result\(10) )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000011111111111111111111111111111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataf => \conv_inst|ALT_INV_conv_result\(10),
	combout => \conv_inst|conv_out[10]~feeder_combout\);

-- Location: FF_X83_Y4_N29
\conv_inst|conv_out[10]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|conv_out[10]~feeder_combout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(10));

-- Location: MLABCELL_X84_Y4_N33
\conv_inst|Add3~45\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~45_sumout\ = SUM(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(11) ) + ( \conv_inst|Add3~42\ ))
-- \conv_inst|Add3~46\ = CARRY(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(11) ) + ( \conv_inst|Add3~42\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000101010101010101000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataa => \conv_inst|ALT_INV_conv_result\(11),
	datad => \conv_inst|ALT_INV_Add2~33_sumout\,
	cin => \conv_inst|Add3~42\,
	sumout => \conv_inst|Add3~45_sumout\,
	cout => \conv_inst|Add3~46\);

-- Location: FF_X84_Y4_N35
\conv_inst|conv_result[11]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~45_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(11));

-- Location: FF_X83_Y4_N17
\conv_inst|conv_out[11]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \conv_inst|conv_result\(11),
	clrn => \ALT_INV_rst~input_o\,
	sload => VCC,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(11));

-- Location: MLABCELL_X84_Y4_N36
\conv_inst|Add3~49\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~49_sumout\ = SUM(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(12) ) + ( \conv_inst|Add3~46\ ))
-- \conv_inst|Add3~50\ = CARRY(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(12) ) + ( \conv_inst|Add3~46\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111100001111000000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_conv_result\(12),
	datad => \conv_inst|ALT_INV_Add2~33_sumout\,
	cin => \conv_inst|Add3~46\,
	sumout => \conv_inst|Add3~49_sumout\,
	cout => \conv_inst|Add3~50\);

-- Location: FF_X84_Y4_N38
\conv_inst|conv_result[12]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~49_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(12));

-- Location: FF_X84_Y4_N58
\conv_inst|conv_out[12]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \conv_inst|conv_result\(12),
	clrn => \ALT_INV_rst~input_o\,
	sload => VCC,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(12));

-- Location: MLABCELL_X84_Y4_N39
\conv_inst|Add3~53\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~53_sumout\ = SUM(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(13) ) + ( \conv_inst|Add3~50\ ))
-- \conv_inst|Add3~54\ = CARRY(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(13) ) + ( \conv_inst|Add3~50\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111100001111000000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_conv_result\(13),
	datad => \conv_inst|ALT_INV_Add2~33_sumout\,
	cin => \conv_inst|Add3~50\,
	sumout => \conv_inst|Add3~53_sumout\,
	cout => \conv_inst|Add3~54\);

-- Location: FF_X84_Y4_N41
\conv_inst|conv_result[13]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~53_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(13));

-- Location: LABCELL_X83_Y4_N9
\conv_inst|conv_out[13]~feeder\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|conv_out[13]~feeder_combout\ = ( \conv_inst|conv_result\(13) )

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000011111111111111111111111111111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	dataf => \conv_inst|ALT_INV_conv_result\(13),
	combout => \conv_inst|conv_out[13]~feeder_combout\);

-- Location: FF_X83_Y4_N10
\conv_inst|conv_out[13]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|conv_out[13]~feeder_combout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(13));

-- Location: MLABCELL_X84_Y4_N42
\conv_inst|Add3~57\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~57_sumout\ = SUM(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(14) ) + ( \conv_inst|Add3~54\ ))
-- \conv_inst|Add3~58\ = CARRY(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(14) ) + ( \conv_inst|Add3~54\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000110011001100110000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datab => \conv_inst|ALT_INV_conv_result\(14),
	datad => \conv_inst|ALT_INV_Add2~33_sumout\,
	cin => \conv_inst|Add3~54\,
	sumout => \conv_inst|Add3~57_sumout\,
	cout => \conv_inst|Add3~58\);

-- Location: FF_X84_Y4_N44
\conv_inst|conv_result[14]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~57_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(14));

-- Location: FF_X83_Y4_N26
\conv_inst|conv_out[14]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \conv_inst|conv_result\(14),
	clrn => \ALT_INV_rst~input_o\,
	sload => VCC,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(14));

-- Location: MLABCELL_X84_Y4_N45
\conv_inst|Add3~61\ : cyclonev_lcell_comb
-- Equation(s):
-- \conv_inst|Add3~61_sumout\ = SUM(( \conv_inst|Add2~33_sumout\ ) + ( \conv_inst|conv_result\(15) ) + ( \conv_inst|Add3~58\ ))

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000111100001111000000000000000000000000000011111111",
	shared_arith => "off")
-- pragma translate_on
PORT MAP (
	datac => \conv_inst|ALT_INV_conv_result\(15),
	datad => \conv_inst|ALT_INV_Add2~33_sumout\,
	cin => \conv_inst|Add3~58\,
	sumout => \conv_inst|Add3~61_sumout\);

-- Location: FF_X84_Y4_N47
\conv_inst|conv_result[15]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	d => \conv_inst|Add3~61_sumout\,
	clrn => \ALT_INV_rst~input_o\,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_result\(15));

-- Location: FF_X84_Y4_N55
\conv_inst|conv_out[15]\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \conv_inst|conv_result\(15),
	clrn => \ALT_INV_rst~input_o\,
	sload => VCC,
	ena => \conv_inst|process_0~0_combout\,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|conv_out\(15));

-- Location: FF_X87_Y4_N37
\conv_inst|out_valid\ : dffeas
-- pragma translate_off
GENERIC MAP (
	is_wysiwyg => "true",
	power_up => "low")
-- pragma translate_on
PORT MAP (
	clk => \clk~inputCLKENA0_outclk\,
	asdata => \conv_inst|process_0~0_combout\,
	clrn => \ALT_INV_rst~input_o\,
	sload => VCC,
	devclrn => ww_devclrn,
	devpor => ww_devpor,
	q => \conv_inst|out_valid~q\);

-- Location: IOIBUF_X8_Y0_N52
\param_a[0]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_a(0),
	o => \param_a[0]~input_o\);

-- Location: IOIBUF_X89_Y25_N55
\param_a[1]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_a(1),
	o => \param_a[1]~input_o\);

-- Location: IOIBUF_X22_Y81_N35
\param_a[2]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_a(2),
	o => \param_a[2]~input_o\);

-- Location: IOIBUF_X36_Y0_N18
\param_a[3]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_a(3),
	o => \param_a[3]~input_o\);

-- Location: IOIBUF_X70_Y0_N1
\param_a[4]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_a(4),
	o => \param_a[4]~input_o\);

-- Location: IOIBUF_X8_Y81_N18
\param_a[5]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_a(5),
	o => \param_a[5]~input_o\);

-- Location: IOIBUF_X74_Y0_N58
\param_a[6]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_a(6),
	o => \param_a[6]~input_o\);

-- Location: IOIBUF_X26_Y0_N75
\param_a[7]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_a(7),
	o => \param_a[7]~input_o\);

-- Location: IOIBUF_X89_Y25_N38
\param_b[0]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_b(0),
	o => \param_b[0]~input_o\);

-- Location: IOIBUF_X88_Y0_N2
\param_b[1]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_b(1),
	o => \param_b[1]~input_o\);

-- Location: IOIBUF_X18_Y81_N92
\param_b[2]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_b(2),
	o => \param_b[2]~input_o\);

-- Location: IOIBUF_X12_Y0_N35
\param_b[3]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_b(3),
	o => \param_b[3]~input_o\);

-- Location: IOIBUF_X64_Y0_N35
\param_b[4]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_b(4),
	o => \param_b[4]~input_o\);

-- Location: IOIBUF_X22_Y81_N52
\param_b[5]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_b(5),
	o => \param_b[5]~input_o\);

-- Location: IOIBUF_X14_Y0_N52
\param_b[6]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_b(6),
	o => \param_b[6]~input_o\);

-- Location: IOIBUF_X40_Y0_N35
\param_b[7]~input\ : cyclonev_io_ibuf
-- pragma translate_off
GENERIC MAP (
	bus_hold => "false",
	simulate_z_as => "z")
-- pragma translate_on
PORT MAP (
	i => ww_param_b(7),
	o => \param_b[7]~input_o\);

-- Location: LABCELL_X9_Y39_N3
\~QUARTUS_CREATED_GND~I\ : cyclonev_lcell_comb
-- Equation(s):

-- pragma translate_off
GENERIC MAP (
	extended_lut => "off",
	lut_mask => "0000000000000000000000000000000000000000000000000000000000000000",
	shared_arith => "off")
-- pragma translate_on
;
END structure;


