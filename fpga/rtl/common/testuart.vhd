library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
library work;
use work.zxinterfacepkg.all;

entity testuart is
  port (
    clk_i           : in std_logic;
    arst_i          : in std_logic;
    rx_i            : in std_logic;
    tx_o            : out std_logic;
    -- RX fifo access
    fifo_used_o     : out std_logic_vector(1 downto 0);
    fifo_empty_o    : out std_logic;
    fifo_rd_i       : in std_logic;
    fifo_data_o     : out std_logic_vector(7 downto 0);
    -- TX
    uart_tx_en_i    : in std_logic;
    uart_tx_data_i  : in std_logic_vector(7 downto 0);
    uart_tx_busy_o  : out std_logic
  );
end entity testuart;

architecture beh of testuart is

  signal uart_data_s        : std_logic_vector(7 downto 0);
  signal fifo_rd_s          : std_logic;
  signal uart_data_valid_s  : std_logic;
  signal rxtick_s           : std_logic;
  signal txtick_s           : std_logic;
  signal fifo_full_s        : std_logic;
  signal uart_read_en_s     : std_logic;
  signal uart_data_avail_s  : std_logic;
  signal uart_read_data_s   : std_logic_vector(7 downto 0);

begin

  rxfifo_inst: entity work.smallfifo
	  port map (
		  aclr		=> arst_i,
		  clock		=> clk_i,
		  data		=> uart_data_s,
		  rdreq		=> fifo_rd_s,
		  sclr		=> '0',
		  wrreq		=> uart_data_valid_s,
		  empty		=> fifo_empty_o,
		  full		=> fifo_full_s,
		  q		    => fifo_data_o,
		  usedw		=> fifo_used_o
	);

  -- baudrate generators

  brgen_rx_inst: entity work.uart_brgen
    port map (
      clk     => clk_i,
      rst     => arst_i,
      en      => '1',
      count   => x"0033", -- 115200
      clkout  => rxtick_s
    );

  brgen_tx_inst: entity work.uart_brgen
    port map (
      clk     => clk_i,
      rst     => arst_i,
      en      => rxtick_s,
      clkout  => txtick_s,
      count   => x"000f"
    );

  -- RX unit

  uart_rx_inst: entity work.uart_rx
    port map (
      clk       => clk_i,
      rst       => arst_i,
      rx        => rx_i,
      rxclk     => rxtick_s,
      read      => uart_read_en_s,
      data      => uart_read_data_s,
      data_av   => uart_data_avail_s
  );

  -- TX unit

  uart_tx_inst: entity work.TxUnit
    port map(
      clk_i     => clk_i,
      reset_i   => arst_i,
      enable_i  => txtick_s,
      load_i    => uart_tx_en_i,
      txd_o     => tx_o,
      busy_o    => uart_tx_busy_o,
      --intx_o   : out std_logic;  -- In transmit
      datai_i   => uart_tx_data_i
    );

end beh;

