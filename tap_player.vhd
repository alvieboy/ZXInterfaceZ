library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

entity tap_player is
  port (
    clk_i     : in std_logic;
    arst_i    : in std_logic;
    tstate_i  : in std_logic; -- '1' each 3.5Mhz
    enable_i  : in std_logic;
    restart_i : in std_logic;

    ready_i   : in std_logic;
    data_i    : in std_logic_vector(7 downto 0);
    rd_o      : out std_logic;

    audio_o   : out std_logic
);
end entity tap_player;


architecture beh of tap_player is
  

  type state_type is (
    IDLE,
    LOAD_SIZE_LSB,
    LOAD_SIZE_MSB,
    LOAD_TYPE,
    SEND_PILOT,
    GAP,
    PLAY_DATA
  );

  function PILOT_HEADER return natural is
    variable r: natural;
  begin
    r:=8063;
    -- synthesis translate_off
    r:=8;
    -- synthesis translate_on
    return r;
  end function;

  function PILOT_DATA return natural is
    variable r: natural;
  begin
    r:=3223;
    -- synthesis translate_off
    r:=3;
    -- synthesis translate_on
    return r;
  end function;

  function GAP_DELAY return natural is
    variable r: natural;
  begin
    r:=(3500000*2)-1;
    -- synthesis translate_off
    r:=8192;
    -- synthesis translate_on
    return r;
  end function;

  type regs_type is record
    state           : state_type;
    chunk_size      : unsigned(15 downto 0);
    cnt             : unsigned(15 downto 0);
    pilot_dly       : natural range 0 to PILOT_HEADER;
    gap_dly         : natural range 0 to GAP_DELAY;
    bitindex        : natural range 0 to 7;
    chunk_type      : std_logic_vector(7 downto 0);
  end record;

  signal pulse_ready_s  : std_logic;
  signal pulse_rd_s     : std_logic;
  --signal pulse_idle_s   : std_logic;
  signal pulse_data_r   : std_logic_vector(1 downto 0);
  signal pulse_data_s   : std_logic_vector(1 downto 0);

  signal r: regs_type;

begin

  pulse_inst: entity work.tap_pulse
  port map (
    clk_i     => clk_i,
    arst_i    => arst_i,
    tstate_i  => tstate_i,

    ready_i   => pulse_ready_s,
    data_i    => pulse_data_r,
    rd_o      => pulse_rd_s,
--    idle_o    => pulse_idle_s,
    audio_o   => audio_o
  );

  process(clk_i)
  begin
    if rising_edge(clk_i) then
      if pulse_rd_s='1' then
        pulse_data_r <= pulse_data_s;
      end if;
    end if;
  end process;

  process(clk_i,arst_i,r,enable_i,ready_i, pulse_rd_s, restart_i, data_i, tstate_i)
    variable w: regs_type;
  begin
    w     := r;
    rd_o   <= '0';
    pulse_ready_s <= '0';
    pulse_data_s <= "XX";

    case r.state is
      when IDLE =>
        rd_o <= ready_i AND enable_i;
        if enable_i='1' and restart_i='0' and ready_i='1' then
          w.state := LOAD_SIZE_LSB;
        end if;
      when LOAD_SIZE_LSB =>
        rd_o <= ready_i AND enable_i;
        if ready_i='1' and enable_i='1' then
          w.state := LOAD_SIZE_MSB;
          w.chunk_size(7 downto 0) := unsigned(data_i);
        end if;
        if restart_i='1' then
          w.state := IDLE;
        end if;

      when LOAD_SIZE_MSB =>
        rd_o <= ready_i AND enable_i;
        if ready_i='1' and enable_i='1' then
          w.state := LOAD_TYPE;
          w.chunk_size(15 downto 8) := unsigned(data_i);
        end if;
        if restart_i='1' then
          w.state := IDLE;
        end if;

      when LOAD_TYPE =>
        rd_o <= '0';
        if ready_i='1' and enable_i='1' and tstate_i='1' then
          w.state := SEND_PILOT;
          w.chunk_type(7 downto 0) := data_i;
          if data_i=x"00" then
            w.pilot_dly := PILOT_HEADER;
          else
            w.pilot_dly := PILOT_DATA;
          end if;
        end if;
        if restart_i='1' then
          w.state := IDLE;
        end if;

      when SEND_PILOT =>
        if r.pilot_dly=0 then
          pulse_data_s    <= "01"; -- Sync
        else
          pulse_data_s    <= "00"; -- Pilot
        end if;
        pulse_ready_s   <= '1';
        if pulse_rd_s='1' then
          if r.pilot_dly=0 then
            w.state := PLAY_DATA;
            w.bitindex := 7;
          else
            w.pilot_dly := r.pilot_dly - 1;
          end if;
        end if;

      when PLAY_DATA  =>
        pulse_data_s <= "1" & data_i(r.bitindex);
        pulse_ready_s <= '1';
        rd_o <= '0';
        if pulse_rd_s='1' then
          if r.bitindex=0 then
            if r.chunk_size=0 then
              w.state := GAP;
              w.gap_dly := GAP_DELAY;
            else
              w.bitindex := 7;
              --rd_o <= '1';
              rd_o <= ready_i AND enable_i;
              if ready_i='0' or enable_i='0' then
                w.state := IDLE;
              end if;

              w.chunk_size := r.chunk_size - 1;
            end if;
          else
            w.bitindex := r.bitindex-1;
          end if;
        end if;

      when GAP =>
        pulse_ready_s <= '0';
        if tstate_i='1' then
          if r.gap_dly=0 then
            w.state := LOAD_SIZE_LSB;
          else
            w.gap_dly := r.gap_dly - 1;
          end if;
        end if;
        if ready_i='0' or enable_i='0' then
          w.state := IDLE;
        end if;
      when others =>
      report "Uninmplemneted" severity failure;

    end case;

    if enable_i='0' then
      w.state := IDLE;
      pulse_ready_s <= '0';
    end if;

    if arst_i='1' then
      r.state <= IDLE;
    elsif rising_edge(clk_i) then
      r <= w;
    end if;

  end process;

end beh;

