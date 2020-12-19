use work.tbc_device_p.all;
use work.logger.all;

architecture t010 of tbc_device is

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
    logger_start("T010","USB");

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    Usbdevice_Cmd.Enabled <= true;
    Usbdevice_Cmd.FullSpeed <= false;

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

    wait on CtrlPins_Data.IO27 for 50 us;

    checkInterrupt('0');
    --Check("103: USB interrupt", CtrlPins_Data.USB_INTn, '0');

    spiPayload_in_s(0) <= x"61"; -- USB write
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"03"; -- Address
    spiPayload_in_s(3) <= x"FF"; -- Clear all interrupts
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    wait for 1 us;

    --Check("104: USB interrupt", CtrlPins_Data.USB_INTn, '1');
    checkInterrupt('1');

    wait on Usbdevice_Data.SOFStamp for 100 us;

    Usbdevice_Cmd.AckMode <= Ack;
    Usbdevice_Cmd.TxData(0) <= x"FA";
    Usbdevice_Cmd.TxData(1) <= x"09";
    Usbdevice_Cmd.TxData(2) <= x"04";
    Usbdevice_Cmd.TxData(3) <= x"00";
    Usbdevice_Cmd.TxData(4) <= x"00";
    Usbdevice_Cmd.TxData(5) <= x"01";
    Usbdevice_Cmd.TxData(6) <= x"03";
    Usbdevice_Cmd.TxData(7) <= x"00";

    Usbdevice_Cmd.TxDataLen <= 8;




    report "Setting up transaction";
    -- Configure channel.

    spiPayload_in_s(0) <= x"61"; -- USB write
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"80"; -- Adresss

    spiPayload_in_s(3) <= x"C8"; -- EPtype 0, max size 64
    spiPayload_in_s(4) <= x"81"; 
    spiPayload_in_s(5) <= x"81"; -- Address 28, enabled
    spiPayload_in_s(6) <= x"FF"; -- All interrupts enabled
    spiPayload_in_s(7) <= x"FF"; -- All interrupt clear
    spiPayload_in_s(8) <= x"01"; -- Interval                 --5
    spiPayload_in_s(9) <= x"00"; -- Interval
    spiPayload_in_s(10) <= x"00"; -- Interval
    spiPayload_in_s(11)  <= x"64"; -- Transaction 1: dpid 00   --8
    spiPayload_in_s(12)  <= x"80";  -- epaddr                   --9
    spiPayload_in_s(13) <= x"88"; -- Transaction 1: size 6, count 8  --10

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 14, spiPayload_in_s, spiPayload_out_s);

    wait on CtrlPins_Data.IO27 for 400 us;

    checkInterrupt('0');
    --Check("014: USB interrupt", CtrlPins_Data.USB_INTn, '0');

    report "USB interrupt";
    -- Get interrupts

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
      report "****** USB interrupt **********";
    end if;
  end process;

end t010;
