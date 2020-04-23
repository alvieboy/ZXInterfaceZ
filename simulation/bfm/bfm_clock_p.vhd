LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.txt_util.all;

package bfm_clock_p is

  type Cmd_Clock_type is record
    Period  : time;
    Enabled : boolean;
  end record;

  constant Cmd_Clock_Defaults: Cmd_Clock_type := ( 0 ps, false );

  component bfm_clock is
    port (
      Cmd_i : in Cmd_Clock_type;
      -- Outputs.
      clk_o : out std_logic
    );
  end component bfm_clock;

end package;
