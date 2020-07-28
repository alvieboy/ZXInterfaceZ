LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.txt_util.all;
use work.bfm_reset_p.all;
use work.bfm_clock_p.all;

package tbc_device_p is

  shared variable errorCounter: natural := 0;

  procedure setDefaults(
    signal SysRst_Cmd_o    : out Cmd_Reset_type;
    signal SysClk_Cmd_o    : out Cmd_Clock_type;
    signal SpectRst_Cmd_o  : out Cmd_Reset_type;
    signal SpectClk_Cmd_o  : out Cmd_Clock_type
  );

  procedure PowerUpAndReset (
    signal SysRst_Cmd_o    : out Cmd_Reset_type;
    signal SysClk_Cmd_o    : out Cmd_Clock_type;
    signal SpectRst_Cmd_o  : out Cmd_Reset_type;
    signal SpectClk_Cmd_o  : out Cmd_Clock_type
  );

  procedure FinishTest(
    signal SysClk_Cmd_o    : out Cmd_Clock_type;
    signal SpectClk_Cmd_o  : out Cmd_Clock_type
  );

  procedure Check(
    what: in string;
    actual: in std_logic_vector;
    expected: in std_logic_vector);

  procedure Check(
    what: in string;
    actual: in std_logic;
    expected: in std_logic);

  procedure Check(
    what: in string;
    b: in boolean);

end package tbc_device_p;

package body tbc_device_p is

  procedure setDefaults(
    signal SysRst_Cmd_o    : out Cmd_Reset_type;
    signal SysClk_Cmd_o    : out Cmd_Clock_type;
    signal SpectRst_Cmd_o  : out Cmd_Reset_type;
    signal SpectClk_Cmd_o  : out Cmd_Clock_type
  ) is
  begin
    SysClk_Cmd_o    <= Cmd_Clock_Defaults;
    SysRst_Cmd_o    <= Cmd_Reset_Defaults;
    SpectClk_Cmd_o  <= Cmd_Clock_Defaults;
    SpectRst_Cmd_o  <= Cmd_Reset_Defaults;
  end procedure;

  procedure PowerUpAndReset (
    signal SysRst_Cmd_o    : out Cmd_Reset_type;
    signal SysClk_Cmd_o    : out Cmd_Clock_type;
    signal SpectRst_Cmd_o  : out Cmd_Reset_type;
    signal SpectClk_Cmd_o  : out Cmd_Clock_type
  ) is
  begin
    -- Standard power up and reset.
    errorCounter := 0;

    wait for 0 ps;
    SysClk_Cmd_o.Period     <= 31.25 ns;
    SysClk_Cmd_o.Enabled    <= true;

    SpectClk_Cmd_o.Period   <= 285.71 ns;
    SpectClk_Cmd_o.Enabled  <= true;
    wait for 1 us;
    -- Assert reset
    SysRst_Cmd_o.Reset_Time <= 50 ns;
    wait for 0 ps;

  end procedure;

  procedure FinishTest (
    signal SysClk_Cmd_o    : out Cmd_Clock_type;
    signal SpectClk_Cmd_o  : out Cmd_Clock_type
  ) is
  begin
    -- Stop all clocks
    SysClk_Cmd_o.Enabled    <= false;
    SpectClk_Cmd_o.Enabled  <= false;

    if errorCounter=0 then
      report "Overall test status: PASS";
    else
      report "Overall test status: ** FAIL ** (" &str(errorCounter)&" errors)";
    end if;
    wait;
  end procedure;


  procedure Check(
    what: in string;
    actual: in std_logic_vector;
    expected: in std_logic_vector) is
  begin
    report "chk1";
    if (actual /= expected) then
      report "CHECK FAILED: "&what&": expected " & hstr(expected)& ", got " &hstr(actual);
      errorCounter := errorCounter + 1;
    end if;
  end procedure;

  procedure Check(
    what: in string;
    actual: in std_logic;
    expected: in std_logic) is
  begin
    if (actual /= expected) then
      report "CHECK FAILED: "&what&": expected " & str(expected)& ", got " &str(actual);
      errorCounter := errorCounter + 1;
    end if;
  end procedure;

  procedure Check(
    what: in string;
    b: in boolean) is
  begin
    if (not b) then
      report "CHECK FAILED: " & what & " is FALSE";
      errorCounter := errorCounter + 1;
    end if;
  end procedure;

end package body;
