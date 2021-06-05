library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
library work;
use work.tappkg.all;

entity tap_pulse is
  port (
    clk_i     : in std_logic;
    arst_i    : in std_logic;
    tstate_i  : in std_logic; -- '1' each 3.5Mhz
    pause_i   : in std_logic; -- Pause signal.

    valid_i   : in std_logic;
    data_i    : in std_logic_vector(3 downto 0);
    len_i     : in std_logic_vector(11 downto 0);
    busy_o    : out std_logic;
    audio_o   : out std_logic
);
end entity tap_pulse;

architecture beh of tap_pulse is

  subtype pulse_type is unsigned(11 downto 0);

  type state_type is (
    IDLE,
    PULSE
  );

  type regs_type is record
    state : state_type;
    dly   : unsigned(11 downto 0);
    cnt   : unsigned(11 downto 0);
    audio : std_logic;
    repeat: std_logic;
  end record;

  signal r: regs_type;

begin

  process(clk_i, arst_i, data_i, valid_i, r, tstate_i)
    variable w: regs_type;
  begin
    w := r;
    case r.state is
      when IDLE =>
        --busy_o <= '0';
        if valid_i='1' then
        --  w.len   := len_i;
          if data_i=PULSE_SINGLE then
            w.repeat := '0';
          else
            w.repeat := '1';
          end if;
          w.dly   := unsigned(len_i);
          w.cnt   := unsigned(len_i);
          w.state := PULSE;
        end if;

      when PULSE =>
        --busy_o  <= '1';
        if tstate_i='1' then
          if r.dly=0 then
            w.audio := not r.audio;
            if r.repeat='0' then
              w.state := IDLE;
            end if;
            w.dly := r.cnt;
            w.repeat := '0';
          else
            w.dly := r.dly - 1;
          end if;
        end if;

    end case;

    if arst_i='1' then
      r.state   <= IDLE;
      r.dly     <= (others => '0');
      r.repeat  <= '0';
      r.audio   <= '0';
    elsif rising_edge(clk_i) then
      if pause_i='0' then
        r <= w;
      end if;
    end if;
  end process;

  audio_o <= r.audio;
  busy_o <= '1' when r.state=PULSE else pause_i;  

end beh;
