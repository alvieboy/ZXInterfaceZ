use work.tbc_device_p.all;
use work.logger.all;

architecture t017 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
  begin
    logger_start("T016","ROM hook tests");

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    -- Setup external ROM

    QSPIRam0_Cmd.Enabled <= true;
    Ram_Cmd.Enabled <= true;

    romWrite(Rom_Cmd, 0, x"01");
    romWrite(Rom_Cmd, 1, x"FE");
    romWrite(Rom_Cmd, 2, x"BB");  -- LD BC, 0xBBAA
    romWrite(Rom_Cmd, 3, x"05");  -- DEC B
    romWrite(Rom_Cmd, 4, x"00");  -- NOP
    romWrite(Rom_Cmd, 5, x"78");  -- LD A, B
    romWrite(Rom_Cmd, 6, x"ED");  --
    romWrite(Rom_Cmd, 7, x"79");  -- OUT [C], A
    romWrite(Rom_Cmd, 8, x"18");
    romWrite(Rom_Cmd, 9, x"FE");  -- JR -1


    qspiramWrite(QSPIRam0_Cmd, 3 ,x"00"); -- NOP
    qspiramWrite(QSPIRam0_Cmd, 4 ,x"04"); -- INC B

    
    spiPayload_in_s(0) <= x"65";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"40";
    spiPayload_in_s(3) <= x"03";   -- Base low
    spiPayload_in_s(4) <= x"00";   -- Base high
    spiPayload_in_s(5) <= x"00";   -- Base len
    spiPayload_in_s(6) <= x"00";   -- Not active

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 7, spiPayload_in_s, spiPayload_out_s);


    wait for 1 us;
    
    Spectrum_Cmd.Cmd <= RUNZ80;
    wait for 0 ps;
    Spectrum_Cmd.Cmd <= NONE;

    wait on Ula_Data.Data for 100 us;
    Check("If ULA got a write", Ula_Data.Data'event);
    Check("If ULA write is correct", Ula_Data.Data, x"BA");

    wait for 1 us;
    Spectrum_Cmd.Cmd <= STOPZ80;
    wait for 0 ps;
    Spectrum_Cmd.Cmd <= NONE;

    -- Activate hook

    spiPayload_in_s(0) <= x"65";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"43";
    spiPayload_in_s(3) <= x"01";   -- Active

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    wait for 1 us;
    
    Spectrum_Cmd.Cmd <= RUNZ80;
    wait for 0 ps;
    Spectrum_Cmd.Cmd <= NONE;

    wait on Ula_Data.Data for 100 us;
    Check("If ULA got a write", Ula_Data.Data'event);
    Check("If ULA write (hook) is correct", Ula_Data.Data, x"BB");

    wait for 1 us;
    Spectrum_Cmd.Cmd <= STOPZ80;
    wait for 0 ps;
    Spectrum_Cmd.Cmd <= NONE;


    -- Modify hook to include 2nd instruction

    spiPayload_in_s(0) <= x"65";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"42";
    spiPayload_in_s(3) <= x"01"; -- 2 bytes
    spiPayload_in_s(4) <= x"01"; -- Active

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);


    wait for 1 us;
    
    Spectrum_Cmd.Cmd <= RUNZ80;
    wait for 0 ps;
    Spectrum_Cmd.Cmd <= NONE;

    wait on Ula_Data.Data for 100 us;
    Check("If ULA got a write", Ula_Data.Data'event);
    Check("If ULA write (hook) is correct", Ula_Data.Data, x"BC");

    wait for 1 us;
    Spectrum_Cmd.Cmd <= STOPZ80;
    wait for 0 ps;
    Spectrum_Cmd.Cmd <= NONE;

    -- Test clearing of ROMCS upon RET
-- 0000:31FE5F     LD SP, 5FFE
-- 0003:CD0800     CALL 0008
-- 0006:           _end: 0006
-- 0006:18FE       JR 0006
-- 0008:           _local1: 0008
-- 0008:3E01       LD A,01
-- 000A:D36F       OUT [6F], A
-- 000C:00         NOP
-- 000D:00         NOP
-- 000E:00         NOP
-- 000F:C9         RET
-- 
    romWrite(Rom_Cmd, 6, x"3E");
    romWrite(Rom_Cmd, 7, x"BB"); -- LD A, $BB
    romWrite(Rom_Cmd, 8, x"D3");
    romWrite(Rom_Cmd, 9, x"FE"); -- OUT ($FE), A
    romWrite(Rom_Cmd, 10, x"18");
    romWrite(Rom_Cmd, 11, x"FE"); -- JR -1

    qspiramWrite(QSPIRam0_Cmd, 0 ,x"31");
    qspiramWrite(QSPIRam0_Cmd, 1 ,x"FE");
    qspiramWrite(QSPIRam0_Cmd, 2 ,x"5F"); -- LD SP, 5FFE
    qspiramWrite(QSPIRam0_Cmd, 3 ,x"CD");
    qspiramWrite(QSPIRam0_Cmd, 4 ,x"08");
    qspiramWrite(QSPIRam0_Cmd, 5 ,x"00"); -- CALL 0x0008
    qspiramWrite(QSPIRam0_Cmd, 6 ,x"18");
    qspiramWrite(QSPIRam0_Cmd, 7 ,x"FE"); -- JR -1

    qspiramWrite(QSPIRam0_Cmd, 8 ,x"3e");
    qspiramWrite(QSPIRam0_Cmd, 9 ,x"01"); -- LD A, $01
    qspiramWrite(QSPIRam0_Cmd, 10 ,x"d3"); --
    qspiramWrite(QSPIRam0_Cmd, 11 ,x"6f"); -- OUT ($6F), A
    qspiramWrite(QSPIRam0_Cmd, 12 ,x"00"); -- NOP
    qspiramWrite(QSPIRam0_Cmd, 13 ,x"00"); -- NOP
    qspiramWrite(QSPIRam0_Cmd, 14 ,x"00"); -- NOP
    qspiramWrite(QSPIRam0_Cmd, 15 ,x"C9"); -- RET

    -- Activate ROMCS
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= "00000100"; -- ForceROMCS
    spiPayload_in_s(3) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    wait for 1 us;

    
    Spectrum_Cmd.Cmd <= RUNZ80;
    wait for 0 ps;
    Spectrum_Cmd.Cmd <= NONE;

    wait on Ula_Data.Data for 100 us;
    Check("If ULA got a write", Ula_Data.Data'event);
    Check("If ULA write (hook) is correct", Ula_Data.Data, x"BB");

    wait for 1 us;
    Spectrum_Cmd.Cmd <= STOPZ80;
    wait for 0 ps;
    Spectrum_Cmd.Cmd <= NONE;

        Check("1 ROMCS is not forced", CtrlPins_Data.ROMCS, '0');

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

    logger_end;

  end process;


end t017;
