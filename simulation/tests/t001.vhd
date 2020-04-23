use work.tbc_device_p.all;

architecture t001 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
  begin
    report "T001: Read FPGA ID";

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    
    spiPayload_in_s(0) <= x"9E";
    spiPayload_in_s(1) <= x"FF";
    spiPayload_in_s(2) <= x"FF";
    spiPayload_in_s(3) <= x"FF";
    spiPayload_in_s(4) <= x"FF";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

    Check("(9E) command 1st byte", spiPayload_out_s(1), x"A5");
    Check("(9E) command 2nd byte", spiPayload_out_s(2), x"10");
    Check("(9E) command 3rd byte", spiPayload_out_s(3), x"03");

    spiPayload_in_s(0) <= x"9F";
    spiPayload_in_s(1) <= x"FF";
    spiPayload_in_s(2) <= x"FF";
    spiPayload_in_s(3) <= x"FF";
    spiPayload_in_s(4) <= x"FF";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

    Check("(9F) command 1st byte", spiPayload_out_s(1), x"A5");
    Check("(9F) command 2nd byte", spiPayload_out_s(2), x"10");
    Check("(9F) command 3rd byte", spiPayload_out_s(3), x"03");
    

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;


end t001;
