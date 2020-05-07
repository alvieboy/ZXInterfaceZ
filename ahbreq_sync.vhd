LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;
LIBRARY work;
use work.ahbpkg.all;

entity ahbreq_sync is
  port (
    mclk_i    : in std_logic;
    marst_i   : in std_logic;

    sclk_i    : in std_logic;
    sarst_i   : in std_logic;


    addr_i    : in std_logic_vector(31 downto 0);
    trans_i   : in std_logic;
    valid_o   : out std_logic;
    data_o    : out std_logic_vector(31 downto 0);

    m_i       : in AHB_S2M;
    m_o       : out AHB_M2S
  );
end entity ahbreq_sync;

architecture beh of ahbreq_sync is

  signal addr_sync_s    : std_logic_vector(31 downto 0);
  signal data_sync_s    : std_logic_vector(31 downto 0);
  signal trans_sync_r   : std_logic;
  signal trans_sync_s   : std_logic;
  signal valid_syncp_s  : std_logic;

begin

  async_inst: entity work.syncv
    generic map (
      WIDTH   => 32,
      RESET   => '0'
    ) port map (
      clk_i   => mclk_i,
      arst_i  => marst_i,
      din_i   => addr_i,
      dout_o  => addr_sync_s
    );

  trans_inst: entity work.sync
    generic map (
      RESET   => '0'
    ) port map (
      clk_i   => mclk_i,
      arst_i  => marst_i,
      din_i   => trans_i,
      dout_o  => trans_sync_s
    );

  dsync_inst: entity work.syncv
    generic map (
      WIDTH   => 32,
      RESET   => '0'
    ) port map (
      clk_i   => sclk_i,
      arst_i  => sarst_i,
      din_i   => data_sync_s,
      dout_o  => data_o
    );

  vsync_inst: entity work.async_pulse2
    port map (
      clki_i   => mclk_i,
      arst_i  => marst_i,
      clko_i   => sclk_i,
      pulse_i => valid_syncp_s,
      pulse_o => valid_o
    );

  req_inst: entity work.ahbreq
    port map (
      clk_i   => mclk_i,
      arst_i  => marst_i,
      addr_i  => addr_sync_s,
      trans_i => trans_sync_s,  -- TEST
      valid_o => valid_syncp_s,
      data_o  => data_sync_s,
      m_i     => m_i,
      m_o     => m_o
    );



end beh;
