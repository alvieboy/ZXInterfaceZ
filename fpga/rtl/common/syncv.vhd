library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity syncv is
  GENERIC (
    WIDTH: natural;
    RESET: std_logic
  );
  port (
    clk_i   : in std_logic;
    arst_i  : in std_logic;
    din_i   : in std_logic_vector(WIDTH-1 downto 0);
    dout_o  : out std_logic_vector(WIDTH-1 downto 0)
  );                                                  
end entity syncv;

architecture beh of syncv is

begin

  g1: for i in 0 to WIDTH-1  generate
    s_i: entity work.sync
      generic map ( RESET => RESET )
      port map (
        clk_i   => clk_i,
        arst_i  => arst_i,
        din_i   => din_i(i),
        dout_o  => dout_o(i)
      );
  end generate;

end beh;

