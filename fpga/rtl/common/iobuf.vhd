library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity iobuf is
  generic (
    WIDTH: natural := 1;
    tOE:  time := 3 ns;
    tOP:   time := 3 ns;
    tIP:   time := 3 ns
  );
  port (
    i_i     : in std_logic_vector(WIDTH-1 downto 0);
    o_o     : out std_logic_vector(WIDTH-1 downto 0);
    pad_io  : inout std_logic_vector(WIDTH-1 downto 0);
    oe_i    : in std_logic_vector(WIDTH-1 downto 0)
  );
end entity iobuf;

architecture io of iobuf is

  signal i_dly_s :   std_logic_vector(WIDTH-1 downto 0);
  signal o_dly_s :   std_logic_vector(WIDTH-1 downto 0);
  signal oe_dly_s:   std_logic_vector(WIDTH-1 downto 0);
begin

  i_dly_s <= transport pad_io after tIP;
  o_dly_s <= transport i_i after tOP;
  oe_dly_s <= transport oe_i after tOE;

  o_o     <= i_dly_s;
  g:for i in 0 to WIDTH-1 generate
    pad_io(i)  <= o_dly_s(i) when oe_dly_s(i)='1' else 'Z';
  end generate;
end io;
