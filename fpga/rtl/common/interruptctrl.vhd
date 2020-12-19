LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.NUMERIC_STD.ALL;


entity interruptctrl is
  port (
    clk_i     : in std_logic;
    arst_i    : in std_logic;

    int_i     : in std_logic;   -- Interrupt in, LEVEL
    inten_i   : in std_logic;   -- Interrupt enable, after processing the interrupt on host side

    intackn_i : in std_logic; --
    intackn_sync_o: out std_logic;
    intn_o    : out std_logic
  );

end entity interruptctrl;

architecture beh of interruptctrl is

  signal intackn_s    : std_logic;
  signal intn_r       : std_logic;
  signal inten_r      : std_logic;

begin

  ack_sync: entity work.sync generic map (RESET => '1')
      port map ( clk_i => clk_i, arst_i => arst_i, din_i => intackn_i, dout_o => intackn_s );



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
        if inten_r='1' and int_i='1' then
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

end beh;
