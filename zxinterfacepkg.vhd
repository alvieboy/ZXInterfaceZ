library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

package zxinterfacepkg is


  constant COMPRESS_BITS: natural := 7;

  constant CLK_KHZ: natural := 96000;

  constant CAPTURE_MEMWIDTH_BITS: natural := 11;
  constant SCREENCAP_ENABLED: boolean := true;
  constant ROM_ENABLED: boolean := true;
  constant CAPTURE_ENABLED: boolean := false;
  constant SIGTAP_ENABLED: boolean := false;


  constant FPGAID0: std_logic_vector(7 downto 0) := x"A5";
  constant FPGAID1: std_logic_vector(7 downto 0) := x"10";
  constant FPGAID2: std_logic_vector(7 downto 0) := x"03";
  --constant FPGAID3: std_logic_vector(7 downto 0) := x"00";

  constant C_ENABLE_VGA: boolean := true;

end package;

--package body zxinterfacepkg is

--end package body;
