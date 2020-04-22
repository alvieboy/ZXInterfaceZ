library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
library work;
use work.zxinterfacepkg.all;

entity audio_fifo is
begin
  clk_i       : in std_logic;
  arst_i      : in std_logic;

  prescale_i  : in std_logic_vector(15 downto 0);
  enable_i    : in std_logic;
  reset_i     : in std_logic;
  audio_i     : in std_logic;
  fifo_full_o : out std_logic;

  -- FIFO connection
  fifo_clk_i  : in std_logic;
  fifo_rd_i   : in std_logic;
  fifo_data_o : out std_logic_vector(7 downto 0);
  fifo_empty_o: out std_logic;
  
  resourcefifo_inst: entity work.gh_fifo_async_sr_wf
  generic map (
    add_width   => 10, -- 1024 entries
    data_width   => 8
  )
  port map (
    clk_WR      => SPI_SCK_i,
    clk_RD      => clk_i,
    rst         => arst_i,
    srst        => resfifo_reset_s,
    wr          => resfifo_wr_s,
    rd          => resfifo_rd_s,
    D           => resfifo_write_s,
    Q           => resfifo_read_s,
    full        => resfifo_full_s(0),
    qfull       => resfifo_full_s(1),
    hfull       => resfifo_full_s(2),
    qqqfull     => resfifo_full_s(3),
    empty       => resfifo_empty_s
  );
