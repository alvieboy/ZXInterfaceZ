LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

LIBRARY work;
use work.bfm_ram_p.all;
use work.txt_util.all;


entity bfm_ram is
  port (
    Cmd_i   : in Cmd_Ram_type;

    A_i     : in std_logic_vector(15 downto 0);
    MREQn_i : in std_logic;
    D_io    : inout std_logic_vector(7 downto 0);
    RDn_i   : in std_logic;
    WRn_i   : in std_logic
  );
end entity bfm_ram;

architecture sim of bfm_ram is

  signal d_s: std_logic_vector(7 downto 0);
  subtype memword_type is std_logic_vector(7 downto 0);
  type mem_type is array(0 to 49152) of memword_type;

  shared variable ram: mem_type := (others => (others => '0'));

  signal is_ram_access: std_logic;
  signal ram_addr: unsigned(15 downto 0);

begin                                                         

  process(A_i)
  begin
    if not(is_x(A_i)) then
    if A_i(15 downto 14)/="00" then
      is_ram_access <= '1';
      ram_addr <= unsigned(A_i) - x"4000";
    else
      is_ram_access <= '0';
    end if;
    end if;
  end process;

  D_io <= (others => 'Z') when Cmd_i.Enabled=false or is_ram_access='0' or MREQn_i='1' OR RDn_i='1' else d_s;

  process(is_ram_access, ram_addr, MREQn_i, RDn_i, WRn_i)
    variable a: natural;
  begin
    if is_ram_access='1' AND MREQn_i='0' AND RDn_i='0' then
      a := to_integer(ram_addr);
      report "Reading spectrum memory @" &hstr(std_logic_vector(ram_addr));
      d_s <= ram(a);
    elsif is_ram_access='1' AND MREQn_i='0' AND WRn_i='0' then
      a := to_integer(ram_addr);
      --wait for 300 ns;
      report "Writing spectrum memory @" &hstr(std_logic_vector(ram_addr))&": "&hstr(D_io);
      ram(a) := D_io;-- after 300 ns;
    end if;
  end process;

  process
  begin
  report "Wait";
    wait on Cmd_i.Cmd;
    if Cmd_i.Cmd = SETMEM then
      report "Writing memory " &str(Cmd_i.Address)&" data "&hstr(Cmd_i.Data);
      ram( Cmd_i.Address ) := Cmd_i.Data;
    end if;
  end process;
end sim;
