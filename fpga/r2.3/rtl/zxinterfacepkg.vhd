library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

package zxinterfacepkg is

  constant C_CLK_KHZ                : natural := 96000;

  constant C_SCREENCAP_ENABLED      : boolean := true;
  constant C_BIT_ENABLED            : boolean := false;

  constant C_FPGAID0                : std_logic_vector(7 downto 0) := x"A6";
  constant C_FPGAID1                : std_logic_vector(7 downto 0) := x"10";
  constant C_FPGAID2                : std_logic_vector(7 downto 0) := x"04";

  constant C_ENABLE_VGA             : boolean := true;
  constant C_CAPTURE_ENABLED        : boolean := true;


  constant C_MEM_READ_DELAY_PULSE   : natural := 33;  -- This is used to capture signals.
  constant C_MEM_READ_FILTER_DELAY  : natural := 1;
  constant C_IO_READ_DELAY_PULSE    : natural := 15;

  -- 0 x 1 x x x 1 1

  constant SPECT_PORT_SCRATCH0              : std_logic_vector(7 downto 0) := "00100011";   -- x"23"
  constant SPECT_PORT_MISCCTRL              : std_logic_vector(7 downto 0) := "00100111";   -- x"27"
  constant SPECT_PORT_CMD_FIFO_STATUS       : std_logic_vector(7 downto 0) := "00101011";   -- x"2B"
  constant SPECT_PORT_RESOURCE_FIFO_STATUS  : std_logic_vector(7 downto 0) := "00101111";   -- x"2F"
  constant SPECT_PORT_RESOURCE_FIFO_DATA    : std_logic_vector(7 downto 0) := "00110011";   -- x"33"
  constant SPECT_PORT_RAM_ADDR_LOW          : std_logic_vector(7 downto 0) := "00110111";   -- x"37"
  constant SPECT_PORT_RAM_ADDR_MIDDLE       : std_logic_vector(7 downto 0) := "00111011";   -- x"3B"
  constant SPECT_PORT_RAM_ADDR_HIGH         : std_logic_vector(7 downto 0) := "00111111";   -- x"3F"
  constant SPECT_PORT_RAM_DATA              : std_logic_vector(7 downto 0) := "01100011";   -- x"63"
  constant SPECT_PORT_CMD_FIFO_DATA         : std_logic_vector(7 downto 0) := "01100111";   -- x"67"
  constant SPECT_PORT_MEMSEL                : std_logic_vector(7 downto 0) := "01101011";   -- x"6B";
  constant SPECT_PORT_NMIREASON             : std_logic_vector(7 downto 0) := "01101111";   -- x"6F";
  -- UNUSED: "01110011"; -- x"73"
  -- UNUSED: "01110111"; -- x"77"
  -- UNUSED: "01111011"; -- x"7b"
  -- UNUSED: "01111111"; -- x"7f"

  constant SPECT_PORT_KEMPSTON_JOYSTICK       : std_logic_vector(15 downto 0) := "00000000" & "00000001";
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

  constant SPECT_PORT_128PAGE_REGISTER        : std_logic_vector(15 downto 0) := "00000000" & "00000001"; -- AKA 7ffd port
  constant SPECT_PORT_128PAGE_REGISTER_MASK   : std_logic_vector(15 downto 0) := "10000000" & "00000011";

  constant SPECT_PORT_2A_PMC_REGISTER         : std_logic_vector(15 downto 0) := "01000000" & "00000001";  -- Conflicts with 128K page
  constant SPECT_PORT_2A_PMC_REGISTER_MASK    : std_logic_vector(15 downto 0) := "11000000" & "00000011";

  constant SPECT_PORT_2A_SMC_REGISTER         : std_logic_vector(15 downto 0) := "00010000" & "00000001";  -- Conflicts with 128K page
  constant SPECT_PORT_2A_SMC_REGISTER_MASK    : std_logic_vector(15 downto 0) := "10000000" & "00000011";  -- AKA 1ffd port


  type bit_to_cpu_t is record
    bit_request   : std_logic;
    rx_data       : std_logic_vector(7 downto 0);
    --rx_data_valid : std_logic;
    rx_avail_size : std_logic_vector(3 downto 0);
    rx_avail      : std_logic;
    tx_busy       : std_logic;
    bit_data      : std_logic_vector(31 downto 0);
  end record;

  type bit_from_cpu_t is record
    bit_enable    : std_logic;
    tx_data       : std_logic_vector(7 downto 0);
    tx_data_valid : std_logic;
    rx_read       : std_logic;
    bit_data      : std_logic_vector(31 downto 0);
  end record;


  constant ROM_MAX_HOOKS: natural := 8;

  type rom_hookflag_t is record
    valid     : std_logic;
    romno     : std_logic; -- 0 or 1 for now.
    prepost   : std_logic; -- '0': pre-trigger, '1': post-trigger
    setreset  : std_logic; -- '0': reset ROMCS, '1': enable ROMCS
    ranged    : std_logic; -- If setreset is '1', then ranged means if we apply ROMCS only on this range, of if we latch it.
  end record;

  type rom_hook_t is record
    base    : unsigned(13 downto 0);
    len     : unsigned(7 downto 0);  -- Max 256 bytes
    flags   : rom_hookflag_t;
  end record;


  type rom_hook_array_t  is array (0 to ROM_MAX_HOOKS-1) of rom_hook_t;

  function to_01(a: in std_logic_vector) return std_logic_vector;

end package;

package body zxinterfacepkg is

  function to_01(a: in std_logic_vector) return std_logic_vector is
    variable l: std_logic_vector(a'range);
  begin
    l:=a;
    -- synthesis translate_off
    l1: for i in a'low to a'high loop
      if a(i)='H' or a(i)='1' then
        l(i):='1';
      else
        l(i):='0';
      end if;
    end loop;
    -- synthesis translate_on
    return l;
  end function;

end package body;
