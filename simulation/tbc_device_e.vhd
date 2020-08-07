LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;
USE std.textio.all;

LIBRARY work;
use work.txt_util.all;
use work.bfm_reset_p.all;
use work.bfm_clock_p.all;
use work.bfm_spimaster_p.all;
use work.bfm_spectrum_p.all;
use work.bfm_ctrlpins_p.all;
use work.bfm_ula_p.all;
use work.bfm_audiocap_p.all;
use work.bfm_qspiram_p.all;
use work.bfm_usbdevice_p.all;

entity tbc_device is
  port (
    SysRst_Cmd      : out Cmd_Reset_type := Cmd_Reset_Defaults;
    SysClk_Cmd      : out Cmd_Clock_type := Cmd_Clock_Defaults;

    SpectRst_Cmd    : out Cmd_Reset_type := Cmd_Reset_Defaults;
    SpectClk_Cmd    : out Cmd_Clock_type := Cmd_Clock_Defaults;

    Spimaster_Cmd   : out Cmd_Spimaster_type := Cmd_Spimaster_Defaults;
    Spectrum_Cmd    : out Cmd_Spectrum_type   := Cmd_Spectrum_Defaults;
    CtrlPins_Cmd    : out Cmd_CtrlPins_type   := Cmd_CtrlPins_Defaults;
    Ula_Cmd         : out Cmd_Ula_type   := Cmd_Ula_Defaults;
    Audiocap_Cmd    : out Cmd_Audiocap_type := Cmd_Audiocap_Defaults;
    QSPIRam0_Cmd    : out Cmd_QSPIRam_type;
    QSPIRam1_Cmd    : out Cmd_QSPIRam_type;
    UsbDevice_Cmd   : out Cmd_UsbDevice_type;

    -- Inputs
    Spimaster_Data  : in Data_Spimaster_type;
    Spectrum_Data   : in Data_Spectrum_type;
    CtrlPins_Data   : in Data_CtrlPins_type;
    Ula_Data        : in Data_Ula_type;
    Audiocap_Data   : in Data_Audiocap_type;
    Usbdevice_Data  : in Data_Usbdevice_type;
    QSPIRam0_Data   : in Data_QSPIRam_type
  );
end tbc_device;
