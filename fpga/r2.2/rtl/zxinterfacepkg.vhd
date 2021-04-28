library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

package zxinterfacepkg is

  constant C_CLK_KHZ                : natural := 96000;

  constant C_SCREENCAP_ENABLED      : boolean := true;
  constant C_BIT_ENABLED            : boolean := false;

  constant C_FPGAID0                : std_logic_vector(7 downto 0) := x"A5";
  constant C_FPGAID1                : std_logic_vector(7 downto 0) := x"10";
  constant C_FPGAID2                : std_logic_vector(7 downto 0) := x"04";

  constant C_ENABLE_VGA             : boolean := true;
  constant C_CAPTURE_ENABLED        : boolean := true;


  constant C_MEM_READ_DELAY_PULSE   : natural := 33;  -- This is used to capture signals.
  constant C_MEM_READ_FILTER_DELAY  : natural := 1;
  constant C_IO_READ_DELAY_PULSE    : natural := 15;


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


  constant ROM_MAX_HOOKS: natural := 16;
  constant ENABLE_HOOK_READ: boolean := false;

  type rom_hookflag_t is record
    valid     : std_logic;
    romno     : std_logic; -- 0 or 1 for now.
    prepost   : std_logic; -- '0': pre-trigger, '1': post-trigger
    setreset  : std_logic; -- '0': reset ROMCS, '1': enable ROMCS
    ranged    : std_logic; -- If setreset is '1', then ranged means if we apply ROMCS only on this range, of if we latch it.
  end record;

  type rom_hook_t is record
    base    : std_logic_vector(13 downto 0);
    masklen : std_logic_vector(1 downto 0);  -- 1,2,8 or 256 bytes (aligned)
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
