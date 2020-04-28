library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
library work;
use work.zxinterfacepkg.all;

entity businterface is
  port (
    clk_i         : in std_logic;
    arst_i        : in std_logic;

    -- ZX Spectrum address bus
    XA_i          : in std_logic_vector(15 downto 0);

    -- ZX Spectrum data bus
    XD_io         : inout std_logic_vector(7 downto 0);

    -- ZX Spectrum control signal sampling
    XCK_i         : in std_logic;
    XINT_i        : in std_logic;
    XMREQ_i       : in std_logic;
    XIORQ_i       : in std_logic;
    XRD_i         : in std_logic;
    XWR_i         : in std_logic;
    XM1_i         : in std_logic;
    XRFSH_i       : in std_logic;

    D_BUS_DIR_o   : out std_logic;
    D_BUS_OE_o    : out std_logic;
    CTRL_OE_o     : out std_logic;
    A_BUS_OE_o    : out std_logic;

    -- Main interface. Synchronous to clk_i

    d_i           : in std_logic_vector(7 downto 0);  -- Data to send to spectrum
    oe_i          : in std_logic;                      -- Data valid/Output enable

    d_o           : out std_logic_vector(7 downto 0);  -- Data from spectrum
    a_o           : out std_logic_vector(15 downto 0);  -- Address from spectrum

    m1_o          : out std_logic;
    intr_p_o      : out std_logic;
    bus_idle_o    : out std_logic;

    io_rd_p_o     : out std_logic; -- IO read pulse
    io_wr_p_o     : out std_logic; -- IO write pulse
    io_active_o   : out std_logic; -- IO active. Stays high until RD/WR is released
    mem_rd_p_o    : out std_logic; -- Memory read pulse
    mem_wr_p_o    : out std_logic; -- Memory write pulse
    mem_active_o  : out std_logic; -- Mem active. Stays high until RD/WR is released
    opcode_rd_p_o : out std_logic -- Opcode(M1) read pulse
  );

end entity businterface;

architecture beh of businterface is

  signal dbus_oe_s          : std_logic;   -- Tri-state control
  signal dbus_oe_q_r        : std_logic;   -- Tri-state control latch

  -- Input signals synchronized to clk_i. Delay: 2 cycles.

  signal XCK_sync_s         : std_logic;
  signal XINT_sync_s        : std_logic;
  signal XMREQ_sync_s       : std_logic;
  signal XIORQ_sync_s       : std_logic;
  signal XRD_sync_s         : std_logic;
  signal XWR_sync_s         : std_logic;
  signal XM1_sync_s         : std_logic;
  signal XRFSH_sync_s       : std_logic;
  signal XA_sync_s          : std_logic_vector(15 downto 0);
  signal XD_sync_s          : std_logic_vector(7 downto 0);

  signal rd_glitch_s        : std_logic;
  signal wr_glitch_s        : std_logic;
  signal rd_dly_s           : std_logic;
  signal wr_dly_s           : std_logic;

  signal memrd_s            : std_logic; -- Memory read request
  signal memrd_latch_s      : std_logic;
  signal memrd_p_s          : std_logic; -- Memory read pulse
  signal memwr_s            : std_logic; 
  signal memwr_latch_s      : std_logic;
  signal memwr_p_s          : std_logic; 
  signal iord_s             : std_logic; 
  signal iord_latch_s       : std_logic;
  signal iord_p_s           : std_logic; 
  signal iowr_s             : std_logic; 
  signal iowr_latch_s       : std_logic;
  signal iowr_p_s           : std_logic;
  signal intr_s             : std_logic;
  signal intr_latch_s       : std_logic;
  signal intr_p_s           : std_logic;

  signal a_r                : std_logic_vector(15 downto 0);
  signal d_r                : std_logic_vector(7 downto 0);

