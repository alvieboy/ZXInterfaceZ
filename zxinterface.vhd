library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

entity zxinterface is
  port (
    clk_i         : in std_logic;
    arst_i        : in std_logic;

    D_BUS_DIR_o   : out std_logic;

    D_BUS_OE_io   : inout std_logic;
    CTRL_OE_io    : inout std_logic;
    A_BUS_OE_io   : inout std_logic;

    FORCE_ROMCS_o : out std_logic;
    FORCE_RESET_o : out std_logic;
    FORCE_INT_o   : out std_logic;

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

    -- SPI
    SPI_SCK_i     : in std_logic;
    SPI_NCS_i     : in std_logic;
    SPI_D_io      : inout std_logic_vector(3 downto 0);

    -- Wishbone bus (master)
    wb_dat_o      : out std_logic_vector(7 downto 0);
    wb_dat_i      : in std_logic_vector(7 downto 0);
    wb_adr_o      : out std_logic_vector(23 downto 0);
    wb_we_o       : out std_logic;
    wb_cyc_o      : out std_logic;
    wb_stb_o      : out std_logic;
    wb_sel_o      : out std_logic_vector(3 downto 0);
    wb_ack_i      : in std_logic;
    wb_stall_i    : in std_logic
  );

end entity zxinterface;

architecture beh of zxinterface is

  signal dbus_oe_s          : std_logic;   -- Tri-state control
  signal dbus_oe_q_r        : std_logic;   -- Tri-state control latch
  signal data_o_s           : std_logic_vector(7 downto 0); -- Data out signal.

  signal romdata_o_s        : std_logic_vector(7 downto 0); -- ROM Data out signal.
  signal ramdata_o_s        : std_logic_vector(7 downto 0); -- RAM Data out signal.

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

  signal PC_s               : natural;


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

  signal rom_enable_s       : std_logic;
  signal ram_enable_s       : std_logic;

  signal fifo_rd_s          : std_logic;
  signal fifo_wr_s          : std_logic;
  signal fifo_full_s        : std_logic;
  signal fifo_empty_s       : std_logic;
  signal fifo_write_s       : std_logic_vector(24 downto 0);
  signal fifo_read_s        : std_logic_vector(31 downto 0);
  signal fifo_size_s        : unsigned(7 downto 0);

  signal a_r                : std_logic_vector(15 downto 0); -- Latched address
  signal d_r                : std_logic_vector(7 downto 0); -- Latched data
  signal d_valid_s          : std_logic;
  constant C_CAPTURE_DELAY: natural := 16;

  signal d_cap_shr_r: std_logic_vector(C_CAPTURE_DELAY-1 downto 0);

