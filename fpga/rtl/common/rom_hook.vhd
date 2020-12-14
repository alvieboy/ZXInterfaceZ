library IEEE;   
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
use IEEE.std_logic_misc.all;
library work;
use work.zxinterfacepkg.all;

entity rom_hook is
  port (
    clk_i         : in std_logic;
    arst_i        : in std_logic;

    a_i           : in std_logic_vector(15 downto 0);
    rdn_i         : in std_logic;
    mreqn_i       : in std_logic;
    hook_base_i   : in rom_hook_base_t;
    hook_len_i    : in rom_hook_len_t;
    hook_valid_i  : in std_logic_vector(ROM_MAX_HOOKS-1 downto 0);
    force_romcs_o : out std_logic
  );
end entity rom_hook;

architecture beh of rom_hook is

  signal hook_match_r : std_logic_vector(ROM_MAX_HOOKS-1 downto 0);

begin

  process(clk_i, arst_i)
    variable a_u_v: unsigned(13 downto 0);
  begin
    if arst_i='1' then
      hook_match_r    <= (others => '0');
    elsif rising_edge(clk_i) then

      if a_i(15 downto 14)="00" and rdn_i='0' and mreqn_i='0' then

        a_u_v := unsigned(a_i(13 downto 0));

        hookcheck: for i in 0 to ROM_MAX_HOOKS-1 loop
          if hook_valid_i(i)='1' and a_u_v>=hook_base_i(i) and a_u_v <= hook_len_i(i)+hook_base_i(i) then
            hook_match_r(i) <= '1';
          else
            hook_match_r(i) <= '0';
          end if;
        end loop;
      else
        hook_match_r <= (others => '0');
      end if;
    end if;
  end process;

  force_romcs_o <= or_reduce(hook_match_r);

end beh;
