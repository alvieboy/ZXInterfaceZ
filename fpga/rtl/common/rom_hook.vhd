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
    romsel_i      : in std_logic;
    rdn_i         : in std_logic;
    mreqn_i       : in std_logic;
    rfsh_i        : in std_logic;
    m1_i          : in std_logic;

    hook_i        : in rom_hook_array_t;

    trig_force_romcs_on_o   : out std_logic;
    trig_force_romcs_off_o  : out std_logic;

    range_romcs_o : out std_logic
  );
end entity rom_hook;

architecture beh of rom_hook is

  signal hook_matchaddress_s  : std_logic_vector(ROM_MAX_HOOKS-1 downto 0);
  signal opcode_read_s        : boolean;
  signal refresh_finished_s   : boolean;

  signal rfsh_r               : std_logic;
  signal hook_post_r          : std_logic_vector(ROM_MAX_HOOKS-1 downto 0);
  signal ranged_match_r       : std_logic_vector(ROM_MAX_HOOKS-1 downto 0);
  signal force_romcs_on_r     : std_logic;
  signal force_romcs_off_r    : std_logic;

begin

  match: for i in 0 to ROM_MAX_HOOKS-1 generate

    process(a_i, romsel_i, hook_i(i))
      variable a_u_v  : unsigned(13 downto 0);
      variable len    : unsigned(13 downto 0);
    begin
      a_u_v := unsigned( a_i(13 downto 0) );
      len(7 downto 0)   := unsigned( hook_i(i).len );
      len(13 downto 8)  := (others => '0');

      if not is_x(std_logic_vector(a_u_v)) and a_u_v >= hook_i(i).base
         and a_u_v <= len + hook_i(i).base 
         and hook_i(i).flags.valid = '1'
         and hook_i(i).flags.romno = romsel_i then

          hook_matchaddress_s(i) <= '1';
      else
          hook_matchaddress_s(i) <= '0';
      end if;
    end process;
  end generate;

  opcode_read_s <= true when rdn_i='0' and mreqn_i='0' and m1_i='0' else false;


--    prepost   : std_logic; -- '0': pre-trigger, '1': post-trigger
--    setreset  : std_logic; -- '0': reset ROMCS, '1': enable ROMCS


  -- Rising edge of refresh
  refresh_finished_s <= true when rfsh_i='1' and rfsh_r='0' else false;

  process(clk_i, arst_i)
  begin
    if arst_i='1' then

      force_romcs_on_r  <= '0';
      force_romcs_off_r <= '0';
      rfsh_r            <= '1';
      hook_post_r       <= (others => '0');

    elsif rising_edge(clk_i) then

      rfsh_r <= rfsh_i;
      force_romcs_on_r    <= '0';
      force_romcs_off_r   <= '0';

      if a_i(15 downto 14)="00" then

        hookcheck: for i in 0 to ROM_MAX_HOOKS-1 loop
          if hook_matchaddress_s(i)='1' and hook_i(i).flags.ranged='0' and opcode_read_s then
            -- Opcode read.
            if hook_i(i).flags.prepost='0' then -- pre-trigger
              force_romcs_on_r  <= hook_i(i).flags.setreset;
              force_romcs_off_r <= not hook_i(i).flags.setreset;
            else
              -- Post-trigger
              hook_post_r(i) <= '1';
            end if;
          end if;

          -- Check post-triggers
          if hook_post_r(i)='1' and refresh_finished_s then
            force_romcs_on_r    <= hook_i(i).flags.setreset;
            force_romcs_off_r   <= not hook_i(i).flags.setreset;
          end if;

        end loop;

      end if;
    end if;
  end process;

  -- Ranged matching (sync)
  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      ranged_match_r <= (others => '0');
    elsif rising_edge(clk_i) then

      rangecheck: for i in 0 to ROM_MAX_HOOKS-1 loop
        if hook_matchaddress_s(i)='1' and hook_i(i).flags.ranged='1' and hook_i(i).flags.setreset='1' then
          ranged_match_r(i) <= '1';
        else
          ranged_match_r(i) <= '0';
        end if;
      end loop;
    end if;
  end process;

  range_romcs_o <= or_reduce(ranged_match_r);
  trig_force_romcs_on_o   <= force_romcs_on_r;
  trig_force_romcs_off_o  <= force_romcs_off_r;

end beh;
