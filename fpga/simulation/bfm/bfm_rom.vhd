LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

LIBRARY work;
use work.bfm_rom_p.all;
use work.txt_util.all;


entity bfm_rom is
  port (
    Cmd_i   : in Cmd_Rom_type;

    A_i     : in std_logic_vector(15 downto 0);
    MREQn_i : in std_logic;
    D_o     : out std_logic_vector(7 downto 0);
    RDn_i   : in std_logic;
    OEn_i   : in std_logic
  );
end entity bfm_rom;

architecture sim of bfm_rom is

  signal d_s: std_logic_vector(7 downto 0);
  subtype memword_type is std_logic_vector(7 downto 0);
  type mem_type is array(0 to 16383) of memword_type;
  shared variable rom: mem_type := (others => (others => '0'));

begin                                                         

  D_o <= (others => 'Z') when Cmd_i.Enabled=false or A_i(15 downto 14)/="00" or MREQn_i='1' OR RDn_i='1' OR OEn_i='1' else d_s;

  process(A_i, MREQn_i, RDn_i)
    variable a: natural;
  begin
    if A_i(15 downto 14)="00" AND MREQn_i='0' AND RDn_i='0' AND OEn_i='0' then
      a := to_integer(unsigned(A_i(13 downto 0)));
      report "Reading memory " &str(a);
      d_s <= rom(a );
    else
      d_s <= (others => 'Z');
    end if;
  end process;

  process
  begin
--  report "Wait";
    wait on Cmd_i.Cmd;
    if Cmd_i.Cmd = SETMEM then
      report "Writing memory " &str(Cmd_i.Address)&" data "&hstr(Cmd_i.Data);
      rom( Cmd_i.Address ) := Cmd_i.Data;
    end if;
  end process;
end sim;
