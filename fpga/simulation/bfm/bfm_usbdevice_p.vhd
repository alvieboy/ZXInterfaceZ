LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

LIBRARY work;
use work.txt_util.all;

package bfm_usbdevice_p is

  type UsbdeviceCmd is (
    NONE,
    ATTACH
  );

  type AckMode_type is ( Ack, Nack, Timeout );

  subtype word is std_logic_vector(7 downto 0);

  type usb_txdata_type is array (0 to 63) of word;


  type Data_Usbdevice_type is record
    Busy      : boolean;
    ResetStamp : time;
    SOFStamp   : time;
  end record;

  type Cmd_Usbdevice_type is record
    Cmd       : UsbdeviceCmd;
    Enabled   : boolean;
    FullSpeed : boolean;
    AckMode   : AckMode_type;
    TxData    : usb_txdata_type;
    TxDataLen : natural;
  end record;

  component bfm_usbdevice is
    port (
      Cmd_i   : in Cmd_Usbdevice_type;
      Data_o  : out Data_Usbdevice_type;
      DM_io   : inout std_logic;
      DP_io   : inout std_logic
    );
  end component bfm_usbdevice;

  constant Cmd_Usbdevice_Defaults: Cmd_Usbdevice_type := (
    Cmd       => NONE,
    Enabled   => false,
    FullSpeed => true,
    AckMode   => Timeout,
    TxData    => (others => (others => '0')),
    TxDataLen => 0
  );

end package;

package body bfm_usbdevice_p is



end package body;

