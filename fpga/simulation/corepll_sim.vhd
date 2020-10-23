LIBRARY ieee;
USE ieee.std_logic_1164.all;

ENTITY corepll IS
	PORT
	(
		inclk0		: IN STD_LOGIC  := '0';
		c0		    : OUT STD_LOGIC ;
		c1		    : OUT STD_LOGIC ;
    c2		    : OUT STD_LOGIC ;
    c3		    : OUT STD_LOGIC ;
    c4		    : OUT STD_LOGIC ;
		locked		: OUT STD_LOGIC 
	);
END corepll;

ARCHITECTURE sim of corepll IS

  signal clk_s:   std_logic := '0';

  signal meas_period      : time := 0 ps;
  signal mainclk_period   : time := 0 ps;
  signal locked_in_s      : std_logic := '0';

  signal activity   :  natural := 0;
  signal l_activity :  natural := 0;
  signal l_count    :  natural := 0;


  signal videoclk0_period : time := 0 ps;
  signal videoclk1_period : time := 0 ps;
  signal usbclk_period    : time := 0 ps;

  signal videoclk0_s    : std_logic := '0';
  signal videoclk1_s    : std_logic := '0';
  signal usbclk_s       : std_logic := '0';
begin

  -- Measure input clock
  process
    variable t1, t2, delta: time;
  begin
    wait on inclk0;

    l1: loop
      wait until rising_edge(inclk0) for 50 ns;
      if not (inclk0'event) then
        locked_in_s <= '0';
        exit l1;
      end if;
      t1 := now;
      wait until rising_edge(inclk0) for 50 ns;
      if not (inclk0'event) then
        locked_in_s <= '0';
        exit l1;
        end if;
      t2 := now;
      activity <= activity + 1;
      delta := t2 - t1;
      if delta=meas_period then
        meas_period       <= delta;
        mainclk_period    <= delta / 3;
        videoclk0_period  <= delta * 4 / 5;
        videoclk1_period  <= delta * 17 / 15;
        usbclk_period     <= delta * 2 / 3;
        --report "Detected clock period " & time'image(delta);
        locked_in_s <= '1';
      else
        meas_period   <= delta;
        report "Lock lost " &time'image(delta);
        locked_in_s <= '0';
      end if;
    end loop;
  end process;


  process
  begin
    if locked_in_s='0' then
      clk_s <= '0';
     -- report "Wait for clock";
      wait on locked_in_s;
    else
      clk_s <= '0';
      wait for mainclk_period/2;
      clk_s <= '1';
      wait for mainclk_period/2;
      --clk_s <= '0';
      end if;
  end process;

  process
  begin
    if locked_in_s='0' then
      videoclk0_s <= '0';
     -- report "Wait for clock";
      wait on locked_in_s;
    else
      videoclk0_s <= '0';
      wait for videoclk0_period/2;
      videoclk0_s <= '1';
      wait for videoclk0_period/2;
      end if;
  end process;

  process
  begin
    if locked_in_s='0' then
      videoclk1_s <= '0';
     -- report "Wait for clock";
      wait on locked_in_s;
    else
      videoclk1_s <= '0';
      wait for videoclk1_period/2;
      videoclk1_s <= '1';
      wait for videoclk1_period/2;
      end if;
  end process;

  process
  begin
    if locked_in_s='0' then
      usbclk_s <= '0';
     -- report "Wait for clock";
      wait on locked_in_s;
    else
      usbclk_s <= '0';
      wait for usbclk_period/2;
      usbclk_s <= '1';
      wait for usbclk_period/2;
      end if;
  end process;

  process(clk_s, locked_in_s)
  begin
    if locked_in_s='0' then
      locked <= '0';
    elsif rising_edge(clk_s) then
      locked <= '1';
    end if;
  end process;
  
  c0 <= clk_s;
  c1 <= usbclk_s;--transport clk_s after 3 ns;
  c3 <= videoclk0_s;
  c4 <= videoclk1_s;
end sim;
