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


  -- 0 x 1 x x x 1 1

  constant SPECT_PORT_SCRATCH0              : std_logic_vector(7 downto 0) := "00100011";   -- x"23"
  constant SPECT_PORT_SCRATCH1              : std_logic_vector(7 downto 0) := "00100111";   -- x"27"
  constant SPECT_PORT_CMD_FIFO_STATUS       : std_logic_vector(7 downto 0) := "00101011";   -- x"2B"
  constant SPECT_PORT_RESOURCE_FIFO_STATUS  : std_logic_vector(7 downto 0) := "00101111";   -- x"2F"
  constant SPECT_PORT_RESOURCE_FIFO_DATA    : std_logic_vector(7 downto 0) := "00110011";   -- x"33"
  constant SPECT_PORT_RAM_ADDR_LOW          : std_logic_vector(7 downto 0) := "00110111";   -- x"37"
  constant SPECT_PORT_RAM_ADDR_MIDDLE       : std_logic_vector(7 downto 0) := "00111011";   -- x"3B"
  constant SPECT_PORT_RAM_ADDR_HIGH         : std_logic_vector(7 downto 0) := "00111111";   -- x"3F"
  constant SPECT_PORT_RAM_DATA              : std_logic_vector(7 downto 0) := "01100011";   -- x"63"
  constant SPECT_PORT_CMD_FIFO_DATA         : std_logic_vector(7 downto 0) := "01100111";   -- x"67"
  -- UNUSED: "01101011";
  -- UNUSED: "01101111";
  -- UNUSED: "01110011";
  -- UNUSED: "01110111";
  -- UNUSED: "01111011";
  -- UNUSED: "01111111";

  constant SPECT_PORT_KEMPSTON_JOYSTICK       : std_logic_vector(15 downto 0) := "0000000000011111";
  constant SPECT_PORT_KEMPSTON_JOYSTICK_MASK  : std_logic_vector(15 downto 0) := "0000000011100001";
  constant SPECT_PORT_KEMPSTON_MOUSEX         : std_logic_vector(15 downto 0) := "0000001100011111";
  constant SPECT_PORT_KEMPSTON_MOUSEX_MASK    : std_logic_vector(15 downto 0) := "0000011100100001";
  constant SPECT_PORT_KEMPSTON_MOUSEY         : std_logic_vector(15 downto 0) := "0000011100011111";
  constant SPECT_PORT_KEMPSTON_MOUSEY_MASK    : std_logic_vector(15 downto 0) := "0000011100100001";
  constant SPECT_PORT_KEMPSTON_MOUSEB         : std_logic_vector(15 downto 0) := "0000001000011111";
  constant SPECT_PORT_KEMPSTON_MOUSEB_MASK    : std_logic_vector(15 downto 0) := "0000001100100001";


end package;

--package body zxinterfacepkg is

--end package body;
