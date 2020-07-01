--
--  Volume for YM2149/AY-2-891x
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

entity ym2149_volume is
  port (
    volume_i  : in std_logic_vector(3 downto 0);
    audio_o   : out std_logic_vector(7 downto 0)  
  );
end entity ym2149_volume;

architecture beh of ym2149_volume is

  signal audio_s  : std_logic_vector(7 downto 0);

begin

  process(volume_i)
  begin
    case volume_i is
      when "1111"	=> audio_s <= "11111111";
      when "1110"	=> audio_s <= "10110100";
      when "1101"	=> audio_s <= "01111111";
      when "1100"	=> audio_s <= "01011010";
      when "1011"	=> audio_s <= "00111111";
      when "1010"	=> audio_s <= "00101101";
      when "1001"	=> audio_s <= "00011111";
      when "1000"	=> audio_s <= "00010110";
      when "0111"	=> audio_s <= "00001111";
      when "0110"	=> audio_s <= "00001011";
      when "0101"	=> audio_s <= "00000111";
      when "0100"	=> audio_s <= "00000101";
      when "0011"	=> audio_s <= "00000011";
      when "0010"	=> audio_s <= "00000010";
      when "0001"	=> audio_s <= "00000001";
      when "0000"	=> audio_s <= "00000000";
      when others => audio_s <= "00000000";
    end case;
  end process;

  audio_o <= audio_s;

end beh;
