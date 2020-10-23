library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity sync is
  GENERIC (
    RESET: std_logic;
    STAGES: natural := 2
  );
  port (
    clk_i   : in std_logic;
    arst_i  : in std_logic;
    din_i   : in std_logic;
    dout_o  : out std_logic
  );
end entity sync;

architecture beh of sync is

  type regs_type is array (0 to STAGES-1) of std_logic;
  signal q_r: regs_type;

begin

  process(arst_i, clk_i)
  begin
    if arst_i='1' then
      q_r <= (others => RESET);
    elsif rising_edge(clk_i) then
      l: for i in STAGES-1 downto 1 loop
        q_r(i) <= q_r(i-1);
      end loop;
      q_r(0) <= din_i;
    end if;
  end process;

  dout_o <= q_r(STAGES-1);

end beh;

