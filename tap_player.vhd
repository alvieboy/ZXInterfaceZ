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

    valid_i   : in std_logic;
    data_i    : in std_logic_vector(8 downto 0);
    rd_o      : out std_logic;

    audio_o   : out std_logic
);
end entity tap_player;


architecture beh of tap_player is
  

  type state_type is (
    IDLE,
    LOAD_CMD_DATA0,
    EXEC_CMD,
    PULSE,
    GAP,
    PLAY_DATA
  );

  type regs_type is record
    state           : state_type;
    chunk_size      : unsigned(23 downto 0);
    cnt             : unsigned(15 downto 0);
    repeat          : unsigned(15 downto 0);
    gap             : unsigned(15 downto 0);
    pulse_len       : std_logic_vector(11 downto 0);
    pulse_logic1    : std_logic_vector(11 downto 0);
    pulse_logic0    : std_logic_vector(11 downto 0);
    bitindex        : natural range 0 to 7;
    chunk_type      : std_logic_vector(7 downto 0);
    data            : std_logic_vector(7 downto 0);
    pulse_data      : std_logic_vector(7 downto 0); -- LSB of pulse
    last_byte_len   : unsigned(2 downto 0);
  end record;

  signal pulse_ready_s  : std_logic;
  signal pulse_busy_s   : std_logic;
  signal pulse_data_s   : std_logic_vector(3 downto 0);
  signal pulse_len_s    : std_logic_vector(11 downto 0);
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

    valid_i   => pulse_ready_s,
    data_i    => pulse_data_s,
    busy_o    => pulse_busy_s,
    len_i     => pulse_len_s,
    audio_o   => audio_o
  );

  process(clk_i,arst_i,r,enable_i,valid_i, pulse_busy_s, restart_i, data_i, tstate_i)
    variable w: regs_type;
    variable wait_pulse: boolean;
  begin
    w     := r;
    rd_o          <= '0';
    pulse_ready_s <= '0';
    pulse_len_s     <= (others => 'X');

    if enable_i='1' then

      case r.state is
        when IDLE =>

          w.bitindex := 7;

          if valid_i='1' then
            w.data   := data_i(7 downto 0);
            if data_i(8)='1' then
              -- Command access
              if data_i(7)='1' then
                w.state := LOAD_CMD_DATA0;
              else
                -- Simple command without arguments
              end if;
              rd_o <= '1';
            else
              w.state := PLAY_DATA;
            end if;
          end if; -- Valid_i


        when LOAD_CMD_DATA0 =>
          rd_o <= '1';
          if valid_i='1' then
            w.pulse_data(7 downto 0) := data_i(7 downto 0);
            w.state := EXEC_CMD;
          end if;

        when EXEC_CMD =>

          if valid_i='1' then

            case r.data(2 downto 0) is
              when "000"    =>
                w.pulse_logic0:= data_i(3 downto 0) & r.pulse_data;
                w.state       := IDLE;
              when "001"    =>
                w.pulse_logic1:= data_i(3 downto 0) & r.pulse_data;
                w.state       := IDLE;
              when "010"    =>
                w.cnt         := unsigned( data_i(7 downto 0) & r.pulse_data);
                w.state       := GAP;
              when "011"    =>
                w.chunk_size(15 downto 0)   := unsigned(data_i(7 downto 0) & r.pulse_data);
                w.state       := IDLE;
              when "100"    =>
                w.chunk_size(23 downto 16)  := unsigned(r.pulse_data);
                w.last_byte_len             := unsigned(data_i(2 downto 0));
                w.state       := IDLE;
              when "110"    =>
                w.pulse_len   := data_i(3 downto 0) & r.pulse_data;
                w.state       := PULSE;
                w.cnt         := r.repeat;

              when "101"    =>
                w.repeat    := unsigned( data_i(7 downto 0) & r.pulse_data);
                w.state       := IDLE;

              when others =>
                w.state := IDLE;
            end case;

            rd_o <= '1';

          end if;

        when PULSE =>
          
          pulse_data_s    <= PULSE_SINGLE;
          pulse_ready_s   <= '1';
          pulse_len_s     <= r.pulse_len;

          if pulse_busy_s='0' then
            if r.repeat=0 then
              w.state := IDLE;
            else
              w.repeat := r.repeat - 1;
            end if;
          end if;
  
        when PLAY_DATA  =>

          pulse_data_s    <= PULSE_DUAL;
          pulse_ready_s   <= '1';

          if data_i(r.bitindex)='1' then
            pulse_len_s   <= r.pulse_logic1;
          else
            pulse_len_s   <= r.pulse_logic0;
          end if;

          if pulse_busy_s='0' then
            if (r.bitindex=0 or (r.chunk_size=0 and r.bitindex=r.last_byte_len)) then -- Last bit being played.
              rd_o  <= '1';
              if r.chunk_size=0 then
             -- pulse_ready_s <= '0';
                w.state := IDLE;
              else
                w.bitindex := 7;
                w.chunk_size := r.chunk_size - 1;
              end if;
            else
              w.bitindex := r.bitindex-1;
            end if;
          end if;
  
        when GAP =>
          if r.cnt=0 then
            w.state := IDLE;
          elsif tick_1ms_s='1' then
            w.cnt := r.cnt - 1;
          end if;

        when others =>
        report "Uninmplemented" severity failure;
  
      end case;

    else -- enable_i='0'
      w.state := IDLE;
      pulse_ready_s <= '0';
    end if;

    if arst_i='1' then
      r.state <= IDLE;

      --r.pilot_header_len <= to_unsigned(8063,12);
      --r.pilot_data_len   <= to_unsigned(3223,12);
      --r.gap_len          <= to_unsigned(2000,12);
    elsif rising_edge(clk_i) then
      r <= w;
    end if;

  end process;


end beh;

