LIBRARY ieee;
USE ieee.std_logic_1164.all;

ENTITY clockmux IS
	PORT
	(
      inclk1x   : in  std_logic := 'X'; -- inclk1x
			inclk0x   : in  std_logic := 'X'; -- inclk0x
			clkselect : in  std_logic := 'X'; -- clkselect
			outclk    : out std_logic         -- outclk
	);
END clockmux;

architecture sim of clockmux is

begin

  outclk <= inclk0x when clkselect='0' else inclk1x;

end sim;
