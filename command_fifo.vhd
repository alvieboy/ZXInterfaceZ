library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

entity command_fifo is
  port (
    wclk_i    : in std_logic;
    rclk_i    : in std_logic;
    arst_i    : in std_logic;
    rreset_i  : in std_logic;
    wr_i      : in std_logic;
    rd_i      : in std_logic;
    wD_i      : in std_logic_vector(7 downto 0);
    rQ_o      : out std_logic_vector(7 downto 0);
    wfull_o   : out std_logic;
    wempty_o  : out std_logic;
    rempty_o  : out std_logic
  );
end entity command_fifo;


architecture beh of command_fifo is

  signal wd_r       : std_logic_vector(7 downto 0);
  signal rq1_r      : std_logic_vector(7 downto 0);

  signal wlatched_r : std_logic;
  signal rlatched_s : std_logic;
  signal wclr_s     : std_logic;
  signal wreset_s   : std_logic;

begin

  clr_i: entity work.async_pulse2 port map (
    clki_i      => rclk_i,
    clko_i      => wclk_i,
    arst_i      => arst_i,
    pulse_i     => rd_i,
    pulse_o     => wclr_s
  );

  rst_inst: entity work.async_pulse2 port map (
    clki_i      => rclk_i,
    clko_i      => wclk_i,
    arst_i      => arst_i,
    pulse_i     => rreset_i,
    pulse_o     => wreset_s
  );

  process(wclk_i, arst_i)
  begin
    if arst_i='1' then
      wlatched_r <= '0';
    elsif rising_edge(wclk_i) then
      if wr_i='1' and wlatched_r='0' then
        wd_r <= wD_i;
        wlatched_r <= '1';
      elsif wclr_s='1' or wreset_s='1' then
        wlatched_r <= '0';
      end if;
    end if;
  end process;

  latch_sync: entity work.sync generic map (RESET => '0')
    port map ( clk_i => rclk_i, arst_i => arst_i, din_i => wlatched_r, dout_o => rlatched_s );

  process(rclk_i)
  begin
    if rising_edge(rclk_i) then
      rq1_r <= wd_r;
    end if;
  end process;

  wfull_o   <= wlatched_r;
  wempty_o  <= not wlatched_r;
  rempty_o  <= not rlatched_s;
  rQ_o      <= rq1_r;

end beh;
