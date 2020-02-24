LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.txt_util.all;

ENTITY tb_interfacez_top IS
END tb_interfacez_top;

ARCHITECTURE sim OF tb_interfacez_top IS
  -- constants                                                 
-- signals
  SIGNAL A_BUS_OE_io : STD_LOGIC;
  SIGNAL ASDO_s: STD_LOGIC;
  SIGNAL CLK_i : STD_LOGIC;
  SIGNAL CTRL_OE_io : STD_LOGIC;
  SIGNAL D_BUS_DIR_o : STD_LOGIC;
  SIGNAL D_BUS_OE_io : STD_LOGIC;
  SIGNAL DATA0_s : STD_LOGIC;
  SIGNAL DCLK_s : STD_LOGIC;
  SIGNAL ESP_IO14_io : STD_LOGIC;
  SIGNAL ESP_IO25_io : STD_LOGIC;
  SIGNAL ESP_IO26_io : STD_LOGIC;
  SIGNAL ESP_IO27_io : STD_LOGIC;
  SIGNAL ESP_MISO_io : STD_LOGIC;
  SIGNAL ESP_MOSI_io : STD_LOGIC;
  SIGNAL ESP_NCSO_i : STD_LOGIC;
  SIGNAL ESP_QHD_io : STD_LOGIC;
  SIGNAL ESP_QWP_io : STD_LOGIC;
  SIGNAL ESP_SCK_i : STD_LOGIC;
  SIGNAL FORCE_INT_o : STD_LOGIC;
  SIGNAL FORCE_RESET_o : STD_LOGIC;
  SIGNAL FORCE_ROMCS_o : STD_LOGIC;
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
  SIGNAL XCK_s : STD_LOGIC;
  SIGNAL XD_io : STD_LOGIC_VECTOR(7 DOWNTO 0);
  SIGNAL XINT_s : STD_LOGIC;
  SIGNAL XIORQ_s : STD_LOGIC;
  SIGNAL XM1_s : STD_LOGIC;
  SIGNAL XMREQ_s : STD_LOGIC;
  SIGNAL XRD_s : STD_LOGIC;
  SIGNAL XRFSH_s : STD_LOGIC;
  SIGNAL XWR_s : STD_LOGIC;

  signal ZX_A_s:  std_logic_vector(15 downto 0);
  signal ZX_D_s:  std_logic_vector(7 downto 0);

  constant ZXPERIOD: time := 1 ms / 3500;

  signal vcc: real := 0.0;

