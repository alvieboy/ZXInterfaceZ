library IEEE;
use IEEE.STD_LOGIC_1164.all;
library work;
use work.zxinterfacepkg.all;

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
    D_BUS_OE_o    : out std_logic;

    -- ZX Spectrum control
    FORCE_ROMCS_o : out std_logic;
    FORCE_RESET_o : out std_logic;
    FORCE_INT_o   : out std_logic;
    FORCE_NMI_o   : out std_logic;
    FORCE_IORQULA_o: out std_logic;
    FORCE_WAIT_o  : out std_logic;
    FORCE_2AROMCS_o: out std_logic;
    -- ESP32 IOs
    ESP_IO26_o   : out std_logic;
    ESP_IO27_o   : out std_logic;

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
    RAMD_io       : inout std_logic_vector(3 downto 0);
    RAMCLK_o      : out std_logic;
    RAMNCS_o      : out std_logic;

    -- USB PHY
    USB_VP_i      : in std_logic;
    USB_VM_i      : in std_logic;
    USB_RCV_i     : in std_logic;
    USB_OE_o      : out std_logic;
    USB_MODE_o    : out std_logic;
    USB_SPEED_o   : out std_logic;
    USB_SUSPEND_o : out std_logic;
    USB_VMO_o     : out std_logic;
    USB_VPO_o     : out std_logic;
    -- USB power control
    USB_FLT_i     : in std_logic;
    USB_PWREN_o   : out std_logic;
    -- Extension connector
    EXT_io        : inout std_logic_vector(13 downto 0);

    -- Testpoints
    TP4_o         : out std_logic
  );
end interfacez_top;

architecture str of interfacez_top is
  
  signal sysclk_s     : std_logic;
  signal sysrst_s     : std_logic;
  signal plllock_s    : std_logic;
  signal capclk_s     : std_logic;
  signal clk48_s      : std_logic;

  signal FORCE_ROMCS_s: std_logic;
  signal FORCE_2AROMCS_s : std_logic;
  signal FORCE_NMI_s  : std_logic;
  signal FORCE_INT_s  : std_logic;
  signal FORCE_IORQULA_s: std_logic;

  signal wb_rdat      : std_logic_vector(7 downto 0);
  signal wb_wdat      : std_logic_vector(7 downto 0);
  signal wb_adr       : std_logic_vector(23 downto 0);
  signal wb_we        : std_logic;
  signal wb_cyc       : std_logic;
  signal wb_stb       : std_logic;
  --signal wb_sel       : std_logic;--_vector(3 downto 0);
  signal wb_ack       : std_logic;
  signal wb_stall     : std_logic;

  signal videoclk_s   : std_logic_vector(2 downto 0);
  signal hsync_s      : std_logic;
  signal vsync_s      : std_logic;
  signal bright_s     : std_logic;
  signal grb_s        : std_logic_vector(2 downto 0);
  signal spec_nreq_s  : std_logic;

  signal RAMD_o_s     : std_logic_vector(7 downto 0);
  signal RAMD_i_s     : std_logic_vector(7 downto 0);
  signal RAMD_oe_s    : std_logic_vector(7 downto 0);
  signal RAMCLK_s   : std_logic;
  signal RAMNCS_s   : std_logic;

  signal audio_r_s  : std_logic;
  signal audio_l_s  : std_logic;

  signal dbg_s      : std_logic_vector(15 downto 0);

  alias EXT_VGA_HSYNC   : std_logic is EXT_io(0);
  alias EXT_VGA_VSYNC   : std_logic is EXT_io(1);
  alias EXT_VGA_RED1    : std_logic is EXT_io(2);
  alias EXT_VGA_RED0    : std_logic is EXT_io(3);
  alias EXT_VGA_GREEN1  : std_logic is EXT_io(4);
  alias EXT_VGA_GREEN0  : std_logic is EXT_io(5);
  alias EXT_VGA_BLUE1   : std_logic is EXT_io(6);
  alias EXT_VGA_BLUE0   : std_logic is EXT_io(7);
  alias EXT_VGA_BLUE2   : std_logic is EXT_io(8);
  alias EXT_VGA_RED2    : std_logic is EXT_io(9);
  alias EXT_VGA_GREEN2  : std_logic is EXT_io(10);

