use work.tbc_device_p.all;
use work.logger.all;

architecture t021 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
    variable data: std_logic_vector(7 downto 0);
    variable stamp: time;
  begin

    logger_start("T021","Kempston tests");

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    wait for 1 us;

    spiPayload_in_s(0) <= x"EE"; -- Write REG5
    spiPayload_in_s(1) <= x"05";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"00";
    spiPayload_in_s(4) <= x"00";
    spiPayload_in_s(5) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 6, spiPayload_in_s, spiPayload_out_s);

    SpectrumReadIO(Spectrum_Cmd, Spectrum_Data, x"001F", data);

    Check("That joystick is not replying", data, x"FF");

    spiPayload_in_s(0) <= x"EE"; -- Write CONFIG1
    spiPayload_in_s(1) <= x"02";
    spiPayload_in_s(2) <= x"00"; -- Keyboard + Joystick
    spiPayload_in_s(3) <= x"00";
    spiPayload_in_s(4) <= x"00";
    spiPayload_in_s(5) <= x"03";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 6, spiPayload_in_s, spiPayload_out_s);

    SpectrumReadIO(Spectrum_Cmd, Spectrum_Data, x"001F", data);

    Check("That joystick is replying", data, x"00");

    spiPayload_in_s(0) <= x"EE"; -- Write REG5
    spiPayload_in_s(1) <= x"05";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"FF";
    spiPayload_in_s(4) <= x"00";
    spiPayload_in_s(5) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 6, spiPayload_in_s, spiPayload_out_s);

    SpectrumReadIO(Spectrum_Cmd, Spectrum_Data, x"001F", data);
    Check("That joystick is replying", data, x"3F");


    report hstr(data);

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;

end t021;
