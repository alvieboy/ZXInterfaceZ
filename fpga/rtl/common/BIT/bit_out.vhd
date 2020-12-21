LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;
LIBRARY work;
use work.zxinterfacepkg.all;

entity bit_out is
  generic (
    WIDTH: natural := 1;
    START: natural := 0
  );
  port (
    --clk_i             : in std_logic;
    --arst_i            : in std_logic;
    data_o            : out std_logic_vector(WIDTH-1 downto 0);
    data_i            : in std_logic_vector(WIDTH-1 downto 0);
    bit_from_cpu_i    : in bit_from_cpu_t
  );
end entity bit_out;

architecture beh of bit_out is

begin


  data_o <= bit_from_cpu_i.bit_data(START+WIDTH-1 downto START)
    when bit_from_cpu_i.bit_enable='1' else data_i;


end beh;
