library IEEE;   
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
use IEEE.std_logic_misc.all;
library work;
use work.zxinterfacepkg.all;

entity rom_hook_single is
  port (
    clk_i         : in std_logic;
    arst_i        : in std_logic;
    hook_i        : in rom_hook_t;
    a_i           : in std_logic_vector(15 downto 0);
    romsel_i      : in std_logic;
    opcode_read_i : in std_logic;
    refresh_finished_i : in std_logic;
    force_romcs_on_o   : out std_logic;
    force_romcs_off_o  : out std_logic;

    range_match_o : out std_logic
  );
end entity rom_hook_single;

architecture beh of rom_hook_single is

  function len_to_mask(len: std_logic_vector(1 downto 0)) return std_logic_vector is
    variable mask: std_logic_vector(13 downto 0);
  begin
    case len is
      when "00" => mask := "11111111111111";   -- 1
      when "01" => mask := "11111111111110";   -- 2
      when "10" => mask := "11111111111000";   -- 8
      when "11" => mask := "11111100000000";   -- 256
      when others => mask := "11111111111111";
    end case;
    return mask;
  end function;

  signal hook_matchaddress_s  : std_logic;
  signal hook_post_r          : std_logic;
  signal hook_set_post_s      : std_logic;
  signal hook_clr_post_s      : std_logic;

  signal ranged_match_r       : std_logic;

begin

  process(a_i, romsel_i, hook_i)
    variable a_v    : std_logic_vector(13 downto 0);
    variable mask   : std_logic_vector(13 downto 0);
  begin
    a_v := a_i(13 downto 0);

    mask := len_to_mask(hook_i.masklen);

    if not is_x(a_v) and a_i(15 downto 14)="00" then -- ROM area access
      if ((hook_i.base and mask) = (a_v and mask))
       and hook_i.flags.valid = '1'
       and hook_i.flags.romno = romsel_i then
        hook_matchaddress_s <= '1';
      else
        hook_matchaddress_s <= '0';
      end if;
    else
      hook_matchaddress_s <= '0';
    end if;

  end process;

  process(hook_matchaddress_s, hook_i, opcode_read_i, refresh_finished_i, hook_post_r)
  begin
    force_romcs_off_o   <= '0';
    force_romcs_on_o    <= '0';
    hook_set_post_s     <= '0';
    hook_clr_post_s     <= '0';

    if hook_matchaddress_s='1' and hook_i.flags.ranged='0' and opcode_read_i='1' then
      -- Opcode read.
      if hook_i.flags.prepost='0' then -- pre-trigger
        force_romcs_on_o  <= hook_i.flags.setreset;
        force_romcs_off_o <= not hook_i.flags.setreset;
      else
        hook_set_post_s <= '1';    -- Post-trigger
      end if;
    end if;

    if hook_post_r='1' and refresh_finished_i='1' then
      force_romcs_on_o    <= hook_i.flags.setreset;
      force_romcs_off_o   <= not hook_i.flags.setreset;
      hook_clr_post_s     <= '1';
    end if;

  end process;

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      hook_post_r <= '0';
    elsif rising_edge(clk_i) then
      if hook_clr_post_s='1' or hook_i.flags.valid='0' then
        hook_post_r <= '0';
      elsif hook_set_post_s='1' then
        hook_post_r <= '1';
      end if;
    end if;
  end process;

  -- Ranged matching (sync)
  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      ranged_match_r <= '0';

    elsif rising_edge(clk_i) then

      if hook_matchaddress_s='1' and hook_i.flags.ranged='1' and hook_i.flags.setreset='1' then
        ranged_match_r <= '1';
      else
        ranged_match_r <= '0';
      end if;

      -- If invalid, reset match
      if hook_i.flags.valid='0' then
        ranged_match_r <= '0';
      end if;
    end if;
  end process;


  range_match_o <= ranged_match_r;

end beh;
