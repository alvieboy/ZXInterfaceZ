--
--  USB Filter
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

entity usb_filter is
  generic (
    WIDTH:  natural  := 8
  );
  port (
    clk_i   : in std_logic; -- 48Mhz clock
    speed_i : in std_logic; -- Speed indicator (for bypass): '1': Full Speed, '0': Low Speed

    dp_i    : in std_logic;
    dm_i    : in std_logic;
    d_i     : in std_logic;

    dp_o    : out std_logic;
    dm_o    : out std_logic;
    d_o     : out std_logic
  );
end entity usb_filter;

architecture beh of usb_filter is

  subtype d_type is std_logic_vector(2 downto 0);
  type d_array_type is array (0 to WIDTH-1) of d_type;

  signal d_q    : d_array_type;

  signal dout_q : d_type;
  signal d_filt : std_logic;
  signal d_filt_in : std_logic_vector(WIDTH-1 downto 0);

  function count_ones(s: in std_logic_vector) return natural is
    variable r: natural := 0;
  begin
    l: for i in s'LOW to s'HIGH loop
      if s(i)='1' then
        r := r + 1;
      end if;
    end loop;
    return r;
  end function;

  signal maj_dp: std_logic;
  signal maj_dm: std_logic;

begin

  process(clk_i)
  begin
    if rising_edge(clk_i) then
      d_q(0) <= ( d_i & dp_i  & dm_i );
      g: for i in 1 to WIDTH-1 loop
        d_q(i) <= d_q(i-1);
      end loop;
    end if;
  end process;

  -- Handle differential and single-ended separately

  -- This is for differential

  process(clk_i)
    variable same: boolean;
  begin
    if rising_edge(clk_i) then
      same := true;
      g: for i in 1 to WIDTH-1 loop
        if d_q(i)(1 downto 0) /= d_q(i-1)(1 downto 0) then
          same := false;
        end if;
      end loop;
      if same then
        dout_q(1 downto 0) <= d_q(WIDTH-1)(1 downto 0);
      end if;
    end if;
  end process;

  mfilt1: entity work.maj_filter
    port map (
      clk_i => clk_i,
      d_i => dp_i,
      d_o => maj_dp
    );

  mfilt2: entity work.maj_filter
    port map (
      clk_i => clk_i,
      d_i => dm_i,
      d_o => maj_dm
    );

  -- And this is for single-ended

  --process(clk_i)
  --  variable same: boolean;
  --begin
  --  if rising_edge(clk_i) then
  --    same := true;
  --    g: for i in 1 to WIDTH-1 loop
  --      if d_q(i)(2) /= d_q(i-1)(2) then
  --        same := false;
  --      end if;
  --    end loop;
  --    if same then
  --      dout_q(2) <= d_q(WIDTH-1)(2);
  --    end if;
  --  end if;
  --end process;

  k: for i in 0 to WIDTH-1 generate
    d_filt_in(i) <= d_q(i)(2);
  end generate;

  d_filt <= '1' when count_ones(d_filt_in)>=4 else '0';

  dp_o  <= maj_dp when speed_i='0' else dp_i;
  dm_o  <= maj_dm when speed_i='0' else dm_i;
  --d_o   <= dout_q(2) when speed_i='0' else d_i;
  d_o   <= d_filt when speed_i='0' else d_i;
end beh;
