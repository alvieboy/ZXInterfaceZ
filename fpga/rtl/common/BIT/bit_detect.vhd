LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;
LIBRARY work;
use work.zxinterfacepkg.all;
-- synthesis translate_off
use work.txt_util.all;
-- synthesis translate_on
entity bit_detect is
  port (
    clk_i       : in std_logic;
    arst_i      : in std_logic;
    det_i       : in std_logic;
    bit_o       : out std_logic
  );
end entity bit_detect;

architecture beh of bit_detect is

    -- 976.5625Hz

  constant C_EXPECTED_FREQUENCY_HZ  : natural := 976;
    -- 98460.538  (975)
    -- 98359.656  (976)
    -- 98258.98   (977)

  constant C_TOLERANCE_CYCLES       : natural := 100;
  constant C_EXPECTED_CYCLES        : natural := ((C_CLK_KHZ*1000)/C_EXPECTED_FREQUENCY_HZ) - 1;


  constant C_EXPECTED_PERIODS       : natural := 7;

  
  signal det_r        : std_logic;
  signal det_sync_s   : std_logic;
  signal rise_event_s : std_logic;


  signal bit_r        : std_logic;

  function LOW return natural is
    variable r: natural;
  begin
    r := C_EXPECTED_CYCLES - C_TOLERANCE_CYCLES;
    -- synthesis translate_off
    r := 9;
    -- synthesis translate_on
    return r;
  end function;

  function HIGH return natural is
    variable r: natural;
  begin
    r := C_EXPECTED_CYCLES + C_TOLERANCE_CYCLES;
    -- synthesis translate_off
    r := 11;
    -- synthesis translate_on

    return r;
  end function;

  signal counter_r    : natural range 0 to HIGH + 1;
  signal periodcnt_r  : natural range 0 to C_EXPECTED_PERIODS - 1;

begin

  syncin: entity work.sync
    generic map (
      RESET   => '0',
      STAGES  => 2
    ) port map (
      clk_i   => clk_i,
      arst_i  => arst_i,
      din_i   => det_i,
      dout_o  => det_sync_s
    );

  process(clk_i,arst_i)
  begin
    if arst_i='1' then
      det_r <= '0';
    elsif rising_edge(clk_i) then
      det_r <= det_sync_s;
    end if;
  end process;

  rise_event_s <= det_sync_s and not det_r;

  process(clk_i,arst_i)
  begin
    if arst_i='1' then
      counter_r <= 0;
      bit_r <='0';
    elsif rising_edge(clk_i) then
      if rise_event_s='1' then
        if counter_r >= LOW and counter_r <= HIGH then
          -- Good period
          -- synthesis translate_off
          if bit_r='0' then
            report "Good period " & str(counter_r) & " counter now " & str(periodcnt_r);
          end if;
          -- synthesis translate_on
          if periodcnt_r = C_EXPECTED_PERIODS-1 then
            bit_r <= '1'; -- Enter BIT mode
          else
            periodcnt_r <= periodcnt_r+1;
          end if;
        else
          -- Out of bounds period.
          -- synthesis translate_off
          if bit_r='0' then
            report "Bad period " & str(counter_r);
          end if;
          -- synthesis translate_on

          periodcnt_r <= 0;
        end if;
        counter_r <= 0;
      else
        if counter_r /= HIGH+1 then
          counter_r <= counter_r + 1;
        end if;
      end if;
    end if;
  end process;

  bit_o <= bit_r;

end beh;