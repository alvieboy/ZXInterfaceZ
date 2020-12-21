--
--  Registers for YM2149/AY-2-891x
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

entity ym2149_regs is
  port (
    clk_i           : in std_logic;
    arst_i          : in std_logic;
    dat_i           : in std_logic_vector(7 downto 0);
    dat_o           : out std_logic_vector(7 downto 0);
    adr_i           : in std_logic_vector(3 downto 0);
    we_i            : in std_logic;
    rd_i            : in std_logic;

    -- Connection to envelope
    env_reset_o     : out std_logic;
    env_clken_i     : in  std_logic;

    -- Registers out
    a_period_o      : out std_logic_vector(11 downto 0);
    b_period_o      : out std_logic_vector(11 downto 0);
    c_period_o      : out std_logic_vector(11 downto 0);
    noise_period_o  : out std_logic_vector(4 downto 0);
    env_period_o    : out std_logic_vector(15 downto 0);
    enable_o        : out std_logic_vector(7 downto 0);
    a_volume_o      : out std_logic_vector(4 downto 0);
    b_volume_o      : out std_logic_vector(4 downto 0);
    c_volume_o      : out std_logic_vector(4 downto 0);
    env_shape_o     : out std_logic_vector(3 downto 0)
  );
end entity ym2149_regs;

architecture beh of ym2149_regs is

  signal a_period_r       : std_logic_vector(11 downto 0);
  signal b_period_r       : std_logic_vector(11 downto 0);
  signal c_period_r       : std_logic_vector(11 downto 0);
  signal noise_period_r   : std_logic_vector(4 downto 0);
  signal env_period_r     : std_logic_vector(15 downto 0);
  signal enable_r         : std_logic_vector(7 downto 0);
  signal a_volume_r       : std_logic_vector(4 downto 0);
  signal b_volume_r       : std_logic_vector(4 downto 0);
  signal c_volume_r       : std_logic_vector(4 downto 0);
  signal env_shape_r      : std_logic_vector(3 downto 0);

  signal env_reset_r      : std_logic;

begin

  regs_read: process(clk_i, arst_i)
  begin
    if arst_i='1' then
      dat_o <= (others => '0');
    elsif rising_edge(clk_i) then
      if rd_i='1' then
        case adr_i is
          when x"0" => dat_o <= a_period_r(7 downto 0);
          when x"1" => dat_o <= "0000" & a_period_r(11 downto 8);
          when x"2" => dat_o <= b_period_r(7 downto 0);
          when x"3" => dat_o <= "0000" & b_period_r(11 downto 8);
          when x"4" => dat_o <= c_period_r(7 downto 0);
          when x"5" => dat_o <= "0000" & c_period_r(11 downto 8);
          when x"6" => dat_o <= "000" & noise_period_r;
          when x"7" => dat_o <= enable_r;
          when x"8" => dat_o <= "000"  & a_volume_r;
          when x"9" => dat_o <= "000"  & b_volume_r;
          when x"A" => dat_o <= "000"  & c_volume_r;
          when x"B" => dat_o <= env_period_r(7 downto 0);
          when x"C" => dat_o <= env_period_r(15 downto 8);
          when x"D" => dat_o <= "0000" & env_shape_r;
          when others => dat_o <= (others => '0');
        end case;
      end if;
    end if;
  end process;

  regs_write: process(clk_i, arst_i)
    variable do_env_reset_v: boolean;
  begin
    if arst_i='1' then

      env_reset_r     <= '0';
      a_period_r      <= (others => '0');
      b_period_r      <= (others => '0');
      c_period_r      <= (others => '0');
      noise_period_r  <= (others => '0');
      enable_r        <= (others => '1');  -- TBD.
      a_volume_r      <= (others => '0');
      b_volume_r      <= (others => '0');
      c_volume_r      <= (others => '0');
      env_period_r    <= (others => '0');
      env_shape_r     <= (others => '0');

    elsif rising_edge(clk_i) then

      do_env_reset_v := false;

      if we_i='1' then
        case adr_i is
          when "0000" => a_period_r(7 downto 0)   <= dat_i;
          when "0001" => a_period_r(11 downto 8)  <= dat_i(3 downto 0);
          when "0010" => b_period_r(7 downto 0)   <= dat_i;
          when "0011" => b_period_r(11 downto 8)  <= dat_i(3 downto 0);
          when "0100" => c_period_r(7 downto 0)   <= dat_i;
          when "0101" => c_period_r(11 downto 8)  <= dat_i(3 downto 0);
          when "0110" => noise_period_r           <= dat_i(4 downto 0);
          when "0111" => enable_r                 <= dat_i;
          when "1000" => a_volume_r               <= dat_i(4 downto 0);
          when "1001" => b_volume_r               <= dat_i(4 downto 0);
          when "1010" => c_volume_r               <= dat_i(4 downto 0);
          when "1011" => env_period_r(7 downto 0) <= dat_i;
          when "1100" => env_period_r(15 downto 8)<= dat_i;
          when "1101" => env_shape_r              <= dat_i(3 downto 0);
                         do_env_reset_v := true;
          when others => null;
        end case;

      end if;

      -- Let reset propagate
      if do_env_reset_v then
        env_reset_r <= '1';
      elsif env_clken_i='1' then
        env_reset_r <= '0';
      end if;
    end if;

  end process;

  env_reset_o     <= env_reset_r;
  a_period_o      <= a_period_r;
  b_period_o      <= b_period_r;
  c_period_o      <= c_period_r;
  noise_period_o  <= noise_period_r;
  env_period_o    <= env_period_r;
  enable_o        <= enable_r;
  a_volume_o      <= a_volume_r;
  b_volume_o      <= b_volume_r;
  c_volume_o      <= c_volume_r;
  env_shape_o     <= env_shape_r;

end beh;

