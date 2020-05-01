library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

entity tap_pulse is
  port (
    clk_i     : in std_logic;
    arst_i    : in std_logic;
    tstate_i  : in std_logic; -- '1' each 3.5Mhz

    ready_i   : in std_logic;
    data_i    : in std_logic_vector(1 downto 0);
    rd_o      : out std_logic;
    idle_o    : out std_logic;

    audio_o   : out std_logic
);
end entity tap_pulse;

architecture beh of tap_pulse is

  constant PULSE_PILOT  : std_logic_vector(1 downto 0) := "00";
  constant PULSE_SYNC   : std_logic_vector(1 downto 0) := "01";
  constant PULSE_LOGIC0 : std_logic_vector(1 downto 0) := "10";
  constant PULSE_LOGIC1 : std_logic_vector(1 downto 0) := "11";

  function pulse_high(pulse: in std_logic_vector(1 downto 0)) return unsigned is
    variable t: natural;
  begin
    case pulse is
      when PULSE_PILOT  => t := 2168;
      when PULSE_SYNC   => t := 667;
      when PULSE_LOGIC0 => t := 855;
      when PULSE_LOGIC1 => t := 1710;
      when others =>
    end case;
    return to_unsigned(t-1, 12);
  end function;

  function pulse_low(pulse: in std_logic_vector(1 downto 0)) return unsigned is
    variable t: natural;
  begin
    case pulse is
      when PULSE_PILOT  => t := 2168;
      when PULSE_SYNC   => t := 735;
      when PULSE_LOGIC0 => t := 855;
      when PULSE_LOGIC1 => t := 1710;
      when others =>
    end case;
    return to_unsigned(t-1, 12);
  end function;

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
  end record;

  signal r: regs_type;

begin

  idle_o <= '1' when r.state=IDLE else '0';
  
  process(clk_i, arst_i, data_i, ready_i, r)
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
        w.state := PULSEHIGH;
        w.audio := not r.audio; --'1';
        w.dly   := pulse_high(data_i);
      when PULSEHIGH =>
        rd_o    <= '0';
        if tstate_i='1' then
          if r.dly=0 then
            w.state := PULSELOW;
            w.audio := not r.audio; --'0';
            w.dly   := pulse_low(data_i);
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
              -- Generate the high pulse when we are to stop.
              w.audio := not r.audio;--'1';
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
    elsif rising_edge(clk_i) then
      r <= w;
    end if;
  end process;

  audio_o <= r.audio;

end beh;
