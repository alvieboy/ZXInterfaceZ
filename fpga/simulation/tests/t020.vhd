use work.tbc_device_p.all;
use work.logger.all;

architecture t020 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
    variable data: std_logic_vector(7 downto 0);
    variable stamp: time;

    
    procedure checkInterrupt(v: in std_logic) is
    begin
    --Check("103: USB interrupt", CtrlPins_Data.USB_INTn, '0');
    end procedure;

  begin

    logger_start("T020","Mic idle");

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    wait for 1 us;

    spiPayload_in_s(0) <= x"FD"; -- Read MIC idle
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

    log ("Mic idle: " & hstr(spiPayload_out_s(2)));

    Check("Mic idle, ", spiPayload_out_s(2), x"00");

    wait for 500 us; -- Real world 50ms, 500us for simulation

    spiPayload_in_s(0) <= x"FD"; -- Read MIC idle
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

    Check("Mic idle, ", spiPayload_out_s(2), x"01");

    wait for 500 us; -- Real world 50ms, 500us for simulation


    spiPayload_in_s(0) <= x"FD"; -- Read MIC idle
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

    Check("Mic idle, ", spiPayload_out_s(2), x"02");

    -- Toggle mic
    SpectrumWriteIO(Spectrum_Cmd, Spectrum_Data, x"00FE", x"FF"); 

    spiPayload_in_s(0) <= x"FD"; -- Read MIC idle
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

    Check("Mic idle, ", spiPayload_out_s(2), x"00");

    wait for 500 us; -- Real world 50ms, 500us for simulation

    spiPayload_in_s(0) <= x"FD"; -- Read MIC idle
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

    Check("Mic idle, ", spiPayload_out_s(2), x"01");

    -- No Toggle mic
    SpectrumWriteIO(Spectrum_Cmd, Spectrum_Data, x"00FE", x"FF"); 
                                          

    wait for 500 us; -- Real world 50ms, 500us for simulation

    spiPayload_in_s(0) <= x"FD"; -- Read MIC idle
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

    Check("Mic idle, ", spiPayload_out_s(2), x"02");

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;

end t020;
