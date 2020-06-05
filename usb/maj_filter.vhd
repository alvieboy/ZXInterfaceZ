--
--  Majority Filter
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

entity maj_filter is
  generic (
    WIDTH:  natural  := 8
  );
  port (
    clk_i   : in std_logic;
    d_i     : in std_logic;
    d_o     : out std_logic
  );
end entity maj_filter;

architecture beh of maj_filter is

  signal d_r : std_logic_vector(WIDTH-1 downto 0);

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

begin


  process(clk_i)
  begin
    if rising_edge(clk_i) then
      d_r(0) <= d_i;
      l: for i in 1 to WIDTH-1 loop
        d_r(i) <= d_r(i-1);
      end loop;
    end if;
  end process;

  d_o <= '1' when count_ones(d_r)>(WIDTH/2) else '0';

end beh;
