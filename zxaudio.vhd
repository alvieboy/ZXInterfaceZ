library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_misc.all;
use ieee.numeric_std.all;

entity zxaudio is
  port (
    clk_i   : in std_logic;
    arst_i  : in std_logic;
    ear_i   : in std_logic;
    mic_i   : in std_logic;
    -- YM
    dat_i     : in std_logic_vector(7 downto 0);
    dat_o     : out std_logic_vector(7 downto 0);
    adr_i     : in std_logic_vector(3 downto 0);
    we_i      : in std_logic;
    rd_i      : in std_logic;
    -- Volume control
    left_vol_0_i  : in std_logic_vector(15 downto 0);
    right_vol_0_i : in std_logic_vector(15 downto 0);
    left_vol_1_i  : in std_logic_vector(15 downto 0);
    right_vol_1_i : in std_logic_vector(15 downto 0);
    left_vol_2_i  : in std_logic_vector(15 downto 0);
    right_vol_2_i : in std_logic_vector(15 downto 0);
    left_vol_3_i  : in std_logic_vector(15 downto 0);
    right_vol_3_i : in std_logic_vector(15 downto 0);

    audio_left_o : out std_logic;
    audio_right_o : out std_logic
  );
end entity zxaudio;


architecture beh of zxaudio is

  signal audio_final_left_r      : signed(9 downto 0);
  signal audio_final_right_r      : signed(9 downto 0);
  signal arstn_s      : std_logic;
  signal dacclken_r   : std_logic;
  signal ymclken_r    : std_logic;

  signal ym_audio_a_s   : std_logic_vector(7 downto 0);
  signal ym_audio_b_s   : std_logic_vector(7 downto 0);
  signal ym_audio_c_s   : std_logic_vector(7 downto 0);
  signal mic_audio_s    : std_logic_vector(7 downto 0);

  --signal mixed_s      : unsigned(9 downto 0);

  constant DACDIV     : natural := 64;
  signal clkd_r       : natural range 0 to DACDIV-1;
  constant YMDIV      : natural := 54;
  signal ymclkd_r     : natural range 0 to YMDIV-1;

  signal mixsel_r       : unsigned(1 downto 0);
  signal mixsel_zero_s  : std_logic;
  signal audio_mix_left_s    : std_logic_vector(32 downto 0);
  signal audio_mix_right_s   : std_logic_vector(32 downto 0);
  signal audio_mix_in_s : std_logic_vector(7 downto 0);
  signal ym_update_s    : std_logic;
  signal mix_valid_r1   : std_logic;
  signal mix_valid_r2   : std_logic;

  signal left_vol_s     : std_logic_vector(15 downto 0);
  signal right_vol_s     : std_logic_vector(15 downto 0);