BEGIN

 interfacez : entity work.interfacez_top
	PORT MAP (
  -- list connections between master ports and signals
    A_BUS_OE_io => A_BUS_OE_io,
    ASDO_o => ASDO_s,
    CLK_i => CLK_i,
    CTRL_OE_io => CTRL_OE_io,
    D_BUS_DIR_o => D_BUS_DIR_o,
    D_BUS_OE_io => D_BUS_OE_io,
    DATA0_i => DATA0_s,
    DCLK_o => DCLK_s,
    ESP_IO14_io => ESP_IO14_io,
    ESP_IO25_io => ESP_IO25_io,
    ESP_IO26_io => ESP_IO26_io,
    ESP_IO27_io => ESP_IO27_io,
    ESP_MISO_io => ESP_MISO_io,
    ESP_MOSI_io => ESP_MOSI_io,
    ESP_NCSO_i => ESP_NCSO_i,
    ESP_QHD_io => ESP_QHD_io,
    ESP_QWP_io => ESP_QWP_io,
    ESP_SCK_i => ESP_SCK_i,
    FORCE_INT_o => FORCE_INT_o,
    FORCE_RESET_o => FORCE_RESET_o,
    FORCE_ROMCS_o => FORCE_ROMCS_o,
    NCSO_o => NCSO_o,
    SDRAM_A_o => SDRAM_A_o,
    SDRAM_BA_o => SDRAM_BA_o,
    SDRAM_CK_o => SDRAM_CK_o,
    SDRAM_CKE_o => SDRAM_CKE_o,
    SDRAM_CS_o => SDRAM_CS_o,
    SDRAM_D_io => SDRAM_D_io,
    SDRAM_DQM_o => SDRAM_DQM_o,
    SDRAM_NCAS_o => SDRAM_NCAS_o,
    SDRAM_NRAS_o => SDRAM_NRAS_o,
    SDRAM_NWE_o => SDRAM_NWE_o,
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

  D_BUS_OE_io <= 'H'; -- Pull up

  dbuf_inst: entity work.SN74LCX245FT
  port map (
    A_io  => ZX_D_s,
    B_io  => XD_io,
    nOE_i => D_BUS_OE_io,
    DIR_i => D_BUS_DIR_o
  );

  abuf1_inst: entity work.SN74LCX245FT
  port map (
    A_io  => ZX_A_s(7 downto 0),
    B_io  => XA_s(7 downto 0),
    nOE_i => A_BUS_OE_io,
    DIR_i => '1'
  );

  abuf2_inst: entity work.SN74LCX245FT
  port map (
    A_io  => ZX_A_s(15 downto 8),
    B_io  => XA_s(15 downto 8),
    nOE_i => A_BUS_OE_io,
    DIR_i => '1'
  );

  -- SDRAM
  sdram_inst: entity work.mt48lc16m16a2
    GENERIC MAP (
        addr_bits => 13,
        data_bits => 16,
        col_bits  => 9,
        index     => 0,
	      fname     => "sdram.srec"
    )
    PORT MAP (
        Dq(7 downto 0)    => SDRAM_D_io,
        Addr  => SDRAM_A_o,
        Ba    => SDRAM_BA_o,
        Clk   => SDRAM_CK_o,
        Cke   => SDRAM_CKE_o,
        Cs_n  => SDRAM_CS_o,
        Ras_n => SDRAM_NRAS_o,
        Cas_n => SDRAM_NCAS_o,
        We_n  => SDRAM_NWE_o,
        Dqm   => "11"
    );

  vcc <= 0.0, 3.3 after 50 ns;

  flash_inst: entity work.M25P16
    port map (
      VCC => vcc,
		  C   => DCLK_s,
      D   => ASDO_s,
      S   => ESP_IO27_io,
      W   => '1',
      HOLD => '1',
		  Q   => DATA0_s
    );

  spect: block
    signal spect_clk: std_logic := '0';
    signal spect_rst: std_logic := '0';
  begin

    spect_clk <= not spect_clk after ZXPERIOD/2;
    spect_rst <= '1', '0' after 10 ns, '1' after 1 us;


  zxspect_inst: entity work.spectrum_top
  port map (
    RESET_n         => spect_rst,
    CLK_n           => spect_clk,
    INT_n           => XINT_s,
    NMI_n           => '1',
    M1_n            => XM1_s,
    MREQ_n          => XMREQ_s,
    IORQ_n          => XIORQ_s,
    RD_n            => XRD_s,
    WR_n            => XWR_s,
    RFSH_n          => XRFSH_s,
    A               => XA_s,
    D               => XD_io
  );

  end block;

  -- Spectrum traffic.

  
  -- ESP traffic

  process
    procedure w4(dat: in std_logic_vector(3 downto 0); odat: out std_logic_vector(3 downto 0)) is

    begin
      ESP_SCK_i <= '0';
      ESP_MOSI_io <= dat(0);
      ESP_MISO_io <= dat(1);
      ESP_QWP_io  <= dat(2);
      ESP_QHD_io  <= dat(3);
      report hstr(dat);

      wait for 20 ns;
      ESP_SCK_i <= '1';
      odat(0) := ESP_MOSI_io;
      odat(1) := ESP_MISO_io;
      odat(2) := ESP_QWP_io;
      odat(3) := ESP_QHD_io;
      wait for 20 ns;
    end procedure;

    procedure w8(dat: in std_logic_vector(7 downto 0); odat: out std_logic_vector(7 downto 0)) is
    begin
      w4( dat(7 downto 4), odat(7 downto 4) );
      w4( dat(3 downto 0), odat(3 downto 0) );
    end procedure;

    procedure r4(dat: in std_logic_vector(3 downto 0); odat: out std_logic_vector(3 downto 0)) is

    begin
      ESP_SCK_i <= '0';
      report hstr(dat);

      wait for 20 ns;
      ESP_SCK_i <= '1';
      odat(0) := ESP_MOSI_io;
      odat(1) := ESP_MISO_io;
      odat(2) := ESP_QWP_io;
      odat(3) := ESP_QHD_io;
      wait for 20 ns;
    end procedure;

    procedure r8(dat: in std_logic_vector(7 downto 0); odat: out std_logic_vector(7 downto 0)) is
    begin
      r4( dat(7 downto 4), odat(7 downto 4) );
      r4( dat(3 downto 0), odat(3 downto 0) );
    end procedure;


    procedure w8std(dat: in std_logic_vector(7 downto 0); odat: out std_logic_vector(7 downto 0)) is

    begin
      ESP_MISO_io <= 'Z';
      ESP_QWP_io  <= 'Z';
      ESP_QHD_io  <= 'Z';

      l: for i in 7 downto 0 loop
        ESP_SCK_i <= '0';
        ESP_MOSI_io <= dat(i);
        wait for 20 ns;
        ESP_SCK_i <= '1';
        odat(i) := ESP_MISO_io;
        wait for 20 ns;
      end loop;
    end procedure;



    variable dataread_v: std_logic_vector(7 downto 0);

  begin
    ESP_IO27_io <= '1';
    ESP_NCSO_i <= '1';
    ESP_SCK_i <= '1';
    ESP_MOSI_io <= 'Z';
    ESP_MISO_io <= 'Z';
    ESP_QHD_io <= 'Z';
    ESP_QWP_io <= 'Z';
    wait for 40 us;
    ESP_NCSO_i <= '0';
    wait for 20 ns;

    --w8(x"FC", dataread_v);
    --ESP_MOSI_io <= 'Z';
    --ESP_MISO_io <= 'Z';
    --ESP_QHD_io <= 'Z';
    --ESP_QWP_io <= 'Z';
    --r8(x"00", dataread_v);
    --r8(x"00", dataread_v);
    --r8(x"00", dataread_v);
    --r8(x"DE", dataread_v);
    --r8(x"00", dataread_v);
    --r8(x"00", dataread_v);
    --r8(x"00", dataread_v);

    w8std(x"9F", dataread_v);
    w8std(x"00", dataread_v);
    report "D0: "  & hstr(dataread_v);
    w8std(x"00", dataread_v);
    report "D1: "  & hstr(dataread_v);
    w8std(x"00", dataread_v);
    report "D2: "  & hstr(dataread_v);
    w8std(x"00", dataread_v);
    report "D3: "  & hstr(dataread_v);

    ESP_NCSO_i <= '1';
    wait for 1 us;
    ESP_NCSO_i <= '0';
    w8std(x"9F", dataread_v);
    w8std(x"00", dataread_v);
    report "D0: "  & hstr(dataread_v);
    w8std(x"00", dataread_v);
    report "D1: "  & hstr(dataread_v);
    w8std(x"00", dataread_v);
    report "D2: "  & hstr(dataread_v);
    w8std(x"00", dataread_v);
    report "D3: "  & hstr(dataread_v);

    ESP_NCSO_i <= '1';
    wait for 1 ns;
    ESP_IO27_io <= '0';

    w8std(x"9F", dataread_v);
    w8std(x"00", dataread_v);
    report "FD0: "  & hstr(dataread_v);
    w8std(x"00", dataread_v);
    report "FD1: "  & hstr(dataread_v);
    w8std(x"00", dataread_v);
    report "FD2: "  & hstr(dataread_v);
    w8std(x"00", dataread_v);
    report "FD3: "  & hstr(dataread_v);

    ESP_IO27_io <= '1';



    wait;
  end process;

END sim;
