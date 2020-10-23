set_global_assignment -entity "clockmux" -library "lib_clockmux" -name IP_TOOL_NAME "Qsys"
set_global_assignment -entity "clockmux" -library "lib_clockmux" -name IP_TOOL_VERSION "19.1"
set_global_assignment -entity "clockmux" -library "lib_clockmux" -name IP_TOOL_ENV "Qsys"
set_global_assignment -library "lib_clockmux" -name SPD_FILE [file join $::quartus(sip_path) "../clockmux.spd"]
set_global_assignment -library "lib_clockmux" -name MISC_FILE [file join $::quartus(sip_path) "../../clockmux.qsys"]

set_global_assignment -library "lib_clockmux" -name MISC_FILE [file join $::quartus(sip_path) "clockmux.vhd"]
set_global_assignment -library "lib_clockmux" -name MISC_FILE [file join $::quartus(sip_path) "submodules/clockmux_altclkctrl_0.vhd"]
