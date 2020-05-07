use work.tbc_device_p.all;

architecture t004 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
  begin
    report "T004: NMI/RETN/ROMCS tests";

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );
    romWrite(Rom_Cmd, 102, x"ED");
    romWrite(Rom_Cmd, 103, x"45");

    -- Write internal ROM
    spiPayload_in_s(0) <= x"E1";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"66";
    spiPayload_in_s(3) <= x"ED";
    spiPayload_in_s(4) <= x"45";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

    Check("ROMCS is not forced", CtrlPins_Data.ROMCS, '0');

    -- Activate ROMCS
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= "00000100"; -- ForceROMCS
    spiPayload_in_s(3) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    Check("ROMCS is forced", CtrlPins_Data.ROMCS, '1');

    Spectrum_Cmd.Address  <= x"0066";
    Spectrum_Cmd.Cmd      <= READOPCODE;
    Spectrum_Cmd.Refresh  <= x"0000";
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 0 ps;
    Spectrum_Cmd.Address  <= x"0067";
    Spectrum_Cmd.Cmd      <= READOPCODE;
    Spectrum_Cmd.Refresh  <= x"0000";
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 0 ps;

    Check("ROMCS is forced", CtrlPins_Data.ROMCS, '1');

    -- Now, force RETN to clear ROMCS.

    -- Activate ROMCS
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= "00000110"; -- ForceROMCS+RETN
    spiPayload_in_s(3) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    Check("ROMCS is forced", CtrlPins_Data.ROMCS, '1');

    -- Read opcode again

    Spectrum_Cmd.Address  <= x"0066";
    Spectrum_Cmd.Cmd      <= READOPCODE;
    Spectrum_Cmd.Refresh  <= x"0000";
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 0 ps;
    Spectrum_Cmd.Address  <= x"0067";
    Spectrum_Cmd.Cmd      <= READOPCODE;
    Spectrum_Cmd.Refresh  <= x"0000";
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 0 ps;

    Check("ROMCS is not forced", CtrlPins_Data.ROMCS, '0');

    -- Now, trigger NMI
    Check("NMI is not forced", CtrlPins_Data.NMI, '0');

    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= "01000000"; -- ForceNMI;
    spiPayload_in_s(3) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);
    Check("NMI is forced", CtrlPins_Data.NMI, '1');

    wait for 3 us;


    -- Write internal ROM
    spiPayload_in_s(0) <= x"E1";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"66";
    spiPayload_in_s(3) <= x"FF"; -- Invalid
    spiPayload_in_s(4) <= x"F1"; -- POP AF
    spiPayload_in_s(5) <= x"ED"; -- RETN
    spiPayload_in_s(6) <= x"45"; -- RETN

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 7, spiPayload_in_s, spiPayload_out_s);


    romWrite(Rom_Cmd, 102, x"F5");
    romWrite(Rom_Cmd, 103, x"E5");


    Spectrum_Cmd.Address  <= x"0066";
    Spectrum_Cmd.Cmd      <= READOPCODE;
    Spectrum_Cmd.Refresh  <= x"0000";
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 0 ps;

    -- NMI should be "acknowledged".
    Check("NMI is forced", CtrlPins_Data.NMI, '0');
    Check("ROMCS is forced", CtrlPins_Data.ROMCS, '1');

    Spectrum_Cmd.Address  <= x"0067";
    Spectrum_Cmd.Cmd      <= READOPCODE;
    Spectrum_Cmd.Refresh  <= x"0000";
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 0 ps;

    Spectrum_Cmd.Address  <= x"0068";
    Spectrum_Cmd.Cmd      <= READOPCODE;
    Spectrum_Cmd.Refresh  <= x"0000";
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 0 ps;

    Spectrum_Cmd.Address  <= x"0069";
    Spectrum_Cmd.Cmd      <= READOPCODE;
    Spectrum_Cmd.Refresh  <= x"0000";
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 0 ps;

    Check("ROMCS is not forced", CtrlPins_Data.ROMCS, '0');


    wait for 2 us;

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;


end t004;
