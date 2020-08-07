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
  constant C_CAPTURE_ENABLED: boolean := true;


  constant C_MEM_READ_DELAY_PULSE   : natural := 33;  -- This is used to capture signals.
  constant C_MEM_READ_FILTER_DELAY  : natural := 1;
  constant C_IO_READ_DELAY_PULSE    : natural := 15;

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
  constant SPECT_PORT_MEMSEL                : std_logic_vector(7 downto 0) := "01101011";   -- x"6B";
  -- UNUSED: "01101111";
  -- UNUSED: "01110011";
  -- UNUSED: "01110111";
  -- UNUSED: "01111011";
  -- UNUSED: "01111111";

  constant SPECT_PORT_KEMPSTON_JOYSTICK       : std_logic_vector(15 downto 0) := "00000000" & "00011111";
  constant SPECT_PORT_KEMPSTON_JOYSTICK_MASK  : std_logic_vector(15 downto 0) := "00000000" & "11100001";
  constant SPECT_PORT_KEMPSTON_MOUSEX         : std_logic_vector(15 downto 0) := "00000011" & "00011111";
  constant SPECT_PORT_KEMPSTON_MOUSEX_MASK    : std_logic_vector(15 downto 0) := "00000111" & "00100001";
  constant SPECT_PORT_KEMPSTON_MOUSEY         : std_logic_vector(15 downto 0) := "00000111" & "00011111";
  constant SPECT_PORT_KEMPSTON_MOUSEY_MASK    : std_logic_vector(15 downto 0) := "00000111" & "00100001";
  constant SPECT_PORT_KEMPSTON_MOUSEB         : std_logic_vector(15 downto 0) := "00000010" & "00011111";
  constant SPECT_PORT_KEMPSTON_MOUSEB_MASK    : std_logic_vector(15 downto 0) := "00000011" & "00100001";
  constant SPECT_PORT_AY_REGISTER             : std_logic_vector(15 downto 0) := "11000000" & "00000001";
  constant SPECT_PORT_AY_REGISTER_MASK        : std_logic_vector(15 downto 0) := "11000000" & "00000011";
  constant SPECT_PORT_AY_DATA                 : std_logic_vector(15 downto 0) := "10000000" & "00000001";
  constant SPECT_PORT_AY_DATA_MASK            : std_logic_vector(15 downto 0) := "11000000" & "00000011";

end package;

--package body zxinterfacepkg is

--end package body;
