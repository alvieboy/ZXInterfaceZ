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
    signal ROMCS_n  : std_logic := '1';
  begin

  spect_clk <= not spect_clk after ZXPERIOD/2;
  --spect_rst <= not FORCE_RESET_o;
  XCK_s <= spect_clk;

  process
  begin
    spect_rst <= '1';
    wait for 1 ps;
    spect_rst <= '0';
    wait for 1 us;
    spect_rst <= '1';
    li: loop
      wait on FORCE_RESET_o;
      if FORCE_RESET_o='1' then
        wait for 1 us;
        spect_rst <= '0';
      else
        wait for 1 us;
        spect_rst <= '1';
      end if;
    end loop;
  end process;

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

  process
  begin
    XINT_s <= '1';
    sig: loop
      wait for 20 ms;
      XINT_s <= '0';
      wait for 10 us;
      XINT_s <= '1';
    end loop;
  end process;

  specram_inst: entity work.spectrum_ram
    port map(
      MREQ_n          => XMREQ_s,
      RD_n            => XRD_s,
      WR_n            => XWR_s,
      A               => XA_s,
      D               => XD_io
    );

  ROMCS_n <= '1' when FORCE_ROMCS_o='1' else (XA_s(15) AND NOT XA_s(14));

  specrom_inst: entity work.spectrum_rom_chip
    port map (
      A_i     => XA_s(13 downto 0),
      CSn_i   => XRD_s,
      OE0n_i  => XMREQ_s,
      OE1n_i  => ROMCS_n,
      D_o     => XD_io
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
    variable dataread40_v: std_logic_vector(39 downto 0);

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

    -- Write ROM test

    ESP_NCSO_i <= '0';
    w8std(x"E1", dataread_v);
    w8std(x"00", dataread_v);
    w8std(x"00", dataread_v);
    w8std(x"DE", dataread_v);
    w8std(x"AD", dataread_v);
    w8std(x"BE", dataread_v);
    w8std(x"EF", dataread_v);
    ESP_NCSO_i <= '1';
    wait for 1 us;

    -- Test ROM enable, Reset spectrum, enable ROMCS
    ESP_NCSO_i <= '0';
    w8std(x"EC", dataread_v);
    w8std(x"82", dataread_v);
    ESP_NCSO_i <= '1';
    wait for 1 us;
    -- Test ROM enable, Reset spectrum, disable ROMCS
    ESP_NCSO_i <= '0';
    w8std(x"EC", dataread_v);
    w8std(x"80", dataread_v);
    ESP_NCSO_i <= '1';
    wait for 1 us;

    wait for 40 us;

    -- Reset spectrum.
    ESP_NCSO_i <= '0';
    w8std(x"EC", dataread_v);
    w8std(x"02", dataread_v);
    ESP_NCSO_i <= '1';
    wait for 1 us;

    -- Reset fifo, spectrum, capture
    ESP_NCSO_i <= '0';
    w8std(x"EC", dataread_v);
    w8std(x"07", dataread_v);
    ESP_NCSO_i <= '1';
    wait for 1 us;

    -- Simple test: frame capture ended

    ESP_NCSO_i <= '0';
    w8std(x"EF", dataread_v);
    w8std(x"07", dataread_v);
    ESP_NCSO_i <= '1';
    wait for 1 us;

    -- Set trigger data
    ESP_NCSO_i <= '0';
    w8std(x"ED", dataread_v);
    w8std(x"00", dataread_v); -- Mask
    w8std(x"0F", dataread_v);
    w8std(x"FF", dataread_v);
    w8std(x"FF", dataread_v);
    w8std(x"00", dataread_v);
    ESP_NCSO_i <= '1';
    wait for 1 us;

    -- Set trigger value
    ESP_NCSO_i <= '0';
    w8std(x"ED", dataread_v);
    w8std(x"01", dataread_v); -- Value
    w8std(x"05", dataread_v);
    w8std(x"00", dataread_v);
    w8std(x"01", dataread_v);
    w8std(x"00", dataread_v);
    ESP_NCSO_i <= '1';
    wait for 1 us;


    report "Trigger set, starting capture";

    -- Unreset, enable capture
    ESP_NCSO_i <= '0';
    w8std(x"EC", dataread_v);
    w8std(x"08", dataread_v);
    ESP_NCSO_i <= '1';

    wait for 100 us;

    ESP_NCSO_i <= '0';
    w8std(x"E2", dataread_v);
    w8std(x"00", dataread_v);
    w8std(x"00", dataread_v);
    report "Sta00: "  & hstr(dataread_v);
    w8std(x"00", dataread_v);
    report "Sta1: "  & hstr(dataread_v);
    w8std(x"00", dataread_v);
    report "Sta2: "  & hstr(dataread_v);
    w8std(x"00", dataread_v);
    report "Sta3: "  & hstr(dataread_v);
    ESP_NCSO_i <= '1';

    wait for 1 us;

    ESP_NCSO_i <= '0';
    w8std(x"E0", dataread_v);
    w8std(x"00", dataread_v);

    w8std(x"00", dataread_v);
    w8std(x"00", dataread_v);
    w8std(x"00", dataread_v);
    w8std(x"00", dataread_v);
    w8std(x"00", dataread_v);
    report "Seq 0 " & hstr(dataread_v);
    ESP_NCSO_i <= '1';





    wait for 100 us;



    ESP_NCSO_i <= '0';
    w8std(x"E0", dataread_v);
    w8std(x"00", dataread_v);
    report "D state: "  & hstr(dataread_v);

    lo: for i in 0 to 10 loop
      w8std(x"00", dataread_v);
      --report "D i: "  & hstr(dataread_v);
      dataread40_v := dataread40_v(31 downto 0) & dataread_v;

      w8std(x"00", dataread_v);
      --report "D i: "  & hstr(dataread_v);
      dataread40_v := dataread40_v(31 downto 0) & dataread_v;

      w8std(x"00", dataread_v);
      --report "D i: "  & hstr(dataread_v);
      dataread40_v := dataread40_v(31 downto 0) & dataread_v;

      w8std(x"00", dataread_v);
      --report "D i: "  & hstr(dataread_v);
      dataread40_v := dataread40_v(31 downto 0) & dataread_v;

      w8std(x"00", dataread_v);
      --report "D i: "  & hstr(dataread_v);
      dataread40_v := dataread40_v(31 downto 0) & dataread_v;


      report "D: 0x"  & hstr(dataread40_v);
    end loop;
    ESP_NCSO_i <= '1';

    wait;
  end process;

END sim;
