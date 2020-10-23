LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.txt_util.all;

package bfm_reset_p is

  type Cmd_Reset_type is record
    Reset_Time: time;
  end record;

  component bfm_reset is
    generic (
      RESET_POLARITY: std_logic := '1'
    );
    port (
      Cmd_i:  in Cmd_Reset_type;
      -- Outputs.
      rst_o:  out std_logic
    );
  end component bfm_reset;

end package;
