LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

LIBRARY work;
use work.txt_util.all;

package bfm_qspiram_p is

  type QSPIRamCmd is (
    NONE,
    SETMEM
  );

  --type Data_QSPIRam_type is record
  --  Busy      : boolean;
  --end record;

  type Cmd_QSPIRam_type is record
    Cmd       : QSPIRamCmd;
    Enabled   : boolean;
    Address   : natural range 0 to 65535;
    Data      : std_logic_vector(7 downto 0);
  end record;

  component bfm_qspiram is
    port (
      Cmd_i   : in Cmd_QSPIRam_type;
      --Data_o  : out Data_QSPIRam_type;
      Clk_i   : in std_logic;
      D_io    : inout std_logic_vector(3 downto 0);
      CSn_i   : in std_logic
    );
  end component bfm_qspiram;

  constant Cmd_QSPIRam_Defaults: Cmd_QSPIRam_type := (
    Cmd     => NONE,
    Enabled => false,
    Address => 0,
    Data    => x"00"
  );

  procedure qspiramWrite(signal Cmd: out Cmd_QSPIRam_type; a: in std_logic_vector(15 downto 0); d: in std_logic_vector(7 downto 0));
  procedure qspiramWrite(signal Cmd: out Cmd_QSPIRam_type; a: in natural; d: in std_logic_vector(7 downto 0));
  

end package;

package body bfm_qspiram_p is

  procedure qspiramWrite(signal Cmd: out Cmd_QSPIRam_type; a: in std_logic_vector(15 downto 0); d: in std_logic_vector(7 downto 0))
  is
  begin
    qspiramWrite(Cmd, to_integer(unsigned(a)), d);
  end procedure;

  procedure qspiramWrite(signal Cmd: out Cmd_QSPIRam_type; a: in natural; d: in std_logic_vector(7 downto 0))
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

