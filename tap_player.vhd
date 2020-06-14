library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
library work;
use work.tappkg.all;

entity tap_player is
  port (
    clk_i     : in std_logic;
    arst_i    : in std_logic;
    tstate_i  : in std_logic; -- '1' each 3.5Mhz
    enable_i  : in std_logic;
    restart_i : in std_logic;

    ready_i   : in std_logic;
    data_i    : in std_logic_vector(8 downto 0);
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
    LOAD_CMD,
    LOAD_CMD2,
    DISPATCHCMD,
    SEND_PILOT,
    GAP,
    PLAY_DATA
  );

  type regs_type is record
    state           : state_type;
    chunk_size      : unsigned(15 downto 0);
    cnt             : unsigned(15 downto 0);
    pilot_dly       : unsigned(11 downto 0);
    gap_dly         : unsigned(11 downto 0);
    bitindex        : natural range 0 to 7;
    chunk_type      : std_logic_vector(7 downto 0);
    pulse_data      : std_logic_vector(11 downto 0);
    pilot_header_len : unsigned(11 downto 0);
    pilot_data_len  : unsigned(11 downto 0);
    gap_len         : unsigned(11 downto 0);
  end record;

  signal pulse_ready_s  : std_logic;
  signal pulse_rd_s     : std_logic;
  --signal pulse_idle_s   : std_logic;
  signal pulse_data_r   : std_logic_vector(3 downto 0);
  signal pulse_data_s   : std_logic_vector(3 downto 0);
  signal pulse_custom_s : std_logic_vector(11 downto 0);
  signal tick_1ms_s     : std_logic;
  signal tick_1ms_cnt   : natural range 0 to 96000-1;
  signal r: regs_type;

