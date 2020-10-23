library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.numeric_std.all;

entity async_pulse is
  GENERIC (
      C_POLARITY  : STD_LOGIC := '1';
      C_EDGE      : STD_LOGIC := '1'
  );
  port (
    clk_i: in std_logic;
    rst_i: in std_logic;

    pulse_i: in std_logic;
    pulse_o: out std_logic
  );
end entity async_pulse;

architecture behave of async_pulse is

   signal q1_r,q2,q3,q4: std_logic := C_POLARITY;

   SIGNAL sync1_r    : STD_LOGIC;
   SIGNAL sync2_r    : STD_LOGIC;

begin

  -- Input FFs.
  process(clk_i, rst_i)
  begin
    if rst_i='1' then
      sync1_r  <= C_POLARITY;
      sync2_r  <= C_POLARITY;
      q1_r <= C_POLARITY;
    elsif rising_edge(clk_i) then
      sync1_r  <= pulse_i;
      sync2_r  <= sync1_r;
      q1_r     <= sync2_r;
    end if;
  end process;

  pulse_o <= (q1_r XOR sync2_r) AND (q1_r XOR C_EDGE);


end behave;

