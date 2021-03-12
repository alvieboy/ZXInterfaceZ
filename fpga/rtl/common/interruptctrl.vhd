LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.NUMERIC_STD.ALL;
USE IEEE.STD_LOGIC_misc.ALL;


entity interruptctrl is
  generic (
    C_INTLINES: natural := 1
  );
  port (
    clk_i           : in std_logic;
    arst_i          : in std_logic;

    int_i           : in std_logic_vector(C_INTLINES-1 downto 0);   -- Interrupt in, LEVEL

    inten_i         : in std_logic; -- Interrupt enable, after processing the interrupt on host side

    intackn_i       : in std_logic; -- Interrupt acknowledge from CPU
    intackn_sync_o  : out std_logic;
    intstat_o       : out std_logic_vector(7 downto 0);
    intn_o          : out std_logic; -- Actual interrupt line to CPU
    dbg_o           : out std_logic_vector(7 downto 0)
  );

end entity interruptctrl;

architecture beh of interruptctrl is

  signal intackn_s    : std_logic;
  signal intn_r       : std_logic;
  signal inten_r      : std_logic;

begin

  ack_sync: entity work.sync generic map (RESET => '1')
      port map ( clk_i => clk_i, arst_i => arst_i, din_i => intackn_i, dout_o => intackn_s );


  process(int_i)
  begin
    intstat_o <= (others => '0');
    intstat_o(C_INTLINES-1 downto 0) <= int_i;
  end process;


  process(clk_i,arst_i)
  begin
    if arst_i='1' then
      intn_r    <= '1';
      inten_r   <= '1'; -- Start with interrupts enabled
    elsif rising_edge(clk_i) then

      -- Interrupt already asserted.
      if intn_r='0' then
        -- Interrupt ack
        if intackn_s='0' then
          intn_r <= '1';
        end if;
      else -- No current interrupt
        if inten_r='1' and or_reduce(int_i)='1' then
          intn_r  <= '0'; -- Assert interrupt.
          inten_r <= '0'; -- Disable further interrupts
        end if;
      end if;

      -- Re-enable interrupts if host requested it.
      if inten_i='1' then
        inten_r <= '1';
      end if;
    end if;

  end process;

  intn_o <= intn_r;
  intackn_sync_o <= intackn_s;

  dbg_o(0) <= intn_r;
  dbg_o(1) <= inten_r;
  dbg_o(2) <= intackn_s;

end beh;
