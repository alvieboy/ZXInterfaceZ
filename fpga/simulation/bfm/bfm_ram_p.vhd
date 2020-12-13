LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

LIBRARY work;
use work.txt_util.all;

package bfm_ram_p is

  type RamCmd is (
    NONE,
    SETMEM
  );

  type Cmd_Ram_type is record
    Cmd       : RamCmd;
    Enabled   : boolean;
    Address   : natural range 0 to 16383;
    Data      : std_logic_vector(7 downto 0);
  end record;

  component bfm_ram is
    port (
      Cmd_i   : in Cmd_Ram_type;
      --Data_o  : out Data_Ram_type;
      A_i     : in std_logic_vector(15 downto 0);
      D_o     : inout std_logic_vector(7 downto 0);
      MREQn_i : in std_logic;
      RDn_i   : in std_logic;
      WRn_i   : in std_logic
    );
  end component bfm_ram;

  constant Cmd_Ram_Defaults: Cmd_Ram_type := (
    Cmd     => NONE,
    Enabled => false,
    Address => 0,
    Data    => x"00"
  );

  procedure ramWrite(signal Cmd: out Cmd_Ram_type; a: in std_logic_vector(15 downto 0); d: in std_logic_vector(7 downto 0));
  procedure ramWrite(signal Cmd: out Cmd_Ram_type; a: in natural; d: in std_logic_vector(7 downto 0));
  

end package;

package body bfm_ram_p is

  procedure ramWrite(signal Cmd: out Cmd_Ram_type; a: in std_logic_vector(15 downto 0); d: in std_logic_vector(7 downto 0))
  is
  begin
    ramWrite(Cmd, to_integer(unsigned(a)), d);
  end procedure;

  procedure ramWrite(signal Cmd: out Cmd_Ram_type; a: in natural; d: in std_logic_vector(7 downto 0))
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

