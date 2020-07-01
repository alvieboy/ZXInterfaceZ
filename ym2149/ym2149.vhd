--
--  YM2149/AY-2-891x
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
  
entity ym2149 is
  port (
    clk_i     : in std_logic;
    clken_i   : in std_logic;
    arst_i    : in std_logic;
    dat_i     : in std_logic_vector(7 downto 0);
    dat_o     : out std_logic_vector(7 downto 0);
    adr_i     : in std_logic_vector(3 downto 0);
    we_i      : in std_logic;
    rd_i      : in std_logic;
    audio_a_o : out std_logic_vector(7 downto 0);
    audio_b_o : out std_logic_vector(7 downto 0);
    audio_c_o : out std_logic_vector(7 downto 0)
  );
end entity ym2149;

architecture str of ym2149 is

  type period_array_type  is array(0 to 2) of std_logic_vector(11 downto 0);
  type volume_array_type  is array(0 to 2) of std_logic_vector(4 downto 0);
  type audio_array_type   is array(0 to 2) of std_logic_vector(7 downto 0);

  signal period_s         : period_array_type;
  signal noise_period_s   : std_logic_vector(4 downto 0);
  signal env_period_s     : std_logic_vector(15 downto 0);
  signal enable_s         : std_logic_vector(7 downto 0);
  signal volume_s         : volume_array_type;
  signal env_shape_s      : std_logic_vector(3 downto 0);

  signal env_clken_s      : std_logic;
  signal env_reset_s      : std_logic;
  signal main_clken_s     : std_logic;

  signal noise_s          : std_logic;
  signal tone_s           : std_logic_vector(2 downto 0);-- Array

  signal env_volume_s     : std_logic_vector(3 downto 0);
  signal audio_s          : audio_array_type;

begin

  ymclocking_inst: entity work.ym2149_clocking
    port map (
      clk_i       => clk_i,
      clken_i     => clken_i,
      arst_i      => arst_i,
      clkdiv16_o  => open,--main_clken_s,
      clkdiv8_o   => main_clken_s,
      env_div_o   => env_clken_s
    );

  ymregs_inst: entity work.ym2149_regs
    port map (
      clk_i           => clk_i,
      arst_i          => arst_i,
      dat_i           => dat_i,
      dat_o           => dat_o,
      adr_i           => adr_i,
      we_i            => we_i,
      rd_i            => rd_i,
      env_reset_o     => env_reset_s,
      env_clken_i     => env_clken_s,
      a_period_o      => period_s(0),
      b_period_o      => period_s(1),
      c_period_o      => period_s(2),
      noise_period_o  => noise_period_s,
      env_period_o    => env_period_s,
      enable_o        => enable_s,
      a_volume_o      => volume_s(0),
      b_volume_o      => volume_s(1),
      c_volume_o      => volume_s(2),
      env_shape_o     => env_shape_s
    );

  ymnoise_inst: entity work.ym2149_noisegen
    port map (
      clk_i     => clk_i,
      arst_i    => arst_i,
      clken_i   => main_clken_s,
      period_i  => noise_period_s,
      noise_o   => noise_s
    );

  envelope_inst: entity work.ym2149_envelopegen
    port map (
      clk_i       => clk_i,
      arst_i      => arst_i,
      clken_i     => env_clken_s,
      freq_i      => env_period_s,
      reset_req_i => env_reset_s,
      hold_i      => env_shape_s(0),
      alternate_i => env_shape_s(1),
      attack_i    => env_shape_s(2),
      continue_i  => env_shape_s(3),
      volume_o    => env_volume_s
  );

  chan_gen: for chan in 0 to 2 generate

    ymtone_inst: entity work.ym2149_tonegen
      port map (
        clk_i     => clk_i,
        arst_i    => arst_i,
        clken_i   => main_clken_s,
        period_i  => period_s(chan),
        tone_o    => tone_s(chan)
      );

    ymmixer_inst: entity work.ym2149_mixer
      port map (
        clk_i           => clk_i,
        arst_i          => arst_i,
        clken_i         => main_clken_s,
        disable_tone_i  => enable_s(chan),
        disable_noise_i => enable_s(chan+3),
        freq_i          => tone_s(chan),
        noise_i         => noise_s,
        chan_volume_i   => volume_s(chan),
        env_volume_i    => env_volume_s,
        audio_o         => audio_s(chan)
      );

  end generate;

  audio_a_o <= audio_s(0);
  audio_b_o <= audio_s(1);
  audio_c_o <= audio_s(2);

end str;
