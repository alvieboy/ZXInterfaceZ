LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

LIBRARY work;
use work.txt_util.all;

package bfm_rom_p is

  type RomCmd is (
    NONE,
    SETMEM
  );

  --type Data_Rom_type is record
  --  Busy      : boolean;
  --end record;

  type Cmd_Rom_type is record
    Cmd       : RomCmd;
    Enabled   : boolean;
    Address   : natural range 0 to 16383;
    Data      : std_logic_vector(7 downto 0);
  end record;

  component bfm_rom is
    port (
      Cmd_i   : in Cmd_Rom_type;
      --Data_o  : out Data_Rom_type;
      A_i     : in std_logic_vector(15 downto 0);
      D_o     : out std_logic_vector(7 downto 0);
      MREQn_i : in std_logic;
      RDn_i   : in std_logic;
      OEn_i   : in std_logic
    );
  end component bfm_rom;

  constant Cmd_Rom_Defaults: Cmd_Rom_type := (
    Cmd     => NONE,
    Enabled => false,
    Address => 0,
    Data    => x"00"
  );

  procedure romWrite(signal Cmd: out Cmd_Rom_type; a: in std_logic_vector(15 downto 0); d: in std_logic_vector(7 downto 0));
  procedure romWrite(signal Cmd: out Cmd_Rom_type; a: in natural; d: in std_logic_vector(7 downto 0));
  

end package;

package body bfm_rom_p is

  procedure romWrite(signal Cmd: out Cmd_Rom_type; a: in std_logic_vector(15 downto 0); d: in std_logic_vector(7 downto 0))
  is
  begin
    romWrite(Cmd, to_integer(unsigned(a)), d);
  end procedure;

  procedure romWrite(signal Cmd: out Cmd_Rom_type; a: in natural; d: in std_logic_vector(7 downto 0))
  is
  begin
    Cmd.Enabled <= true;
    Cmd.Address <= a;
    Cmd.Data    <= d;
    Cmd.Cmd     <= SETMEM;
    wait for 0 ps;
    Cmd.Cmd     <= NONE;
    wait for 0 ps;
  end procedure;

end package body;

