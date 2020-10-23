LIBRARY ieee;
USE ieee.std_logic_1164.all;

ENTITY clockmux IS
	PORT
	(
      --inclk3x   : in  std_logic := 'X'; -- inclk1x
      --inclk2x   : in  std_logic := 'X'; -- inclk1x
      inclk1x   : in  std_logic := 'X'; -- inclk1x
			inclk0x   : in  std_logic := 'X'; -- inclk0x
--			clkselect : in  std_logic_vector(1 downto 0) := (others =>'X'); -- clkselect
      clkselect : in  std_logic := 'X'; -- clkselect
			outclk    : out std_logic         -- outclk
	);
END clockmux;

architecture sim of clockmux is

begin

  
 -- with clkselect select outclk <=
 --   inclk0x when "00",
 --   inclk1x when "01",
 --   inclk2x when "10",
 --   inclk3x when "11",
 --   inclk0x when others;

  with clkselect select outclk <=
    inclk0x when '0',
    inclk1x when '1',
    inclk0x when others;
    --clkselect='0' else inclk1x;

end sim;
