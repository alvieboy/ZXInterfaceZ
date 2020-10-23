LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.bfm_clock_p.all;


entity bfm_clock is
  port (
    Cmd_i:  in Cmd_Clock_type := Cmd_Clock_Defaults;
    -- Outputs.
    clk_o:  out std_logic
  );
end entity bfm_clock;

architecture sim of bfm_clock is

begin

  process
  begin
    if Cmd_i.Enabled then
      report "Starting clock period " &time'image(Cmd_i.Period);
      l1: loop
        wait for Cmd_i.Period / 2;
        clk_o <= '1';
        wait for Cmd_i.Period / 2;
        clk_o <= '0';
        if not Cmd_i.Enabled then
          exit l1;
        end if;
      end loop;
    else
      clk_o <= '0';
      wait for 0 ps;
      --report "Wait";
      wait on Cmd_i;
      --report "Waited";
    end if;

  end process;

end sim;
