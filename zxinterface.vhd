library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
library work;
use work.zxinterfacepkg.all;

entity zxinterface is
  port (
    clk_i         : in std_logic;
    capclk_i      : in std_logic; -- for captures
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
    ESP_AS_NCS    : in std_logic;

    -- Active serial flash connections.

    ASDO_o        : out std_logic;
    NCSO_o        : out std_logic;
    DCLK_o        : out std_logic;
    DATA0_i       : in  std_logic;

    -- Debug
    TP5           : out std_logic;
    TP6           : out std_logic;
    --
    spec_int_o    : out std_logic;
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

	component signaltap1 is
		port (
			acq_data_in    : in std_logic_vector(27 downto 0) := (others => 'X'); -- acq_data_in
			acq_trigger_in : in std_logic_vector(0 downto 0)  := (others => 'X'); -- acq_trigger_in
			acq_clk        : in std_logic                     := 'X'              -- clk
		);
	end component signaltap1;

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

  --signal PC_s               : natural;


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


  signal rom_enable_s       : std_logic;
  signal ram_enable_s       : std_logic;

  signal fifo_rd_s          : std_logic;
  signal fifo_2_rd_s        : std_logic;
  signal fifo_wr_s          : std_logic;
  signal fifo_full_s        : std_logic;
  signal fifo_2_full_s      : std_logic;
  signal fifo_2_empty_s     : std_logic;
  signal fifo_empty_s       : std_logic;
  signal fifo_write_s       : std_logic_vector(24 downto 0);
  signal fifo_read_s        : std_logic_vector(31 downto 0);
  signal fifo_2_read_s       : std_logic_vector(31 downto 0);
  signal fifo_size_s        : unsigned(7 downto 0);
  signal fifo_reset_s       : std_logic;

  signal a_r                : std_logic_vector(15 downto 0); -- Latched address
  signal d_r                : std_logic_vector(7 downto 0); -- Latched data
  signal d_valid_s          : std_logic;
  signal d_io_mem_s         : std_logic; -- '1': IO, '0': Mem

  constant C_CAPTURE_DELAY  : natural := 16;

  signal d_cap_shr_r        : std_logic_vector(C_CAPTURE_DELAY-1 downto 0);
  signal d_io_mem_shr_r     : std_logic_vector(C_CAPTURE_DELAY-1 downto 0);

  signal mosi_s             : std_logic;
  signal miso_s             : std_logic;

  signal vidmem_en_s        : std_logic;
  signal vidmem_adr_s       : std_logic_vector(12 downto 0);
  signal vidmem_data_s      : std_logic_vector(7 downto 0);

  --signal fiforeset_s        : std_logic;
  signal spect_reset_s      : std_logic;
  signal spect_inten_spisck_s: std_logic;
  signal spect_capsyncen_spisck_s: std_logic;
  signal frameend_spisck_s: std_logic;
  signal spect_inten_s      : std_logic;
  signal spect_forceromcs_spisck_s: std_logic;
  signal spect_forceromcs_s : std_logic;


  signal spect_capsyncen_s: std_logic;
  signal framecmplt_s: std_logic;
  signal capture_clr_spisck_s:   std_logic;
  signal capture_cmp_spisck_s:   std_logic;
  signal capture_run_spisck_s:   std_logic;
  --signal capture_len_spisck_s:   std_logic_vector(CAPTURE_MEMWIDTH_BITS-1 downto 0);
  signal capture_trig_mask_spisck_s : std_logic_vector(31 downto 0);
  signal capture_trig_val_spisck_s  : std_logic_vector(31 downto 0);
  signal capmem_en_s:     std_logic;
  signal capmem_data_s:   std_logic_vector(35 downto 0);
  signal capmem_adr_s:    std_logic_vector(CAPTURE_MEMWIDTH_BITS-1 downto 0);
  signal capdin_s: std_logic_vector(27 downto 0);

  signal rom_active_s:    std_logic;

  signal rom_wc_en_s      :   std_logic;
  signal rom_wc_we_s:   std_logic;
  signal rom_wc_di_s:   std_logic_vector(7 downto 0);
  signal rom_wc_addr_s:   std_logic_vector(13 downto 0);


  signal capture_clr_s:   std_logic;
  signal capture_cmp_s:   std_logic;
  signal capture_run_s:   std_logic;
  signal capture_len_s:   std_logic_vector(CAPTURE_MEMWIDTH_BITS-1 downto 0);
  signal capture_trig_mask_s    : std_logic_vector(35-COMPRESS_BITS downto 0);
  signal capture_trig_val_s     : std_logic_vector(35-COMPRESS_BITS downto 0);
  signal capture_triggered_s: std_logic;

  signal io_enable_s:   std_logic;
  signal io_active_s:   std_logic;
  signal iodata_s   :   std_logic_vector(7 downto 0);

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


  process(arst_i, clk_i)
  begin
    if arst_i='1' then
      D_BUS_OE_io   <= '1';
      CTRL_OE_io    <= '1';
      A_BUS_OE_io   <= '1';
    elsif rising_edge(clk_i) then
      D_BUS_OE_io   <= '0';
      CTRL_OE_io    <= '0';
      A_BUS_OE_io   <= '0';
    end if;
  end process;

  D_BUS_DIR_o   <= '1' --when dbus_oe_s='0' and dbus_oe_q_r='0' else '0';
                       when  dbus_oe_s='0' else '0';


  memrd_s       <=  NOT XMREQ_sync_s AND NOT XRD_sync_s;
  memwr_s       <=  NOT XMREQ_sync_s AND NOT XWR_sync_s;
  iord_s        <=  NOT XIORQ_sync_s AND NOT XRD_sync_s;
  iowr_s        <=  NOT XIORQ_sync_s AND NOT XWR_sync_s;
  intr_s        <=  NOT XINT_sync_s;

  -- Delay data capture signal.
  process(clk_i, arst_i)
    variable daccess_v: std_logic;
  begin
    if arst_i='1' then
      d_cap_shr_r <= (others => '0');
      d_io_mem_shr_r <= (others => '0'); -- '1': IO, '0': Mem
    elsif rising_edge(clk_i) then
      daccess_v       :=  memwr_p_s or iowr_p_s;
      d_cap_shr_r     <= d_cap_shr_r(C_CAPTURE_DELAY-2 downto 0) & daccess_v;
      d_io_mem_shr_r  <= d_io_mem_shr_r(C_CAPTURE_DELAY-2 downto 0) & iowr_p_s;
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
  d_io_mem_s <= d_io_mem_shr_r(C_CAPTURE_DELAY-1);

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
        a_r <= XA_i;
      end if;
    end if;
  end process;

  --PC_s <= XA_sync_s(13 downto 0);

  r: if ROM_ENABLED generate
  rom: entity work.generic_dp_ram
    generic map (
      ADDRESS_BITS => 14, -- 16Kb
      DATA_BITS => 8
    )
    port map (
      clka    => clk_i,
      ena     => rom_active_s,
      wea     => '0',
      dia     => "00000000",
      doa     => romdata_o_s,
      addra   => XA_sync_s(13 downto 0),

      clkb    => SPI_SCK_i,
      enb     => rom_wc_en_s,
      web     => rom_wc_we_s,
      dib     => rom_wc_di_s,
      dob     => open,
      addrb   => rom_wc_addr_s
    );
  end generate;

  nr: if not ROM_ENABLED generate
    romdata_o_s <= (others => '0');
  end generate;



	rom_enable_s  <= (not XMREQ_sync_s) and not (XA_sync_s(15) or XA_sync_s(14)) and not (XRD_sync_s);



  io_inst: entity work.interfacez_io
    port map (
      clk_i   => clk_i,
      rst_i   => arst_i,

      ioreq_i => XIORQ_sync_s,
      rd_i    => XRD_sync_s,
      wrp_i   => d_io_mem_s,
      adr_i   => XA_sync_s(7 downto 0),
      
      dat_i   => d_r, -- Resynced with delay
      dat_o   => iodata_s,
      enable_o  => io_enable_s
    );

  data_o_s <= romdata_o_s when rom_enable_s='1' else
              --ramdata_o_s when ram_enable_s='1' else
              iodata_s    when io_enable_s='1' else (others => '0');



  -- RAM write access captures

  ramfifo_inst: entity work.gh_fifo_async_sr_wf
  generic map (
    add_width   => 8, -- 256 entries
    data_width   => 25
  )
  port map (
    clk_WR      => clk_i,
    clk_RD      => clk_i,--SPI_SCK_i, -- TBD
    rst         => arst_i,
    srst        => arst_i,
    wr          => fifo_wr_s,
    rd          => fifo_rd_s,
    D           => fifo_write_s,
    Q           => fifo_read_s(24 downto 0),
    full        => fifo_full_s,
    empty       => fifo_empty_s
  );

  ramfifo2_inst: entity work.gh_fifo_async_sr_wf
  generic map (
    add_width   => 8, -- 256 entries
    data_width   => 25
  )
  port map (
    clk_WR      => clk_i,
    clk_RD      => SPI_SCK_i, 
    rst         => arst_i,
    srst        => fifo_reset_s,
    wr          => fifo_wr_s,
    rd          => fifo_2_rd_s,
    D           => fifo_write_s,
    Q           => fifo_2_read_s(24 downto 0),
    full        => fifo_2_full_s,
    empty       => fifo_2_empty_s
  );

  fifo_read_s(31 downto 25) <= (others => '0');
  fifo_2_read_s(31 downto 25) <= (others => '0');

  fifo_wr_s     <= d_valid_s;
  --fifo_rd_s     <= '0';
  fifo_write_s  <= d_io_mem_s & d_r & a_r;

  -- ROM is active on access if FORCE romcs is '1'

  rom_active_s <= rom_enable_s and spect_forceromcs_s;
  io_active_s  <= io_enable_s and NOT XRD_sync_s;

  dbus_oe_s <= rom_active_s or io_active_s;

  --process(clk_i, arst_i)
  --begin
  --  if arst_i='1' then
  --    dbus_oe_s<='0';
  --  elsif rising_edge(clk_i) then
  --    if rom_active_s='1' then
  --     dbus_oe_s<='1';
  --    else
  --      dbus_oe_s<='0';
  --    end if;
  --  end if;
  --end process;

  qspi_inst: entity work.spi_interface
  port map (
    SCK_i         => SPI_SCK_i,
    CSN_i         => SPI_NCS_i,
    arst_i        => arst_i,
    --D_io          => spi_data_s,
    MOSI_i        => mosi_s,
    MISO_o        => miso_s,
    fifo_empty_i  => fifo_2_empty_s,
    fifo_rd_o     => fifo_2_rd_s,
    fifo_data_i   => fifo_2_read_s,

    vidmem_en_o   => vidmem_en_s,
    vidmem_adr_o  => vidmem_adr_s,
    vidmem_data_i => vidmem_data_s,

    capmem_en_o     => capmem_en_s,
    capmem_adr_o    => capmem_adr_s,
    capmem_data_i   => capmem_data_s,

    -- ROM connections
    rom_en_o        => rom_wc_en_s,
    rom_we_o        => rom_wc_we_s,
    rom_di_o        => rom_wc_di_s,
    rom_addr_o      => rom_wc_addr_s,


    capture_clr_o   => capture_clr_spisck_s,
    capture_run_o   => capture_run_spisck_s,
    capture_len_i   => capture_len_s,       -- This is NOT sync!!
    capture_trig_i  => capture_triggered_s, -- This is NOT sync!!
    capture_cmp_o   => capture_cmp_spisck_s,

    capture_trig_mask_o => capture_trig_mask_spisck_s,
    capture_trig_val_o  => capture_trig_val_spisck_s,


    rstfifo_o     => fifo_reset_s,
    rstspect_o    => spect_reset_s,
    intenable_o   => spect_inten_spisck_s,
    capsyncen_o   => spect_capsyncen_spisck_s,
    frameend_o    => frameend_spisck_s,
    forceromcs_o  => spect_forceromcs_spisck_s
  );

  specinten_sync: entity work.sync generic map (RESET => '0')
      port map ( clk_i => clk_i, arst_i => arst_i, din_i => spect_inten_spisck_s, dout_o => spect_inten_s );

  capsync_sync: entity work.sync generic map (RESET => '0')
      port map ( clk_i => clk_i, arst_i => arst_i, din_i => spect_capsyncen_spisck_s, dout_o => spect_capsyncen_s );

  forceromcs_sync: entity work.sync generic map (RESET => '0')
      port map ( clk_i => clk_i, arst_i => arst_i, din_i => spect_forceromcs_spisck_s, dout_o => spect_forceromcs_s );



  sc: if SCREENCAP_ENABLED generate

  fci: entity work.async_pulse port map (
    clk_i     => clk_i,
    rst_i     => arst_i,
    pulse_i   => frameend_spisck_s,
    pulse_o   => framecmplt_s
  );

  screencap_inst: entity work.screencap
    port map (
      clk_i         => clk_i,
      rst_i         => arst_i,

      fifo_empty_i  => fifo_empty_s,
      fifo_rd_o     => fifo_rd_s,
      fifo_data_i   => fifo_read_s,

      vidmem_clk_i  => SPI_SCK_i,
      vidmem_en_i   => vidmem_en_s,
      vidmem_adr_i  => vidmem_adr_s,
      vidmem_data_o => vidmem_data_s,

      capsyncen_i   => spect_capsyncen_s,
      intr_i        => intr_p_s,
      framecmplt_i  => framecmplt_s
    );
  end generate;

  nsc: if not SCREENCAP_ENABLED generate
    fifo_rd_s       <= '0';
    vidmem_data_s   <= (others =>'0');
  end generate;


  cs: if CAPTURE_ENABLED generate
  capture: block
    constant TICK_MAX: natural := 4;

    signal tickcnt_r: natural range 0 to TICK_MAX-1;
    signal tick_r   : std_logic;

    signal capdata_s              : std_logic_vector(35-COMPRESS_BITS downto 0);


  begin

    clr_sync: entity work.sync generic map (RESET => '0')
      port map ( clk_i => clk_i, arst_i => arst_i, din_i => capture_clr_spisck_s, dout_o => capture_clr_s );

    cmp_sync: entity work.sync generic map (RESET => '0')
      port map ( clk_i => clk_i, arst_i => arst_i, din_i => capture_cmp_spisck_s, dout_o => capture_cmp_s );

    run_sync: entity work.sync generic map (RESET => '0')
      port map ( clk_i => clk_i, arst_i => arst_i, din_i => capture_run_spisck_s, dout_o => capture_run_s );

    --len_syncv: entity work.syncv generic map (RESET => '0', WIDTH => capture_len_s'length)
    --  port map ( clk_i => clk_i, arst_i => arst_i, din_i => capture_len_spisck_s, dout_o => capture_len_s );

    trigmask_syncv: entity work.syncv generic map (RESET => '1', WIDTH => 36-COMPRESS_BITS)
      port map ( clk_i => clk_i, arst_i => arst_i, din_i => capture_trig_mask_spisck_s(35-COMPRESS_BITS downto 0), dout_o => capture_trig_mask_s );

    trigval_syncv: entity work.syncv generic map (RESET => '0', WIDTH => 36-COMPRESS_BITS)
      port map ( clk_i => clk_i, arst_i => arst_i, din_i => capture_trig_val_spisck_s(35-COMPRESS_BITS downto 0), dout_o => capture_trig_val_s );


    capdata_s <= XCK_sync_s & XMREQ_sync_s & XIORQ_sync_s & XRD_sync_s & XWR_sync_s & XA_sync_s & XD_sync_s;
  
    -- Stick generator
    process(clk_i,arst_i)
    begin
      if arst_i='1' then
        tick_r <= '0';
        tickcnt_r <= TICK_MAX-1;
      elsif rising_edge(clk_i) then
        tick_r <= '0';
        if tickcnt_r=0 then
          tick_r <= '1';
          tickcnt_r <= TICK_MAX-1;
        else
          tickcnt_r <= tickcnt_r -1;
        end if;
      end if;
    end process;

  logiccap_inst: entity work.logiccapture
  generic map (
    COMPRESS_BITS => COMPRESS_BITS,
    MEMWIDTH_BITS =>CAPTURE_MEMWIDTH_BITS
  )
  port map (
    clk_i       => clk_i,
    arst_i      => arst_i,
    stick_i     => tick_r,

    clr_i       => capture_clr_s,
    run_i       => capture_run_s,

    din_i       => capdata_s,
    compress_i  => capture_cmp_s,
    clen_o      => capture_len_s,
    trig_mask_i => capture_trig_mask_s,
    trig_val_i  => capture_trig_val_s,
    trig_o      => capture_triggered_s,

    ramclk_i    => SPI_SCK_i,
    ramen_i     => capmem_en_s,
    ramaddr_i   => capmem_adr_s,
    ramdo_o     => capmem_data_s
  );
  end block;
  end generate;


  st: if SIGTAP_ENABLED generate
    capdin_s <= XMREQ_sync_s & XIORQ_sync_s & XRD_sync_s & XWR_sync_s & XA_sync_s & XD_sync_s;

  	u0 : signaltap1
		port map (
			acq_data_in    => capdin_s,
			acq_trigger_in(0) => XMREQ_sync_s,
			acq_clk        => capclk_i
		);

  end generate;

  mosi_s          <= SPI_D_io(0);

  --spi_data_s      <= SPI_D_io;
  --SPI_D_io <= spi_data_s;

  SPI_D_io(0)     <= 'Z';       -- MOSI - Change when Quadmode is enabled
  SPI_D_io(1)     <= DATA0_i WHEN ESP_AS_NCS='0' ELSE miso_s; -- MISO
  SPI_D_io(2)     <= 'Z';
  SPI_D_io(3)     <= 'Z';

  ASDO_o        <= SPI_D_io(0) WHEN ESP_AS_NCS='0' else 'Z';
  NCSO_o        <= ESP_AS_NCS;
  DCLK_o        <= SPI_SCK_i WHEN ESP_AS_NCS='0' else '0';


  FORCE_ROMCS_o <= spect_forceromcs_s;
  FORCE_RESET_o <= spect_reset_s;
  FORCE_INT_o   <= '0';

  TP5 <= clk_i;
  TP6 <= XMREQ_sync_s;

  spec_int_o <= '1' when spect_inten_s='0' else XINT_sync_s;

end beh;

