LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.txt_util.all;

package bfm_ctrlpins_p is

  type CtrlPinsCmd is (
    NONE
  );

  type Data_CtrlPins_type is record
    --IO26      : std_logic;
    IO27      : std_logic;
    RESET     : std_logic;
    ROMCS     : std_logic;
    ROMCS2A   : std_logic;
    NMI       : std_logic;
    IORQULA   : std_logic;
    --USB_INTn  : std_logic;

    Busy      : boolean;
    Data      : std_logic_vector(7 downto 0);
  end record;

  type Cmd_CtrlPins_type is record
    Cmd       : CtrlPinsCmd;
    ReqAck    : std_logic;
  end record;

  component bfm_ctrlpins is
    port (
      Cmd_i           : in Cmd_CtrlPins_type;
      Data_o          : out Data_CtrlPins_type;

      --USB_INTn_i      : in std_logic;
      --IO26_i          : in std_logic;
      IO27_i          : in std_logic;
      FORCE_RESET_i   : in std_logic;
      FORCE_ROMCS_i   : in std_logic;
      FORCE_ROMCS2A_i : in std_logic;
      fORCE_NMI_i     : in std_logic;
      FORCE_IORQULA_i : in std_logic
    );
  end component bfm_ctrlpins;

  constant Cmd_CtrlPins_Defaults : Cmd_CtrlPins_type := (
    Cmd => NONE,
    ReqAck => '1'
  );

  procedure ackInterrupt(signal Cmd: out Cmd_CtrlPins_type);

end package;

package body bfm_ctrlpins_p is

  procedure ackInterrupt(signal Cmd: out Cmd_CtrlPins_type) is
  begin
    Cmd.ReqAck <= '0';
    wait for 100 ns;
    Cmd.ReqAck <= '1';
  end procedure;



end package body;
