library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity obuf is
  generic (
    WIDTH: natural := 1;
    tOE:  time := 3 ns;
    tOP:   time := 3 ns;
    tIP:   time := 3 ns
  );
  port (
    i_i     : in std_logic_vector(WIDTH-1 downto 0);
    pad_o  :  out std_logic_vector(WIDTH-1 downto 0)
  );
end entity obuf;

architecture io of obuf is

  signal i_dly_s :   std_logic_vector(WIDTH-1 downto 0);
  signal oe_dly_s:   std_logic_vector(WIDTH-1 downto 0);
begin

  pad_o <= transport i_i after tOP;

end io;
