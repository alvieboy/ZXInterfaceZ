LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.tbc_device_p.all;
use work.bfm_reset_p.all;
use work.bfm_clock_p.all;
use work.bfm_spimaster_p.all;
use work.bfm_spectrum_p.all;
use work.bfm_ctrlpins_p.all;

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
    -- Inputs
    Spimaster_Data  : in Data_Spimaster_type;
    Spectrum_Data   : in Data_Spectrum_type;
    CtrlPins_Data   : in Data_CtrlPins_type

  );
  end component;


  signal SysRst_Cmd_s     : Cmd_Reset_type;
  signal SysClk_Cmd_s     : Cmd_Clock_type;
  signal SpectRst_Cmd_s   : Cmd_Reset_type;
  signal SpectClk_Cmd_s   : Cmd_Clock_type;
  signal Spimaster_Cmd_s  : Cmd_Spimaster_type;
  signal Spectrum_Cmd_s   : Cmd_Spectrum_type;
  signal CtrlPins_Cmd_s   : Cmd_CtrlPins_type;

  signal Spimaster_Data_s : Data_Spimaster_type;
  signal Spectrum_Data_s  : Data_Spectrum_type;
  signal CtrlPins_Data_s  : Data_CtrlPins_type;


  SIGNAL A_BUS_OE_io : STD_LOGIC;
  SIGNAL ASDO_s: STD_LOGIC;
  SIGNAL CLK_s : STD_LOGIC;
  SIGNAL CTRL_OE_io : STD_LOGIC;
  SIGNAL D_BUS_DIR_o : STD_LOGIC;
  SIGNAL D_BUS_OE_io : STD_LOGIC;
  SIGNAL DATA0_s : STD_LOGIC;
  SIGNAL DCLK_s : STD_LOGIC;
  SIGNAL ESP_IO14_io : STD_LOGIC;
  SIGNAL ESP_IO25_io : STD_LOGIC;
  SIGNAL ESP_IO26_io : STD_LOGIC;
  SIGNAL ESP_IO27_io : STD_LOGIC;
  SIGNAL ESP_MISO_s : STD_LOGIC;
  SIGNAL ESP_MOSI_s : STD_LOGIC;
  SIGNAL ESP_NCSO_s : STD_LOGIC;
  SIGNAL ESP_QHD_io : STD_LOGIC;
  SIGNAL ESP_QWP_io : STD_LOGIC;
  SIGNAL ESP_SCK_s : STD_LOGIC;
  SIGNAL FORCE_INT_o : STD_LOGIC;
  SIGNAL FORCE_RESET_o : STD_LOGIC;
  SIGNAL FORCE_ROMCS_o : STD_LOGIC;
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

  signal ZX_A_s:  std_logic_vector(15 downto 0);
  signal ZX_D_s:  std_logic_vector(7 downto 0);

  signal spect_clk_s: std_logic;

  signal RAMD_s       : std_logic_vector(7 downto 0);
  signal RAMCLK_s      : std_logic;
  signal RAMNCS_s      : std_logic;

  -- USB PHY
  signal USB_VP_s      : std_logic;
  signal USB_VM_s      : std_logic;
  signal USB_OE_s      : std_logic;
  signal USB_SOFTCON_s : std_logic;
  signal USB_SPEED_s   : std_logic;
  signal USB_VMO_s     : std_logic;
  signal USB_VPO_s     : std_logic;
  signal USB_RCV_s     : std_logic;
  -- USB power control
  signal USB_FLT_s     : std_logic;
  signal USB_PWREN_s   : std_logic;
  -- Extension connector
  signal EXT_s        : std_logic_vector(7 downto 0);

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
      -- Outputs
      Spimaster_Data  => Spimaster_Data_s,
      Spectrum_Data   => Spectrum_Data_s,
      CtrlPins_Data   => CtrlPins_Data_s
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
      rfsh_o  => XRFSH_s
    );

  ctrlpins_inst: entity work.bfm_ctrlpins
    port map (
      Cmd_i     => CtrlPins_Cmd_s,
      Data_o    => CtrlPins_Data_s,

      IO26_io => ESP_IO26_io,
      IO27_io => ESP_IO27_io
    );

  DUT: entity work.interfacez_top
	PORT MAP (
    -- list connections between master ports and signals
    A_BUS_OE_io => A_BUS_OE_io,
    CLK_i => CLK_s,
    CTRL_OE_io => CTRL_OE_io,
    D_BUS_DIR_o => D_BUS_DIR_o,
    D_BUS_OE_io => D_BUS_OE_io,
    ESP_IO26_io => ESP_IO26_io,
    ESP_IO27_io => ESP_IO27_io,
    ESP_MISO_io => ESP_MISO_s,
    ESP_MOSI_io => ESP_MOSI_s,
    ESP_NCSO_i => ESP_NCSO_s,
    ESP_QHD_io => ESP_QHD_io,
    ESP_QWP_io => ESP_QWP_io,
    ESP_SCK_i => ESP_SCK_s,
    FORCE_INT_o => FORCE_INT_o,
    FORCE_RESET_o => FORCE_RESET_o,
    FORCE_ROMCS_o => FORCE_ROMCS_o,
    FORCE_NMI_o   => FORCE_NMI_s,
    FORCE_IORQULA_o => FORCE_IORQULA_s,

    RAMD_io       => RAMD_s,
    RAMCLK_o      => RAMCLK_s,
    RAMNCS_o      => RAMNCS_s,

    USB_VP_i      => USB_VP_s,
    USB_VM_i      => USB_VM_s,
    USB_OE_o      => USB_OE_s,
    USB_SOFTCON_o => USB_SOFTCON_s,
    USB_SPEED_o   => USB_SPEED_s,
    USB_VMO_o     => USB_VMO_s,
    USB_VPO_o     => USB_VPO_s,
    USB_RCV_i     => USB_RCV_s,
    -- USB power control
    USB_FLT_i     => USB_FLT_s,
    USB_PWREN_o   => USB_PWREN_s,
    -- Extension connector
    EXT_io        => EXT_s,


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


  
end sim;
