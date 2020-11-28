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
    logger_start("T012","Capture infrastructure");

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
    spiPayload_in_s(0) <= x"63";

    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00"; -- Address.

    spiPayload_in_s(3) <= x"FF"; -- Mask
    spiPayload_in_s(4) <= x"FF";
    spiPayload_in_s(5) <= x"00";
    spiPayload_in_s(6) <= x"00";

    spiPayload_in_s(7) <= x"04"; -- Value
    spiPayload_in_s(8) <= x"00";
    spiPayload_in_s(9) <= x"00";
    spiPayload_in_s(10) <= x"00";


    spiPayload_in_s(11) <= x"00"; -- Edge/level
    spiPayload_in_s(12) <= x"00";
    spiPayload_in_s(13) <= x"00";
    spiPayload_in_s(14) <= x"00";


    spiPayload_in_s(15) <= x"00"; -- Unused
    spiPayload_in_s(16) <= x"00";
    spiPayload_in_s(17) <= x"00";
    spiPayload_in_s(18) <= x"00";



    spiPayload_in_s(19) <= x"04"; --clock
    spiPayload_in_s(20) <= x"00"; --divider
    spiPayload_in_s(21) <= x"00";
    spiPayload_in_s(22) <= x"80"; -- Start


    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 23, spiPayload_in_s, spiPayload_out_s);


    -- SPI ram write

    spiPayload_in_s(0) <= x"51";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"00";

    --spiPayload_in_s(4) <= x"21";
    --spiPayload_in_s(5) <= x"00";


    spiPayload_in_s(4) <= x"20"; -- LD HL, $2000
    spiPayload_in_s(5) <= x"3E";
    spiPayload_in_s(6) <= x"04"; -- LD A, $04
    spiPayload_in_s(7) <= x"77"; -- LD (HL), A
    spiPayload_in_s(8) <= x"7E"; -- LD A, (HL)
    spiPayload_in_s(9) <= x"18";
    spiPayload_in_s(10) <= x"FC"; -- JR -4

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 13, spiPayload_in_s, spiPayload_out_s);

    -- Activate ROMCS
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= "00000100"; -- ForceROMCS
    spiPayload_in_s(3) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);


    Spectrum_Cmd.Cmd <= RUNZ80;

    --wait for 10 us;

    waitcap: loop
      spiPayload_in_s(0) <= x"62";

      spiPayload_in_s(1) <= x"00";
      spiPayload_in_s(2) <= x"00"; -- Address.
      spiPayload_in_s(3) <= x"00"; -- Dummy

      spiPayload_in_s(4) <= x"00";


      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

      report "Counter zero " & str(spiPayload_out_s(4)(0));

      -- Check if counter=0
      if spiPayload_out_s(4)(0)='1' then
            exit waitcap;
      end if;

    end loop;


    spiPayload_in_s(0) <= x"62";

    spiPayload_in_s(1) <= x"00"; -- Address.
    spiPayload_in_s(2) <= x"08";
    spiPayload_in_s(3) <= x"00"; -- Dummy

    spiPayload_in_s(4) <= x"00";
    spiPayload_in_s(5) <= x"00";
    spiPayload_in_s(6) <= x"00";
    spiPayload_in_s(7) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 8, spiPayload_in_s, spiPayload_out_s);

      report "Trigger address: 0x" &
        hstr(spiPayload_out_s(7)) &
        hstr(spiPayload_out_s(6)) &
        hstr(spiPayload_out_s(5)) &
        hstr(spiPayload_out_s(4));

    -- test reading out the memory contents

    spiPayload_in_s(0) <= x"62";

    spiPayload_in_s(1) <= x"20";
    spiPayload_in_s(2) <= x"00"; -- Address.
    spiPayload_in_s(3) <= x"00"; -- Dummy

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 32+4, spiPayload_in_s, spiPayload_out_s);




    -- Start again

    -- Enable capture.
    spiPayload_in_s(0) <= x"63";

    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00"; -- Address.


    spiPayload_in_s(3) <= x"00"; -- Mask (LSB)
    spiPayload_in_s(4) <= x"00";
    spiPayload_in_s(5) <= x"00";
    spiPayload_in_s(6) <= x"04";   -- bit 26, spect_reset

    spiPayload_in_s(7) <= x"00"; -- Value
    spiPayload_in_s(8) <= x"00";
    spiPayload_in_s(9) <= x"00";
    spiPayload_in_s(10) <= x"00";  -- Falling edge


    spiPayload_in_s(11) <= x"00"; -- Edge/level
    spiPayload_in_s(12) <= x"00";
    spiPayload_in_s(13) <= x"00";
    spiPayload_in_s(14) <= x"04";  -- Edge for bit 26


    spiPayload_in_s(15) <= x"00"; -- Unused
    spiPayload_in_s(16) <= x"00";
    spiPayload_in_s(17) <= x"00";
    spiPayload_in_s(18) <= x"00";


    spiPayload_in_s(19) <= x"07"; --clock
    spiPayload_in_s(20) <= x"00"; 
    spiPayload_in_s(21) <= x"00";
    spiPayload_in_s(22) <= x"80"; -- Start

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 23, spiPayload_in_s, spiPayload_out_s);


    -- Activate ROMCS and reset.
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"02";  -- Reset active
    spiPayload_in_s(2) <= "00000100"; -- ForceROMCS
    spiPayload_in_s(3) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    -- Activate ROMCS and reset.
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";  -- Reset inactive
    spiPayload_in_s(2) <= "00000100"; -- ForceROMCS
    spiPayload_in_s(3) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);


    waitcap2: loop
      spiPayload_in_s(0) <= x"62";

      spiPayload_in_s(1) <= x"00";
      spiPayload_in_s(2) <= x"00"; -- Address.
      spiPayload_in_s(3) <= x"00"; -- Dummy

      spiPayload_in_s(4) <= x"00";


      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

      report "Counter zero " & str(spiPayload_out_s(4)(0));

      -- Check if counter=0
      if spiPayload_out_s(4)(0)='1' then
            exit waitcap2;
      end if;

    end loop;









    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );
    logger_end;
  end process;


end t012;
