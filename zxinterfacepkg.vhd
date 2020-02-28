library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

package zxinterfacepkg is


  constant CAPTURE_MEMWIDTH_BITS: natural := 12;
  constant SCREENCAP_ENABLED: boolean := true;
  constant ROM_ENABLED: boolean := false;
  constant CAPTURE_ENABLED: boolean := false;
  constant SIGTAP_ENABLED: boolean := false;


  constant FPGAID0: std_logic_vector(7 downto 0) := x"A5";
  constant FPGAID1: std_logic_vector(7 downto 0) := x"10";
  constant FPGAID2: std_logic_vector(7 downto 0) := x"00";
  constant FPGAID3: std_logic_vector(7 downto 0) := x"02";

end package;

package body zxinterfacepkg is

end package body;
