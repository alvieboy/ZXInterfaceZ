library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

entity busopdet is
  port (
    clk_i     : in std_logic;
    arst_i    : in std_logic;
    access_i  : in std_logic;

    latch_o   : out std_logic;
    access_o  : out std_logic
  );
end entity busopdet;

architecture beh of busopdet is

  signal a_q_r  : std_logic;
  signal latch_s: std_logic;
  signal pulse_r: std_logic;

begin

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      a_q_r <= '0';
      pulse_r <= '0';
    elsif rising_edge(clk_i) then
      a_q_r <= access_i;
      pulse_r <= latch_s;
    end if;
  end process;

  latch_s   <= access_i AND NOT a_q_r;

  latch_o   <= latch_s;
  access_o  <= pulse_r;

end beh;
