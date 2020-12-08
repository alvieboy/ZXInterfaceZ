use work.tbc_device_p.all;
use work.logger.all;

architecture t014 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
  begin
    logger_start("T014","Video mem tests");

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    SpectrumWriteMem(Spectrum_Cmd, Spectrum_Data, x"4000", x"DE");
    SpectrumWriteMem(Spectrum_Cmd, Spectrum_Data, x"4001", x"AD");
    SpectrumWriteMem(Spectrum_Cmd, Spectrum_Data, x"4002", x"BE");
    SpectrumWriteMem(Spectrum_Cmd, Spectrum_Data, x"4003", x"EF");

    spiPayload_in_s(0) <= x"DF";
    spiPayload_in_s(1) <= x"00"; -- Addr h
    spiPayload_in_s(2) <= x"00"; -- Addr l
    spiPayload_in_s(3) <= x"00"; -- Dummy
    spiPayload_in_s(4) <= x"00";
    spiPayload_in_s(5) <= x"00";
    spiPayload_in_s(6) <= x"00";
    spiPayload_in_s(7) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 8, spiPayload_in_s, spiPayload_out_s);

    Check("1st byte", spiPayload_out_s(4), x"DE");
    Check("2st byte", spiPayload_out_s(5), x"AD");
    Check("3rd byte", spiPayload_out_s(6), x"BE");
    Check("4th byte", spiPayload_out_s(7), x"EF");

    spiPayload_in_s(0) <= x"DF";
    spiPayload_in_s(1) <= x"00"; -- Addr h
    spiPayload_in_s(2) <= x"02"; -- Addr l
    spiPayload_in_s(3) <= x"00"; -- Dummy
    spiPayload_in_s(4) <= x"00";
    spiPayload_in_s(5) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 6, spiPayload_in_s, spiPayload_out_s);

    Check("1st byte", spiPayload_out_s(4), x"BE");
    Check("2nd byte", spiPayload_out_s(5), x"EF");

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

    logger_end;

  end process;


end t014;
