LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.bfm_reset_p.all;


entity bfm_reset is
  generic (
      RESET_POLARITY: std_logic := '1'
    );
    port (
      Cmd_i:  in Cmd_Reset_type;
      -- Outputs.
      rst_o:  out std_logic
    );
end entity bfm_reset;

architecture sim of bfm_reset is

begin

  process
  begin
    rst_o <= not RESET_POLARITY;
    wait for 0 ps;
    l1: loop
      wait on Cmd_i;
      rst_o <= RESET_POLARITY;
      wait for Cmd_i.Reset_Time;
      rst_o <= not RESET_POLARITY;
      wait for 0 ps;
    end loop;
  end process;

end sim;
