use work.tbc_device_p.all;
use work.zxinterfacepkg.all;
use work.logger.all;

architecture t013 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
    variable data: std_logic_vector(7 downto 0);
    variable exp:  std_logic_vector(7 downto 0);
    variable ramaddr:  unsigned(23 downto 0);

    procedure checkram(orval: in std_logic_vector(7 downto 0)) is
    begin
      log("Check RAM writes");

      rloop: for ram in 0 to 7 loop
      -- Select RAM 0.

      SpectrumWriteIO( Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_MEMSEL, std_logic_vector(to_unsigned(ram,8)));

      exp := orval or std_logic_vector(to_unsigned(ram*2, 8));

      SpectrumWriteMem( Spectrum_Cmd, Spectrum_Data, x"2000", exp);
      SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"2000", data);
      Check("4: RAM data consistent", data, exp);

      exp := exp or x"01";

      SpectrumWriteMem( Spectrum_Cmd, Spectrum_Data, x"3FFF", exp);
      SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"3FFF", data);
      Check("5: RAM data consistent", data, exp);

      memcheck: for ch in 0 to ram loop

        log("Checking RAM bank " & str(ch));
        ramaddr := x"010000" + ch*8192;
  
        qspiramRead(QSPIRam0_Cmd, QSPIRam0_Data, std_logic_vector(ramaddr), data);

        exp := orval or std_logic_vector(to_unsigned(ch*2, 8));

        Check("6: Memory data consistent", data, exp);
  
        ramaddr := ramaddr + x"001FFF";
        exp := exp or x"01";
  
        qspiramRead(QSPIRam0_Cmd, QSPIRam0_Data, std_logic_vector(ramaddr), data);

        Check("7: Memory data consistent", data, exp);
      end loop;

    end loop;
    end procedure;
  begin
    logger_start("T013","RAM/ROM flag manipulation");

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    wait for 10 ns;

    QSPIRam0_Cmd.Enabled <= true;

