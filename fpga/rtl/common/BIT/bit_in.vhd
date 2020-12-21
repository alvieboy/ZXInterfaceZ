LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;
LIBRARY work;
use work.zxinterfacepkg.all;

entity bit_in is
  generic (
    WIDTH: natural := 1;
    START: natural := 0;
    SYNC: boolean := false
  );
  port (
    clk_i             : in std_logic;
    data_i            : in std_logic_vector(WIDTH-1 downto 0);
    --bit_from_cpu_i    : in bit_from_cpu_t;
    bit_to_cpu_o      : inout bit_to_cpu_t
  );
end entity bit_in;

architecture beh of bit_in is

  signal data_s       : std_logic_vector(WIDTH-1 downto 0);

begin

  s: if SYNC generate

  process(clk_i)
  begin
    if rising_edge(clk_i) then
      data_s <= data_i;
    end if;
  end process;

  end generate;

  ns: if not SYNC generate
    data_s <= data_i;
  end generate;
  
  bit_to_cpu_o.bit_data(START+WIDTH-1 downto START) <= data_s;


end beh;
