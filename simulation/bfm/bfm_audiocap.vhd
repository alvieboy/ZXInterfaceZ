LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

LIBRARY work;
use work.bfm_audiocap_p.all;
use work.txt_util.all;


entity bfm_audiocap is
  port (
    Cmd_i   : in Cmd_Audiocap_type;
    Data_o  : out Data_Audiocap_type;
    audio_i : in std_logic
  );
end entity bfm_audiocap;

architecture sim of bfm_audiocap is


begin

  process
    variable delta: time;
    variable rise:  time;
    variable fall:  time;
  begin
    if Cmd_i.Enabled then
      l: loop
        wait until rising_edge(audio_i);
        rise := now;
        wait until falling_edge(audio_i) for 1 ms;
        fall := now;
  
        delta := fall - rise;
        report "Edge " & time'image(delta);
      end loop;
    else
      wait on Cmd_i.Enabled;
    end if;
  end process;

end sim;
