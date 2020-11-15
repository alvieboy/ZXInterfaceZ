library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
use IEEE.STD_logic_misc.all;

entity spi_slave_fifo is
  port (
    -- SPI signals
    SCK_i         : in std_logic;
    CSN_i         : in std_logic;
    MOSI_i        : in std_logic;
    MISO_o        : out std_logic;
    -- Main clock signals
    clk_i         : in std_logic;
    arst_i        : in std_logic;

    -- TX fifo
    tx_we_i       : in std_logic;
    tx_wdata_i    : in std_logic_vector(7 downto 0);
    tx_full_o     : out std_logic;
    -- TX accept
    tx_accept_o   : out std_logic;
    -- RX fifo
    rx_rd_i       : in std_logic;
    rx_rdata_o    : out std_logic_vector(8 downto 0);
    rx_empty_o    : out std_logic;
    --
    csn_o         : out std_logic
  );

end entity spi_slave_fifo;

architecture beh of spi_slave_fifo is

  signal shreg_r    : std_logic_vector(7 downto 0);
  signal outreg_r   : std_logic_vector(7 downto 0);
  signal cnt_r      : unsigned(2 downto 0);
  signal first_rx_r : std_logic;
  signal txout_s    : std_logic;
  signal cnt_half_s : std_logic;

  signal last_bit_s : std_logic;
  signal tx_bit_s   : std_logic;
  signal tx_dat_s       : std_logic_vector(7 downto 0);
  signal tx_dat_r       : std_logic_vector(7 downto 0);
  signal rx_dat_s       : std_logic_vector(8 downto 0);
  signal tx_fifo_pop_s  : std_logic;
  signal tx_fifo_empty_s  : std_logic;
  signal rx_fifo_we_s     : std_logic;
  signal rx_full_s        : std_logic;
  signal CS_s             : std_logic;

begin

  CS_s <= not CSN_i;

  cssync: entity work.sync
    generic map (
      RESET => '1'
    ) port map (
      clk_i   => clk_i,
      arst_i  => arst_i,
      din_i   => CSN_i,
      dout_o  => csn_o
    );

  -- SPI receive shift register and counter
  process(SCK_i, CSN_i)
  begin
    if CSN_i='1' then
      shreg_r     <= (others => '0');
      cnt_r       <= (others => '0');
      first_rx_r  <= '1';
      tx_dat_r    <= (others => '1');
    elsif rising_edge(SCK_i) then
      shreg_r   <= shreg_r(6 downto 0) & MOSI_i;
      cnt_r <= cnt_r + 1;
      if cnt_r="111" then
        first_rx_r <= '0';
        tx_dat_r <= tx_dat_s;
      end if;
    end if;
  end process;

  last_bit_s  <= '1' when cnt_r = "111" else '0';
  tx_bit_s    <= tx_dat_r(7-to_integer(cnt_r));
  rx_fifo_we_s <= last_bit_s and not CSN_i;


  cnt_half_s <= '1' when cnt_r = "011" and first_rx_r='0' else '0';

  tx_accept_inst: entity work.async_pulse2
  generic map (
    WIDTH => 3
  ) port map (
    clki_i  => SCK_i,
    clko_i  => clk_i,
    arst_i  => arst_i,
    pulse_i => cnt_half_s,
    pulse_o => tx_accept_o
  );


  txfifo: entity work.swfifo
  generic map (
    WIDTH => 8
  )
	port map (
    aclr    => CSN_i,
		data		=> tx_wdata_i,
		rdclk		=> SCK_i,
		rdreq		=> tx_fifo_pop_s,
		wrclk		=> clk_i,
		wrreq		=> tx_we_i,
		q		    => tx_dat_s,
		rdempty		=> tx_fifo_empty_s,
		wrfull		=> tx_full_o
	);

  rxfifo: entity work.swfifo
  generic map (
    WIDTH => 9
  )
	port map (
    aclr    => arst_i,
		data		=> rx_dat_s,
		rdclk		=> clk_i,
		rdreq		=> rx_rd_i,
		wrclk		=> SCK_i,
		wrreq		=> rx_fifo_we_s,
    q       => rx_rdata_o,
		rdempty		=> rx_empty_o,
		wrfull		=> rx_full_s
	);

  process(SCK_i, CSN_i)
  begin
    if CSN_i='1' then

    elsif falling_edge(SCK_i) then
      MISO_o <= tx_bit_s;
    end if;
  end process;

  tx_fifo_pop_s <= not tx_fifo_empty_s and last_bit_s;
  rx_dat_s <= first_rx_r & shreg_r(6 downto 0) & MOSI_i;

end beh;



