library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
use IEEE.STD_logic_misc.all;

-- SPI slave, using resync to main clock.
--

entity qspi_slave_resync is
  port (
    clk_i         : in std_logic;
    arst_i        : in std_logic;

    SCK_i         : in std_logic;
    CSN_i         : in std_logic;

    MOSI_i        : in std_logic;
    MISO_o        : out std_logic;

    txdat_i       : in std_logic_vector(7 downto 0);
    txload_i      : in std_logic;
    txready_o     : out std_logic;
    txden_i       : in std_logic;
    qen_i         : in std_logic; -- Quad enabled

    dat_o         : out std_logic_vector(7 downto 0);
    dat_valid_o   : out std_logic;
    sck_rise_o    : out std_logic;
    sck_fall_o    : out std_logic;
    csn_o         : out std_logic
  );

end entity qspi_slave_resync;

architecture beh of qspi_slave_resync is

  signal shreg_r  : std_logic_vector(7 downto 0);
  signal outreg_r : std_logic_vector(7 downto 0);
  --signal nibble_r : std_logic;
  signal cnt_r    : unsigned(2 downto 0);
  --signal nibbleout_r : std_logic;
  signal din_s    : std_logic_vector(7 downto 0);
  signal outen_r  : std_logic;
  signal load_s   : std_logic;
  signal txready_r : std_logic;

  signal sck_resync_s : std_logic;
  signal csn_resync_s : std_logic;
  signal sck_r        : std_logic;
  signal sck_fall_s   : std_logic;
  signal sck_rise_s   : std_logic;

begin

  rsck: entity work.sync
    generic map (
      RESET => '1'
    )
    port map (
    clk_i   => clk_i,
    arst_i  => arst_i,
    din_i   => SCK_i,
    dout_o  => sck_resync_s
  );

  rncs: entity work.sync
    generic map (
      RESET => '1'
    )
    port map (
    clk_i   => clk_i,
    arst_i  => arst_i,
    din_i   => CSN_i,
    dout_o  => csn_resync_s
    );

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      sck_r <= '1';
    elsif rising_edge(clk_i) then
      sck_r <= sck_resync_s;
    end if;                 
  end process;

  sck_fall_s <= sck_r and not sck_resync_s;
  sck_rise_s <= not sck_r and sck_resync_s;

  process(clk_i, csn_resync_s)
  begin
    if csn_resync_s='1' then
      shreg_r     <= (others => '0');
      cnt_r       <= (others => '0');
      txready_r   <= '1';
    elsif rising_edge(clk_i) then
      if sck_rise_s='1' then
        if qen_i='1' then
          --shreg_r   <= shreg_r(3 downto 0) & D_io;
          shreg_r   <= shreg_r(6 downto 0) & MOSI_i;
        else
          shreg_r   <= shreg_r(6 downto 0) & MOSI_i;
        end if;
        cnt_r <= cnt_r + 1;
        txready_r <= load_s;
      end if;
    end if;
  end process;

  process(cnt_r, qen_i)
  begin
    if qen_i='1' then
      load_s <= cnt_r(0);
    else
      load_s <= and_reduce(std_logic_vector(cnt_r));
    end if;
  end process;

  process(clk_i, csn_resync_s)
  begin
    if csn_resync_s='1' then
      outreg_r      <= (others => '0');
      --nibbleout_r   <= '1';
    elsif rising_edge(clk_i) then
      if sck_fall_s='1' then
        if txready_r='1' then
          outen_r <= txden_i;
        end if;
        if txden_i='1' then
          if txload_i='1' and txready_r='1' then
            outreg_r <= txdat_i;
          else
            if qen_i='1' then
              outreg_r(7 downto 4) <= outreg_r(3 downto 0);
            else
              outreg_r(7 downto 0) <= outreg_r(6 downto 0) & '0';
            end if;
          end if;
        end if;
      end if;
    end if;
  end process;

  txready_o     <= txready_r;

  process(qen_i, shreg_r, MOSI_i, outen_r, outreg_r)
  begin
    if qen_i='1' then
      --din_s         <= shreg_r(3 downto 0) & D_io;
      din_s         <= shreg_r(6 downto 0) & MOSI_i;
    else
      din_s         <= shreg_r(6 downto 0) & MOSI_i;
    end if;

    if outen_r='0' then
      MISO_o <= 'Z';--D_io <= (others => 'Z');
    else
      if qen_i='0' then
        --QWP <= 'Z';
        --QHD <= 'Z';
        --MOSI <= 'Z';
        MISO_o <= outreg_r(7);
      else
        --D_io <= outreg_r(7 downto 4);
        MISO_o <= outreg_r(7);
      end if;
    end if;

  end process;

  dat_o         <= din_s;
  dat_valid_o   <= load_s;
  sck_rise_o    <= sck_rise_s;
  sck_fall_o    <= sck_fall_s;
  csn_o         <= csn_resync_s;

end architecture;