begin

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      tick_1ms_cnt <= 96000-1;
      tick_1ms_s <= '0';
    elsif rising_edge(clk_i) then
      tick_1ms_s <= '0';
      if tick_1ms_cnt=0 then
        tick_1ms_s <= '1';
        tick_1ms_cnt <= 96000-1;
      else
        tick_1ms_cnt <= tick_1ms_cnt -1;
      end if;
    end if;
  end process;

  pulse_inst: entity work.tap_pulse
  port map (
    clk_i     => clk_i,
    arst_i    => arst_i,
    tstate_i  => tstate_i,

    ready_i   => pulse_ready_s,
    data_i    => pulse_data_r,
    rd_o      => pulse_rd_s,
    len_i     => pulse_custom_s,
--    idle_o    => pulse_idle_s,
    audio_o   => audio_o
  );

  pulse_custom_s <= std_logic_vector(r.pulse_data);

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
    pulse_data_s <= (others => 'X');

    case r.state is
      when IDLE =>
        rd_o <= ready_i AND enable_i;
        if enable_i='1' and restart_i='0' and ready_i='1' then
          w.state := LOAD_SIZE_LSB;
        end if;

      when LOAD_CMD =>
        rd_o <= ready_i AND enable_i;
        if ready_i='1' and enable_i='1' then
          case r.chunk_size(7 downto 0) is
            when x"01" | x"02" | x"03" | x"04" | x"05" | x"06" | x"07" | x"08" | x"09"=> -- Set pulses
              w.pulse_data(7 downto 0) := data_i(7 downto 0);
              w.state := LOAD_CMD2;
            when others =>
          end case;
        end if;

      when LOAD_CMD2 =>
        --rd_o <= ready_i AND enable_i;
        if ready_i='1' and enable_i='1' then
          case r.chunk_size(7 downto 0) is
            when x"01" | x"02" | x"03" | x"04" | x"05" | x"06" | x"07" | x"08" | x"09"=> -- Set pulses
              w.pulse_data(11 downto 8) := data_i(3 downto 0);
              w.state := DISPATCHCMD;
            when others =>

          end case;
        end if;

      when DISPATCHCMD =>
        rd_o <= ready_i AND enable_i;
        if ready_i='1' and enable_i='1' then
          case r.chunk_size(7 downto 0) is
            when x"01"  =>  pulse_data_s <= SET_PULSE_PILOT;-- Set PILOT pulse
                            pulse_ready_s <= '1';
            when x"02"  =>  pulse_data_s <= SET_PULSE_SYNC0;
                            pulse_ready_s <= '1';
            when x"03"  =>  pulse_data_s <= SET_PULSE_SYNC1;
                            pulse_ready_s <= '1';
            when x"04"  =>  pulse_data_s <= SET_PULSE_LOGIC0;
                            pulse_ready_s <= '1';
            when x"05"  =>  pulse_data_s <= SET_PULSE_LOGIC1;
                            pulse_ready_s <= '1';
            when x"06"  =>  pulse_data_s <= SET_PULSE_TAP; -- Reset to defaults
                            pulse_ready_s <= '1';
                            w.pilot_header_len := to_unsigned(8063,12);
                            w.pilot_data_len   := to_unsigned(3223,12);
                            w.gap_len          := to_unsigned(2000,12);

            when x"07"  =>  w.pilot_header_len := unsigned(r.pulse_data);
                            w.state := LOAD_SIZE_LSB;
            when x"08"  =>  w.pilot_data_len := unsigned(r.pulse_data);
                            w.state := LOAD_SIZE_LSB;
            when x"09"  =>  w.gap_len := unsigned(r.pulse_data);
                            w.state := LOAD_SIZE_LSB;
            when others =>

          end case;
          if pulse_rd_s='1' then
            w.state := LOAD_SIZE_LSB;
          end if;
        end if;
        

      when LOAD_SIZE_LSB =>
        rd_o <= ready_i AND enable_i;
        if ready_i='1' and enable_i='1' then
          -- This is reused for command parsing.
          w.chunk_size(7 downto 0) := unsigned(data_i(7 downto 0));
          if data_i(8)='0' then
            w.state := LOAD_SIZE_MSB;
          else
            w.state := LOAD_CMD;
          end if;
        end if;
        if restart_i='1' then
          w.state := IDLE;
        end if;

      when LOAD_SIZE_MSB =>
        rd_o <= ready_i AND enable_i;
        if ready_i='1' and enable_i='1' then
          w.state := LOAD_TYPE;
          w.chunk_size(15 downto 8) := unsigned(data_i(7 downto 0));
        end if;
        if restart_i='1' then
          w.state := IDLE;
        end if;

      when LOAD_TYPE =>
        rd_o <= '0';
        if ready_i='1' and enable_i='1' and tstate_i='1' then
          w.state := SEND_PILOT;
          w.chunk_type(7 downto 0) := data_i(7 downto 0);
          if data_i=x"00" then
            w.pilot_dly := unsigned(r.pilot_header_len);
          else
            w.pilot_dly := unsigned(r.pilot_data_len);
          end if;
        end if;
        if restart_i='1' then
          w.state := IDLE;
        end if;

      when SEND_PILOT =>
        if r.pilot_dly=0 then
          pulse_data_s    <= PULSE_SYNC; -- Sync
        else
          pulse_data_s    <= PULSE_PILOT; -- Pilot
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
        if data_i(r.bitindex)='1' then
          pulse_data_s <= PULSE_LOGIC1;
        else
          pulse_data_s <= PULSE_LOGIC0;
        end if;
        pulse_ready_s <= '1';
        rd_o <= '0';
        if pulse_rd_s='1' then
          if r.bitindex=0 then
            if r.chunk_size=0 then
              w.state := GAP;
              w.gap_dly := r.gap_len;
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
          elsif tick_1ms_s='1' then
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

      r.pilot_header_len <= to_unsigned(8063,12);
      r.pilot_data_len   <= to_unsigned(3223,12);
      r.gap_len          <= to_unsigned(2000,12);

    elsif rising_edge(clk_i) then
      r <= w;
    end if;

  end process;

end beh;

