library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity tb_SN74LCX245FT is
end entity;

architecture sim of tb_SN74LCX245FT is

  signal a_s    : std_logic_vector(7 downto 0);
  signal b_s    : std_logic_vector(7 downto 0);
  signal oe_s   : std_logic;
  signal dir_s  : std_logic;

begin

  uut: entity work.SN74LCX245FT
  port map (
    A_io  => a_s,
    B_io  => b_s,
    nOE_i => oe_s,
    DIR_i => dir_s
  );


  stimuli: process
  begin
    oe_s <= '0';
    dir_s <= '1';
    wait for 20 ns;
    oe_s <= '1';
    wait for 20 ns;
    a_s <= x"55";
    b_s <= x"AA";
    dir_s <= '0'; -- B to A
    oe_s <= '0';
    wait for 20 ns; -- Should yield X on A
    a_s <= (others => 'Z');
    wait for 10 ns; -- Should yield "AA" in A port.
    dir_s <= '1'; -- A to B
    a_s <= x"CC";
    wait for 20 ns; -- Should yield X on B
    b_s <= (others => 'Z');
    wait for 20 ns;  -- Should yield "CC" in B port.

    wait;
  end process;

end sim;
