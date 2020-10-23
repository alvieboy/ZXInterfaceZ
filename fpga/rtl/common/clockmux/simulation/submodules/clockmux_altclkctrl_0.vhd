--altclkctrl CBX_SINGLE_OUTPUT_FILE="ON" CLOCK_TYPE="Global Clock" DEVICE_FAMILY="Cyclone IV E" ENA_REGISTER_MODE="falling edge" USE_GLITCH_FREE_SWITCH_OVER_IMPLEMENTATION="OFF" clkselect ena inclk outclk
--VERSION_BEGIN 19.1 cbx_altclkbuf 2019:09:22:08:02:34:SJ cbx_cycloneii 2019:09:22:08:02:34:SJ cbx_lpm_add_sub 2019:09:22:08:02:34:SJ cbx_lpm_compare 2019:09:22:08:02:34:SJ cbx_lpm_decode 2019:09:22:08:02:34:SJ cbx_lpm_mux 2019:09:22:08:02:34:SJ cbx_mgl 2019:09:22:09:26:20:SJ cbx_nadder 2019:09:22:08:02:34:SJ cbx_stratix 2019:09:22:08:02:34:SJ cbx_stratixii 2019:09:22:08:02:34:SJ cbx_stratixiii 2019:09:22:08:02:34:SJ cbx_stratixv 2019:09:22:08:02:34:SJ  VERSION_END


-- Copyright (C) 2019  Intel Corporation. All rights reserved.
--  Your use of Intel Corporation's design tools, logic functions 
--  and other software and tools, and any partner logic 
--  functions, and any output files from any of the foregoing 
--  (including device programming or simulation files), and any 
--  associated documentation or information are expressly subject 
--  to the terms and conditions of the Intel Program License 
--  Subscription Agreement, the Intel Quartus Prime License Agreement,
--  the Intel FPGA IP License Agreement, or other applicable license
--  agreement, including, without limitation, that your use is for
--  the sole purpose of programming logic devices manufactured by
--  Intel and sold by Intel or its authorized distributors.  Please
--  refer to the applicable agreement for further details, at
--  https://fpgasoftware.intel.com/eula.



 LIBRARY cycloneive;
 USE cycloneive.all;

--synthesis_resources = clkctrl 1 
 LIBRARY ieee;
 USE ieee.std_logic_1164.all;

 ENTITY  clockmux_altclkctrl_0_sub IS 
	 PORT 
	 ( 
		 clkselect	:	IN  STD_LOGIC_VECTOR (1 DOWNTO 0) := (OTHERS => '0');
		 ena	:	IN  STD_LOGIC := '1';
		 inclk	:	IN  STD_LOGIC_VECTOR (3 DOWNTO 0) := (OTHERS => '0');
		 outclk	:	OUT  STD_LOGIC
	 ); 
 END clockmux_altclkctrl_0_sub;

 ARCHITECTURE RTL OF clockmux_altclkctrl_0_sub IS

	 ATTRIBUTE synthesis_clearbox : natural;
	 ATTRIBUTE synthesis_clearbox OF RTL : ARCHITECTURE IS 1;
	 SIGNAL  wire_clkctrl1_outclk	:	STD_LOGIC;
	 SIGNAL  clkselect_wire :	STD_LOGIC_VECTOR (1 DOWNTO 0);
	 SIGNAL  inclk_wire :	STD_LOGIC_VECTOR (3 DOWNTO 0);
	 COMPONENT  cycloneive_clkctrl
	 GENERIC 
	 (
		clock_type	:	STRING;
		ena_register_mode	:	STRING := "falling edge";
		lpm_type	:	STRING := "cycloneive_clkctrl"
	 );
	 PORT
	 ( 
		clkselect	:	IN STD_LOGIC_VECTOR(1 DOWNTO 0);
		ena	:	IN STD_LOGIC;
		inclk	:	IN STD_LOGIC_VECTOR(3 DOWNTO 0);
		outclk	:	OUT STD_LOGIC
	 ); 
	 END COMPONENT;
 BEGIN

	clkselect_wire <= ( clkselect);
	inclk_wire <= ( inclk);
	outclk <= wire_clkctrl1_outclk;
	clkctrl1 :  cycloneive_clkctrl
	  GENERIC MAP (
		clock_type => "Global Clock",
		ena_register_mode => "falling edge"
	  )
	  PORT MAP ( 
		clkselect => clkselect_wire,
		ena => ena,
		inclk => inclk_wire,
		outclk => wire_clkctrl1_outclk
	  );

 END RTL; --clockmux_altclkctrl_0_sub
