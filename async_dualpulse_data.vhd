library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.numeric_std.all;

entity async_dualpulse_data is
  generic (
    WIDTH : natural := 3;
    DWIDTH: natural := 8
  );
  port (
    clki_i  : in std_logic;
    clko_i  : in std_logic;
    arst_i  : in std_logic;
    pulse_i : in std_logic_vector(1 downto 0);
    data_i  : in std_logic_vector(DWIDTH-1 downto 0);
    pulse_o : out std_logic_vector(1 downto 0);
    data_o  : out std_logic_vector(DWIDTH-1 downto 0)
  );
end entity async_dualpulse_data;

architecture behave of async_dualpulse_data is

  SIGNAL iq0_r    : std_logic;
  SIGNAL iq1_r    : std_logic;
  SIGNAL oq0_r    : std_logic_vector(1 to WIDTH);
  SIGNAL oq1_r    : std_logic_vector(1 to WIDTH);

  type data_q_type is array(1 to WIDTH) of std_logic_vector(DWIDTH-1 downto 0);

  signal dq_r   : data_q_type;
  signal idq_r   : std_logic_vector(DWIDTH-1 downto 0);
begin

  process(clki_i, arst_i)
  begin
    if arst_i='1' then
      iq0_r  <= '0';
      iq1_r  <= '0';
    elsif rising_edge(clki_i) then
      if pulse_i(0)='1' then
        iq0_r    <= not iq0_r;
      end if;
      if pulse_i(1)='1' then
        iq1_r    <= not iq1_r;
      end if;
      if pulse_i(0)='1' or pulse_i(1)='1' then
        idq_r   <= data_i;
      end if;
    end if;
  end process;

  process(clko_i, arst_i)
  begin
    if arst_i='1' then
      l: for i in 1 to WIDTH loop
        oq0_r(i)   <= '0';
        oq1_r(i)   <= '0';
      end loop;
    elsif rising_edge(clko_i) then
      oq0_r(1)   <= iq0_r;
      oq1_r(1)   <= iq1_r;
      dq_r(1)   <= idq_r;
      l1: for i in 2 to WIDTH loop
        oq0_r(i) <= oq0_r(i-1);
        oq1_r(i) <= oq1_r(i-1);
        dq_r(i) <= dq_r(i-1);
      end loop;
    end if;
  end process;

  pulse_o(0) <= oq0_r(oq0_r'HIGH-1)  xor oq0_r(oq0_r'HIGH);
  pulse_o(1) <= oq1_r(oq1_r'HIGH-1)  xor oq1_r(oq1_r'HIGH);

  data_o <= dq_r(WIDTH-1);

end behave;