--| 0x000000 | 0x001FFF |  8kB | Main InterfaceZ ROM       | 0 | X | |   0x0000 |   0x1FFF |
--| 0x002000 | 0x003FFF |  8kB | Alternate small ROM       | 1 | X | |   0x0000 |   0x1FFF |
--| 0x004000 | 0x007FFF | 16kB | Alternate big 16 ROM      | 2 | X | |   0x0000 |   0x3FFF |
--| 0x008000 | 0x00FFFF | 32kB | Alternate big 32 ROM      | 3 | X | |   0x0000 |   0x7FFF | * only for +2A/+3 models, TBD
--| 0x010000 | 0x011FFF |  8kB | RAM bank 0                |0,1| 0 | |   0x2000 |   0x3FFF |
--| 0x012000 | 0x013FFF |  8kB | RAM bank 1                |0,1| 1 | |   0x2000 |   0x3FFF |
--| 0x014000 | 0x015FFF |  8kB | RAM bank 2                |0,1| 2 | |   0x2000 |   0x3FFF |
--| 0x016000 | 0x017FFF |  8kB | RAM bank 3                |0,1| 3 | |   0x2000 |   0x3FFF |
--| 0x018000 | 0x019FFF |  8kB | RAM bank 4                |0,1| 4 | |   0x2000 |   0x3FFF |
--| 0x01A000 | 0x01BFFF |  8kB | RAM bank 5                |0,1| 5 | |   0x2000 |   0x3FFF |
--| 0x01C000 | 0x01DFFF |  8kB | RAM bank 6                |0,1| 6 | |   0x2000 |   0x3FFF |
--| 0x01E000 | 0x01FFFF |  8kB | RAM bank 7                |0,1| 7 | |   0x2000 |   0x3FFF |
--| 0x020000 | 0x7FFFFF |      | Unused                    | - | - | |   -      |   -      |


    log("Writing on ROM 0 location (0x000000 [0x0000]) : 0xDE 0xAD");
    qspiramWrite(QSPIRam0_Cmd, x"000000", x"DE");
    qspiramWrite(QSPIRam0_Cmd, x"000001", x"AD");
    qspiramWrite(QSPIRam0_Cmd, x"001FFF", x"CC");

    log("Writing on ROM 1 location (0x002000 [0x0000]) : 0xBE 0xEF");
    qspiramWrite(QSPIRam0_Cmd, x"002000", x"BE");
    qspiramWrite(QSPIRam0_Cmd, x"002001", x"EF");
    qspiramWrite(QSPIRam0_Cmd, x"003FFF", x"CD");

    log("Writing on ROM 2 location (0x004000 [0x0000]) : 0xCA 0xFE");
    qspiramWrite(QSPIRam0_Cmd, x"004000", x"CA");
    qspiramWrite(QSPIRam0_Cmd, x"004001", x"FE");
    qspiramWrite(QSPIRam0_Cmd, x"007FFF", x"CE");


    log("Writing on RAM locations (0x010000-0x01E000 [0x2000]) : 0x50 to 0x57");
    qspiramWrite(QSPIRam0_Cmd, x"010000", x"50");
    qspiramWrite(QSPIRam0_Cmd, x"012000", x"51");
    qspiramWrite(QSPIRam0_Cmd, x"014000", x"52");
    qspiramWrite(QSPIRam0_Cmd, x"016000", x"53");
    qspiramWrite(QSPIRam0_Cmd, x"018000", x"54");
    qspiramWrite(QSPIRam0_Cmd, x"01A000", x"55");
    qspiramWrite(QSPIRam0_Cmd, x"01C000", x"56");
    qspiramWrite(QSPIRam0_Cmd, x"01E000", x"57");

    -- Activate ROMCS
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= "00000100"; -- ForceROMCS
    spiPayload_in_s(3) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    -- Select ROM 0
    spiPayload_in_s(0) <= x"EB";
    spiPayload_in_s(1) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 2, spiPayload_in_s, spiPayload_out_s);

    -- So that ROMCS is activated we need to perform an opcode read, which sets M1 properly.
    SpectrumReadOpcode( Spectrum_Cmd, Spectrum_Data, x"0000", data);
    Check("1: ROM0 data", data, x"DE");
    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"0001", data);
    Check("2: ROM0 data", data, x"AD");
    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"1FFF", data);
    Check("3: ROM0 data", data, x"CC");

    log("Check that ROM writes do not work");
    SpectrumWriteMem( Spectrum_Cmd, Spectrum_Data, x"0000", x"F1");
    SpectrumWriteMem( Spectrum_Cmd, Spectrum_Data, x"0001", x"F1");
    SpectrumWriteMem( Spectrum_Cmd, Spectrum_Data, x"1FFF", x"F1");

    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"0000", data);
    Check("1: ROM0 data", data, x"DE");
    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"0001", data);
    Check("2: ROM0 data", data, x"AD");
    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"1FFF", data);
    Check("3: ROM0 data", data, x"CC");

    checkram(x"B0");

    -- Select ROM 1 - 8K
    spiPayload_in_s(0) <= x"EB";
    spiPayload_in_s(1) <= x"01";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 2, spiPayload_in_s, spiPayload_out_s);

    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"0000", data);
    Check("1: ROM0 data", data, x"BE");
    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"0001", data);
    Check("2: ROM0 data", data, x"EF");
    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"1FFF", data);
    Check("3: ROM0 data", data, x"CD");

    log("Check that ROM writes do not work");
    SpectrumWriteMem( Spectrum_Cmd, Spectrum_Data, x"0000", x"F1");
    SpectrumWriteMem( Spectrum_Cmd, Spectrum_Data, x"0001", x"F1");
    SpectrumWriteMem( Spectrum_Cmd, Spectrum_Data, x"1FFF", x"F1");

    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"0000", data);
    Check("1: ROM0 data", data, x"BE");
    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"0001", data);
    Check("2: ROM0 data", data, x"EF");
    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"1FFF", data);
    Check("3: ROM0 data", data, x"CD");

    checkram(x"A0");

    -- Select ROM 2 - 16K
    spiPayload_in_s(0) <= x"EB";
    spiPayload_in_s(1) <= x"02";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 2, spiPayload_in_s, spiPayload_out_s);

    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"0000", data);
    Check("1: ROM0 data", data, x"CA");
    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"0001", data);
    Check("2: ROM0 data", data, x"FE");
    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"3FFF", data);
    Check("3: ROM0 data", data, x"CE");

    log("Check that ROM writes do not work");
    SpectrumWriteMem( Spectrum_Cmd, Spectrum_Data, x"0000", x"F1");
    SpectrumWriteMem( Spectrum_Cmd, Spectrum_Data, x"0001", x"F1");
    SpectrumWriteMem( Spectrum_Cmd, Spectrum_Data, x"3FFF", x"F1");

    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"0000", data);
    Check("1: ROM0 data", data, x"CA");
    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"0001", data);
    Check("2: ROM0 data", data, x"FE");
    SpectrumReadMem( Spectrum_Cmd, Spectrum_Data, x"3FFF", data);
    Check("3: ROM0 data", data, x"CE");

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );
    logger_end;
  end process;


end t013;
