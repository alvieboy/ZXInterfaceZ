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
    range_romcs_o   : out std_logic
  );
end entity rom_hook;

architecture beh of rom_hook is

  signal opcode_read_s            : std_logic;
  signal refresh_finished_s       : std_logic;

  signal ranged_match_s           : std_logic_vector(ROM_MAX_HOOKS-1 downto 0);
  signal force_romcs_on_s         : std_logic_vector(ROM_MAX_HOOKS-1 downto 0);
  signal force_romcs_off_s        : std_logic_vector(ROM_MAX_HOOKS-1 downto 0);

  signal force_romcs_on_r         : std_logic;
  signal force_romcs_off_r        : std_logic;
  signal rfsh_r                   : std_logic;

begin

  match: for i in 0 to ROM_MAX_HOOKS-1 generate

    hook_1_inst: entity work.rom_hook_single
      port map (
        clk_i               => clk_i,
        arst_i              => arst_i,
        hook_i              => hook_i(i),
        a_i                 => a_i,
        romsel_i            => romsel_i,
        opcode_read_i       => opcode_read_s,
        refresh_finished_i  => refresh_finished_s,
        force_romcs_on_o    => force_romcs_on_s(i),
        force_romcs_off_o   => force_romcs_off_s(i),
        range_match_o       => ranged_match_s(i)
      );

  end generate;

  opcode_read_s       <= '1' when rdn_i='0' and mreqn_i='0' and m1_i='0' else '0';
  refresh_finished_s  <= '1' when rfsh_i='1' and rfsh_r='0' else '0';

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      rfsh_r            <= '1';
    elsif rising_edge(clk_i) then
      rfsh_r <= rfsh_i;
    end if;
  end process;

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      force_romcs_on_r    <= '0';
      force_romcs_off_r   <= '0';

    elsif rising_edge(clk_i) then
      force_romcs_on_r    <= '0';
      force_romcs_off_r   <= '0';

      if or_reduce(force_romcs_off_s)='1' then
        force_romcs_off_r<='1';
      end if;

      if or_reduce(force_romcs_on_s)='1' then
        force_romcs_on_r<='1';
      end if;
    end if;
  end process;

  range_romcs_o <= or_reduce(ranged_match_s);
  trig_force_romcs_on_o   <= force_romcs_on_r;
  trig_force_romcs_off_o  <= force_romcs_off_r;

end beh;
