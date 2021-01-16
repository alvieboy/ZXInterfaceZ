--
--  TX clock generator
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

entity usb_txclkgen is
  port (
    clk_i     : in std_logic; -- 48Mhz
    arstn_i   : in std_logic;
    srst_i    : in std_logic;
    speed_i   : in std_logic; -- '0': Low-speed, '1': High-speed
    tick_o    : out std_logic
  );
end entity usb_txclkgen;

-- DPLL runs at 4x oversampling for Full-Speed.
-- DPLL runs at 32x oversampling for Low-Speed.

architecture beh of usb_txclkgen is

  signal pll_counter_r: unsigned(4 downto 0) := (others =>'0');

begin

  process(clk_i, arstn_i)
  begin
    if arstn_i='0' then
      pll_counter_r <= (others => '0');
    elsif rising_edge(clk_i) then
      if srst_i='1' then
        pll_counter_r <= (others =>'0');
      else
        pll_counter_r <= pll_counter_r + 1;
      end if;
    end if;
  end process;

  tick_o <= '1' when (speed_i='0' and pll_counter_r="00000") or (speed_i='1' and pll_counter_r(1 downto 0)="00")  else '0';
  
end beh;

