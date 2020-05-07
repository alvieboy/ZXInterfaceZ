use work.tbc_device_p.all;

architecture t007 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
    variable data: std_logic_vector(7 downto 0);
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
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"0011", x"14"); -- Address low
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"0013", x"45"); -- Address high
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"0015", x"DE"); -- Data
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"0015", x"AD"); -- Data
  
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"0011", x"14"); -- Address low
      SpectrumWriteIo(Spectrum_Cmd, Spectrum_Data, x"0013", x"45"); -- Address high
      SpectrumReadIo(Spectrum_Cmd, Spectrum_Data, x"0015", data); -- Data
      Check("First data read is OK", data, x"DE");
      SpectrumReadIo(Spectrum_Cmd, Spectrum_Data, x"0015", data); -- Data
      Check("Second data read is OK", data, x"AD");
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

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;


end t007;
