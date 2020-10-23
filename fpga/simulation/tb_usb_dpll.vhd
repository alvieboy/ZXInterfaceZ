library ieee;
use ieee.std_logic_1164.all;

entity tb_usb_dpll is
end entity tb_usb_dpll;

architecture sim of tb_usb_dpll is

  signal clk48_s: std_logic := '0';
  signal rst48_s: std_logic := '0';
  signal rstn_s: std_logic := '0';

  signal clk_test_s: std_logic := '0';

  signal en_s         : std_logic;
  signal data_s       : std_logic;
  signal data_usb_s   : std_logic;
  signal tick_s       : std_logic;
  signal speed_s      : std_logic;
  
  signal clken_s      : boolean := false;

  constant PERIOD     : time := 1 ms / 48000;

  signal DATA_PERIOD : time := 1 ms / 12000;

begin

 -- 12.00±0.03 Mbit/s, and 1.50±0.18 Mbit/s.
  process
  begin
    if clken_s then
      clk48_s <= '1';
      wait for PERIOD/2;
      clk48_s <= '0';
      wait for PERIOD/2;
    else
      wait on clken_s;
    end if;
  end process;


  uut: entity work.usb_dpll
  generic map (
    COMPENSATION_DELAY => 3
  )
  port map (
    clk_i     => clk48_s,
    arstn_i   => rstn_s,
    en_i      => en_s,
    data_i    => data_s,

    speed_i   => speed_s,

    tick_o    => tick_s
  );

  rstn_s <= not rst48_s;

  process(clk48_s)
  begin
    if rising_edge(clk48_s) then
      data_s <= data_usb_s;
    end if;
  end process;


  stim: process
    procedure stream(s: in std_logic_vector) is
    begin
      l: for i in s'LOW to s'HIGH loop
        data_usb_s <= s(i);
        wait for DATA_PERIOD;
      end loop;
    end stream;
  begin
    clken_s <= true;
    speed_s <= '1';
    en_s    <= '1';
    wait for 0 ps;
    rst48_s <= '1';
    wait for 40 ns;
    rst48_s <= '0';

    stream("010101010101010000001");

    wait for 1 us;
    en_s    <= '0';
    wait for 1 us;
    en_s    <= '1';
    wait for 1 us;

    DATA_PERIOD <= 1 ms / 11970;

    stream("01010101010101000001");

    wait for 1 us;
    en_s    <= '0';
    wait for 1 us;
    en_s    <= '1';
    wait for 1 us;

    DATA_PERIOD <= 1 ms / 12030;

    stream("01010101010101000001");

    wait for 1 us;
    en_s    <= '0';
    wait for 1 us;
    speed_s <= '0';
    en_s    <= '1';
    wait for 1 us;

    DATA_PERIOD <= 1 ms / 1320;
    wait for 1 ns;

    stream("01010101010101000001");

    wait for 1 us;
    en_s    <= '0';
    wait for 1 us;
    en_s    <= '1';
    wait for 1 us;

    DATA_PERIOD <= 1 ms / 1680;
    wait for 1 ns;

    stream("01010101010101000001");

    wait for 1 us;
    en_s    <= '0';
    wait for 1 us;
    en_s    <= '1';
    wait for 1 us;

    DATA_PERIOD <= 1 ms / 1500;
    wait for 1 ns;

    stream("01010101010101000001");

    wait for 1 us;
    en_s    <= '0';
    wait for 1 us;
    en_s    <= '1';
    wait for 1 us;

    -- Finish
    clken_s <= false;
    wait;
  end process;

end sim;
