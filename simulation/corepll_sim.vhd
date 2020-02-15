LIBRARY ieee;
USE ieee.std_logic_1164.all;

ENTITY corepll IS
	PORT
	(
		inclk0		: IN STD_LOGIC  := '0';
		c0		    : OUT STD_LOGIC ;
		c1		    : OUT STD_LOGIC ;
		locked		: OUT STD_LOGIC 
	);
END corepll;

ARCHITECTURE sim of corepll IS

  CONSTANT C_PERIOD: time := 1 ms / 96000;
  signal clk_s:   std_logic := '0';

begin

  clk_s <= not clk_s after C_PERIOD/2;

  process
  begin
    locked <= '0';
    wait for 200 ns;
    wait until rising_edge(clk_s);
    locked <= '1';
    wait;
  end process;

  c0 <= clk_s;
  c1 <= transport clk_s after 3 ns;
end sim;
