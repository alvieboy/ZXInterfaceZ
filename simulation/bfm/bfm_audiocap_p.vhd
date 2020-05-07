LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.txt_util.all;

package bfm_audiocap_p is

  type AudiocapCmd is (
    NONE,
    SETMEM
  );

  type DetectedPulse is (
    PULSE_PILOT,
    PULSE_SYNC,
    PULSE_DATA0,
    PULSE_DATA1,
    PULSE_INVALID
  );

  type Data_Audiocap_type is record
    Trans     : natural;
    Pulse     : DetectedPulse;
    PeriodHigh: time;
    PeriodLow : time;
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