begin

  ck_sync: entity work.sync generic map (RESET => '0')
    port map ( clk_i => clk_i, arst_i => arst_i, din_i => XCK_i, dout_o => XCK_sync_s );

  int_sync: entity work.sync generic map (RESET => '1')
    port map ( clk_i => clk_i, arst_i => arst_i, din_i => XINT_i, dout_o => XINT_sync_s );

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


  XD_io         <= (others =>'Z') when dbus_oe_s='0' or dbus_oe_q_r='0' else data_o_s;

  D_BUS_OE_io   <= 'Z' when arst_i='1' else '0';
  CTRL_OE_io    <= 'Z' when arst_i='1' else '0';
  A_BUS_OE_io   <= 'Z' when arst_i='1' else '0';

  D_BUS_DIR_o   <= '1' --when dbus_oe_s='0' and dbus_oe_q_r='0' else '0';
                       when  dbus_oe_s='0' else '0';

  memrd_s       <=  NOT XMREQ_sync_s AND NOT XRD_sync_s;
  memwr_s       <=  NOT XMREQ_sync_s AND NOT XWR_sync_s;
  iord_s        <=  NOT XIORQ_sync_s AND NOT XRD_sync_s;
  iowr_s        <=  NOT XIORQ_sync_s AND NOT XWR_sync_s;

  -- Delay data capture signal.
  process(clk_i, arst_i)
    variable daccess_v: std_logic;
  begin
    if arst_i='1' then
      d_cap_shr_r <= (others => '0');
    elsif rising_edge(clk_i) then
      daccess_v   :=  memwr_p_s or iowr_p_s;
      d_cap_shr_r <= d_cap_shr_r(C_CAPTURE_DELAY-2 downto 0) & daccess_v;
    end if;
  end process;

  -- Capture input data after specific delay
  process(clk_i, arst_i)
  begin
    if rising_edge(clk_i) then
      if d_cap_shr_r(C_CAPTURE_DELAY-2)='1' then
        d_r <= XD_sync_s;
      end if;
    end if;
  end process;

  d_valid_s <= d_cap_shr_r(C_CAPTURE_DELAY-1);

  memrddet_inst: entity work.busopdet
    port map ( clk_i => clk_i, arst_i => arst_i, access_i => memrd_s, latch_o => memrd_latch_s, access_o => memrd_p_s );
  memwrdet_inst: entity work.busopdet
    port map ( clk_i => clk_i, arst_i => arst_i, access_i => memwr_s, latch_o => memwr_latch_s, access_o => memwr_p_s );
  iorddet_inst: entity work.busopdet
    port map ( clk_i => clk_i, arst_i => arst_i, access_i => iord_s, latch_o => iord_latch_s, access_o => iord_p_s );
  iowrdet_inst: entity work.busopdet
    port map ( clk_i => clk_i, arst_i => arst_i, access_i => iowr_s, latch_o => iowr_latch_s, access_o => iowr_p_s );

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
    elsif rising_edge(clk_i) then
      if memrd_latch_s='1' or memwr_latch_s='1' or iord_latch_s='1' or iowr_latch_s='1' then
        a_r <= XA_i;
      end if;
    end if;
  end process;

  PC_s <= to_integer(unsigned(XA_i(13 downto 0)));
  rom: entity work.spectrum_rom
    port map (
      PC_i    => PC_s,
      clk_i   => clk_i,
      en_i    => memrd_p_s,
      dat_o   => romdata_o_s
      );

  data_o_s <= romdata_o_s when rom_enable_s='1' else
              ramdata_o_s when ram_enable_s='1' else (others => '0');

	rom_enable_s  <= (not XMREQ_sync_s) and not (a_r(15) or a_r(14));
	ram_enable_s  <= not (XMREQ_sync_s or rom_enable_s);


  -- RAM write access captures

  ramfifo_inst: entity work.gh_fifo_async_sr_wf
  generic map (
    add_width   => 8, -- 256 entries
    data_width   => 25
  )
  port map (
    clk_WR      => clk_i,
    clk_RD      => SPI_SCK_i, -- TBD
    rst         => arst_i,
    srst        => arst_i,
    wr          => fifo_wr_s,
    rd          => fifo_rd_s,
    D           => fifo_write_s,
    Q           => fifo_read_s(24 downto 0),
    full        => fifo_full_s,
    empty       => fifo_empty_s
  );

  fifo_read_s(31 downto 25) <= (others => '0');

  fifo_wr_s     <= d_valid_s;
  --fifo_rd_s     <= '0';
  fifo_write_s  <= '0' & d_r & a_r;

  -- Test
  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      dbus_oe_s<='0';
--      data_o_s <= x"5A";
    elsif rising_edge(clk_i) then
      if XMREQ_sync_s='0' AND XRD_sync_s='0' then
        dbus_oe_s<='1';
      else
        dbus_oe_s<='0';
      end if;
    end if;
  end process;

  qspi_inst: entity work.spi_interface
  port map (
    SCK_i         => SPI_SCK_i,
    CSN_i         => SPI_NCS_i,
    D_io          => SPI_D_io,
    fifo_empty_i  => fifo_empty_s,
    fifo_rd_o     => fifo_rd_s,
    fifo_data_i   => fifo_read_s
  );



  FORCE_ROMCS_o <= '0';
  FORCE_RESET_o <= '0';
  FORCE_INT_o   <= '0';


end beh;

