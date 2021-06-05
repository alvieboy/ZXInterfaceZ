library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

entity tap_engine is
  port (
    clk_i     : in std_logic;
    arst_i    : in std_logic;

    enable_i  : in std_logic;
    restart_i : in std_logic;
    pause_i   : in std_logic;

    --fclk_i    : in std_logic;
    fdata_i   : in std_logic_vector(8 downto 0);
    fwr_i     : in std_logic;
    ffull_o   : out std_logic;
    fused_o   : out std_logic_vector(9 downto 0);
    tstate_o  : out std_logic;
    tstateclk_o : out std_logic;

    audio_o   : out std_logic
);
end entity tap_engine;


architecture beh of tap_engine is

  signal tstate_s     : std_logic;
  signal tstateclk_r  : std_logic;
  constant TSTATECNT  : natural := 27;
  signal count_r      : natural range 0 to TSTATECNT-1 := TSTATECNT-1;
  signal freset_s     : std_logic;
  signal data_s       : std_logic_vector(8 downto 0);
  signal rd_s         : std_logic;
  signal empty_s      : std_logic;
  signal ready_s      : std_logic;

begin

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      count_r <= TSTATECNT-1;
      tstate_s    <='0';
      tstateclk_r <='0';
    elsif rising_edge(clk_i) then
      if count_r=0 then
        tstate_s    <='1';
        tstateclk_r <= '1';
        count_r     <= TSTATECNT-1;
      else
        tstate_s <='0';
        count_r <= count_r - 1;
        if count_r=(TSTATECNT/2) then
          tstateclk_r <= '0';
        end if;
      end if;
    end if;
  end process;


  -- TODO: convert this into a single clock FIFO
  fifo_inst: entity work.tap_fifo
  port map (
    clk_i     => clk_i,
    -- Write port
    wdata_i   => fdata_i,
    wen_i     => fwr_i,
    usedw_o   => fused_o,
    full_o    => ffull_o,

    -- Read port
    --rclk_i    => clk_i,
    rdata_o   => data_s,
    ren_i     => rd_s,
    empty_o  => empty_s,

    -- Others
    aclr_i    => freset_s
  );

  freset_s  <= arst_i or restart_i;
  ready_s   <= not empty_s;

  player_inst: entity work.tap_player
  port map (
    clk_i     => clk_i,
    arst_i    => arst_i,
    tstate_i  => tstate_s,
    enable_i  => enable_i,
    restart_i => restart_i,
    pause_i   => pause_i,

    valid_i   => ready_s,
    data_i    => data_s,
    rd_o      => rd_s,

    audio_o   => audio_o
  );

  tstate_o    <= tstate_s;
  tstateclk_o <= tstateclk_r;

end beh;

