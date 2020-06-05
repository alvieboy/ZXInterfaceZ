--
--  Digital PLL for USB
--
--  Copyright 2020 Alvaro Lopes <alvieboy@alvie.com>
--
--  The FreeBSD license
--
--  Redistribution and use in source and binary forms, with or without
--  modification, are permitted provided that the following conditions
--  are met:
--
--  1. Redistributions of source code must retain the above copyright
--     notice, this list of conditions and the following disclaimer.
--  2. Redistributions in binary form must reproduce the above
--     copyright notice, this list of conditions and the following
--     disclaimer in the documentation and/or other materials
--     provided with the distribution.
--
--  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
--  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
--  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
--  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
--  ZPU PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
--  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
--  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
--  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
--  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
--  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
--  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
--  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--
--

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity usb_dpll is
  generic (
    COMPENSATION_DELAY: natural := 3
  );
  port (
    clk_i     : in std_logic; -- 48Mhz
    arstn_i   : in std_logic;
    en_i      : in std_logic;
    data_i    : in std_logic;

    speed_i   : in std_logic; -- '0': Low-speed, '1': High-speed

    tick_o    : out std_logic
  );
end entity usb_dpll;

-- DPLL runs at 4x oversampling for Full-Speed.
-- DPLL runs at 32x oversampling for Low-Speed.

architecture beh of usb_dpll is

  signal lock_r       : std_logic;
  signal data_r       : std_logic;
  signal edge_s       : std_logic;
  signal do_lock_s    : std_logic;
  signal pll_reset_s  : std_logic;
  signal pll_counter_r: unsigned(4 downto 0);

begin

  process(clk_i)
  begin
    if rising_edge(clk_i) then
      data_r  <= data_i;
    end if;
  end process;

  edge_s <= data_r xor data_i;

  do_lock_s <= '1' when en_i='1' and edge_s='1' else '0';

  pll_reset_s <= '1' when do_lock_s='1' and lock_r='0' else '0';--en_i='0' or lock_r='0' else '0';

  process(clk_i, arstn_i)
  begin
    if arstn_i='0' then
      lock_r <= '0';
    elsif rising_edge(clk_i) then
      if en_i='0' then
        lock_r <= '0';
      else
        if do_lock_s='1' then
          lock_r <= '1';
        end if;
      end if;
    end if;
  end process;

  process(clk_i, arstn_i)
    variable next_pll_counter: unsigned(4 downto 0);
  begin
    if arstn_i='0' then
      pll_counter_r <= (others => '0');
    elsif rising_edge(clk_i) then
      if pll_reset_s='1' then
        pll_counter_r <= (others => '0');
      else

        next_pll_counter := pll_counter_r + 1;

        if edge_s='1' then
          if speed_i='1' then
            case pll_counter_r(1 downto 0) is
              when "00" => next_pll_counter(1 downto 0) := "00";  -- Phase -1/4 Slow down
              when "01" => next_pll_counter(1 downto 0) := "10";  -- Phase +-2/4 Out-of-phase
              when "10" => next_pll_counter(1 downto 0) := "00";  -- Phase +1/4 Speed up
              when "11" => next_pll_counter(1 downto 0) := "00";  -- On phase
              when others =>
            end case;
          else
            case pll_counter_r is
              when "00000" => next_pll_counter := "00000";  -- Phase -1/32 Slow down
              when "00001" => next_pll_counter := "00000";  -- Phase -2/32 Slow down
              when "00010" => next_pll_counter := "00000";  -- Phase -3/32 Slow down
              when "00011" => next_pll_counter := "00000";  -- Phase -4/32 Slow down
              when "00100" => next_pll_counter := "00000";  -- Phase -5/32 Slow down
              when "00101" => next_pll_counter := "00000";  -- Phase -6/32 Slow down
              when "00110" => next_pll_counter := "00000";  -- Phase -7/32 Slow down
              when "00111" => next_pll_counter := "00000";  -- Phase -8/32 Slow down
              when "01000" => next_pll_counter := "00000";  -- Phase -9/32 Slow down
              when "01001" => next_pll_counter := "00000";  -- Phase -10/32 Slow down
              when "01010" => next_pll_counter := "00000";  -- Phase -11/32 Slow down
              when "01011" => next_pll_counter := "00000";  -- Phase -12/32 Slow down
              when "01100" => next_pll_counter := "00000";  -- Phase -13/32 Slow down
              when "01101" => next_pll_counter := "00000";  -- Phase -14/32 Slow down
              when "01110" => next_pll_counter := "00000";  -- Phase -15/32 Slow down
              when "01111" => next_pll_counter := "00000";  -- Phase +-16/32 Slow down
              when "10000" => next_pll_counter := "00000";  -- Phase +15/32 Slow down
              when "10001" => next_pll_counter := "00000";  -- Phase +14/32 Slow down
              when "10010" => next_pll_counter := "00000";  -- Phase +13/32 Slow down
              when "10011" => next_pll_counter := "00000";  -- Phase +12/32 Slow down
              when "10100" => next_pll_counter := "00000";  -- Phase +11/32 Slow down
              when "10101" => next_pll_counter := "00000";  -- Phase +10/32 Slow down
              when "10110" => next_pll_counter := "00000";  -- Phase +9/32 Slow down
              when "10111" => next_pll_counter := "00000";  -- Phase +8/32 Slow down
              when "11000" => next_pll_counter := "00000";  -- Phase +7/32 Slow down
              when "11001" => next_pll_counter := "00000";  -- Phase +6/32 Slow down
              when "11010" => next_pll_counter := "00000";  -- Phase +5/32 Slow down
              when "11011" => next_pll_counter := "00000";  -- Phase +4/32 Slow down
              when "11100" => next_pll_counter := "00000";  -- Phase +3/32 Slow down
              when "11101" => next_pll_counter := "00000";  -- Phase +2/32 Slow down
              when "11110" => next_pll_counter := "00000";  -- Phase +1/32 Slow down
              when "11111" => next_pll_counter := "00000";  -- On phase
              when others =>
            end case;

          end if;
        end if;
        --if speed_i='1' then
          --if pll_counter_r="11" then
          --  pll_counter_r <= (others => '0');
          --else
          --  pll_counter_r := next_pll_counter;
          --end if;
        --else
          pll_counter_r <= next_pll_counter;
        --end if;
      end if;
    end if;
  end process;

  tick_o <= '1' when (speed_i='0' and pll_counter_r="10000") or (speed_i='1' and pll_counter_r(1 downto 0)="01")  else '0';
  
end beh;

