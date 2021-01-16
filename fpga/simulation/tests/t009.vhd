use work.tbc_device_p.all;
use work.logger.all;

architecture t009 of tbc_device is

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

    logger_start("T009","USB");

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    Usbdevice_Cmd.Enabled <= true;
    Usbdevice_Cmd.FullSpeed <= true;

    wait for 2 us;

    spiPayload_in_s(0) <= x"60"; -- USB read
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00"; -- Adresss
    spiPayload_in_s(3) <= x"00"; -- Dummy
    spiPayload_in_s(4) <= x"00"; -- data
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);
    Check("001: Nothing on USB bus", spiPayload_out_s(4)(0), '0');
    Check("002: No connection event on USB bus", spiPayload_out_s(4)(1), '0');

    -- Activate interrupts

    spiPayload_in_s(0) <= x"61"; -- USB write
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00"; -- Address
    spiPayload_in_s(3) <= x"20"; -- Power ON
    spiPayload_in_s(4) <= x"00"; -- n/a
    spiPayload_in_s(5) <= x"FF"; -- All interrupts enabled.
    spiPayload_in_s(6) <= x"00"; --
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 7, spiPayload_in_s, spiPayload_out_s);


    Usbdevice_Cmd.Cmd <= ATTACH;
    wait for 0 ns;
    Usbdevice_Cmd.Cmd <= NONE;
    wait for 0 ns;

