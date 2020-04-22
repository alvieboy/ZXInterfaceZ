library IEEE;   
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
library work;
use work.zxinterfacepkg.all;
-- synthesis translate_off
use work.txt_util.all;
-- synthesis translate_on

entity border_capture is
  generic (
    CAPTURE_SLOTS_32:  natural := 64
  );
  port (
    clk_i         : in std_logic;
    arst_i        : in std_logic;

    border_ear_i  : in std_logic_vector(3 downto 0);
    start_delay_i : in std_logic_vector(7 downto 0);
    seq_o         : out std_logic_vector(2 downto 0);
    off_o         : out natural;
    intr_i        : std_logic -- 50Hz interrupt
  );
end entity border_capture;

architecture beh of border_capture is

  signal shreg32_r    : std_logic_vector(31 downto 0);    -- 8 nibbles
  signal seq_r        : natural range 0 to 7;
  signal captick_s    : std_logic;
  signal run_r        : std_logic;
  signal store_r      : std_logic;
  signal storeslot_r  : std_logic;

  signal store_off_r: natural range 0 to CAPTURE_SLOTS_32-1;

  type mem_type is array(0 to (2*CAPTURE_SLOTS_32)-1) of std_logic_vector(31 downto 0);
  shared variable mem: mem_type;

  constant TICK_DELAY: natural := ((CLK_KHZ*1000)/(50*CAPTURE_SLOTS_32*8));

  signal tick_r       : natural range 0 to TICK_DELAY-1;
  signal start_delay_r: unsigned(7 downto 0);

  -- For reporting.

  signal seq_report_r: std_logic_vector(2 downto 0);
  signal off_report_r: natural;

begin

  seq_o <= seq_report_r;
  off_o <= off_report_r;

  -- Generate tick

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      tick_r    <= TICK_DELAY-1;
    elsif rising_edge(clk_i) then
      if tick_r=0 then
        tick_r    <= TICK_DELAY-1;
      else
        tick_r    <= tick_r - 1;
      end if;
    end if;
  end process;
  captick_s <= '1' when tick_r=0 else '0';
  --run_s     <= '1';

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      seq_r   <= 0;
      store_r <= '0';
      start_delay_r <= unsigned(start_delay_i);--(others => '0');
    elsif rising_edge(clk_i) then
      store_r <= '0';
      if captick_s='1' and run_r='1' then
        if start_delay_r/=0 then
          start_delay_r <= start_delay_r -1;
        -- synthesis translate_off
          report "Delay " & hstr(std_logic_vector(start_delay_r));
        -- synthesis translate_on
        else
          shreg32_r(4*(seq_r+1)-1 downto 4*seq_r) <= border_ear_i;
          if seq_r/=7 then
            seq_r <= seq_r + 1;
          else
            store_r <= '1';
            seq_r <= 0;
          end if;
        end if;
      end if;

      if intr_i='1' then
        seq_r<=0;
        seq_report_r <= std_logic_vector(to_unsigned(seq_r,3));
        start_delay_r <= unsigned(start_delay_i);
        if run_r='1' then
          -- Still running. Write last words.
          store_r <= '1';
        end if;
        -- synthesis translate_off
        report "Border intr: run="&str(run_r)&" seq "& str(seq_r);
        -- synthesis translate_on
        --store_r<='0';
      end if;
    end if;
  end process;

  --store_s <='1' when captick_s='1' and run_s='1' and seq_r=7 else '0';

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      store_off_r   <= 0;
      storeslot_r   <= '0';
      run_r         <= '1';
    elsif rising_edge(clk_i) then
      if store_r='1' and run_r='1' then
        if store_off_r=CAPTURE_SLOTS_32-1 then
          run_r <='0';
        else
          store_off_r <= store_off_r + 1;
        end if;
      end if;
      if intr_i='1' then
        run_r <= '1';
        -- synthesis translate_off
        report "Border intr: storeoff " & str(store_off_r);
        -- synthesis translate_on
        storeslot_r <= not storeslot_r;
        store_off_r <= 0;
        off_report_r <= store_off_r;
      end if;
    end if;
  end process;

  process(clk_i, arst_i)
   variable storeoff: natural range 0 to (2*CAPTURE_SLOTS_32)-1;
  begin
    if rising_edge(clk_i) then
      if store_r='1' and run_r='1' then
        if storeslot_r='1' then
          storeoff := store_off_r + CAPTURE_SLOTS_32;
        else
          storeoff := store_off_r;
        end if;
        mem(storeoff) := shreg32_r;
      end if;
    end if;
  end process;

end beh;