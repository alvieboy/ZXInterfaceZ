use work.tbc_device_p.all;
use work.logger.all;

architecture t005 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
    variable data: std_logic_vector(7 downto 0);
  begin
    logger_start("T005","ULA tests");

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    Ula_Cmd.Enabled <= true;
    Ula_Cmd.Audio   <= '1';
    Ula_Cmd.Cmd     <= SETAUDIODATA;
    wait for 0 ps;
    Ula_Cmd.Cmd     <= NONE;

    Ula_Cmd.Enabled <= true;
    Ula_Cmd.KeybScan <= "11110";
    Ula_Cmd.KeybAddr <= 0;
    Ula_Cmd.Cmd     <= SETKBDDATA;
    wait for 0 ps;
    Ula_Cmd.Cmd     <= NONE;

    SpectrumReadIO(Spectrum_Cmd, Spectrum_Data, x"fefe", data);

    Check("Idle data with keyboard", data, x"fe");

    -- Activate ULA hack.

    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"01";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    SpectrumReadIO(Spectrum_Cmd, Spectrum_Data, x"fefe", data);

    Check("Data with keyboard", data(4 downto 0), "11110");

    wait for 2 us;

    -- Disable ULA hack, activate keyboard

    spiPayload_in_s(0) <= x"EE";
    spiPayload_in_s(1) <= x"02";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"00";
    spiPayload_in_s(4) <= x"00";
    spiPayload_in_s(5) <= x"0F"; -- KBD, Joy,Mouse,AY

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 6, spiPayload_in_s, spiPayload_out_s);

    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    SpectrumReadIO(Spectrum_Cmd, Spectrum_Data, x"fefe", data);

    Check("Idle data with keyboard", data, x"fe");
    -- Inject keyboard at 1st column

    spiPayload_in_s(0) <= x"EE";
    spiPayload_in_s(1) <= x"03";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"00";
    spiPayload_in_s(4) <= x"00";
    spiPayload_in_s(5) <= x"01"; -- Key pressed
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 6, spiPayload_in_s, spiPayload_out_s);

    SpectrumReadIO(Spectrum_Cmd, Spectrum_Data, x"fefe", data);
    Check("Idle data with keyboard", data, x"be");

    wait for 1 us;


    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;


end t005;
