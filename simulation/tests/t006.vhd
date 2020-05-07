use work.tbc_device_p.all;

architecture t006 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
    variable data: std_logic_vector(7 downto 0);
  begin
    report "T006: TAP play";

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    wait for 100 ns;
    Audiocap_Cmd.Enabled <= true;

    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"06"; -- TAP reset + enable

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"04"; -- TAP enable

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    spiPayload_in_s(0) <= x"E5";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 2, spiPayload_in_s, spiPayload_out_s);

    Check("SPI fifo MSB is cleared", spiPayload_out_s(1), x"00");
    Check("SPI fifo LSB is cleared", spiPayload_out_s(2), x"00");
    

    -- Play
    spiPayload_in_s(0) <= x"E4";
    spiPayload_in_s(1) <= x"13";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"00";
    spiPayload_in_s(4) <= x"00";
    spiPayload_in_s(5) <= x"4d";
    spiPayload_in_s(6) <= x"2e";
    spiPayload_in_s(7) <= x"4d";
    spiPayload_in_s(8) <= x"49";
    spiPayload_in_s(9) <= x"4e";
    spiPayload_in_s(10) <= x"45";
    spiPayload_in_s(11) <= x"52";
    spiPayload_in_s(12) <= x"20";
    spiPayload_in_s(13) <= x"32";
    spiPayload_in_s(14) <= x"20";
    spiPayload_in_s(15) <= x"43";
    spiPayload_in_s(16) <= x"01";
    spiPayload_in_s(17) <= x"00";
    spiPayload_in_s(18) <= x"00";
    spiPayload_in_s(19) <= x"0a";
    spiPayload_in_s(20) <= x"01";
    spiPayload_in_s(21) <= x"45";
    spiPayload_in_s(22) <= x"09";
    spiPayload_in_s(23) <= x"00";
    spiPayload_in_s(24) <= x"ff";
    spiPayload_in_s(25) <= x"00";
    spiPayload_in_s(26) <= x"00";
    spiPayload_in_s(27) <= x"a5";
    spiPayload_in_s(28) <= x"00";
    spiPayload_in_s(29) <= x"e7";
    spiPayload_in_s(30) <= x"c3";
    spiPayload_in_s(31) <= x"a7";
    spiPayload_in_s(32) <= x"3a";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 33, spiPayload_in_s, spiPayload_out_s);


    wait for 100 ms;

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;


end t006;