begin

  rstgen_inst: entity work.rstgen
    generic map (
      POLARITY => '0'
    ) port map (
      arst_i    => plllock_s,
      clk_i     => sysclk_s,
      rst_o     => sysrst_s
    );

  corepll_inst: entity work.corepll
    port map (
      inclk0  => CLK_i,
      c0      => sysclk_s,
      c1      => clk48_s,--sdramclk2_s,
      c2      => videoclk_s(2),  --
      c3      => videoclk_s(1),  -- 40Mhz
      c4      => videoclk_s(0),   -- 28.24Mhz
      locked  => plllock_s
  );

  interface_inst: entity work.zxinterface
    port map (
      clk_i         => sysclk_s,
      clk48_i       => clk48_s,
      capclk_i      => capclk_s,
      videoclk_i    => videoclk_s,
      arst_i        => sysrst_s,
      D_BUS_DIR_o   => D_BUS_DIR_o,
      D_BUS_OE_o    => D_BUS_OE_o,
      --CTRL_OE_o     => CTRL_OE_o,
      --A_BUS_OE_o    => A_BUS_OE_o,
      FORCE_ROMCS_o => FORCE_ROMCS_s,
      FORCE_2AROMCS_o => FORCE_2AROMCS_s,
      FORCE_RESET_o => FORCE_RESET_o,
      FORCE_INT_o   => FORCE_INT_s,
      FORCE_NMI_o   => FORCE_NMI_s,
      FORCE_IORQULA_o => FORCE_IORQULA_s,
      FORCE_WAIT_o  => FORCE_WAIT_o,
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
      SPI_MOSI_i    => ESP_MOSI_io,
      SPI_MISO_o    => ESP_MISO_io,
      --SPI_D_io(2)   => ESP_QWP_io, -- Write-Protect
      --SPI_D_io(3)   => ESP_QHD_io, -- Hold
      -- RAM interface
      RAMD_o        => RAMD_o_s,
      RAMD_i        => RAMD_i_s,
      RAMD_oe_o     => RAMD_oe_s,
      RAMCLK_o      => RAMCLK_s,
      RAMNCS_o      => RAMNCS_s,

  
      -- USB PHY
      USB_VP_i      => USB_VP_i,
      USB_VM_i      => USB_VM_i,
      USB_RCV_i     => USB_RCV_i,
      USB_OE_o      => USB_OE_o,
      USB_SOFTCON_o => open,
      USB_SPEED_o   => USB_SPEED_o,
      USB_MODE_o    => USB_MODE_o,
      USB_SUSPEND_o => USB_SUSPEND_o,
      USB_VMO_o     => USB_VMO_o,
      USB_VPO_o     => USB_VPO_o,
      -- USB power control
      USB_FLT_i     => USB_FLT_i,
      USB_PWREN_o   => USB_PWREN_o,
      USB_INTN_o    => ESP_QWP_io, -- TODO: Rename
      -- Debug
      TP4           => TP4_o,
      dbg_o         => dbg_s,
      -- Interrupts

      spec_int_o    => ESP_IO26_o,
      spec_nreq_o   => spec_nreq_s, -- Request from spectrum
          -- video out
      hsync_o       => hsync_s,
      vsync_o       => vsync_s,
      bright_o      => bright_s,
      grb_o         => grb_s,
      audio_l_o     => audio_l_s,
      audio_r_o     => audio_r_s
    );

  FORCE_ROMCS_o <= FORCE_ROMCS_s;

  extconn_vga: if C_ENABLE_VGA generate

    EXT_VGA_HSYNC <= hsync_s;
    EXT_VGA_VSYNC <= vsync_s;
    EXT_VGA_RED2  <= grb_s(1); -- Red 1
    EXT_VGA_RED1  <= bright_s and grb_s(1); -- Red 0
    EXT_VGA_RED0  <= bright_s and grb_s(1); -- Red 0

    EXT_VGA_GREEN2  <= grb_s(2); 
    EXT_VGA_GREEN1  <= bright_s and grb_s(2); 
    EXT_VGA_GREEN0  <= bright_s and grb_s(2); 

    EXT_VGA_BLUE2   <= grb_s(0);
    EXT_VGA_BLUE1   <= bright_s and grb_s(0);
    EXT_VGA_BLUE0   <= bright_s and grb_s(0);
  
  
  end generate;

  extconn_dbg: if not C_ENABLE_VGA generate
    EXT_io(7 downto 0)     <= dbg_s(7 downto 0);
  end generate;

  EXT_io(11) <= audio_l_s;
  EXT_io(12) <= audio_r_s;

  EXT_io(13) <= 'Z';

  FORCE_ROMCS_o   <= FORCE_ROMCS_s;
  FORCE_NMI_o     <= FORCE_NMI_s;
  FORCE_IORQULA_o <= FORCE_IORQULA_s;
  FORCE_2AROMCS_o <= FORCE_2AROMCS_s;
  FORCE_INT_o     <= FORCE_INT_s;

  ESP_IO27_o     <= spec_nreq_s;

  FLED_o(0)     <= dbg_s(8);
  FLED_o(1)     <= not FORCE_IORQULA_s;
  FLED_o(2)     <= not FORCE_ROMCS_s;
  LED2_o        <= '1';

  -- The delays are only used for simulation
  ram_buf: entity work.iobuf
  generic map (
    WIDTH => 4,
    tOE   => 3.2 ns,
    tOP   => 3.7 ns,
    tIP   => 1.55 ns
  )
  port map (
    i_i     => RAMD_o_s(3 downto 0),
    o_o     => RAMD_i_s(3 downto 0),
    pad_io  => RAMD_io(3 downto 0),
    oe_i    => RAMD_oe_s(3 downto 0)
  );

  -- The delays are only used for simulation
  ram2_buf: entity work.obuf
  generic map (
    WIDTH => 2,
    tOP   => 3.637 ns
  )
  port map (
    i_i(0)    => RAMCLK_s,
    i_i(1)    => RAMNCS_s,
    pad_o(0)  => RAMCLK_o,
    pad_o(1)  => RAMNCS_o
  );



end str;

