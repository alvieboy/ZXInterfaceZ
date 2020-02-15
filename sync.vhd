library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity sync is
  GENERIC (
    RESET: std_logic
  );
  port (
    clk_i   : in std_logic;
    arst_i  : in std_logic;
    din_i   : in std_logic;
    dout_o  : out std_logic
  );
end entity sync;

architecture beh of sync is

  signal q1_r, q2_r: std_logic;

begin

  process(arst_i, clk_i)
  begin
    if arst_i='1' then
      q1_r <= RESET;
      q2_r <= RESET;
    elsif rising_edge(clk_i) then
      q2_r <= q1_r;
      q1_r <= din_i;
    end if;
  end process;

  dout_o <= q2_r;

end beh;

