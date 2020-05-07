LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

LIBRARY work;
use work.bfm_ula_p.all;
use work.txt_util.all;


entity bfm_ula is
  port (
    Cmd_i   : in Cmd_Ula_type;
    Data_o  : out Data_Ula_type;

    A_i     : in std_logic_vector(15 downto 0);
    D_io    : inout std_logic_vector(7 downto 0);
    IOREQn_i: in std_logic;
    RDn_i   : in std_logic;
    WRn_i   : in std_logic;
    OEn_i   : in std_logic
  );
end entity bfm_ula;

architecture sim of bfm_ula is

  signal d_s: std_logic_vector(7 downto 0);
  signal d_q: std_logic_vector(7 downto 0);

  subtype memword_type is std_logic_vector(4 downto 0);

  type mem_type is array(0 to 7) of memword_type;

  shared variable keyboard: mem_type := (others => (others => '1'));
  signal audio_r  : std_logic := '0';

begin                                                         

  D_io <= (others => 'Z') when Cmd_i.Enabled=false or A_i(0)/='0' or IOREQn_i='1' OR RDn_i='1' OR OEn_i='1' else d_s;

  process(A_i, IOREQn_i, RDn_i)
    variable a: natural;
    variable k: std_logic_vector(4 downto 0);
  begin
    if A_i(0)='0' AND IOREQn_i='0' AND RDn_i='0' AND OEn_i='0' then
      -- Iterate through all high address bytes
      k := (others => '1');
      for i in 0 to 7 loop
        if A_i(8+i)='0' then
          k := k AND keyboard(i);
        end if;
      end loop;
      d_s <= (others => 'X'), "1"&audio_r&"1" & k after 285.7 ns;

    else
      d_s <= (others => 'Z');
    end if;

    if A_i(0)='0' AND IOREQn_i='0' AND WRn_i='0' then
      d_q <= D_io;
    end if;

  end process;

  process
  begin
  report "Wait";
    wait on Cmd_i.Cmd;
    if Cmd_i.Cmd = SETKBDDATA then
      report "Writing keyboard " &str(Cmd_i.KeybAddr)&" data "&hstr(Cmd_i.KeybScan);
      keyboard(Cmd_i.KeybAddr) := Cmd_i.KeybScan;
    end if;
    if Cmd_i.Cmd = SETAUDIODATA then
      audio_r <= Cmd_i.Audio;
    end if;
  end process;

  Data_o.Data <= d_q;

end sim;
