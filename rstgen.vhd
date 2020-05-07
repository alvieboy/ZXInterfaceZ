library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity rstgen is
  generic (
    POLARITY : std_logic := '1'
  );
  port (
    arst_i      : in std_logic;
    clk_i       : in std_logic;
    rst_o       : out std_logic
  );
end entity rstgen;

architecture beh of rstgen is

  signal rstq1_r  : std_logic := '1';
  signal rstq2_r  : std_logic := '1';

begin

  process(arst_i, clk_i)
  begin
    if arst_i=POLARITY then
      rstq1_r <= '1';
      rstq2_r <= '1';
    elsif rising_edge(clk_i) then
      rstq1_r <= '0';
      rstq2_r <= rstq1_r;
    end if;
  end process;

  rst_o <= rstq2_r;

end beh;
