LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.txt_util.all;

package bfm_audiocap_p is

  type AudiocapCmd is (
    NONE,
    SETMEM
  );

  type Data_Audiocap_type is record
    Trans     : natural;
    Delta     : time;
    Polarity  : std_logic;
  end record;

  type Cmd_Audiocap_type is record
    Cmd       : AudiocapCmd;
    Enabled   : boolean;
  end record;

  component bfm_audiocap is
    port (
      Cmd_i   : in Cmd_Audiocap_type;
      Data_o  : out Data_Audiocap_type;
      audio_i  : in std_logic
    );
  end component bfm_audiocap;

  constant Cmd_Audiocap_Defaults: Cmd_Audiocap_type := (
    Cmd     => NONE,
    Enabled => false
  );
  

end package;
