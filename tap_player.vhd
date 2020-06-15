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
    LOAD_SIZE_LSB,
    LOAD_SIZE_MSB,
    LOAD_TYPE,
    LOAD_DATA0,
    LOAD_DATA1,
    SEND_PILOT,
    GAP,
    PLAY_DATA
  );

  type regs_type is record
    state           : state_type;
    chunk_size      : unsigned(23 downto 0);
    cnt             : unsigned(15 downto 0);
    pilot_dly       : unsigned(11 downto 0);
    gap_dly         : unsigned(11 downto 0);
    bitindex        : natural range 0 to 7;
    chunk_type      : std_logic_vector(7 downto 0);
    cmd             : std_logic_vector(6 downto 0);
    pulse_data      : std_logic_vector(7 downto 0); -- LSB of pulse
    pilot_header_len : unsigned(11 downto 0);
    pilot_data_len  : unsigned(11 downto 0);
    gap_len         : unsigned(11 downto 0);
    len_outside_blk : std_logic;
    last_byte_len   : unsigned(2 downto 0);
  end record;

  signal pulse_ready_s  : std_logic;
  signal pulse_busy_s   : std_logic;
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

    valid_i   => pulse_ready_s,
    data_i    => pulse_data_s,
    busy_o    => pulse_busy_s,
    len_i     => pulse_custom_s,
    audio_o   => audio_o
  );

  pulse_custom_s <= data_i(3 downto 0) & std_logic_vector(r.pulse_data);

  process(clk_i,arst_i,r,enable_i,valid_i, pulse_busy_s, restart_i, data_i, tstate_i)
    variable w: regs_type;
    variable wait_pulse: boolean;
  begin
    w     := r;
    rd_o          <= '0';
    pulse_ready_s <= '0';
    pulse_data_s  <= (others => 'X');
    wait_pulse    := false;

    if enable_i='1' then

      case r.state is
        when IDLE =>
          if valid_i='1' then
            if data_i(8)='1' then
              -- Command access
              if data_i(7)='0' then -- Pulse information
                w.state := LOAD_DATA0;
                w.cmd := data_i(6 downto 0);
              else
                -- Simple cmd
                case data_i(6 downto 0) is
                  when "0000000" =>
                    -- Reset to defaults
                    pulse_data_s        <= SET_PULSE_TAP; -- Reset to defaults
                    pulse_ready_s       <= '1';
                    wait_pulse          := true;
                    w.pilot_header_len  := to_unsigned(8063,12);
                    w.pilot_data_len    := to_unsigned(3223,12);
                    w.gap_len           := to_unsigned(2000,12);
                    w.len_outside_blk   := '0';

                  when "0000010" =>
                    w.len_outside_blk := '0';
                  when "0000011" =>
                    w.len_outside_blk := '1';

                  when others =>

                end case;
              end if;

              if wait_pulse then
                rd_o <= not pulse_busy_s;
              else
                rd_o <= '1';
              end if;

            else
              -- Data access
              if r.len_outside_blk='0' then
                w.chunk_size(7 downto 0) := unsigned(data_i(7 downto 0));
                w.chunk_size(23 downto 16) := (others => '0');
                w.state := LOAD_SIZE_MSB;
                rd_o <= '1';
              else
                -- This is data w/o size header.
                w.state := LOAD_TYPE;
                -- Don't remove from FIFO.
                rd_o <= '0';
              end if;

            end if;


          end if; -- Valid_i


        when LOAD_DATA0 =>
          rd_o <= '1';
          if valid_i='1' then
            w.pulse_data(7 downto 0) := data_i(7 downto 0);
            w.state := LOAD_DATA1;
          end if;

        when LOAD_DATA1 =>

          if valid_i='1' then
            pulse_ready_s <= not r.cmd(3); -- 0 to 7 are handled by pulse generator

            case r.cmd(2 downto 0) is
              when "000"  =>  pulse_data_s        <= SET_PULSE_PILOT;-- Set PILOT pulse
              when "001"  =>  pulse_data_s        <= SET_PULSE_SYNC0;
              when "010"  =>  pulse_data_s        <= SET_PULSE_SYNC1;
              when "011"  =>  pulse_data_s        <= SET_PULSE_LOGIC0;
              when others =>  pulse_data_s        <= SET_PULSE_LOGIC1;
            end case;

            if r.cmd(3)='1' then
              -- Internal configuration
              case r.cmd(2 downto 0) is
                when "000"    =>  w.pilot_header_len      := unsigned(data_i(3 downto 0) & r.pulse_data);
                when "001"    =>  w.pilot_data_len        := unsigned(data_i(3 downto 0) & r.pulse_data);
                when "010"    =>  w.gap_len               := unsigned(data_i(3 downto 0) & r.pulse_data);
                when "011"    =>  w.chunk_size(15 downto 0) := unsigned(data_i(7 downto 0) & r.pulse_data);
                when "100"    =>  w.chunk_size(23 downto 16) := unsigned(r.pulse_data);
                                  w.last_byte_len         := unsigned(data_i(2 downto 0));
                when others =>
              end case;

            end if;

            if r.cmd(3)='0' then
              rd_o <= not pulse_busy_s;
              w.state := IDLE;
            else
              rd_o <= '1';
              w.state := IDLE;
            end if;

          end if;



        when LOAD_SIZE_MSB =>
          rd_o <= '1';
          if valid_i='1' then
            w.state := LOAD_TYPE;
            w.chunk_size(15 downto 8) := unsigned(data_i(7 downto 0));
          end if;
          if restart_i='1' then
            w.state := IDLE;
          end if;
  
        when LOAD_TYPE =>

          if tstate_i='1' then
            if valid_i='1' then
              rd_o <= '1';
              w.state := SEND_PILOT;
              w.chunk_type(7 downto 0) := data_i(7 downto 0);
              if data_i=x"00" then
                w.pilot_dly := unsigned(r.pilot_header_len);
              else
                w.pilot_dly := unsigned(r.pilot_data_len);
              end if;
            end if;
          end if;
  
        when SEND_PILOT =>
          if r.pilot_dly=0 then
            pulse_data_s    <= PULSE_SYNC; -- Sync
          else
            pulse_data_s    <= PULSE_PILOT; -- Pilot
          end if;
          pulse_ready_s   <= '1';

          if pulse_busy_s='0' then
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

          if pulse_busy_s='0' then
            if (r.bitindex=0 or (r.chunk_size=0 and r.bitindex=r.last_byte_len)) then -- Last bit being played.
              if r.chunk_size=0 then
                w.state := GAP;
                w.gap_dly := r.gap_len;
              else
                w.bitindex := 7;
                rd_o  <= '1';
                w.chunk_size := r.chunk_size - 1;
              end if;
            else
              w.bitindex := r.bitindex-1;
            end if;
          end if;
  
        when GAP =>
          if tstate_i='1' then
            if r.gap_dly=0 then

              w.state := LOAD_SIZE_LSB;
            elsif tick_1ms_s='1' then
              w.gap_dly := r.gap_dly - 1;
            end if;
          end if;

        when others =>
        report "Uninmplemneted" severity failure;
  
      end case;

    else -- enable_i='0'
      w.state := IDLE;
      pulse_ready_s <= '0';
    end if;

    if arst_i='1' then
      r.state <= IDLE;

      r.pilot_header_len <= to_unsigned(8063,12);
      r.pilot_data_len   <= to_unsigned(3223,12);
      r.gap_len          <= to_unsigned(2000,12);
      r.len_outside_blk  <= '0';
    elsif rising_edge(clk_i) then
      r <= w;
    end if;

  end process;

end beh;

