LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

LIBRARY work;
use work.txt_util.all;
use work.bfm_reset_p.all;
use work.bfm_clock_p.all;
use work.bfm_spimaster_p.all;
use work.bfm_spectrum_p.all;
use work.bfm_ctrlpins_p.all;

entity tbc_device is
  port (
    SysRst_Cmd      : out Cmd_Reset_type;
    SysClk_Cmd      : out Cmd_Clock_type;

    SpectRst_Cmd    : out Cmd_Reset_type;
    SpectClk_Cmd    : out Cmd_Clock_type;

    Spimaster_Cmd   : out Cmd_Spimaster_type;
    Spectrum_Cmd    : out Cmd_Spectrum_type;
    CtrlPins_Cmd    : out Cmd_CtrlPins_type;

    -- Inputs
    Spimaster_Data  : in Data_Spimaster_type;
    Spectrum_Data   : in Data_Spectrum_type;
    CtrlPins_Data   : in Data_CtrlPins_type
  );
end tbc_device;
