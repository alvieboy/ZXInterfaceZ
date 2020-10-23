library ieee;
use ieee.std_logic_1164.all;

entity oddrff is
  port (
    CLK:  in std_ulogic;
    D0:   in std_logic;
    D1:   in std_logic;
    O:   out std_ulogic
  );

end entity oddrff;

architecture sim of oddrff is

begin
  process(CLK)
  begin
    if rising_edge(CLK) then
      O <= D0;
    elsif falling_edge(CLK) then
      O <= D1;
    end if;
  end process;

end sim;