--    wait for 5 us; -- Wait for attach detection.

    wait on CtrlPins_Data.IO27 for 50 us;

    checkInterrupt('0');
    --Check("103: USB interrupt", CtrlPins_Data.USB_INTn, '0');
    spiPayload_in_s(0) <= x"61"; -- USB write
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"03"; -- Address
    spiPayload_in_s(3) <= x"FF"; -- Clear all interrupts
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    wait for 1 us;

    checkInterrupt('1');
    --Check("104: USB interrupt", CtrlPins_Data.USB_INTn, '1');

    spiPayload_in_s(0) <= x"60"; -- USB read
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00"; -- Adresss
    spiPayload_in_s(3) <= x"00"; -- Dummy
    spiPayload_in_s(4) <= x"00"; -- data
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

    report "Status: "&hstr(spiPayload_out_s(4))& " " & str(spiPayload_out_s(4));


    Check("002: Device on USB bus", spiPayload_out_s(4)(0), '1');
    --Check("003: Connection event on USB bus", spiPayload_out_s(4)(1), '1');
    Check("004: Low-speed operation", spiPayload_out_s(4)(6), '0');

    spiPayload_in_s(0) <= x"60"; -- USB read
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00"; -- Adresss
    spiPayload_in_s(3) <= x"00"; -- Dummy
    spiPayload_in_s(4) <= x"00"; -- data
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

    Check("006: Device on USB bus", spiPayload_out_s(4)(0), '1');

    stamp := now;

    -- Reset device
    spiPayload_in_s(0) <= x"61"; -- USB write
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00"; -- Adresss
    spiPayload_in_s(3) <= "00110000"; -- RESET active, PWRON
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    --wait for 1 us;

    spiPayload_in_s(0) <= x"60"; -- USB read
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00"; -- Adresss
    spiPayload_in_s(3) <= x"00";
    spiPayload_in_s(4) <= x"00"; -- data
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

    Check("007: Connection event on USB bus", spiPayload_out_s(4)(1), '0');
    Check("008: Device on USB bus", spiPayload_out_s(4)(0), '1');
    Check("009: USB in reset", spiPayload_out_s(4)(4), '1');

    wait for 15 us;

    stamp := Usbdevice_Data.ResetStamp - stamp;
    Check("010: USB reset in time", stamp > 0 us and stamp < 10 us);
    report "USB reset in " & time'image(stamp);

    spiPayload_in_s(0) <= x"60"; -- USB read
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00"; -- Adresss
    spiPayload_in_s(3) <= x"00"; -- Dummy
    spiPayload_in_s(4) <= x"00"; -- data
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

    Check("011: Connection event on USB bus", spiPayload_out_s(4)(1), '0');
    Check("012: Device on USB bus", spiPayload_out_s(4)(0), '1');
    Check("013: USB NOT in reset", spiPayload_out_s(4)(4), '0');

    wait on Usbdevice_Data.SOFStamp for 10 us;

    checkInterrupt('1');
    --Check("014: USB not interrupt", CtrlPins_Data.USB_INTn, '1');

    -- Test setup frame

    spiPayload_in_s(0) <= x"61"; -- USB write
    spiPayload_in_s(1) <= x"04";
    spiPayload_in_s(2) <= x"00"; -- Adresss
    spiPayload_in_s(3) <= x"80"; -- bmRequestType
    spiPayload_in_s(4) <= x"06"; -- get descriptor
    spiPayload_in_s(5) <= x"01"; --
    spiPayload_in_s(6) <= x"00"; -- device
    spiPayload_in_s(7) <= x"00";
    spiPayload_in_s(8) <= x"00"; -- wIndex
    spiPayload_in_s(9) <= x"00";
    spiPayload_in_s(10) <= x"12"; -- wLength
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 11, spiPayload_in_s, spiPayload_out_s);



    Usbdevice_Cmd.AckMode <= Ack;

    -- Configure channel.

    spiPayload_in_s(0) <= x"61"; -- USB write
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"80"; -- Adresss

    spiPayload_in_s(3) <= x"3F"; -- EPtype 0, max size 64
    spiPayload_in_s(4) <= x"00"; -- Ep 0, OUT, High-speed
    spiPayload_in_s(5) <= x"80"; -- Address 0, enabled
    spiPayload_in_s(6) <= x"FF"; -- All interrupts enabled

    spiPayload_in_s(7) <= x"FF"; -- All interrupt clear
    spiPayload_in_s(8) <= x"01"; -- Interval
    spiPayload_in_s(9) <= x"00"; --
    spiPayload_in_s(10) <= x"00"; --


    spiPayload_in_s(11) <= x"03"; -- Transaction 1: dpid 11
    spiPayload_in_s(12) <= x"00";  -- epaddr
    spiPayload_in_s(13) <= x"86"; -- Transaction 1: size 6, count 1

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 14, spiPayload_in_s, spiPayload_out_s);

    wait on CtrlPins_Data.IO27 for 200 us;

    checkInterrupt('0');
    --Check("014: USB interrupt", CtrlPins_Data.USB_INTn, '0');

    report "USB interrupt";
    -- Get interrupts

    spiPayload_in_s(0) <= x"60"; -- USB read
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"44"; -- Adresss
    spiPayload_in_s(3) <= x"00"; -- dummy
    spiPayload_in_s(4) <= x"00"; -- dummy
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);
    report "Read " &hstr(spiPayload_out_s(4));
    Check("015: Transaction completed interrupt flag", spiPayload_out_s(4)(0),'1');

    spiPayload_in_s(0) <= x"61"; -- USB write
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"44"; -- Address
    spiPayload_in_s(3) <= x"FF";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);


    spiPayload_in_s(0) <= x"60"; -- USB read
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"44"; -- Address
    spiPayload_in_s(3) <= x"00"; -- dummy
    spiPayload_in_s(4) <= x"00"; -- dummy
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);
    report "Read " &hstr(spiPayload_out_s(4));
    Check("015: Transaction completed interrupt flag", spiPayload_out_s(4)(0),'0');


    Usbdevice_Cmd.AckMode <= Ack;


    report "Sending OUT request";

    spiPayload_in_s(0) <= x"61"; -- USB write
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"44"; -- Address+ep

    spiPayload_in_s(3) <= x"FF"; -- All interrupt clear
    spiPayload_in_s(4) <= x"00"; -- Transaction 1: dpid "00" : int
    spiPayload_in_s(5) <= x"01"; -- Transaction 1: epaddr
    spiPayload_in_s(6) <= x"80"; -- Transaction 1: size 0, count 1

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 7, spiPayload_in_s, spiPayload_out_s);

    wait on CtrlPins_Data.IO27 for 200 us;

    checkInterrupt('0');
    --Check("016: USB interrupt", CtrlPins_Data.USB_INTn, '0');

    spiPayload_in_s(0) <= x"60"; -- USB read
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"44"; -- Address
    spiPayload_in_s(3) <= x"00"; -- dummy
    spiPayload_in_s(4) <= x"00"; -- dummy
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);
    report "Read " &hstr(spiPayload_out_s(4));
    Check("017: Transaction completed interrupt flag", spiPayload_out_s(4)(0),'1');









    wait for 200 us;
    Usbdevice_Cmd.Enabled <= false;

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;


  process
  begin
    wait on CtrlPins_Data.IO27;
    if      CtrlPins_Data.IO27='0' then
      report "****** Interrupt **********";
    end if;
  end process;

end t009;