begin

  ck_sync: entity work.sync generic map (RESET => '0')
    port map ( clk_i => clk_i, arst_i => arst_i, din_i => XCK_i, dout_o => XCK_sync_s );

  int_sync: entity work.sync generic map (RESET => '1')
    port map ( clk_i => clk_i, arst_i => arst_i, din_i => XINT_i, dout_o => XINT_sync_s );

  m1_sync: entity work.sync generic map (RESET => '1')
    port map ( clk_i => clk_i, arst_i => arst_i, din_i => XM1_i, dout_o => XM1_sync_s );

  mreq_sync: entity work.sync generic map (RESET => '1')
    port map ( clk_i => clk_i, arst_i => arst_i, din_i => XMREQ_i, dout_o => XMREQ_sync_s );

  ioreq_sync: entity work.sync generic map (RESET => '1')
    port map ( clk_i => clk_i, arst_i => arst_i, din_i => XIORQ_i, dout_o => XIORQ_sync_s );

  rd_sync: entity work.sync generic map (RESET => '1')
    port map ( clk_i => clk_i, arst_i => arst_i, din_i => XRD_i, dout_o => XRD_sync_s );

  wr_sync: entity work.sync generic map (RESET => '1')
    port map ( clk_i => clk_i, arst_i => arst_i, din_i => XWR_i, dout_o => XWR_sync_s );

  a_sync: entity work.syncv generic map (RESET => '0', WIDTH => 16)
    port map ( clk_i => clk_i, arst_i => arst_i, din_i => XA_i, dout_o => XA_sync_s );

  d_sync: entity work.syncv generic map (RESET => '0', WIDTH => 8)
    port map ( clk_i => clk_i, arst_i => arst_i, din_i => XD_io, dout_o => XD_sync_s );

  -- Glitch filters for rd/wr
  rd_glitch_filter_inst: entity work.glitch_filter
    generic map (
      RESET => '1'
    )
    port map (
      clk_i   => clk_i,
      arst_i  => arst_i,
      i_i     => XRD_sync_s,
      o_o     => rd_glitch_s
    );

  wr_glitch_filter_inst: entity work.glitch_filter
    generic map (
      RESET => '1'
    )
    port map (
      clk_i   => clk_i,
      arst_i  => arst_i,
      i_i     => XWR_sync_s,
      o_o     => wr_glitch_s
    );

  -- Delay detectors for RD/WR

  rd_delay_filter_inst: entity work.delay_filter
    generic map (
      RESET => '1'
    )
    port map (
      clk_i   => clk_i,
      arst_i  => arst_i,
      i_i     => rd_glitch_s,
      o_o     => rd_dly_s
    );

  wr_delay_filter_inst: entity work.delay_filter
    generic map (
      RESET => '1'
    )
    port map (
      clk_i   => clk_i,
      arst_i  => arst_i,
      i_i     => wr_glitch_s,
      o_o     => wr_dly_s
    );


  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      dbus_oe_q_r <= '0';
    elsif rising_edge(clk_i) then
      dbus_oe_q_r <= dbus_oe_s;
    end if;
  end process;

  -- Requirement for OE sequencing.
  --
  -- Req. OE   Clock0                       Clock1
  --  0->1     Dir=1, nOEo=0, OE=0 [01]     Dir=1, nOEo=0, OE=1  [11]
  --  1->0     Dir=1, nOEo=0, OE=0 [10]     Dir=0, nOEo=0, OE=0  [00]

  XD_io         <= (others =>'Z') when dbus_oe_s='0' or dbus_oe_q_r='0' else d_i;
  D_BUS_DIR_o   <= '1' when dbus_oe_s='0' else '0';

  dbus_oe_s     <= '0' when rd_dly_s='0' or oe_i='0' else '1';

  process(arst_i, clk_i)
  begin
    if arst_i='1' then
      D_BUS_OE_o   <= '1';
      CTRL_OE_o    <= '1';
      A_BUS_OE_o   <= '1';
    elsif rising_edge(clk_i) then
      D_BUS_OE_o   <= '0';
      CTRL_OE_o    <= '0';
      A_BUS_OE_o   <= '0';
    end if;
  end process;


  memrd_s <= rd_dly_s AND NOT XMREQ_sync_s;
  memwr_s <= wr_dly_s AND NOT XMREQ_sync_s;
  iord_s  <= rd_dly_s AND NOT XIORQ_sync_s AND XM1_sync_s;
  iowr_s  <= wr_dly_s AND NOT XIORQ_sync_s AND XM1_sync_s;

  memrddet_inst: entity work.busopdet
    port map ( clk_i => clk_i, arst_i => arst_i, access_i => memrd_s, latch_o => memrd_latch_s, access_o => memrd_p_s );

  memwrdet_inst: entity work.busopdet
    port map ( clk_i => clk_i, arst_i => arst_i, access_i => memwr_s, latch_o => memwr_latch_s, access_o => memwr_p_s );

  iorddet_inst: entity work.busopdet
    port map ( clk_i => clk_i, arst_i => arst_i, access_i => iord_s, latch_o => iord_latch_s, access_o => iord_p_s );

  iowrdet_inst: entity work.busopdet
    port map ( clk_i => clk_i, arst_i => arst_i, access_i => iowr_s, latch_o => iowr_latch_s, access_o => iowr_p_s );

  intrdet_inst: entity work.busopdet
    port map ( clk_i => clk_i, arst_i => arst_i, access_i => intr_s, latch_o => intr_latch_s, access_o => intr_p_s );

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
    elsif rising_edge(clk_i) then
      if memrd_latch_s='1' or memwr_latch_s='1' or iord_latch_s='1' or iowr_latch_s='1' then
        a_r <= XA_sync_s;
        d_r <= XD_sync_s;
      end if;
    end if;
  end process;


  d_o           <= d_r;
  a_o           <= a_r;
  io_rd_p_o     <= iord_p_s;
  io_wr_p_o     <= iowr_p_s;
  io_active_o   <= iord_s OR iowr_s;
  mem_rd_p_o    <= memrd_p_s;
  mem_wr_p_o    <= memwr_p_s;
  mem_active_o  <= memrd_s OR memwr_s;
  opcode_rd_p_o <= '0';
  m1_o          <= XM1_sync_s;
  intr_p_o      <= intr_p_s;
  bus_idle_o    <= XRD_sync_s AND XWR_sync_s AND XMREQ_sync_s AND XIORQ_sync_s;


end beh;
