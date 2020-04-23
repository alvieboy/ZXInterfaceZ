library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity interfacez_top is
  port (
    CLK_i         : in std_logic;

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

    -- Buffer control

    D_BUS_DIR_o   : out std_logic;
    D_BUS_OE_io   : inout std_logic;
    CTRL_OE_io    : inout std_logic;
    A_BUS_OE_io   : inout std_logic;

    -- ZX Spectrum control
    FORCE_ROMCS_o : out std_logic;
    FORCE_RESET_o : out std_logic;
    FORCE_INT_o   : out std_logic;
    FORCE_NMI_o   : out std_logic;
    FORCE_IORQULA_o: out std_logic;


    -- ESP32 IOs
    ESP_IO26_io   : inout std_logic;
    ESP_IO27_io   : inout std_logic;

    -- ESP32 SPI interface
    ESP_QHD_io    : inout std_logic;
    ESP_MISO_io   : inout std_logic;
    ESP_NCSO_i    : in std_logic;
    ESP_SCK_i     : in std_logic;
    ESP_QWP_io    : inout std_logic;
    ESP_MOSI_io   : inout std_logic;
    -- LED outputs
    LED2_o        : out std_logic;
    FLED_o        : out std_logic_vector(2 downto 0);

    -- RAM interface
    RAMD_io       : inout std_logic_vector(4 downto 0);
    RAMCLK_o      : out std_logic;
    RAMNCS_o      : out std_logic;

    -- USB PHY
    USB_VP_i      : in std_logic;
    USB_VM_i      : in std_logic;
    USB_OE_o      : out std_logic;
    USB_SOFTCON_o : out std_logic;
    USB_SPEED_o   : out std_logic;
    USB_VMO_o     : out std_logic;
    USB_VPO_o     : out std_logic;
    -- USB power control
    USB_FLT_i     : in std_logic;
    USB_PWREN_o   : out std_logic;
    -- Extension connector
    EXT_io        : inout std_logic_vector(7 downto 0);

    -- Testpoints
    TP5_o         : out std_logic
  );
end interfacez_top;

architecture str of interfacez_top is
  
  signal sysclk_s     : std_logic;
  signal sysrst_s     : std_logic;
  signal plllock_s    : std_logic;
  signal capclk_s     : std_logic;

  signal wb_rdat      : std_logic_vector(7 downto 0);
  signal wb_wdat      : std_logic_vector(7 downto 0);
  signal wb_adr       : std_logic_vector(23 downto 0);
  signal wb_we        : std_logic;
  signal wb_cyc       : std_logic;
  signal wb_stb       : std_logic;
  --signal wb_sel       : std_logic;--_vector(3 downto 0);
  signal wb_ack       : std_logic;
  signal wb_stall     : std_logic;


begin

  rstgen_inst: entity work.rstgen
    port map (
      arstn_i   => plllock_s,
      clk_i     => sysclk_s,
      rst_o     => sysrst_s
    );

  corepll_inst: entity work.corepll
    port map (
      inclk0  => CLK_i,
      c0      => sysclk_s,
      c1      => open,--sdramclk2_s,
      c2      => capclk_s,
      locked  => plllock_s
  );

  interface_inst: entity work.zxinterface
    port map (
      clk_i         => sysclk_s,
      capclk_i      => capclk_s,
      arst_i        => sysrst_s,
      D_BUS_DIR_o   => D_BUS_DIR_o,
      D_BUS_OE_io   => D_BUS_OE_io,
      CTRL_OE_io    => CTRL_OE_io,
      A_BUS_OE_io   => A_BUS_OE_io,
      FORCE_ROMCS_o => FORCE_ROMCS_o,
      FORCE_RESET_o => FORCE_RESET_o,
      FORCE_INT_o   => FORCE_INT_o,
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
      SPI_SCK_i     => ESP_SCK_i,
      SPI_NCS_i     => ESP_NCSO_i,
      SPI_D_io(0)   => ESP_MOSI_io,
      SPI_D_io(1)   => ESP_MISO_io,
      SPI_D_io(2)   => ESP_QWP_io, -- Write-Protect
      SPI_D_io(3)   => ESP_QHD_io, -- Hold
      spec_int_o    => ESP_IO26_io,
      TP5           => TP5_o,
      spec_nreq_o   => ESP_IO27_io -- Request from spectrum
    );


  -- Temporary USB.
  USB_OE_o      <= '0';
  USB_SOFTCON_o <= '0';
  USB_SPEED_o   <= '0';
  USB_VMO_o     <= '0';
  USB_VPO_o     <= '1';
  USB_PWREN_o   <= '0';
  -- Temporary RAM
  RAMCLK_o      <= '0';
  RAMNCS_o      <= '0';

end str;

