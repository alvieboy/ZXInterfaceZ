library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
library work;
use work.zxinterfacepkg.all;
use work.ahbpkg.all;
-- synthesis translate_off
use work.txt_util.all;
-- synthesis translate_on
entity zxinterface is
  port (
    clk_i         : in std_logic;
    clk48_i       : in std_logic;
    capclk_i      : in std_logic; -- for captures
    videoclk_i    : in std_logic_vector(2 downto 0); -- 46.5Mhz/28.24Mhz/40Mhz input

    arst_i        : in std_logic;

    D_BUS_DIR_o   : out std_logic;
    D_BUS_OE_o    : out std_logic;
    CTRL_OE_o     : out std_logic;
    A_BUS_OE_o    : out std_logic;

    FORCE_ROMCS_o : out std_logic;
    FORCE_2AROMCS_o : out std_logic;
    FORCE_RESET_o : out std_logic;
    FORCE_INT_o   : out std_logic;
    FORCE_WAIT_o  : out std_logic;
    FORCE_NMI_o   : out std_logic;
    FORCE_IORQULA_o   : out std_logic;

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
    --SPI_D_io      : inout std_logic_vector(3 downto 0);
    SPI_MISO_o    : out std_logic;
    SPI_MOSI_i    : in std_logic;
    -- Debug
    TP5           : out std_logic;
    TP4           : out std_logic;
    dbg_o         : out std_logic_vector(15 downto 0);
    bit_i         : in std_logic;
    --

    -- USB PHY
    USB_VP_i      : in std_logic;
    USB_VM_i      : in std_logic;
    USB_RCV_i     : in std_logic;
    USB_OE_o      : out std_logic;
    USB_SOFTCON_o : out std_logic;
    USB_MODE_o    : out std_logic;
    USB_SUSPEND_o : out std_logic;
    USB_SPEED_o   : out std_logic;
    USB_VMO_o     : out std_logic;
    USB_VPO_o     : out std_logic;
    -- USB power control
    USB_FLT_i     : in std_logic;
    USB_PWREN_o   : out std_logic;
    USB_INTN_o    : out std_logic;

    spec_int_o    : out std_logic;
    spec_nreq_o   : out std_logic; -- Spectrum data request

    -- RAM interface
    RAMD_i        : in std_logic_vector(7 downto 0);
    RAMD_o        : out std_logic_vector(7 downto 0);
    RAMD_oe_o     : out std_logic_vector(7 downto 0);
    RAMCLK_o      : out std_logic;
    RAMNCS_o      : out std_logic;

    -- video out
    hsync_o       : out std_logic;
    vsync_o       : out std_logic;
    bright_o      : out std_logic;
    grb_o         : out std_logic_vector(2 downto 0);
    -- Audio
    audio_l_o     : out std_logic;
    audio_r_o     : out std_logic;
    audio_enable_o: out std_logic;
    -- Test UART
    testuart_tx_o : out std_logic;
    testuart_rx_i : in std_logic;
    bit_o         : out bit_from_cpu_t
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

	component clockmux is
		port (
			inclk1x   : in  std_logic                    := 'X';             -- inclk1x
			inclk0x   : in  std_logic                    := 'X';             -- inclk0x
			clkselect : in  std_logic                    := 'X';
			outclk    : out std_logic                                        -- outclk
		);
	end component clockmux;

  signal romdata_o_s            : std_logic_vector(7 downto 0); -- ROM Data out signal.
  signal ramdata_o_s            : std_logic_vector(7 downto 0); -- RAM Data out signal.



  signal rom_enable_s           : std_logic;
  signal ram_enable_s           : std_logic;

  signal rom_write_s            : std_logic;

  signal fifo_rd_s              : std_logic;
  signal fifo_wr_s              : std_logic;
  signal fifo_full_s            : std_logic;
  signal fifo_empty_s           : std_logic;
  signal fifo_write_s           : std_logic_vector(24 downto 0);
  signal fifo_read_s            : std_logic_vector(31 downto 0);
  signal fifo_size_s            : unsigned(7 downto 0);
  signal fifo_reset_s           : std_logic;

  -- Resynchronized ZX spectrum signals
  signal XCK_sync_s             : std_logic;
  signal XINT_sync_s            : std_logic;
  signal XMREQ_sync_s           : std_logic;
  signal XIORQ_sync_s           : std_logic;
  signal XRD_sync_s             : std_logic;
  signal XWR_sync_s             : std_logic;
  signal XM1_sync_s             : std_logic;
  signal XRFSH_sync_s           : std_logic;

  signal intr_p_s               : std_logic;
  signal bus_idle_s             : std_logic;

  signal a_s                    : std_logic_vector(15 downto 0); -- Latched address
  signal a_unlatched_s          : std_logic_vector(15 downto 0); -- Un-Latched address
  signal d_s                    : std_logic_vector(7 downto 0); -- Latched data (read). Read accesses from CPU
  signal d_unlatched_s          : std_logic_vector(7 downto 0); -- Un-latched data (read). Read accesses from CPU

  signal data_o_s               : std_logic_vector(7 downto 0); -- Data to Spectrum, multiplexed
  signal data_o_valid_s         : std_logic;
  signal data_o_postbit_s       : std_logic_vector(7 downto 0); -- Data to Spectrum, post-BIT
  signal data_o_postbit_valid_s : std_logic;

  signal io_rd_p_s              : std_logic; -- IO read pulse
  signal io_wr_p_s              : std_logic; -- IO write pulse
  signal mem_rd_p_s             : std_logic; -- Mem read pulse
  signal mem_wr_p_s             : std_logic; -- Mem write pulse
  signal opcode_rd_p_s          : std_logic; -- Opcode read
  signal mem_active_s           : std_logic; -- Memory access active

  signal mosi_s                 : std_logic;
  signal miso_s                 : std_logic;

  signal vidmem_en_s            : std_logic;
  signal vidmem_adr_s           : std_logic_vector(12 downto 0);
  signal vidmem_data_s          : std_logic_vector(7 downto 0);

  signal spect_reset_s          : std_logic;
  signal spect_inten_s          : std_logic;
  signal spect_forceromcs_s     : std_logic;
  signal spect_forceromcs_q_r   : std_logic;
  signal spect_forceromcs_bussync_s : std_logic;
  signal forceromonretn_trig_s    : std_logic;
  signal forceromcs_on_s        : std_logic;
  signal forceromcs_off_s       : std_logic;
  signal forceromonretn_r       : std_logic;
  signal forcenmi_on_s          : std_logic;
  signal forcenmi_off_s         : std_logic;
  signal nmireason_s            : std_logic_vector(7 downto 0);
  signal wait_s                 : std_logic;

  signal retn_det_s             : std_logic;
  signal spect_capsyncen_s      : std_logic;
  signal framecmplt_s           : std_logic;

  signal rom_active_s           : std_logic;

  signal io_enable_s            : std_logic;
  signal io_active_s            : std_logic;
  signal iodata_s               : std_logic_vector(7 downto 0);

  signal resfifo_wr_s           : std_logic;
  signal resfifo_rd_s           : std_logic;
  signal resfifo_write_s        : std_logic_vector(7 downto 0);
  signal resfifo_reset_s        : std_logic; 
  signal resfifo_read_s         : std_logic_vector(7 downto 0);
  signal resfifo_full_s         : std_logic_vector(3 downto 0); -- main clock
  signal resfifo_empty_s        : std_logic;                    

  signal tapfifo_reset_s        : std_logic;
  signal tapfifo_wr_s           : std_logic;
  signal tapfifo_write_s        : std_logic_vector(8 downto 0);
  signal tapfifo_full_s         : std_logic;
  signal tapfifo_used_s         : std_logic_vector(9 downto 0);
  signal tap_enable_s           : std_logic;
  signal tap_audio_s            : std_logic;


  signal cmdfifo_wr_s           : std_logic;
  signal cmdfifo_rd_s           : std_logic;
  signal cmdfifo_write_s        : std_logic_vector(7 downto 0);
  signal cmdfifo_reset_s        : std_logic;

  signal cmdfifo_read_s         : std_logic_vector(7 downto 0);
  signal cmdfifo_full_s         : std_logic;
  signal cmdfifo_empty_s        : std_logic;
  signal cmdfifo_used_s         : std_logic_vector(2 downto 0);
  signal cmdfifo_intack_s       : std_logic;

  signal port_fe_s              : std_logic_vector(5 downto 0);

  signal border_seq_s           : std_logic_vector(2 downto 0);
  signal border_off_s           : natural;

  signal start_delay_s          : std_logic_vector(7 downto 0);

  signal spec_nreq_r            : std_logic;
  constant SPEC_NREC_DELAY_MAX  : natural := 127;

  signal spec_nreq_delay_r      : natural range 0 to SPEC_NREC_DELAY_MAX := SPEC_NREC_DELAY_MAX;

  signal pixclk_s               : std_logic;
  signal vidmode_s              : std_logic_vector(1 downto 0);
  signal pixrst_s               : std_logic := '1';

  signal pc_s                   : std_logic_vector(15 downto 0);
  signal pc_valid_s             : std_logic;

  signal pc_r                   : std_logic_vector(15 downto 0);
  --signal pc_spisck_r            : std_logic_vector(15 downto 0);

  signal vidmode_resync_s       : std_logic_vector(1 downto 0);
  signal nmi_access_s           : std_logic;
  signal mem_rd_p_dly_s         : std_logic;
  signal io_rd_p_dly_s          : std_logic;

  signal nmi_r                  : std_logic;
  signal nmi_request_r          : std_logic;
  signal in_nmi_rom_r           : std_logic;
  signal ulahack_s              : std_logic;
  --signal ulahack_spisck_s       : std_logic;

  signal psram_ahb_m2s          : AHB_M2S;
  signal psram_ahb_s2m          : AHB_S2M;
  signal psram_hp_ahb_m2s       : AHB_M2S; -- High-priotity requests
  signal psram_hp_ahb_s2m       : AHB_S2M; -- High-priotity requests

  signal ram_addr_s             : std_logic_vector(23 downto 0);
  signal ram_dat_wr_s           : std_logic_vector(7 downto 0);
  signal ram_dat_rd_s           : std_logic_vector(7 downto 0);
  signal ram_wr_s               : std_logic;
  signal ram_rd_s               : std_logic;
  signal ram_ack_s              : std_logic;

  signal ahb_spi_m2s            : AHB_M2S;
  signal ahb_spi_s2m            : AHB_S2M;

  signal ahb_spect_m2s          : AHB_M2S;
  signal ahb_spect_s2m          : AHB_S2M;

  signal ahb_null_m2s           : AHB_M2S;
  signal ahb_null_s2m           : AHB_S2M;

  signal scope_ahb_m2s_s        : AHB_M2S;
  signal scope_ahb_s2m_s        : AHB_S2M;

  signal rst48_s                : std_logic;

  signal usb_int_s              : std_logic;
  signal usb_int_async_s        : std_logic;

  signal keyb_trigger_s         : std_logic;

  signal kbd_en_s               : std_logic;
  signal kbd_force_press_s      : std_logic_vector(39 downto 0); -- 40 keys.
  signal joy_en_s               : std_logic;
  signal joy_data_s             : std_logic_vector(4 downto 0);
  signal mouse_en_s             : std_logic;
  signal mouse_x_s              : std_logic_vector(7 downto 0);
  signal mouse_y_s              : std_logic_vector(7 downto 0);
  signal mouse_buttons_s        : std_logic_vector(1 downto 0);

  signal audio_left_s           : std_logic;
  signal audio_right_s          : std_logic;
  signal ear_s                  : std_logic;
  signal mic_s                  : std_logic;

  signal ay_we_s                : std_logic;
  signal ay_din_s               : std_logic_vector(7 downto 0);
  signal ay_adr_s               : std_logic_vector(3 downto 0);
  signal ay_dout_s              : std_logic_vector(7 downto 0);

  signal volume_s               : std_logic_vector(63 downto 0);

  signal romsel_s               : std_logic_vector(1 downto 0);
  signal current_rom_s          : std_logic_vector(1 downto 0);
  signal memsel_s               : std_logic_vector(2 downto 0);

  signal spect_clk_rise_s       : std_logic;
  signal spect_clk_fall_s       : std_logic;
  signal spect_m1_fall_s       : std_logic;

  signal capture_rd_s           : std_logic;
  signal capture_wr_s           : std_logic;
  signal capture_dat_s          : std_logic_vector(7 downto 0);

  signal memromsel_s            : std_logic_vector(2 downto 0);
  signal memsel_we_s            : std_logic;
  signal romsel_we_s            : std_logic;

  signal tstate_s               : std_logic;
  signal ay_en_s                : std_logic;
  signal ay_en_reads_s          : std_logic;
  
  signal mode2a_s               : std_logic;
  signal page128_pmc_s          : std_logic_vector(7 downto 0);
  signal page128_smc_s          : std_logic_vector(7 downto 0);

  function genvolume(vol: in std_logic_vector(7 downto 0)) return std_logic_vector is
  begin
    return x"00" & vol;
  end function;

  signal bit_to_cpu_s           : bit_to_cpu_t;
  signal bit_from_cpu_s         : bit_from_cpu_t;

  signal testuart_rx_empty      : std_logic;
  signal bit_control_in_s       : std_logic_vector(7 downto 0);
  signal force_iorqula_s        : std_logic;

  signal ahb_spi_m2s_s          : AHB_M2S;
  signal ahb_spi_s2m_s          : AHB_S2M;
  signal ahb_vram_m2s_s         : AHB_M2S;
  signal ahb_vram_s2m_s         : AHB_S2M;
  signal ahb_systemctrl_m2s_s   : AHB_M2S;
  signal ahb_systemctrl_s2m_s   : AHB_S2M;
  signal ahb_usb_m2s_s          : AHB_M2S;
  signal ahb_usb_s2m_s          : AHB_S2M;
  signal ahb_usb_clk48_m2s_s    : AHB_M2S;
  signal ahb_usb_clk48_s2m_s    : AHB_S2M;

  signal force_romcs_s          : std_logic;
  signal force_2aromcs_s        : std_logic;

  signal nmi_m1fall_q_r         : std_logic;
begin

  rst48_inst: entity work.rstgen
    port map (
      arst_i  => arst_i,
      clk_i   => clk48_i,
      rst_o   => rst48_s
    );



  clockmux_inst : component clockmux
		port map (
			--inclk3x   => '0',--videoclk_i(2),
			--inclk2x   => '0',--videoclk_i(0),
			inclk1x   => videoclk_i(0),
			inclk0x   => videoclk_i(1),
			clkselect => vidmode_resync_s(0),
			outclk    => pixclk_s
    );

  process(pixclk_s, arst_i)
  begin
    if arst_i='1' then
      pixrst_s <= '1';
    elsif rising_edge(pixclk_s) then
      pixrst_s <= '0';
    end if;
  end process;

  vmsync_inst: entity work.syncv
    generic map (
      WIDTH => 2,
      RESET => '0'
    ) port map (
      clk_i   => pixclk_s,
      arst_i  => arst_i,
      din_i   => vidmode_s,
      dout_o  => vidmode_resync_s
    );

  -- VGA END

  businterface_inst: entity work.businterface
    port map (
      clk_i         => clk_i,
      arst_i        => arst_i,
      bit_i         => bit_from_cpu_s.bit_enable,
      XA_i          => XA_i,
      XD_io         => XD_io,
      XCK_i         => XCK_i,
      XINT_i        => XINT_i,
      XMREQ_i       => XMREQ_i,
      XIORQ_i       => XIORQ_i,
      XRD_i         => XRD_i,
      XWR_i         => XWR_i,
      XM1_i         => XM1_i,
      XRFSH_i       => XRFSH_i,

      D_BUS_DIR_o   => D_BUS_DIR_o,
      D_BUS_OE_o    => D_BUS_OE_o,
      CTRL_OE_o     => CTRL_OE_o,
      A_BUS_OE_o    => A_BUS_OE_o,
  
      d_i           => data_o_postbit_s,
      oe_i          => data_o_postbit_valid_s,
  
      d_o           => d_s,
      d_unlatched_o => d_unlatched_s,
      a_o           => a_s,
      a_unlatched_o => a_unlatched_s,
  
      io_rd_p_o     => io_rd_p_s,
      io_rd_p_dly_o => io_rd_p_dly_s,
      io_wr_p_o     => io_wr_p_s,
      io_active_o   => io_active_s,
      mem_rd_p_o    => mem_rd_p_s,
      mem_rd_p_dly_o=> mem_rd_p_dly_s, -- Used for opcode capture
      mem_wr_p_o    => mem_wr_p_s,
      mem_active_o  => mem_active_s,
      opcode_rd_p_o => opcode_rd_p_s,
      intr_p_o      => intr_p_s,
      bus_idle_o    => bus_idle_s,
      XCK_sync_o    => XCK_sync_s,
      XINT_sync_o   => XINT_sync_s,
      XMREQ_sync_o  => XMREQ_sync_s,
      XIORQ_sync_o  => XIORQ_sync_s,
      XRD_sync_o    => XRD_sync_s,
      XWR_sync_o    => XWR_sync_s,
      XM1_sync_o    => XM1_sync_s,
      XRFSH_sync_o  => XRFSH_sync_s,

      clk_rise_o    => spect_clk_rise_s,
      clk_fall_o    => spect_clk_fall_s,
      m1_fall_o     => spect_m1_fall_s
  );

  data_o_s <= romdata_o_s when rom_enable_s='1' else
      iodata_s when io_enable_s='1' else (others => '0');

  rom_enable_s  <= mem_active_s and not (a_s(15) or a_s(14));


  -- BIT
  bit_a:    entity work.bit_in generic map ( WIDTH=>16, START=>0 )
              port map ( clk_i=>clk_i, data_i=>a_unlatched_s, bit_to_cpu_o => bit_to_cpu_s);
  bit_d:    entity work.bit_in generic map ( WIDTH=>8, START=>16 )
              port map ( clk_i=>clk_i, data_i=>d_unlatched_s, bit_to_cpu_o => bit_to_cpu_s);

  bit_control_in_s <= XCK_sync_s & XRFSH_sync_s & XM1_sync_s & XWR_sync_s & XRD_sync_s & XIORQ_sync_s &
    XMREQ_sync_s & XINT_sync_s;

  bit_c:    entity work.bit_in generic map ( WIDTH=>8, START=>24 )
              port map ( clk_i=>clk_i, data_i=>bit_control_in_s, bit_to_cpu_o => bit_to_cpu_s);

  -- BIT output

  bit_do:   entity work.bit_out generic map ( WIDTH=>8, START=>0)
              port map ( data_i => data_o_s, data_o => data_o_postbit_s, bit_from_cpu_i => bit_from_cpu_s );

  bit_doe:  entity work.bit_out generic map ( WIDTH=>1, START=>8)
              port map ( data_i(0) => data_o_valid_s, data_o(0) => data_o_postbit_valid_s, bit_from_cpu_i => bit_from_cpu_s );



  io_inst: entity work.interfacez_io
    port map (
      clk_i           => clk_i,
      rst_i           => arst_i,

      wrp_i           => io_wr_p_s,
      rdp_i           => io_rd_p_s,
      rdp_dly_i       => io_rd_p_dly_s,
      active_i        => io_active_s,
      adr_i           => a_s,
      dat_i           => d_unlatched_s,
      dat_o           => iodata_s,
      enable_o        => io_enable_s,
      forceiorqula_o  => force_iorqula_s,
      nmireason_i     => nmireason_s,
      keyb_trigger_o  => keyb_trigger_s,
      audio_i         => tap_audio_s,

      port_fe_o       => port_fe_s,
      ear_o           => ear_s,
      mic_o           => mic_s,
      ulahack_i       => ulahack_s,

      resfifo_rd_o    => resfifo_rd_s,
      resfifo_read_i  => resfifo_read_s,
      resfifo_empty_i => resfifo_empty_s,

      cmdfifo_wr_o    => cmdfifo_wr_s,
      cmdfifo_write_o => cmdfifo_write_s,
      cmdfifo_full_i  => cmdfifo_full_s ,

      ram_addr_o      => ram_addr_s,
      ram_dat_o       => ram_dat_wr_s,
      ram_dat_i       => ram_dat_rd_s,
      ram_wr_o        => ram_wr_s,
      ram_rd_o        => ram_rd_s,
      ram_ack_i       => ram_ack_s,

      kbd_en_i        => kbd_en_s,
      kbd_force_press_i => kbd_force_press_s,
      joy_en_i        => joy_en_s,
      joy_data_i      => joy_data_s,
      mouse_en_i      => mouse_en_s,
      mouse_x_i       => mouse_x_s,
      mouse_y_i       => mouse_y_s,
      mouse_buttons_i => mouse_buttons_s,

      ay_en_i         => ay_en_s,
      ay_en_reads_i   => ay_en_reads_s,
      ay_wr_o         => ay_we_s,
      ay_din_i        => ay_din_s,
      ay_adr_o        => ay_adr_s,
      ay_dout_o       => ay_dout_s,

      romsel_o        => romsel_s,
      memsel_o        => memsel_s,

      romsel_i        => memromsel_s(1 downto 0),
      memsel_i        => memromsel_s(2 downto 0),
      memsel_we_i     => memsel_we_s,
      romsel_we_i     => romsel_we_s,
      mode2a_i        => mode2a_s,

      page128_pmc_o   => page128_pmc_s,
      page128_smc_o   => page128_smc_s,


      dbg_o           => dbg_o(15 downto 8)
  );


  start_delay_s <= x"02";

  border_capture_inst: entity work.border_capture
    port map (
      clk_i         => clk_i,
      arst_i        => arst_i,

      border_ear_i  => port_fe_s(3 downto 0),
      start_delay_i => start_delay_s,
      seq_o         => border_seq_s,
      off_o         => border_off_s,
      intr_i        => '0'--intr_p_s
    );

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

  fifo_read_s(31 downto 25) <= (others => '0');

  fifo_wr_s     <= mem_wr_p_s OR io_wr_p_s;
  fifo_write_s  <= io_wr_p_s & d_s & a_s;

  -- ROM is active on access if FORCE romcs is '1'

  -- TODO: we should have more then one ROM here.

  rom_active_s    <= rom_enable_s and spect_forceromcs_bussync_s;
  --io_active_s     <= io_enable_s;-- and NOT XRD_sync_s;

  data_o_valid_s  <= rom_active_s or io_enable_s;--io_active_s;

  --
  -- Resource FIFO. This FIFO is between ESP and spectrum. ESP writes, Spectrum reads.
  --
  resourcefifo_inst: entity work.resource_fifo
  port map (
    wclk_i      => clk_i,--SPI_SCK_i,
    rclk_i      => clk_i,
    aclr_i      => resfifo_reset_s,
    wen_i       => resfifo_wr_s,
    ren_i       => resfifo_rd_s,
    wdata_i     => resfifo_write_s,
    rdata_o     => resfifo_read_s,
    wfull_o     => resfifo_full_s(0),
    wqfull_o    => resfifo_full_s(1),
    whfull_o    => resfifo_full_s(2),
    wqqqfull_o  => resfifo_full_s(3),
    rempty_o    => resfifo_empty_s
  );

  --
  -- Command FIFO. This FIFO is between Spectrum and ESP. Spectrum writes, ESP32 reads.
  --
  cmdfifo_inst: entity work.command_fifo_single
  port map (
    clk_i     => clk_i,
    arst_i      => arst_i,
    wr_i        => cmdfifo_wr_s,
    rd_i        => cmdfifo_rd_s,
    reset_i     => cmdfifo_reset_s,
    wD_i        => cmdfifo_write_s,
    rQ_o        => cmdfifo_read_s,
    full_o      => cmdfifo_full_s,
    empty_o     => cmdfifo_empty_s,
    used_o      => cmdfifo_used_s
  );


  bit_to_cpu_s.bit_request <= bit_i;


  qspi_inst: entity work.spi_interface
  port map (
    SCKx_i        => SPI_SCK_i,
    CSNx_i        => SPI_NCS_i,
    arst_i        => arst_i,
    clk_i         => clk_i,
    MOSI_i        => mosi_s,
    MISO_o        => miso_s,

    ahb_m2s_o     => ahb_spi_m2s_s,
    ahb_s2m_i     => ahb_spi_s2m_s
  );

  -- System controller
  systemctrl_inst: entity work.systemctrl
  port map (
    clk_i                 => clk_i,
    arst_i                => arst_i,
  
    ahb_m2s_i             => ahb_systemctrl_m2s_s,
    ahb_s2m_o             => ahb_systemctrl_s2m_s,

    pc_i          => pc_r,
    nmireason_o   => nmireason_s,
    bit_to_cpu_i  => bit_to_cpu_s,
    bit_from_cpu_o=> bit_from_cpu_s,

    vidmem_en_o   => vidmem_en_s,
    vidmem_adr_o  => vidmem_adr_s,
    vidmem_data_i => vidmem_data_s,

    vidmode_o     => vidmode_s,
    ulahack_o     => ulahack_s,

    rstfifo_o     => fifo_reset_s,
    rstspect_o    => spect_reset_s,
    intenable_o   => spect_inten_s,
    frameend_o    => framecmplt_s,
    mode2a_o      => mode2a_s,

    page128_pmc_i   => page128_pmc_s,
    page128_smc_i   => page128_smc_s,



    resfifo_reset_o => resfifo_reset_s,
    resfifo_wr_o    => resfifo_wr_s,
    resfifo_write_o => resfifo_write_s,
    resfifo_full_i  => resfifo_full_s,

    -- TAP fifo/control
    tapfifo_reset_o   => tapfifo_reset_s,
    tapfifo_wr_o      => tapfifo_wr_s,
    tapfifo_write_o   => tapfifo_write_s,
    tapfifo_full_i    => tapfifo_full_s,
    tapfifo_used_i    => tapfifo_used_s,
    tap_enable_o      => tap_enable_s,

    -- Command FIFO

    cmdfifo_reset_o       => cmdfifo_reset_s,
    cmdfifo_rd_o          => cmdfifo_rd_s,
    cmdfifo_read_i        => cmdfifo_read_s,
    cmdfifo_empty_i       => cmdfifo_empty_s,
    cmdfifo_intack_o      => cmdfifo_intack_s,
    cmdfifo_used_i        => cmdfifo_used_s,

    forceromonretn_trig_o => forceromonretn_trig_s,
    forceromcs_trig_on_o  => forceromcs_on_s,
    forceromcs_trig_off_o => forceromcs_off_s,
    forcenmi_trig_on_o    => forcenmi_on_s,
    forcenmi_trig_off_o    => forcenmi_off_s,
    -- USB
    capture_rd_o          => capture_rd_s,
    capture_wr_o          => capture_wr_s,
    capture_dat_i         => capture_dat_s,

    kbd_en_o              => kbd_en_s,
    kbd_force_press_o     => kbd_force_press_s,
    joy_en_o              => joy_en_s,
    joy_data_o            => joy_data_s,
    mouse_en_o            => mouse_en_s,
    mouse_x_o             => mouse_x_s,
    mouse_y_o             => mouse_y_s,
    mouse_buttons_o       => mouse_buttons_s,
    ay_en_o               => ay_en_s,
    ay_en_reads_o         => ay_en_reads_s,
    volume_o              => volume_s,
    audio_enable_o        => audio_enable_o,
    memromsel_o           => memromsel_s,
    memsel_we_o           => memsel_we_s,
    romsel_we_o           => romsel_we_s
  );

  -- Main AHB intercon.
  intercon_inst: entity work.ahb_intercon5
    generic map (
      -- Video RAM 
      S0_ADDR_MASK    => "00000001100000000010000000000000",
      S0_ADDR_VALUE   => "00000000000000000000000000000000",
      -- System controller
      S1_ADDR_MASK    => "00000001100000000011000000000000",
      S1_ADDR_VALUE   => "00000000000000000010000000000000",
      -- USB controller
      S2_ADDR_MASK    => "00000001100000000011000000000000",
      S2_ADDR_VALUE   => "00000000000000000011000000000000",
      -- PSRAM
      S3_ADDR_MASK    => "00000001000000000000000000000000",
      S3_ADDR_VALUE   => "00000001000000000000000000000000",
      -- CAPTURE
      S4_ADDR_MASK    => "00000001100000000000000000000000",
      S4_ADDR_VALUE   => "00000000100000000000000000000000"
    )
    port map (
      clk_i     => clk_i,
      arst_i    => arst_i,
      -- Master: SPI
      HMAST_I   => ahb_spi_m2s_s,
      HMAST_O   => ahb_spi_s2m_s,
      -- S0: Video RAM
      HSLAV0_I  => ahb_vram_s2m_s,
      HSLAV0_O  => ahb_vram_m2s_s,
      -- S1: System controller
      HSLAV1_I  => ahb_systemctrl_s2m_s,
      HSLAV1_O  => ahb_systemctrl_m2s_s,
      -- S2: USB controller
      HSLAV2_I  => ahb_usb_s2m_s,
      HSLAV2_O  => ahb_usb_m2s_s,
      -- S3: PSRAM
      HSLAV3_I  => psram_ahb_s2m,
      HSLAV3_O  => psram_ahb_m2s,
      -- S4: CAPTURE
      HSLAV4_I  => scope_ahb_s2m_s,
      HSLAV4_O  => scope_ahb_m2s_s
    );









  -- Interrupt generation for command FIFO
  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      spec_nreq_r   <= '1';
      spec_nreq_delay_r <= SPEC_NREC_DELAY_MAX;

    elsif rising_edge(clk_i) then
      if (spec_nreq_delay_r/=0) then
        spec_nreq_delay_r <= spec_nreq_delay_r - 1;
      end if;

      if cmdfifo_empty_s='0' and spec_nreq_delay_r=0 then
        spec_nreq_r <= '0';
      elsif cmdfifo_intack_s='1' then
        spec_nreq_r <= '1';
        spec_nreq_delay_r <= SPEC_NREC_DELAY_MAX;
      end if;
    end if;
  end process;

  -- Audio
  zxaudio_inst: entity work.zxaudio
  port map (
    clk_i   => clk_i,
    arst_i  => arst_i,
    ear_i   => ear_s,
    mic_i   => mic_s,

    dat_i   => ay_dout_s,
    dat_o   => ay_din_s,
    adr_i   => ay_adr_s,
    we_i    => ay_we_s,
    rd_i    => '1',
    left_vol_0_i  => genvolume(volume_s(7 downto 0)),
    right_vol_0_i => genvolume(volume_s(15 downto 8)),
    left_vol_1_i  => genvolume(volume_s(23 downto 16)),
    right_vol_1_i => genvolume(volume_s(31 downto 24)),
    left_vol_2_i  => genvolume(volume_s(39 downto 32)),
    right_vol_2_i => genvolume(volume_s(47 downto 40)),
    left_vol_3_i  => genvolume(volume_s(55 downto 48)),
    right_vol_3_i => genvolume(volume_s(63 downto 56)),

    audio_left_o => audio_left_s,
    audio_right_o => audio_right_s
  );

  -- TAP player.

  tap_engine_inst: entity work.tap_engine
  port map (
    clk_i     => clk_i,
    arst_i    => arst_i,

    enable_i  => tap_enable_s,
    restart_i => tapfifo_reset_s,

    fclk_i    => clk_i,--SPI_SCK_i,
    fdata_i   => tapfifo_write_s,
    fwr_i     => tapfifo_wr_s,
    ffull_o   => tapfifo_full_s,
    fused_o   => tapfifo_used_s,
    tstate_o  => tstate_s,

    audio_o   => tap_audio_s
  );


  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      spect_forceromcs_s  <='0';
      forceromonretn_r    <= '0';
    elsif rising_edge(clk_i) then
      if forceromonretn_trig_s='1' then
        forceromonretn_r <= '1';
      end if;
      if forceromcs_on_s='1' then
        spect_forceromcs_s<='1';
      elsif forceromcs_off_s='1' or (forceromonretn_r='1' and retn_det_s='1') then
        spect_forceromcs_s<='0';
        if (forceromonretn_r='1' and retn_det_s='1') then
          forceromonretn_r<='0';
        end if;
      end if;
    end if;
  end process;

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      nmi_r         <= '0';
      nmi_request_r <= '0';
      in_nmi_rom_r  <= '0';
    elsif rising_edge(clk_i) then

      if nmi_request_r='1' and spect_m1_fall_s='1' then
        nmi_r         <= '1'; -- M1 cycle, activate NMI so it can be properly latched
        nmi_request_r <= '0';
      end if;

      if forcenmi_off_s='1' then
        nmi_r         <= '0';
        in_nmi_rom_r  <= '0';
        nmi_request_r <= '0';

      elsif (in_nmi_rom_r='0') and (forcenmi_on_s='1' or keyb_trigger_s='1') then
       -- nmi_r           <= '1';
        nmi_request_r   <= '1'; -- Latch NMI request. We will wait for M1 fall
      end if;


      if nmi_access_s='1' then -- Entered NMI.
        in_nmi_rom_r    <= nmi_r;
        -- Force ROM to index 0. This allows us to use NMI in other ROMs
        --nmi_saved_rom   <= romsel_s;
        nmi_r           <= '0';
      end if;

      if retn_det_s='1' then -- If we detect a RETN, leave ROM.
        in_nmi_rom_r <= '0';
      end if;

    end if;
  end process;

  insndet_inst: entity work.insn_detector
    port map (
      clk_i       => clk_i,
      arst_i      => arst_i,
      valid_i     => mem_rd_p_dly_s,
      a_i         => a_s,
      d_i         => d_unlatched_s,
      m1_i        => XM1_sync_s,
      pc_o        => pc_s,
      pc_valid_o  => pc_valid_s,
      retn_det_o  => retn_det_s,
      nmi_access_o=> nmi_access_s
    );

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
    elsif rising_edge(clk_i) then
      if pc_valid_s='1' then
        pc_r <= pc_s;
      end if;
    end if;
  end process;

  sc: if C_SCREENCAP_ENABLED generate

  screencap_inst: entity work.screencap
    port map (
      clk_i         => clk_i,
      rst_i         => arst_i,

      fifo_empty_i  => fifo_empty_s,
      fifo_rd_o     => fifo_rd_s,
      fifo_data_i   => fifo_read_s,

      vidmem_clk_i  => clk_i,--SPI_SCK_i,
      --vidmem_en_i   => vidmem_en_s,
      --vidmem_adr_i  => vidmem_adr_s,
      --vidmem_data_o => vidmem_data_s,
      ahb_m2s_i     => ahb_vram_m2s_s,
      ahb_s2m_o     => ahb_vram_s2m_s,
      capsyncen_i   => spect_capsyncen_s,
      intr_i        => intr_p_s,
      framecmplt_i  => framecmplt_s,
      --
      vidmode_i     => vidmode_resync_s,
      border_i      => port_fe_s(2 downto 0),
      pixclk_i      => pixclk_s,
      pixrst_i      => pixrst_s,

      hsync_o       => hsync_o,
      vsync_o       => vsync_o,
      bright_o      => bright_o,
      grb_o         => grb_o
    );

  end generate;

  nsc: if not C_SCREENCAP_ENABLED generate
    fifo_rd_s       <= '0';
    vidmem_data_s   <= (others =>'0');
  end generate;

  --
  -- Do NOT allow changes to ROMCS while bus is busy, wait for start of M1 cycle
  --
  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      spect_forceromcs_bussync_s  <= '0';
      nmi_m1fall_q_r <= '0';
    elsif rising_edge(clk_i) then

      -- Force ON/OFF follows a different path.
      if spect_forceromcs_s='0' and (in_nmi_rom_r='0' and nmi_r='0') then
        spect_forceromcs_bussync_s <= '0';
      else
        if spect_m1_fall_s='1' and spect_forceromcs_s='1' then
          spect_forceromcs_bussync_s <= '1';
        elsif spect_m1_fall_s='1' and nmi_r='1' then
          nmi_m1fall_q_r         <= '1';
          if nmi_m1fall_q_r='1' then
            spect_forceromcs_bussync_s <= '1';--spect_forceromcs_s or (in_nmi_rom_r or nmi_r);  -- Also force ROM on NMI.
            nmi_m1fall_q_r <= '0'; -- Clear
          end if;
        end if;
      end if;
    end if;
  end process;

  psram_inst: entity work.psram
    port map (
      clk_i   => clk_i,
      arst_i  => arst_i,

      ahb_i   => psram_ahb_m2s,
      ahb_o   => psram_ahb_s2m,

      hp_ahb_i   => psram_hp_ahb_m2s,
      hp_ahb_o   => psram_hp_ahb_s2m,

      cs_n_o  => RAMNCS_o,
      clk_o   => RAMCLK_o,
      d_o     => RAMD_o(3 downto 0),
      oe_o    => RAMD_oe_o(3 downto 0),
      d_i     => RAMD_i(3 downto 0)
    );

  ramadapt_inst: entity work.ram_adaptor
    port map (
      clk_i           => clk_i,
      arst_i          => arst_i,
      ahb_i           => ahb_spect_s2m,
      ahb_o           => ahb_spect_m2s,

      ram_addr_i      => ram_addr_s,
      ram_dat_o       => ram_dat_rd_s,
      ram_dat_i       => ram_dat_wr_s,
      ram_wr_i        => ram_wr_s,
      ram_rd_i        => ram_rd_s,
      ram_ack_o       => ram_ack_s,

      -- Spectrum interface
      spect_addr_i    => a_s,
      spect_data_i    => d_s,
      spect_data_o    => romdata_o_s,
      rom_active_i    => rom_active_s,
      spect_clk_rise_i => spect_clk_rise_s,
      spect_clk_fall_i => spect_clk_fall_s,
      spect_wait_o    => wait_s,
      -- Ticks
      spect_mem_rd_p_i => mem_rd_p_s,
      spect_mem_wr_p_i => mem_wr_p_s,
      romsel_i        => current_rom_s,
      memsel_i        => memsel_s
  );


  current_rom_s <= romsel_s when in_nmi_rom_r='0' else "00"; -- Force ROM0 with NMI

  psram_hp_ahb_m2s    <= ahb_spect_m2s;
  ahb_spect_s2m       <= psram_hp_ahb_s2m;

  -- AHB cross-clock
  usbahbintercon_inst: entity work.ahb2ahb
  generic map (
    AWIDTH  => 12,
    DWIDTH  => 8
  )
  port map (
    -- Master
    mclk_i  => clk_i,
    arst_i  => arst_i,
    m2s_i   => ahb_usb_m2s_s,
    s2m_o   => ahb_usb_s2m_s,
    -- Slave
    sclk_i  => clk48_i,
    m2s_o   => ahb_usb_clk48_m2s_s,
    s2m_i   => ahb_usb_clk48_s2m_s
  );


  usb_inst: ENTITY work.usbhostctrl
  PORT map (
    usbclk_i      => clk48_i,
    ausbrst_i     => rst48_s,

    ahb_m2s_i     => ahb_usb_clk48_m2s_s,
    ahb_s2m_o     => ahb_usb_clk48_s2m_s,

    -- Clk/reset for interrupt sync
    clk_i         => clk_i,
    arst_i        => arst_i,
    int_o         => usb_int_s,
    int_async_o   => usb_int_async_s,

    -- Interface to transceiver
    softcon_o     => USB_SOFTCON_o,
    noe_o         => USB_OE_o,
    speed_o       => USB_SPEED_o,
    vpo_o         => USB_VPO_o,
    vmo_o         => USB_VMO_o,
    mode_o        => USB_MODE_o,
    suspend_o     => USB_SUSPEND_o,

    rcv_i         => USB_RCV_i,
    vp_i          => USB_VP_i,
    vm_i          => USB_VM_i,
    pwren_o       => USB_PWREN_o,
    pwrflt_i      => USB_FLT_i,
    dbg_o         => dbg_o(7 downto 0)
  );

  capinst: if C_CAPTURE_ENABLED generate
    capb: block
      signal trig_s: std_logic_vector(30 downto 0);
      signal nontrig_s: std_logic_vector(9 downto 0);
    begin

      trig_s(15 downto 0) <= a_s;
      trig_s(16) <= XCK_sync_s;
      trig_s(17) <= XINT_sync_s;
      trig_s(18) <= XMREQ_sync_s;
      trig_s(19) <= XIORQ_sync_s;
      trig_s(20) <= XRD_sync_s;
      trig_s(21) <= XWR_sync_s;
      trig_s(22) <= XM1_sync_s;
      trig_s(23) <= XRFSH_sync_s;
      trig_s(24) <= not wait_s;
      trig_s(25) <= not nmi_r;
      trig_s(26) <= not spect_reset_s;
      trig_s(27) <= spect_forceromcs_bussync_s;
      trig_s(28) <= force_iorqula_s;
      trig_s(29) <= usb_int_async_s;
      trig_s(30) <= spec_nreq_r;

      nontrig_s(7 downto 0) <= d_unlatched_s;
      nontrig_s(8) <= force_romcs_s;
      nontrig_s(9) <= force_2aromcs_s;

      scope_inst: entity work.scope
        generic map (
          NONTRIGGERABLE_WIDTH  => 10,
          TRIGGERABLE_WIDTH     => 31,
          WIDTH_BITS            => 10
        )
        port map (
          clk_i         => clk_i,
          arst_i        => arst_i,

          nontrig_i     => nontrig_s,
          trig_i        => trig_s,

          ahb_m2s_i     => scope_ahb_m2s_s,
          ahb_s2m_o     => scope_ahb_s2m_s
       );
    end block;
  end generate capinst;

--  ainst: entity work.audiorec
--  port map (
--    clk_i       => clk_i,
--    arst_i      => arst_i,
--  
--    tick_i      => tstate_s,
--  
--    enable_i    => '1',
--    reset_i     => '0',
--  
--    idle_o      => open,
--    err_o       => open,
--
--    -- FIFO interface
--    rd_i        => '0',
--    data_o      => open,
--    empty_o     => open,
--    used_o      => open,
--    audio_i     => tap_audio_s
--  );



  testuart_inst: entity work.testuart
  port map (
    clk_i           => clk_i,
    arst_i          => arst_i,
    rx_i            => testuart_rx_i,
    tx_o            => testuart_tx_o,
    -- RX fifo access
    fifo_used_o     => bit_to_cpu_s.rx_avail_size,
    fifo_empty_o    => testuart_rx_empty,
    fifo_rd_i       => bit_from_cpu_s.rx_read,
    fifo_data_o     => bit_to_cpu_s.rx_data,
    -- TX fifo
    uart_tx_en_i    => bit_from_cpu_s.tx_data_valid,
    uart_tx_data_i  => bit_from_cpu_s.tx_data,
    uart_tx_busy_o  => bit_to_cpu_s.tx_busy
  );

  bit_to_cpu_s.rx_avail <= not testuart_rx_empty;


  mosi_s          <= SPI_MOSI_i;
  SPI_MISO_o      <= miso_s;

    force_romcs_s   <= spect_forceromcs_bussync_s; -- Always enabled -- and not mode2a_s;
    force_2aromcs_s <= spect_forceromcs_bussync_s and mode2a_s; -- Only in 2A+ mode, due to VIDEO signal on same pin

    bit_int: entity work.bit_out generic map ( WIDTH=>1, START=>9)
              port map ( data_i(0) => '0', data_o(0) => FORCE_INT_o, bit_from_cpu_i => bit_from_cpu_s );

    bit_nmi: entity work.bit_out generic map ( WIDTH=>1, START=>10)
              port map ( data_i(0) => nmi_r, data_o(0) => FORCE_NMI_o, bit_from_cpu_i => bit_from_cpu_s );

    bit_wait: entity work.bit_out generic map ( WIDTH=>1, START=>11)
              port map ( data_i(0) => wait_s, data_o(0) => FORCE_WAIT_o, bit_from_cpu_i => bit_from_cpu_s );

    bit_romcs: entity work.bit_out generic map ( WIDTH=>1, START=>12)
              port map ( data_i(0) => force_romcs_s, data_o(0) => FORCE_ROMCS_o, bit_from_cpu_i => bit_from_cpu_s );

    bit_2aromcs: entity work.bit_out generic map ( WIDTH=>1, START=>13)
              port map ( data_i(0) => force_2aromcs_s, data_o(0) => FORCE_2AROMCS_o, bit_from_cpu_i => bit_from_cpu_s );

    bit_iorqula: entity work.bit_out generic map ( WIDTH=>1, START=>14)
              port map ( data_i(0) => force_iorqula_s, data_o(0) => FORCE_IORQULA_o, bit_from_cpu_i => bit_from_cpu_s );

    bit_rst: entity work.bit_out generic map ( WIDTH=>1, START=>15)
              port map ( data_i(0) => spect_reset_s, data_o(0) => FORCE_RESET_o, bit_from_cpu_i => bit_from_cpu_s );

   bit_o <= bit_from_cpu_s;

  TP5 <= tap_audio_s;

  spec_int_o <= '1' when spect_inten_s='0' else XINT_i;--sync_s; TBD
  spec_nreq_o <= spec_nreq_r;

  ahb_null_m2s <= C_AHB_NULL_M2S;
  USB_INTN_o <= not usb_int_async_s;

  audio_l_o <= audio_left_s;
  audio_r_o <= audio_right_s;

  
end beh;

