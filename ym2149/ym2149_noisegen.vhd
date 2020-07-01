--
--  Noise generator for YM2149/AY-2-891x
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

entity ym2149_noisegen is
  port (
    clk_i     : in std_logic;
    arst_i    : in std_logic;
    clken_i   : in std_logic; -- One tick each YM Mhz
    period_i  : in std_logic_vector(4 downto 0);

    noise_o   : out std_logic
  );
end entity ym2149_noisegen;

architecture beh of ym2149_noisegen is

  signal shreg_r    : unsigned(16 downto 0);
  signal counter_r  : unsigned(4 downto 0);
  signal noise_r    : std_logic;

begin

  -- LFSR.

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      counter_r <= (others => '0');
      shreg_r   <= (0 => '1', others => '0');
      noise_r   <= '0';
    elsif rising_edge(clk_i) then
      if clken_i='1' then
         if counter_r/=0 then
            counter_r <= counter_r -1;
         elsif unsigned(period_i)/=0 then
            counter_r <= unsigned(period_i) - 1;
         end if;

         if counter_r = 0 then
            shreg_r <= (shreg_r(0) xor shreg_r(2)) & shreg_r(16 downto 1);
         end if;

         noise_r <= shreg_r(0);

      end if;
    end if;
  end process;

end beh;
