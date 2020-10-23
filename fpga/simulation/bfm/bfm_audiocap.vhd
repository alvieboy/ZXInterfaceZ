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
    variable last_event       : time;
    variable last_event_valid : boolean;
    variable trans            : natural := 0;
  begin
    if Cmd_i.Enabled then
      wait on Cmd_i.Enabled, audio_i;
      if Cmd_i.Enabled then
        if audio_i'event then
          if last_event_valid then
            trans := trans + 1;
            Data_o.Delta <= now - last_event;
            Data_o.Polarity <= audio_i;
            Data_o.Trans <= trans;
          end if;
          last_event        := now;
          last_event_valid  := true;
        end if;
      end if;
    else
      last_event_valid := false;
      wait on Cmd_i.Enabled;
    end if;
  end process;

  Data_o.Audio <= audio_i;

end sim;
