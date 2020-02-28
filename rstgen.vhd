library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity rstgen is
  port (
    arstn_i     : in std_logic;
    clk_i       : in std_logic;
    rst_o       : out std_logic
  );
end entity rstgen;

architecture beh of rstgen is

  signal rstq1_r  : std_logic := '0';
  signal rstq2_r  : std_logic := '0';

begin

  process(arstn_i, clk_i)
  begin
    if arstn_i='0' then
      rstq1_r <= '1';
      rstq2_r <= '1';
    elsif rising_edge(clk_i) then
      rstq1_r <= '0';
      rstq2_r <= rstq1_r;
    end if;
  end process;

  rst_o <= rstq2_r;

end beh;
