use work.tbc_device_p.all;
use work.zxinterfacepkg.all;

architecture t007 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
    variable data: std_logic_vector(7 downto 0);
    variable exp:  std_logic_vector(7 downto 0);
  begin
    report "T007: RAM manipulation";

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    wait for 10 ns;

    QSPIRam0_Cmd.Enabled <= true;
    QSPIRam1_Cmd.Enabled <= true;

    wait for 2 us;

    -- Re-write on top to ensure addresses are OK
    l: for i in 1 to 2 loop
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_RAM_ADDR_LOW, x"14"); -- Address low
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_RAM_ADDR_MIDDLE, x"45"); -- Address high
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_RAM_ADDR_HIGH, x"00"); -- Address high
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_RAM_DATA, x"DE"); -- Data
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_RAM_DATA, x"AD"); -- Data
  
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data,  x"00" & SPECT_PORT_RAM_ADDR_LOW, x"14"); -- Address low
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data,  x"00" & SPECT_PORT_RAM_ADDR_MIDDLE, x"45"); -- Address high
      SpectrumReadIo(Spectrum_Cmd, Spectrum_Data,   x"00" & SPECT_PORT_RAM_DATA, data); -- Data
      exp := x"de";
      Check("First data read is OK", data, exp);
      SpectrumReadIo(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_RAM_DATA, data); -- Data
      exp := x"ad";
      Check("Second data read is OK", data, exp);
    end loop;

    wait for 2 us;

    -- SPI ram read

    spiPayload_in_s(0) <= x"50";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"45";
    spiPayload_in_s(3) <= x"14";
    spiPayload_in_s(4) <= x"00";
    spiPayload_in_s(5) <= x"00";
    spiPayload_in_s(6) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 7, spiPayload_in_s, spiPayload_out_s);

    Check("First SPI data read is OK", spiPayload_out_s(5), x"DE");
    Check("Second SPI data read is OK", spiPayload_out_s(6), x"AD");

    -- SPI ram write

    spiPayload_in_s(0) <= x"51";
    spiPayload_in_s(1) <= x"01";
    spiPayload_in_s(2) <= x"20";
    spiPayload_in_s(3) <= x"03";
    spiPayload_in_s(4) <= x"c0";
    spiPayload_in_s(5) <= x"fe";
    spiPayload_in_s(6) <= x"ca";
    spiPayload_in_s(7) <= x"a5";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 8, spiPayload_in_s, spiPayload_out_s);

    spiPayload_in_s(0) <= x"50";
    spiPayload_in_s(1) <= x"01";
    spiPayload_in_s(2) <= x"20";
    spiPayload_in_s(3) <= x"03";
    spiPayload_in_s(4) <= x"00"; -- Dummy
    spiPayload_in_s(5) <= x"00";
    spiPayload_in_s(6) <= x"00";
    spiPayload_in_s(7) <= x"00";
    spiPayload_in_s(8) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 9, spiPayload_in_s, spiPayload_out_s);

    Check("First SPI data read is OK", spiPayload_out_s(5), x"c0");
    Check("Second SPI data read is OK", spiPayload_out_s(6), x"fe");
    Check("Third SPI data read is OK", spiPayload_out_s(7), x"ca");
    Check("Fourth SPI data read is OK", spiPayload_out_s(8), x"a5");

    -- Try reading from Spectrum now.
    SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_RAM_ADDR_LOW, x"03"); -- Address low
    SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_RAM_ADDR_MIDDLE, x"20"); -- Address high
    SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data,  x"00" & SPECT_PORT_RAM_ADDR_HIGH, x"01"); -- Address high
    SpectrumReadIo(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_RAM_DATA, data); -- Data
    Check("data read is OK", data, x"c0");
    SpectrumReadIo(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_RAM_DATA, data); -- Data
    Check("data read is OK", data, x"fe");
    SpectrumReadIo(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_RAM_DATA, data); -- Data
    Check("data read is OK", data, x"ca");
    SpectrumReadIo(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_RAM_DATA, data); -- Data
    Check("data read is OK", data, x"a5");



    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;


end t007;
