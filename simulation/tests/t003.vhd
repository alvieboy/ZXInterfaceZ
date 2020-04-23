use work.tbc_device_p.all;

architecture t003 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
  begin
    report "T003: Just run";

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    Spectrum_Cmd.Address  <= x"00FE";
    Spectrum_Cmd.Data     <= x"07";
    Spectrum_Cmd.Cmd      <= WRITEIO;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;

    -- Write a simple pattern on screen.

    Spectrum_Cmd.Address  <= x"4000";
    Spectrum_Cmd.Data     <= x"AA";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;
    Spectrum_Cmd.Address  <= x"4001";
    Spectrum_Cmd.Data     <= x"55";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;
    Spectrum_Cmd.Address  <= x"401F";
    Spectrum_Cmd.Data     <= x"55";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;
    Spectrum_Cmd.Address  <= x"4100";
    Spectrum_Cmd.Data     <= x"0F";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;

    Spectrum_Cmd.Address  <= x"5800";
    Spectrum_Cmd.Data     <= x"78";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;

    Spectrum_Cmd.Address  <= x"5801";
    Spectrum_Cmd.Data     <= "01101010";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;

    Spectrum_Cmd.Address  <= x"581F";
    Spectrum_Cmd.Data     <= "01101010";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;

    
    wait for 1000 ms;

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;


end t003;
