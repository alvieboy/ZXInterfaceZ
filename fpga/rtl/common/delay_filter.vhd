library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity delay_filter is
  generic (
    RESET   : std_logic := '1';
    DELAY   : natural := 16
  );
  port (
    clk_i   : in std_logic;
    arst_i  : in std_logic;
    i_i     : in std_logic;
    o_o     : out std_logic
  );
end entity delay_filter;

architecture beh of delay_filter is

  signal q_r:   natural range 0 to DELAY-1 := DELAY-1;
  signal o_r:   std_logic;

begin

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      q_r <= DELAY-1;
      o_r <= '0';
    elsif rising_edge(clk_i) then
      if i_i=RESET then
        o_r <= '0';
        q_r <= DELAY-1;
      else
        if q_r=0 then
          o_r <= '1';
        else
          o_r <= '0';
          q_r <= q_r - 1;
        end if;
      end if;
    end if;
  end process;

  o_o   <= o_r;

end beh;
