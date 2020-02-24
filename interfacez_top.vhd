library IEEE;
use IEEE.STD_LOGIC_1164.all;

entity interfacez_top is
  port (
    CLK_i         : in std_logic;


    -- SPI flash
    ASDO_o        : out std_logic;
    NCSO_o        : out std_logic;
    DCLK_o        : out std_logic;
    DATA0_i       : in  std_logic;

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

    -- SDRAM interconnection

    SDRAM_A_o     : out std_logic_vector(12 downto 0);
    SDRAM_D_io    : inout std_logic_vector(7 downto 0);
    SDRAM_BA_o    : out std_logic_vector(1 downto 0);
    SDRAM_CS_o    : out std_logic;
    SDRAM_NRAS_o  : out std_logic;
    SDRAM_NCAS_o  : out std_logic;
    SDRAM_NWE_o   : out std_logic;
    SDRAM_CKE_o   : out std_logic;
    SDRAM_CK_o    : out std_logic;
    SDRAM_DQM_o   : out std_logic;

    -- Buffer control

    D_BUS_DIR_o   : out std_logic;
    D_BUS_OE_io   : inout std_logic;
    CTRL_OE_io    : inout std_logic;
    A_BUS_OE_io   : inout std_logic;

    -- ZX Spectrum control
    FORCE_ROMCS_o : out std_logic;
    FORCE_RESET_o : out std_logic;
    FORCE_INT_o   : out std_logic;

    -- ESP32 IOs
    ESP_IO25_io   : inout std_logic;
    ESP_IO26_io   : inout std_logic;
    ESP_IO27_io   : inout std_logic;
    ESP_IO14_io   : inout std_logic;

    -- ESP32 SPI interface
    ESP_QHD_io    : inout std_logic;
    ESP_MISO_io   : inout std_logic;
    ESP_NCSO_i    : in std_logic;
    ESP_SCK_i     : in std_logic;
    ESP_QWP_io    : inout std_logic;
    ESP_MOSI_io   : inout std_logic
  );
end interfacez_top;

architecture str of interfacez_top is
  
  signal sysclk_s     : std_logic;
  signal sysrst_s     : std_logic;
  signal plllock_s    : std_logic;

  signal sdramclk2_s  : std_logic;


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
      c1      => sdramclk2_s,
      locked  => plllock_s
  );

  interface_inst: entity work.zxinterface
    port map (
      clk_i         => sysclk_s,
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
      ASDO_o        => ASDO_o,
      NCSO_o        => NCSO_o,
      DCLK_o        => DCLK_o,
      DATA0_i       => DATA0_i,
      ESP_AS_NCS    => ESP_IO27_io,

      wb_dat_i      => wb_rdat,
      wb_dat_o      => wb_wdat,
      wb_adr_o      => wb_adr,
      wb_we_o       => wb_we,
      wb_cyc_o      => wb_cyc,
      wb_stb_o      => wb_stb,
     -- wb_sel_o      => wb_sel,
      wb_ack_i      => wb_ack,
      wb_stall_i    => wb_stall
    );

  sdram_inst: entity work.sdram_ctrl
  port map (
    wb_clk_i    => sysclk_s,
    wb_rst_i    => sysrst_s,

    wb_dat_o    => wb_rdat,
    wb_dat_i    => wb_wdat,
    wb_adr_i    => wb_adr,
    wb_we_i     => wb_we,
    wb_cyc_i    => wb_cyc,
    wb_stb_i    => wb_stb,
    --wb_sel_i    => wb_sel,
    wb_ack_o    => wb_ack,
    wb_stall_o  => wb_stall,

    -- extra clocking
    clk_off_3ns => sdramclk2_s,

    -- SDRAM signals
    DRAM_ADDR   => SDRAM_A_o,
    DRAM_BA     => SDRAM_BA_o,
    DRAM_CAS_N  => SDRAM_NCAS_o,
    DRAM_CKE    => SDRAM_CKE_o,
    DRAM_CLK    => SDRAM_CK_o,
    DRAM_CS_N   => SDRAM_CS_o,
    DRAM_DQ     => SDRAM_D_io,
    DRAM_DQM    => SDRAM_DQM_o,
    DRAM_RAS_N  => SDRAM_NRAS_o,
    DRAM_WE_N   => SDRAM_NWE_o
  );


end str;

