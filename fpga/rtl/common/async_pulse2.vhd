library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.numeric_std.all;

entity async_pulse2 is
  generic (
    WIDTH : natural := 3
  );
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
   SIGNAL oq_r    : std_logic_vector(1 to WIDTH);

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
      l: for i in 1 to WIDTH loop
        oq_r(i)   <= '0';
      end loop;
    elsif rising_edge(clko_i) then
      oq_r(1)   <= iq_r;
      l1: for i in 2 to WIDTH loop
        oq_r(i) <= oq_r(i-1);
      end loop;
    end if;
  end process;

  pulse_o <= oq_r(oq_r'HIGH-1)  xor oq_r(oq_r'HIGH);

end behave;

