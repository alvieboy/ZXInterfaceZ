LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.txt_util.all;

package bfm_ula_p is

  type UlaCmd is (
    NONE,
    SETKBDDATA,
    SETAUDIODATA
  );

  type Data_Ula_type is record
    Data      : std_logic_vector(7 downto 0);
  end record;

  type Cmd_Ula_type is record
    Cmd       : UlaCmd;
    Enabled   : boolean;
    KeybAddr  : natural range 0 to 7;
    Audio     : std_logic;--_vector(7 downto 0);
    KeybScan  : std_logic_vector(4 downto 0);
  end record;

  component bfm_ula is
    port (
      Cmd_i   : in Cmd_Ula_type;
      Data_o  : out Data_Ula_type;
      A_i     : in std_logic_vector(15 downto 0);
      D_io    : inout std_logic_vector(7 downto 0);
      IOREQn_i: in std_logic;
      RDn_i   : in std_logic;
      WRn_i   : in std_logic;
      OEn_i   : in std_logic
    );
  end component bfm_ula;

  constant Cmd_Ula_Defaults: Cmd_Ula_type := (
    Cmd     => NONE,
    Enabled => false,
    Audio     => '0',
    KeybScan => "00000",
    KeybAddr => 0
  );
  

end package;
