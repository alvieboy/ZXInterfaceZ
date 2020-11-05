LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE std.textio.all;
LIBRARY work;
use work.tbc_device_p.all;
use work.bfm_reset_p.all;
use work.bfm_clock_p.all;
use work.bfm_spimaster_p.all;
use work.bfm_spectrum_p.all;
use work.bfm_ctrlpins_p.all;
use work.bfm_ula_p.all;
use work.bfm_audiocap_p.all;
use work.bfm_qspiram_p.all;
use work.bfm_usbdevice_p.all;
use work.txt_util.all;

ENTITY tb_top IS
END tb_top;

architecture sim of tb_top is

  component tbc_device is
  port (
    SysRst_Cmd    : out Cmd_Reset_type;
    SysClk_Cmd    : out Cmd_Clock_type;

    SpectRst_Cmd  : out Cmd_Reset_type;
    SpectClk_Cmd  : out Cmd_Clock_type;

    Spimaster_Cmd   : out Cmd_Spimaster_type;
    Spectrum_Cmd    : out Cmd_Spectrum_type;
    CtrlPins_Cmd    : out Cmd_CtrlPins_type;
    Ula_Cmd         : out Cmd_Ula_type;
    Audiocap_Cmd    : out Cmd_Audiocap_type;
    QSPIRam0_Cmd    : out Cmd_QSPIRam_type;
    QSPIRam1_Cmd    : out Cmd_QSPIRam_type;
    UsbDevice_Cmd   : out Cmd_UsbDevice_type;

    -- Inputs
    Spimaster_Data  : in Data_Spimaster_type;
    Spectrum_Data   : in Data_Spectrum_type;
    CtrlPins_Data   : in Data_CtrlPins_type;
    Ula_Data        : in Data_Ula_type;
    Audiocap_Data   : in Data_Audiocap_type;
    Usbdevice_Data  : in Data_Usbdevice_type;
    QSPIRam0_Data   : in Data_QSPIRam_type;
    EXT_io          : inout std_logic_vector(13 downto 0)
  );
  end component;


  signal SysRst_Cmd_s     : Cmd_Reset_type := Cmd_Reset_Defaults;
  signal SysClk_Cmd_s     : Cmd_Clock_type := Cmd_Clock_Defaults;
  signal SpectRst_Cmd_s   : Cmd_Reset_type := Cmd_Reset_Defaults;
  signal SpectClk_Cmd_s   : Cmd_Clock_type := Cmd_Clock_Defaults;
  signal Spimaster_Cmd_s  : Cmd_Spimaster_type := Cmd_Spimaster_Defaults;
  signal Spectrum_Cmd_s   : Cmd_Spectrum_type  := Cmd_Spectrum_Defaults;
  signal CtrlPins_Cmd_s   : Cmd_CtrlPins_type  := Cmd_CtrlPins_Defaults;
  signal Ula_Cmd_s        : Cmd_Ula_type  := Cmd_Ula_Defaults;
  signal Audiocap_Cmd_s   : Cmd_Audiocap_type  := Cmd_Audiocap_Defaults;
  signal QSPIRam0_Cmd_s   : Cmd_QSPIRam_type  := Cmd_QSPIRam_Defaults;
  signal QSPIRam1_Cmd_s   : Cmd_QSPIRam_type  := Cmd_QSPIRam_Defaults;
  signal Usbdevice_Cmd_s  : Cmd_Usbdevice_type  := Cmd_Usbdevice_Defaults;

  signal Spimaster_Data_s : Data_Spimaster_type;
  signal Spectrum_Data_s  : Data_Spectrum_type;
  signal CtrlPins_Data_s  : Data_CtrlPins_type;
  signal Ula_Data_s       : Data_Ula_type;
  signal Audiocap_Data_s  : Data_Audiocap_type;
  signal Usbdevice_Data_s : Data_Usbdevice_type;
  signal QSPIRam0_Data_s  : Data_QSPIRam_type;

  SIGNAL A_BUS_OE_s : STD_LOGIC;
  SIGNAL ASDO_s: STD_LOGIC;
  SIGNAL CLK_s : STD_LOGIC;
  SIGNAL CTRL_OE_s : STD_LOGIC;
  SIGNAL D_BUS_DIR_s : STD_LOGIC;
  SIGNAL D_BUS_OE_s : STD_LOGIC;
  SIGNAL DATA0_s : STD_LOGIC;
  SIGNAL DCLK_s : STD_LOGIC;
  --SIGNAL ESP_IO14_io : STD_LOGIC := 'Z';
  --SIGNAL ESP_IO25_io : STD_LOGIC := 'Z';
  SIGNAL ESP_IO26_s : STD_LOGIC := 'Z';
  SIGNAL ESP_IO27_s : STD_LOGIC := 'Z';
  SIGNAL ESP_MISO_s : STD_LOGIC;
  SIGNAL ESP_MOSI_s : STD_LOGIC;
  SIGNAL ESP_NCSO_s : STD_LOGIC;
  SIGNAL ESP_QHD_io : STD_LOGIC;
  SIGNAL ESP_QWP_s  : STD_LOGIC;
  SIGNAL ESP_SCK_s : STD_LOGIC;
  SIGNAL FORCE_INT_s : STD_LOGIC;
  SIGNAL FORCE_WAIT_s : STD_LOGIC;
  SIGNAL FORCE_RESET_s : STD_LOGIC;
  SIGNAL FORCE_ROMCS_s : STD_LOGIC;
  SIGNAL FORCE_NMI_s : STD_LOGIC;
  SIGNAL FORCE_IORQULA_s : STD_LOGIC;
  SIGNAL NCSO_o : STD_LOGIC;
  SIGNAL SDRAM_A_o : STD_LOGIC_VECTOR(12 DOWNTO 0);
  SIGNAL SDRAM_BA_o : STD_LOGIC_VECTOR(1 DOWNTO 0);
  SIGNAL SDRAM_CK_o : STD_LOGIC;
  SIGNAL SDRAM_CKE_o : STD_LOGIC;
  SIGNAL SDRAM_CS_o : STD_LOGIC;
  SIGNAL SDRAM_D_io : STD_LOGIC_VECTOR(7 DOWNTO 0);
  SIGNAL SDRAM_DQM_o : STD_LOGIC;
  SIGNAL SDRAM_NCAS_o : STD_LOGIC;
  SIGNAL SDRAM_NRAS_o : STD_LOGIC;
  SIGNAL SDRAM_NWE_o : STD_LOGIC;
  SIGNAL XA_s : STD_LOGIC_VECTOR(15 DOWNTO 0);
  SIGNAL XCK_s : STD_LOGIC := '1';
  SIGNAL XD_io : STD_LOGIC_VECTOR(7 DOWNTO 0);
  SIGNAL XINT_s : STD_LOGIC := '1';
  SIGNAL XIORQ_s : STD_LOGIC := '1';
  SIGNAL XM1_s : STD_LOGIC := '1';
  SIGNAL XMREQ_s : STD_LOGIC := '1';
  SIGNAL XRD_s : STD_LOGIC := '1';
  SIGNAL XRFSH_s : STD_LOGIC;
  SIGNAL XWR_s : STD_LOGIC := '1';
  SIGNAL WAIT_s : STD_LOGIC := '1';

  signal ZX_A_s:  std_logic_vector(15 downto 0);
  signal ZX_D_s:  std_logic_vector(7 downto 0);

  signal spect_clk_s: std_logic;

  signal RAMD_s       : std_logic_vector(3 downto 0);
  signal RAMCLK_s      : std_logic;
  signal RAMNCS_s      : std_logic;

  -- USB PHY
  signal USB_VP_s      : std_logic := 'L';
  signal USB_VM_s      : std_logic := 'L';
  signal USB_OE_s      : std_logic;
  signal USB_SOFTCON_s : std_logic;
  signal USB_SPEED_s   : std_logic;
  signal USB_VMO_s     : std_logic;
  signal USB_VPO_s     : std_logic;
  signal USB_RCV_s     : std_logic := '0';
  -- USB power control
  signal USB_FLT_s     : std_logic := '1';
  signal USB_PWREN_s   : std_logic;
  -- Extension connector
  signal EXT_s        : std_logic_vector(13 downto 0);
  signal audio_s      : std_logic;
  signal USB_dm_s     : std_logic;
  signal USB_dp_s     : std_logic;
  signal NMI_s        : std_logic;

  signal FORCE_ROMCS2A_s : std_logic := '0';
