## Generated SDC file "interfacez.out.sdc"

## Copyright (C) 2019  Intel Corporation. All rights reserved.
## Your use of Intel Corporation's design tools, logic functions 
## and other software and tools, and any partner logic 
## functions, and any output files from any of the foregoing 
## (including device programming or simulation files), and any 
## associated documentation or information are expressly subject 
## to the terms and conditions of the Intel Program License 
## Subscription Agreement, the Intel Quartus Prime License Agreement,
## the Intel FPGA IP License Agreement, or other applicable license
## agreement, including, without limitation, that your use is for
## the sole purpose of programming logic devices manufactured by
## Intel and sold by Intel or its authorized distributors.  Please
## refer to the applicable agreement for further details, at
## https://fpgasoftware.intel.com/eula.


## VENDOR  "Altera"
## PROGRAM "Quartus Prime"
## VERSION "Version 19.1.0 Build 670 09/22/2019 SJ Lite Edition"

## DATE    "Sun May 24 10:16:25 2020"

##
## DEVICE  "EP4CE6E22C8"
##


#**************************************************************
# Time Information
#**************************************************************

set_time_format -unit ns -decimal_places 3



#**************************************************************
# Create Clock
#**************************************************************

create_clock -name {CLK_i} -period 31.250 -waveform { 0.000 15.625 } [get_ports {CLK_i}]
create_clock -name {ESP_SCK_i} -period 25.000 -waveform { 0.000 12.500 } [get_ports {ESP_SCK_i}]


#**************************************************************
# Create Generated Clock
#**************************************************************

create_generated_clock -name {corepll_inst|altpll_component|auto_generated|pll1|clk[0]} -source [get_pins {corepll_inst|altpll_component|auto_generated|pll1|inclk[0]}] -duty_cycle 50/1 -multiply_by 3 -master_clock {CLK_i} [get_pins {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}] 
create_generated_clock -name {corepll_inst|altpll_component|auto_generated|pll1|clk[1]} -source [get_pins {corepll_inst|altpll_component|auto_generated|pll1|inclk[0]}] -duty_cycle 50/1 -multiply_by 3 -divide_by 2 -master_clock {CLK_i} [get_pins {corepll_inst|altpll_component|auto_generated|pll1|clk[1]}] 
create_generated_clock -name {corepll_inst|altpll_component|auto_generated|pll1|clk[3]} -source [get_pins {corepll_inst|altpll_component|auto_generated|pll1|inclk[0]}] -duty_cycle 50/1 -multiply_by 5 -divide_by 4 -master_clock {CLK_i} [get_pins {corepll_inst|altpll_component|auto_generated|pll1|clk[3]}] 
create_generated_clock -name {corepll_inst|altpll_component|auto_generated|pll1|clk[4]} -source [get_pins {corepll_inst|altpll_component|auto_generated|pll1|inclk[0]}] -duty_cycle 50/1 -multiply_by 15 -divide_by 17 -master_clock {CLK_i} [get_pins {corepll_inst|altpll_component|auto_generated|pll1|clk[4]}] 
#create_generated_clock -name {RAMCLK_o} -source [get_pins {interface_inst|psram_inst|clk_inst|auto_generated|ddio_outa[0]|dataout}] -master_clock {corepll_inst|altpll_component|auto_generated|pll1|clk[0]} [get_ports {RAMCLK_o}] 
create_generated_clock -name {RAMCLK_o} -source [get_pins {interface_inst|psram_inst|clk_inst|auto_generated|ddio_outa[0]|dataout}] -phase 90 [get_ports {RAMCLK_o}] 

#**************************************************************
# Set Clock Latency
#**************************************************************



#**************************************************************
# Set Clock Uncertainty
#**************************************************************

derive_clock_uncertainty


#**************************************************************
# Set Input Delay
#**************************************************************

#set_input_delay -add_delay  -clock [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]  10.000 [get_ports {RAMD_io[0]}]
#set_input_delay -add_delay  -clock [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]  10.000 [get_ports {RAMD_io[1]}]
#set_input_delay -add_delay  -clock [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]  10.000 [get_ports {RAMD_io[2]}]
#set_input_delay -add_delay  -clock [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]  10.000 [get_ports {RAMD_io[3]}]
#set_input_delay -add_delay  -clock [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]  10.000 [get_ports {RAMD_io[4]}]
#set_input_delay -add_delay  -clock [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]  10.000 [get_ports {RAMD_io[5]}]
#set_input_delay -add_delay  -clock [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]  10.000 [get_ports {RAMD_io[6]}]
#set_input_delay -add_delay  -clock [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]  10.000 [get_ports {RAMD_io[7]}]


#**************************************************************
# Set Output Delay
#**************************************************************

set_output_delay -max -clock [get_clocks {RAMCLK_o}]  2.000 [get_ports {RAMD_io[*]}]
set_output_delay -add_delay -min -clock [get_clocks {RAMCLK_o}]  -2.000 [get_ports {RAMD_io[*]}]


#**************************************************************
# Set Clock Groups
#**************************************************************



#**************************************************************
# Set False Path
#**************************************************************

#set_false_path  -from  [get_clocks {CLK_i}]  -to  [get_clocks *]
set_false_path  -from  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]  -to  [get_clocks {ESP_SCK_i}]
set_false_path  -from  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[1]}]  -to  [get_clocks {ESP_SCK_i}]
set_false_path  -from  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]  -to  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[3]}]
set_false_path  -from  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]  -to  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[4]}]
set_false_path  -from  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[3]}]  -to  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]
set_false_path  -from  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[4]}]  -to  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]
set_false_path  -from  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[3]}]  -to  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[4]}]
set_false_path  -from  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[4]}]  -to  [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[3]}]
set_false_path -from [get_keepers {*rdptr_g*}] -to [get_keepers {*ws_dgrp|dffpipe_te9:dffpipe9|dffe10a*}]
set_false_path -from [get_keepers {*delayed_wrptr_g*}] -to [get_keepers {*rs_dgwp|dffpipe_se9:dffpipe6|dffe7a*}]
set_false_path -from [get_keepers {*rdptr_g*}] -to [get_keepers {*ws_dgrp|dffpipe_re9:dffpipe18|dffe19a*}]
set_false_path -from [get_keepers {*delayed_wrptr_g*}] -to [get_keepers {*rs_dgwp|dffpipe_qe9:dffpipe15|dffe16a*}]
set_false_path  -from  [get_clocks {ESP_SCK_i}] -to [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[1]}]
set_false_path  -from  [get_clocks {ESP_SCK_i}] -to [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[0]}]
set_false_path  -from  [get_clocks {ESP_SCK_i}] -to [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[3]}]
set_false_path  -from  [get_clocks {ESP_SCK_i}] -to [get_clocks {corepll_inst|altpll_component|auto_generated|pll1|clk[4]}]




#**************************************************************
# Set Multicycle Path
#**************************************************************



#**************************************************************
# Set Maximum Delay
#**************************************************************



#**************************************************************
# Set Minimum Delay
#**************************************************************



#**************************************************************
# Set Input Transition
#**************************************************************

