use work.tbc_device_p.all;
use work.zxinterfacepkg.all;
use work.logger.all;

architecture t012 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
    variable data: std_logic_vector(7 downto 0);
    variable exp:  std_logic_vector(7 downto 0);
  begin
    logger_start("T012","RAM speed tests");

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    wait for 10 ns;

    QSPIRam0_Cmd.Enabled <= true;

    wait for 2 us;

    -- Enable capture.
    spiPayload_in_s(0) <= x"71";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"04";
    spiPayload_in_s(3) <= x"FF"; -- Mask
    spiPayload_in_s(4) <= x"FF";
    spiPayload_in_s(5) <= x"00";
    spiPayload_in_s(6) <= x"00";
    spiPayload_in_s(7) <= x"04"; -- Value
    spiPayload_in_s(8) <= x"00";
    spiPayload_in_s(9) <= x"00";
    spiPayload_in_s(10) <= x"00";
    spiPayload_in_s(11) <= x"04"; --clock
    spiPayload_in_s(12) <= x"00"; --divider
    spiPayload_in_s(13) <= x"00";
    spiPayload_in_s(14) <= x"80"; -- Start


    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 15, spiPayload_in_s, spiPayload_out_s);


    -- SPI ram write

    spiPayload_in_s(0) <= x"51";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"00";

    spiPayload_in_s(4) <= x"21";
    spiPayload_in_s(5) <= x"00";
    spiPayload_in_s(6) <= x"20"; -- LD HL, $2000
    spiPayload_in_s(7) <= x"3E"; 
    spiPayload_in_s(8) <= x"04"; -- LD A, $04
    spiPayload_in_s(9) <= x"77"; -- LD (HL), A
    spiPayload_in_s(10) <= x"7E"; -- LD A, (HL)
    spiPayload_in_s(11) <= x"18";
    spiPayload_in_s(12) <= x"FC"; -- JR -4

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 13, spiPayload_in_s, spiPayload_out_s);

    -- Activate ROMCS
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= "00000100"; -- ForceROMCS
    spiPayload_in_s(3) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);


    Spectrum_Cmd.Cmd <= RUNZ80;

    wait for 10 us;

    -- SPI ram write
    l: for i in 0 to 8 loop
    spiPayload_in_s(0) <= x"51";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"10";
    spiPayload_in_s(3) <= x"00";
    l2: for i in 0 to 63 loop
      spiPayload_in_s(4+i) <= std_logic_vector(to_unsigned(i+1,8));
    end loop;

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 68, spiPayload_in_s, spiPayload_out_s);

    -- readout
    spiPayload_in_s(0) <= x"50";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"10";
    spiPayload_in_s(3) <= x"00";
    spiPayload_in_s(4) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 69, spiPayload_in_s, spiPayload_out_s);

    l3: for i in 0 to 63 loop
      Check("SPI byte correct " & str(i), spiPayload_out_s(5+i), std_logic_vector(to_unsigned(i+1,8)));
    end loop;

    end loop;
    wait for 20 us;

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );
    logger_end;
  end process;


end t012;