begin

   tbc: tbc_device
    port map (
      SysRst_Cmd      => SysRst_Cmd_s,
      SysClk_Cmd      => SysClk_Cmd_s,
      SpectRst_Cmd    => SpectRst_Cmd_s,
      SpectClk_Cmd    => SpectClk_Cmd_s,
      Spimaster_Cmd   => Spimaster_Cmd_s,
      Spectrum_Cmd    => Spectrum_Cmd_s,
      CtrlPins_Cmd    => CtrlPins_Cmd_s,
      Ula_Cmd         => Ula_Cmd_s,
      Audiocap_Cmd    => Audiocap_Cmd_s,
      QSPIRam0_Cmd    => QSPIRam0_Cmd_s,
      QSPIRam1_Cmd    => QSPIRam1_Cmd_s,
      Usbdevice_Cmd   => Usbdevice_Cmd_s,
      -- Outputs
      Spimaster_Data  => Spimaster_Data_s,
      Spectrum_Data   => Spectrum_Data_s,
      CtrlPins_Data   => CtrlPins_Data_s,
      Ula_Data        => Ula_Data_s,
      Audiocap_Data   => Audiocap_Data_s,
      Usbdevice_Data  => Usbdevice_Data_s,
      QSPIRam0_Data   => QSPIRam0_Data_s,
      EXT_io          => EXT_s
    );

  sysclk_inst: entity work.bfm_clock
    port map (
      Cmd_i => SysClk_Cmd_s,
      clk_o => CLK_s
    );

  spectclk_inst: entity work.bfm_clock
    port map ( Cmd_i => SpectClk_Cmd_s, clk_o => spect_clk_s );

  sysrst_inst: entity work.bfm_reset
    port map ( Cmd_i => SysRst_Cmd_s );

  spim_inst: entity work.bfm_spimaster
    port map (
      Cmd_i   => Spimaster_Cmd_s,
      Data_o  => Spimaster_Data_s,

      mosi_o  => ESP_MOSI_s,
      miso_i  => ESP_MISO_s,
      sck_o   => ESP_SCK_s,
      csn_o   => ESP_NCSO_s
    );

  spectrum_inst: entity work.bfm_spectrum
    port map (
      Cmd_i   => Spectrum_Cmd_s,
      Data_o  => Spectrum_Data_s,

      clk_i   => spect_clk_s,
      ck_o    => XCK_s,
      wr_o    => XWR_s,
      rd_o    => XRD_s,
      mreq_o  => XMREQ_s,
      ioreq_o => XIORQ_s,
      a_o     => XA_s,
      d_io    => XD_io,
      m1_o    => XM1_s,
      wait_i  => WAIT_s,
      rfsh_o  => XRFSH_s,
      int_o   => XINT_s,
      nmi_i   => NMI_s
    );

  ula_inst: entity work.bfm_ula
    port map (
      Cmd_i   => Ula_Cmd_s,
      Data_o  => Ula_Data_s,
      A_i     => XA_s,
      D_io    => XD_io,
      IOREQn_i=> XIORQ_s,
      RDn_i   => XRD_s,
      WRn_i   => XWR_s,
      OEn_i   => FORCE_IORQULA_s
    );

  ctrlpins_inst: entity work.bfm_ctrlpins
    port map (
      Cmd_i     => CtrlPins_Cmd_s,
      Data_o    => CtrlPins_Data_s,

      USB_INTn_i    => ESP_QWP_s,
      IO26_i        => ESP_IO26_s,
      IO27_i        => ESP_IO27_s,
      FORCE_RESET_i => FORCE_RESET_s,
      FORCE_ROMCS_i => FORCE_ROMCS_s,
      FORCE_ROMCS2A_i => FORCE_ROMCS2A_s,
      FORCE_NMI_i   => FORCE_NMI_s,
      FORCE_IORQULA_i => FORCE_IORQULA_s

    );

  audiocap_inst: entity work.bfm_audiocap
    port map (
      Cmd_i   => Audiocap_Cmd_s,
      Data_o  => Audiocap_Data_s,
      audio_i => audio_s
    );


  psram0_inst: ENTITY work.bfm_qspiram
    PORT MAP (
        Cmd_i           => QSPIRam0_Cmd_s,
        Data_o          => QSPIRam0_Data_s,
        SCK_i           => RAMCLK_s,
        CSn_i           => RAMNCS_s,
        D_io            => RAMD_s(3 downto 0)
    );

  usbdev_inst: ENTITY work.bfm_usbdevice
    PORT MAP (
        Cmd_i           => Usbdevice_Cmd_s,
        Data_o          => Usbdevice_Data_s,
        DM_io           => USB_dm_s,
        DP_io           => USB_dp_s
    );

  -- USB transceiver

  usbxcvr_inst: entity work.usbxcvr_sim
    port map (
      DP        => USB_dp_s,
      DM        => USB_dm_s,
      VP_o      => USB_VP_s,
      VM_o      => USB_VM_s,
      RCV_o     => USB_RCV_s,
      OE_i      => USB_OE_s,
      SOFTCON_i => '0',--USB_SOFTCON_s,
      SPEED_i   => USB_SPEED_s,
      VMO_i     => USB_VMO_s,
      VPO_i     => USB_VPO_s
    );
  

  --psram1_inst: ENTITY work.bfm_qspiram
  --  PORT MAP (
  --      Cmd_i           => QSPIRam1_Cmd_s,
  --      SCK_i           => RAMCLK_s,
  --      CSn_i           => RAMNCS_s,
  --      D_io            => RAMD_s(7 downto 4)
  --  );

  FORCE_ROMCS2A_s <= FORCE_ROMCS_s;

  DUT: entity work.interfacez_top
	PORT MAP (
    -- list connections between master ports and signals
    A_BUS_OE_o => A_BUS_OE_s,
    CLK_i => CLK_s,
    CTRL_OE_o => CTRL_OE_s,
    D_BUS_DIR_o => D_BUS_DIR_s,
    D_BUS_OE_o => D_BUS_OE_s,
    ESP_IO26_o => ESP_IO26_s,
    ESP_IO27_o => ESP_IO27_s,
    ESP_MISO_io => ESP_MISO_s,
    ESP_MOSI_io => ESP_MOSI_s,
    ESP_NCSO_i => ESP_NCSO_s,
    ESP_QHD_io => ESP_QHD_io,
    ESP_QWP_io => ESP_QWP_s,
    ESP_SCK_i => ESP_SCK_s,
    --FORCE_INT_o => FORCE_INT_s,
    FORCE_WAIT_o => FORCE_WAIT_s,
    FORCE_RESET_o => FORCE_RESET_s,
    FORCE_ROMCS_o => FORCE_ROMCS_s,
    FORCE_NMI_o   => FORCE_NMI_s,
    FORCE_IORQULA_o => FORCE_IORQULA_s,

    RAMD_io       => RAMD_s,
    RAMCLK_o      => RAMCLK_s,
    RAMNCS_o      => RAMNCS_s,

    USB_VP_i      => USB_VP_s,
    USB_VM_i      => USB_VM_s,
    USB_OE_o      => USB_OE_s,
    --USB_SOFTCON_o => USB_SOFTCON_s,
    USB_SPEED_o   => USB_SPEED_s,
    USB_VMO_o     => USB_VMO_s,
    USB_VPO_o     => USB_VPO_s,
    USB_RCV_i     => USB_RCV_s,
    -- USB power control
    USB_FLT_i     => USB_FLT_s,
    USB_PWREN_o   => USB_PWREN_s,
    -- Extension connector
    EXT_io        => EXT_s,

    --TP5_o         => audio_s,

    XA_i => XA_s,
    XCK_i => XCK_s,
    XD_io => XD_io,
    XINT_i => XINT_s,
    XIORQ_i => XIORQ_s,
    XM1_i => XM1_s,
    XMREQ_i => XMREQ_s,
    XRD_i => XRD_s,
    XRFSH_i => XRFSH_s,
    XWR_i => XWR_s
	);

  WAIT_s <= '0' when FORCE_WAIT_s='1' else 'H';
  NMI_s <= '0' when FORCE_NMI_s='1' else 'H';

end sim;
