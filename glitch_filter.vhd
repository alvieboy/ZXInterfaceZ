library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity glitch_filter is
  generic (
    RESET   :  std_logic := '0'
  );
  port (
    clk_i   : in std_logic;
    arst_i  : in std_logic;
    i_i     : in std_logic;
    o_o     : out std_logic
  );
end entity glitch_filter;

architecture beh of glitch_filter is

  signal q_r:   std_logic_vector(1 downto 0);
  signal o_r:   std_logic;

begin

  process(clk_i, arst_i)
    variable v_v: std_logic_vector(2 downto 0);
  begin
    if arst_i='1' then
      q_r <= (others => RESET);
      o_r <= RESET;
    elsif rising_edge(clk_i) then
      q_r(0) <= i_i;
      q_r(1 downto 1) <= q_r(0 downto 0);
      -- Output generation
      v_v := q_r & i_i;
      case v_v is
        when "000" | "001" | "010" | "100" => o_r <= '0';
        when "111" | "110" | "101" | "011" => o_r <= '1';
        when others => null;
      end case;
    end if;
  end process;

  o_o   <= o_r;

end beh;
