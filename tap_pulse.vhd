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

    ready_i   : in std_logic;
    data_i    : in std_logic_vector(3 downto 0);
    len_i     : in std_logic_vector(11 downto 0);
    rd_o      : out std_logic;
    idle_o    : out std_logic;

    audio_o   : out std_logic
);
end entity tap_pulse;

architecture beh of tap_pulse is

  subtype pulse_type is unsigned(11 downto 0);

  type state_type is (
    IDLE,
    LOADDATA,
    PULSEHIGH,
    PULSELOW
  );


  type regs_type is record
    state : state_type;
    dly   : unsigned(11 downto 0);
    audio : std_logic;
    pulse_pilot  : pulse_type;
    pulse_sync0  : pulse_type;
    pulse_sync1  : pulse_type;
    pulse_logic0 : pulse_type;
    pulse_logic1 : pulse_type;
  end record;

  signal r: regs_type;

  function pulse_high(reg: in regs_type; pulse: in std_logic_vector(3 downto 0)) return unsigned is
    variable t: unsigned(11 downto 0);
  begin
    case pulse is
      when PULSE_PILOT  => t := reg.pulse_pilot;
      when PULSE_SYNC   => t := reg.pulse_sync0;
      when PULSE_LOGIC0 => t := reg.pulse_logic0;
      when PULSE_LOGIC1 => t := reg.pulse_logic1;
      when others => t := reg.pulse_pilot;
    end case;
    return t;
  end function;

  function pulse_low(reg: in regs_type; pulse: in std_logic_vector(3 downto 0)) return unsigned is
    variable t: unsigned(11 downto 0);
  begin
    case pulse is
      when PULSE_PILOT  => t := reg.pulse_pilot;
      when PULSE_SYNC   => t := reg.pulse_sync1;
      when PULSE_LOGIC0 => t := reg.pulse_logic0;
      when PULSE_LOGIC1 => t := reg.pulse_logic1;
      when others => t := reg.pulse_pilot;
    end case;
    return t;
  end function;

begin

  idle_o <= '1' when r.state=IDLE else '0';
  
  process(clk_i, arst_i, data_i, ready_i, r, tstate_i)
    variable w: regs_type;
  begin
    w := r;
    case r.state is
      when IDLE =>
        rd_o <= ready_i;
        if ready_i='1' then
          w.state := LOADDATA;
        end if;

      when LOADDATA =>
        rd_o    <= '0';

        w.state := IDLE;
        case data_i is
            when SET_PULSE_PILOT  => w.pulse_pilot  := unsigned(len_i);
            when SET_PULSE_SYNC0  => w.pulse_sync0  := unsigned(len_i);
            when SET_PULSE_SYNC1  => w.pulse_sync1  := unsigned(len_i);
            when SET_PULSE_LOGIC0 => w.pulse_logic0 := unsigned(len_i);
            when SET_PULSE_LOGIC1 => w.pulse_logic1 := unsigned(len_i);
            when SET_PULSE_TAP    =>
              w.pulse_pilot  := to_unsigned(2168,12);
              w.pulse_sync0  := to_unsigned(667,12);
              w.pulse_sync1  := to_unsigned(735,12);
              w.pulse_logic0 := to_unsigned(855,12);
              w.pulse_logic1 := to_unsigned(1710,12);

            when others =>
              w.state := PULSEHIGH;
              w.audio := not r.audio;
              w.dly   := pulse_high(r,data_i);
        end case;
      when PULSEHIGH =>
        rd_o    <= '0';
        if tstate_i='1' then
          if r.dly=0 then
            w.state := PULSELOW;
            w.audio := not r.audio;
            w.dly   := pulse_low(r,data_i);
          else
            w.dly := r.dly - 1;
          end if;
        end if;
      when PULSELOW =>
        rd_o    <= '0';
        if tstate_i='1' then
          if r.dly=0 then
            w.state := IDLE;
            -- Unless we have more data. If so, process it.
            rd_o <= ready_i;
            if ready_i='1' then
              w.state := LOADDATA;
            else
              -- Generate the pulse when we are to stop.
              w.audio := not r.audio;
            end if;
          else
            w.dly := r.dly - 1;
          end if;
        end if;
    end case;

    if arst_i='1' then
      r.state <= IDLE;
      r.dly   <= (others => '0');
      r.audio <= '0';
      r.pulse_pilot  <= to_unsigned(2168,12);
      r.pulse_sync0  <= to_unsigned(667,12);
      r.pulse_sync1  <= to_unsigned(735,12);
      r.pulse_logic0 <= to_unsigned(855,12);
      r.pulse_logic1 <= to_unsigned(1710,12);

    elsif rising_edge(clk_i) then
      r <= w;
    end if;
  end process;

  audio_o <= r.audio;

end beh;
