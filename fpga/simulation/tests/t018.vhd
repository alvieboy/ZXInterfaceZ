use work.tbc_device_p.all;
use work.logger.all;

architecture t018 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
    procedure ackSpectrumInterrupt is
    begin
      -- Clear interrupt
      spiPayload_in_s(0) <= x"A0";
      spiPayload_in_s(1) <= x"04";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 2, spiPayload_in_s, spiPayload_out_s);
    end procedure;

    procedure checkInterrupt(line: in natural) is
    begin
      Check( "CtrlPins27 is 0", CtrlPins_Data.IO27 , '0');
      -- Get interrupt
      spiPayload_in_s(0) <= x"A1";
      spiPayload_in_s(1) <= x"00";
      spiPayload_in_s(2) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

      Check( "Interrupt line " &str(line)&" active", spiPayload_out_s(2)(line),'1');
    end procedure;

 begin

    logger_start("T018","Spectrum interrupt propagation");

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"20"; -- Enable interrupt

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 2, spiPayload_in_s, spiPayload_out_s);


    SpectrumSetPin(Spectrum_Cmd, Spectrum_Data, PIN_INT, '0');
    wait for 100 ns;
    SpectrumSetPin(Spectrum_Cmd, Spectrum_Data, PIN_INT, '1');

    checkInterrupt(2);
    ackInterrupt(CtrlPins_Cmd);

    ackSpectrumInterrupt;


    wait for 10 us;
    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

    logger_end;

  end process;


end t018;
