library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.numeric_std.all;

entity async_pulse_data is
  generic (
    WIDTH : natural := 3;
    DWIDTH: natural := 8
  );
  port (
    clki_i  : in std_logic;
    clko_i  : in std_logic;
    arst_i  : in std_logic;
    pulse_i : in std_logic;
    data_i  : in std_logic_vector(DWIDTH-1 downto 0);
    pulse_o : out std_logic;
    data_o  : out std_logic_vector(DWIDTH-1 downto 0)
  );
end entity async_pulse_data;

architecture behave of async_pulse_data is

  SIGNAL iq_r    : std_logic;
  SIGNAL oq_r    : std_logic_vector(0 to WIDTH);

  type data_q_type is array(1 to WIDTH) of std_logic_vector(DWIDTH-1 downto 0);

  signal dq_r   : data_q_type;
  signal idq_r   : std_logic_vector(DWIDTH-1 downto 0);
begin

  process(clki_i, arst_i)
  begin
    if arst_i='1' then
      iq_r  <= '0';
    elsif rising_edge(clki_i) then
      if pulse_i='1' then
        iq_r    <= not iq_r;
        idq_r   <= data_i;
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
      dq_r(1)   <= idq_r;
      l1: for i in 2 to WIDTH loop
        oq_r(i) <= oq_r(i-1);
        dq_r(i) <= dq_r(i-1);
      end loop;
    end if;
  end process;

  pulse_o <= oq_r(oq_r'HIGH-1)  xor oq_r(oq_r'HIGH);
  data_o <= dq_r(WIDTH-2);

end behave;

