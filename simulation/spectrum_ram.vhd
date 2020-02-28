library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

library work;
use work.txt_util.all;

-- 48K RAM

entity spectrum_ram is
  port (
    MREQ_n          : in std_logic;
    RD_n            : in std_logic;
    WR_n            : in std_logic;
    A               : in std_logic_vector(15 downto 0);
    D               : inout std_logic_vector(7 downto 0)
  );
end entity spectrum_ram;

architecture sim of spectrum_ram is

  --subtype mem_type is std_logic_vector(7 downto 0);
  type mem_type is array(0 to 48*1024) of std_logic_vector(7 downto 0);
  shared variable ram:  mem_type := (others => (others =>'0'));

  signal valid_address: std_logic;

  signal WR_delayed_s:  std_logic := '1';
begin

  valid_address <= '1' when A(15 downto 14)/="00" else '0';


  WR_delayed_s<=transport WR_n after 100 ns;

  process(RD_n, WR_delayed_s, MREQ_n, valid_address)
    variable ram_address: natural;
  begin
    D   <= (others => 'Z');
    if MREQ_n='0' and valid_address='1' then
      if RD_n='0' then
          report "RAM read " & hstr(a);
          ram_address := to_integer(unsigned(A)) - 16384;
          D <= ram(ram_address);
      elsif WR_delayed_s='0' then
          ram_address := to_integer(unsigned(A)) - 16384;
          ram(ram_address) := D;-- after 50 ns;
          report "RAM write " &str(ram_address) & " val "&hstr(D);
      end if;
    else
    end if;
  end process;

end sim;
