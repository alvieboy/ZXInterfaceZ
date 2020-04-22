library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.numeric_std.all;

entity async_pulse2 is
  port (
    clki_i  : in std_logic;
    clko_i  : in std_logic;
    arst_i  : in std_logic;
    pulse_i : in std_logic;
    pulse_o : out std_logic
  );
end entity async_pulse2;

architecture behave of async_pulse2 is

   SIGNAL iq_r    : std_logic;
   SIGNAL oq1_r   : std_logic;
   SIGNAL oq2_r   : std_logic;
   SIGNAL oq3_r   : std_logic;

begin

  process(clki_i, arst_i)
  begin
    if arst_i='1' then
      iq_r  <= '0';
    elsif rising_edge(clki_i) then
      if pulse_i='1' then
        iq_r  <= not iq_r;
      end if;
    end if;
  end process;

  process(clko_i, arst_i)
  begin
    if arst_i='1' then
      oq1_r   <= '0';
      oq2_r   <= '0';
      oq3_r   <= '0';
    elsif rising_edge(clko_i) then
      oq1_r   <= iq_r;
      oq2_r   <= oq1_r;
      oq3_r   <= oq2_r;
    end if;
  end process;

  pulse_o <= oq2_r xor oq3_r;

end behave;

