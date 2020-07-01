--
--  Clocking for YM2149/AY-2-891x
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
--  3. Neither the name of the copyright holder nor the names of its
--     contributors may be used to endorse or promote products derived
--     from this software without specific prior written permission.
--
--  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY
--  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
--  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
--  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
--  ZPU PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
--  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
--  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
--  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
--  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
--  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
--  ARISING IN ANY WAY OUT OF THE USE OF THIS DESIGN, EVEN IF
--  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--  
--

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity ym2149_clocking is
  port (
    clk_i       : in std_logic;
    clken_i     : in std_logic;
    arst_i      : in std_logic;
    clkdiv16_o  : out std_logic;
    clkdiv8_o   : out std_logic;
    env_div_o   : out std_logic
  );
end entity ym2149_clocking;

architecture beh of ym2149_clocking is

  signal cnt_r      : unsigned(3 downto 0);
  signal clkdiv8_s  : std_logic;
  signal clkdiv16_s : std_logic;

begin

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      cnt_r <= (others => '1');
    elsif rising_edge(clk_i) then
      if clken_i='1' then
        if cnt_r=0 then
          cnt_r <= (others => '1');
        else
          cnt_r <= cnt_r - 1;
        end if;
      end if;
    end if;
  end process;

  clkdiv8_s <= '1' when cnt_r(2 downto 0)="000" and clken_i='1' else '0';
  clkdiv16_s <= '1' when cnt_r(3 downto 0)="0000" and clken_i='1' else '0';

  env_div_o <= clkdiv16_s; -- For AY. Use clk8 for YM

  clkdiv8_o <= clkdiv8_s;
  clkdiv16_o <= clkdiv16_s;

end beh;

