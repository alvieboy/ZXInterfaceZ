library IEEE;
use IEEE.STD_LOGIC_1164.all;
use ieee.math_real.all;

package tappkg is

  --
  -- NOTES ABOUT T-CYCLE COMPENSATION
  --
  -- The FPGA system runs at 96MHz. With this speed it is not possible to generate a perfect 3.5MHz toggle. The ratio
  -- is ~27.428571, and internal FPGA uses 27.0 as the clock divider.
  --
  -- In order to compensate for this small difference, adjust your T-state timings by:
  --
  -- * Multiplying for 960 and then dividing for 945.
  --
  -- Example: you want a 2174-cycle delay. 2174*960= 2087040, round(2087040/945) = 2209
  --
  -- t_compensate function is included for hardcoded (constant) value compensation. DO NOT use it for dynamic computations!.
  --

  function t_compensate(cycles: in natural) return natural;

  constant PULSE_SINGLE  : std_logic_vector(3 downto 0) := "0000";
  constant PULSE_DUAL    : std_logic_vector(3 downto 0) := "0001";

  -- TAP commands as placed in FIFO

  constant TAPCMD_LOGIC0_SIZE     : std_logic_vector(7 downto 0) := "10000000";
  constant TAPCMD_LOGIC1_SIZE     : std_logic_vector(7 downto 0) := "10000001";
  constant TAPCMD_GAP             : std_logic_vector(7 downto 0) := "10000010";
  constant TAPCMD_CHUNK_SIZE_LSB  : std_logic_vector(7 downto 0) := "10000011";
  constant TAPCMD_CHUNK_SIZE_MSB  : std_logic_vector(7 downto 0) := "10000100";
  constant TAPCMD_SET_REPEAT      : std_logic_vector(7 downto 0) := "10000101";
  constant TAPCMD_PLAY_PULSE      : std_logic_vector(7 downto 0) := "10000110";
  constant TAPCMD_FLUSH           : std_logic_vector(7 downto 0) := "10000111";



end package;

package body tappkg is

  function t_compensate(cycles: in natural) return natural is
    variable c: real;
  begin
    c := real(cycles) * 960.0;
    c := c / 945.0;
    return natural(round(c));
  end function;

end package body;