begin

  arstn_s <= not arst_i;

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      clkd_r <= DACDIV-1;
      ymclkd_r <= YMDIV-1;
      dacclken_r <='0';
      ymclken_r  <='0';
    elsif rising_edge(clk_i) then
      dacclken_r <='0';
      ymclken_r  <='0';
      if clkd_r=0 then
        dacclken_r <='1';
        clkd_r <= DACDIV-1;
      else
        clkd_r <= clkd_r - 1;
      end if;

      if ymclkd_r=0 then
        ymclken_r <='1';
        ymclkd_r <= YMDIV-1;
      else
        ymclkd_r <= ymclkd_r - 1;
      end if;
    end if;
  end process;



  ay_inst: entity work.ym2149
  port map (
    clk_i     => clk_i,
    clken_i   => ymclken_r,
    update_o  => ym_update_s,
    arst_i    => arst_i,
    dat_i     => dat_i,
    dat_o     => dat_o,
    adr_i     => adr_i,
    we_i      => we_i,
    rd_i      => rd_i,
    audio_a_o => ym_audio_a_s,
    audio_b_o => ym_audio_b_s,
    audio_c_o => ym_audio_c_s
  );

  mic_audio_s <= "00000000" when mic_i='0' else "11111111";

  --mixed_s <= unsigned("00"&ym_audio_a) + unsigned("00"&ym_audio_b) + unsigned("00"&ym_audio_c);

  -- Mixer
  
  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      mixsel_r <= "00";
      mix_valid_r1 <= '0';
      mix_valid_r2 <= '0';
    elsif rising_edge(clk_i) then
      if ym_update_s='1' then
        if mixsel_r="11" then
          mixsel_r <= (others => '0');
        else
          mixsel_r <= mixsel_r + 1;
        end if;
        mix_valid_r2 <= mix_valid_r1;
        mix_valid_r1 <= mixsel_zero_s;
      end if;
    end if;
  end process;

  mixsel_zero_s <= '1' when mixsel_r=0 else '0';

  process(mixsel_r, ym_audio_a_s, ym_audio_b_s, ym_audio_c_s, mic_audio_s)
  begin
    case mixsel_r is
      when "00" =>  audio_mix_in_s  <= ym_audio_a_s;
                    left_vol_s      <= left_vol_0_i;
                    right_vol_s     <= right_vol_0_i;
      when "01" =>  audio_mix_in_s  <= ym_audio_b_s;
                    left_vol_s      <= left_vol_1_i;
                    right_vol_s     <= right_vol_1_i;
      when "10" =>  audio_mix_in_s  <= ym_audio_c_s;
                    left_vol_s      <= left_vol_2_i;
                    right_vol_s     <= right_vol_2_i;
      when "11" =>  audio_mix_in_s  <= mic_audio_s;
                    left_vol_s      <= left_vol_3_i;
                    right_vol_s     <= right_vol_3_i;
      when others =>
    end case;
  end process;

  amixer_left: entity work.audiomult
	port map
	(
		accum_sload		=> mixsel_zero_s,
		clock0		    => clk_i,
		dataa(15 downto 8) 	=> x"00",
    dataa(7 downto 0) => audio_mix_in_s,
		datab		      => left_vol_s,
		ena0		      => ym_update_s,
		result		    => audio_mix_left_s
	);

  amixer_right: entity work.audiomult
	port map
	(
		accum_sload		=> mixsel_zero_s,
		clock0		    => clk_i,
		dataa(15 downto 8) 	=> x"00",
    dataa(7 downto 0) => audio_mix_in_s,
		datab		      => right_vol_s,
		ena0		      => ym_update_s,
		result		    => audio_mix_right_s
	);

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      audio_final_left_r <= (others => '0');
      audio_final_right_r <= (others => '0');
    elsif rising_edge(clk_i) then
      if ym_update_s='1' then
        if mix_valid_r2='1' then
          if or_reduce(audio_mix_left_s(32 downto 18))='1' then
            -- Clip. I think this can be done with the multiplier instead.
            audio_final_left_r <= (others => '1');
          else
            audio_final_left_r <= signed(audio_mix_left_s(17 downto 8));
          end if;

          if or_reduce(audio_mix_right_s(32 downto 18))='1' then
            -- Clip. I think this can be done with the multiplier instead.
            audio_final_right_r <= (others => '1');
          else
            audio_final_right_r <= signed(audio_mix_left_s(17 downto 8));
          end if;
        end if;
      end if;
    end if;
  end process;


  dac_left_inst: entity work.dac_dsm3
  generic map (
    nbits   => 10
  )
  port map(
    din     => audio_final_left_r,
    dout    => audio_left_o,
    clk     => clk_i,
    clk_ena => dacclken_r,
    n_rst   => arstn_s
  );

  dac_right_inst: entity work.dac_dsm3
  generic map (
    nbits   => 10
  )
  port map(
    din     => audio_final_right_r,
    dout    => audio_right_o,
    clk     => clk_i,
    clk_ena => dacclken_r,
    n_rst   => arstn_s
  );

end beh;
