//altclkctrl CBX_SINGLE_OUTPUT_FILE="ON" CLOCK_TYPE="Global Clock" DEVICE_FAMILY="Cyclone IV E" ENA_REGISTER_MODE="falling edge" USE_GLITCH_FREE_SWITCH_OVER_IMPLEMENTATION="OFF" clkselect ena inclk outclk
//VERSION_BEGIN 19.1 cbx_altclkbuf 2019:09:22:08:02:34:SJ cbx_cycloneii 2019:09:22:08:02:34:SJ cbx_lpm_add_sub 2019:09:22:08:02:34:SJ cbx_lpm_compare 2019:09:22:08:02:34:SJ cbx_lpm_decode 2019:09:22:08:02:34:SJ cbx_lpm_mux 2019:09:22:08:02:34:SJ cbx_mgl 2019:09:22:09:26:20:SJ cbx_nadder 2019:09:22:08:02:34:SJ cbx_stratix 2019:09:22:08:02:34:SJ cbx_stratixii 2019:09:22:08:02:34:SJ cbx_stratixiii 2019:09:22:08:02:34:SJ cbx_stratixv 2019:09:22:08:02:34:SJ  VERSION_END
// synthesis VERILOG_INPUT_VERSION VERILOG_2001
// altera message_off 10463



// Copyright (C) 2019  Intel Corporation. All rights reserved.
//  Your use of Intel Corporation's design tools, logic functions 
//  and other software and tools, and any partner logic 
//  functions, and any output files from any of the foregoing 
//  (including device programming or simulation files), and any 
//  associated documentation or information are expressly subject 
//  to the terms and conditions of the Intel Program License 
//  Subscription Agreement, the Intel Quartus Prime License Agreement,
//  the Intel FPGA IP License Agreement, or other applicable license
//  agreement, including, without limitation, that your use is for
//  the sole purpose of programming logic devices manufactured by
//  Intel and sold by Intel or its authorized distributors.  Please
//  refer to the applicable agreement for further details, at
//  https://fpgasoftware.intel.com/eula.



//synthesis_resources = clkctrl 1 
//synopsys translate_off
`timescale 1 ps / 1 ps
//synopsys translate_on
module  clockmux_altclkctrl_0_sub
	( 
	clkselect,
	ena,
	inclk,
	outclk) /* synthesis synthesis_clearbox=1 */;
	input   [1:0]  clkselect;
	input   ena;
	input   [3:0]  inclk;
	output   outclk;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_off
`endif
	tri0   [1:0]  clkselect;
	tri1   ena;
	tri0   [3:0]  inclk;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_on
`endif

	wire  wire_clkctrl1_outclk;
	wire  [1:0]  clkselect_wire;
	wire  [3:0]  inclk_wire;

	cycloneive_clkctrl   clkctrl1
	( 
	.clkselect(clkselect_wire),
	.ena(ena),
	.inclk(inclk_wire),
	.outclk(wire_clkctrl1_outclk)
	// synopsys translate_off
	,
	.devclrn(1'b1),
	.devpor(1'b1)
	// synopsys translate_on
	);
	defparam
		clkctrl1.clock_type = "Global Clock",
		clkctrl1.ena_register_mode = "falling edge",
		clkctrl1.lpm_type = "cycloneive_clkctrl";
	assign
		clkselect_wire = {clkselect},
		inclk_wire = {inclk},
		outclk = wire_clkctrl1_outclk;
endmodule //clockmux_altclkctrl_0_sub
//VALID FILE // (C) 2001-2019 Intel Corporation. All rights reserved.
// Your use of Intel Corporation's design tools, logic functions and other 
// software and tools, and its AMPP partner logic functions, and any output 
// files from any of the foregoing (including device programming or simulation 
// files), and any associated documentation or information are expressly subject 
// to the terms and conditions of the Intel Program License Subscription 
// Agreement, Intel FPGA IP License Agreement, or other applicable 
// license agreement, including, without limitation, that your use is for the 
// sole purpose of programming logic devices manufactured by Intel and sold by 
// Intel or its authorized distributors.  Please refer to the applicable 
// agreement for further details.



// synopsys translate_off
`timescale 1 ps / 1 ps
// synopsys translate_on
module  clockmux_altclkctrl_0  (
    clkselect,
    inclk0x,
    inclk1x,
    outclk);

    input    clkselect;
    input    inclk0x;
    input    inclk1x;
    output   outclk;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_off
`endif
    tri0     clkselect;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_on
`endif

    wire  sub_wire0;
    wire  outclk;
    wire  sub_wire1;
    wire [1:0] sub_wire2;
    wire [0:0] sub_wire3;
    wire  sub_wire4;
    wire  sub_wire5;
    wire [3:0] sub_wire6;
    wire  sub_wire7;
    wire [1:0] sub_wire8;

    assign  outclk = sub_wire0;
    assign  sub_wire1 = clkselect;
    assign sub_wire2[1:0] = {sub_wire3, sub_wire1};
    assign sub_wire3[0:0] = 1'h0;
    assign  sub_wire4 = 1'h1;
    assign  sub_wire5 = inclk0x;
    assign sub_wire6[3:0] = {sub_wire8, sub_wire7, sub_wire5};
    assign  sub_wire7 = inclk1x;
    assign sub_wire8[1:0] = 2'h0;

    clockmux_altclkctrl_0_sub  clockmux_altclkctrl_0_sub_component (
                .clkselect (sub_wire2),
                .ena (sub_wire4),
                .inclk (sub_wire6),
                .outclk (sub_wire0));

endmodule