--VALID FILE -- (C) 2001-2019 Intel Corporation. All rights reserved.
-- Your use of Intel Corporation's design tools, logic functions and other 
-- software and tools, and its AMPP partner logic functions, and any output 
-- files from any of the foregoing (including device programming or simulation 
-- files), and any associated documentation or information are expressly subject 
-- to the terms and conditions of the Intel Program License Subscription 
-- Agreement, Intel FPGA IP License Agreement, or other applicable 
-- license agreement, including, without limitation, that your use is for the 
-- sole purpose of programming logic devices manufactured by Intel and sold by 
-- Intel or its authorized distributors.  Please refer to the applicable 
-- agreement for further details.




LIBRARY ieee;
USE ieee.std_logic_1164.all;

ENTITY clockmux_altclkctrl_0 IS
    PORT
    (
        clkselect       : IN STD_LOGIC  := '0';
        inclk0x       : IN STD_LOGIC;
        inclk1x       : IN STD_LOGIC;
        outclk       : OUT STD_LOGIC
    );
END clockmux_altclkctrl_0;


ARCHITECTURE RTL OF clockmux_altclkctrl_0 IS

    SIGNAL sub_wire0    : STD_LOGIC ;
    SIGNAL sub_wire1    : STD_LOGIC ;
    SIGNAL sub_wire2    : STD_LOGIC_VECTOR  (1 DOWNTO 0);
    SIGNAL sub_wire3_bv : BIT_VECTOR (0 DOWNTO 0);
    SIGNAL sub_wire3    : STD_LOGIC_VECTOR  (0 DOWNTO 0);
    SIGNAL sub_wire4    : STD_LOGIC ;
    SIGNAL sub_wire5    : STD_LOGIC ;
    SIGNAL sub_wire6    : STD_LOGIC_VECTOR  (3 DOWNTO 0);
    SIGNAL sub_wire7    : STD_LOGIC ;
    SIGNAL sub_wire8_bv : BIT_VECTOR (1 DOWNTO 0);
    SIGNAL sub_wire8    : STD_LOGIC_VECTOR  (1 DOWNTO 0);


    COMPONENT  clockmux_altclkctrl_0_sub
    PORT (
            clkselect   : IN STD_LOGIC_VECTOR (1 DOWNTO 0);
            ena : IN STD_LOGIC;
            inclk   : IN STD_LOGIC_VECTOR  (3 DOWNTO 0);
            outclk  : OUT STD_LOGIC
    );


BEGIN
    outclk     <= sub_wire0;
    sub_wire1     <= clkselect;
    sub_wire2     <= sub_wire3 &  sub_wire1;
    sub_wire3_bv(0 DOWNTO 0)  <= "0";
    sub_wire3   <= To_stdlogicvector(sub_wire3_bv);
    sub_wire4     <= '1';
    sub_wire5     <= inclk0x;
    sub_wire6     <= sub_wire8 &  sub_wire7 &  sub_wire5;
    sub_wire7     <= inclk1x;
    sub_wire8_bv(1 DOWNTO 0)  <= "00";
    sub_wire8   <= To_stdlogicvector(sub_wire8_bv);

    clockmux_altclkctrl_0_sub_component : clockmux_altclkctrl_0_sub
    PORT MAP (
        clkselect => sub_wire2,
        ena => sub_wire4,
        inclk => sub_wire6,
        outclk => sub_wire0
    );



END RTL;