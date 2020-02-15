library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity SN74LCX245FT is
  port (
    A_io  : inout std_logic_vector(7 downto 0);
    B_io  : inout std_logic_vector(7 downto 0);
    nOE_i : in std_logic;
    DIR_i : in std_logic
  );
end entity SN74LCX245FT;

architecture sim of SN74LCX245FT is

  constant tPZx:  time := 8.5 ns;
  constant tPxZ:  time := 7.5 ns;
  constant tPD:   time := 7.0 ns;

  signal a_tris:  std_logic := '1';
  signal b_tris:  std_logic := '1';

  signal a_to_b: std_logic_vector(7 downto 0);
  signal b_to_a: std_logic_vector(7 downto 0);

begin

  a_to_b <= transport A_io after tPD;
  b_to_a <= transport B_io after tPD;

  process(nOE_i, DIR_i)
  begin
    if nOE_i='1' or nOE_i='H' then
      a_tris <= '1' after TPxZ;
      b_tris <= '1' after TPxZ;
    else
      if DIR_i'event AND DIR_i='0' then -- A output
        a_tris <= '0' after TPZx;
        b_tris <= '1' after TPxZ;
      else
        b_tris <= '0' after TPZx;
        a_tris <= '1' after TPxZ;
      end if;
    end if;
  end process;

  A_io <= (others =>'Z') when a_tris='1' else b_to_a;
  B_io <= (others =>'Z') when b_tris='1' else a_to_b;
  
end sim;
