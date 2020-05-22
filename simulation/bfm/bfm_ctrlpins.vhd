LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.txt_util.all;
use work.bfm_ctrlpins_p.all;

entity bfm_ctrlpins is
    port (
      Cmd_i   : in Cmd_CtrlPins_type;
      Data_o  : out Data_CtrlPins_type;

      IO26_i : in std_logic;
      IO27_i : in std_logic;
      USB_INTn_i      : in std_logic;
      FORCE_RESET_i   : in std_logic;
      FORCE_ROMCS_i   : in std_logic;
      fORCE_NMI_i     : in std_logic;
      FORCE_IORQULA_i : in std_logic
    );
end entity bfm_ctrlpins;

architecture sim of bfm_ctrlpins is

begin

  Data_o.IO26     <= IO26_i;
  Data_o.IO27     <= IO27_i;
  Data_o.USB_INTn <= USB_INTn_i;
  Data_o.RESET    <= FORCE_RESET_i;
  Data_o.ROMCS    <= FORCE_ROMCS_i;
  Data_o.NMI      <= FORCE_NMI_i;
  Data_o.IORQULA  <= FORCE_IORQULA_i;

end sim;
