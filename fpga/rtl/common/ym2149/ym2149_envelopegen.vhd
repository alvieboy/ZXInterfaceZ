--
--  Envelope generator for YM2149/AY-2-891x
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

entity ym2149_envelopegen is
  generic (
    ENVELOPE_SIZE_BITS: natural := 4
  );
  port (
    clk_i       : in std_logic;
    arst_i      : in std_logic;
    clken_i     : in std_logic; -- One tick
    freq_i      : in std_logic_vector(15 downto 0);
    hold_i      : in std_logic;
    reset_req_i : in std_logic;
    continue_i  : in std_logic;
    alternate_i : in std_logic;
    attack_i    : in std_logic;
    volume_o    : out std_logic_vector(ENVELOPE_SIZE_BITS-1 downto 0)
  );
end entity ym2149_envelopegen;

architecture beh of ym2149_envelopegen is

  signal cnt_r      : unsigned(15 downto 0);
  signal cnt_zero_s : std_logic;
  signal wave_r     : unsigned(ENVELOPE_SIZE_BITS downto 0);
  signal volume_r   : unsigned(ENVELOPE_SIZE_BITS-1 downto 0);

begin

  cnt_zero_s <= '1' when cnt_r=0 else '0';

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      cnt_r <= (others => '0');
    elsif rising_edge(clk_i) then
      if clken_i='1' then
        if cnt_zero_s='0' and reset_req_i='0' then
          cnt_r <= cnt_r - 1;
        else
          if unsigned(freq_i)/=0 then
            cnt_r <= unsigned(freq_i) - 1;
          end if;
        end if;
      end if;
    end if;
  end process;


  process(clk_i, arst_i)
    variable attackvec_v: unsigned(ENVELOPE_SIZE_BITS-1 downto 0);
  begin
    if arst_i='1' then
      wave_r    <= (others => '1');
      volume_r  <= (others => '0');
    elsif rising_edge(clk_i) then
      if clken_i='1' then
        if reset_req_i='1' then
          wave_r <= (others => '1');
        elsif cnt_zero_s='1' and (wave_r(ENVELOPE_SIZE_BITS)='1' or (hold_i='0' and continue_i='1')) then
          wave_r <= wave_r - 1;
        end if;

        attackvec_v := (others => attack_i);

        if wave_r(ENVELOPE_SIZE_BITS)='0' and continue_i='0' then
          volume_r <= (others => '0');
        elsif wave_r(ENVELOPE_SIZE_BITS)='1' or (alternate_i xor hold_i)='0' then
          volume_r <= wave_r(ENVELOPE_SIZE_BITS-1 downto 0) xor attackvec_v;
        else
          volume_r <= wave_r(ENVELOPE_SIZE_BITS-1 downto 0) xor not attackvec_v;
        end if;

      end if;
    end if;
  end process;

  volume_o  <= std_logic_vector(volume_r);
  
end beh;






