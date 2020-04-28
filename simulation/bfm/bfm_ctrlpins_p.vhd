LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.txt_util.all;

package bfm_ctrlpins_p is

  type CtrlPinsCmd is (
    NONE
  );

  type Data_CtrlPins_type is record
    IO26      : std_logic;
    IO27      : std_logic;

    Busy      : boolean;
    Data      : std_logic_vector(7 downto 0);
  end record;

  type Cmd_CtrlPins_type is record
    Cmd       : CtrlPinsCmd;
  end record;

  component bfm_ctrlpins is
    port (
      Cmd_i   : in Cmd_CtrlPins_type;
      Data_o  : out Data_CtrlPins_type;

      IO26_i : in std_logic;
      IO27_i : in std_logic
    );
  end component bfm_ctrlpins;

  constant Cmd_CtrlPins_Defaults : Cmd_CtrlPins_type := (
    Cmd => NONE
  );

end package;
