library ieee;
use ieee.std_logic_1164.all;

package usbpkg is

  type usb_transaction_status_type is (
    IDLE,
    BUSY,
    TIMEOUT,
    BABBLE,
    ACK,
    NACK
  );

  function reverse(value: in std_logic_vector) return std_logic_vector;

  -- PID Encodings
  constant USBF_T_PID_OUT		: std_logic_vector(3 downto 0) := "0001";
  constant USBF_T_PID_IN		: std_logic_vector(3 downto 0) := "1001";
  constant USBF_T_PID_SOF		: std_logic_vector(3 downto 0) := "0101";
  constant USBF_T_PID_SETUP	: std_logic_vector(3 downto 0) := "1101";

  constant USBF_T_PID_DATA0	: std_logic_vector(3 downto 0) := "0011";
  constant USBF_T_PID_DATA1	: std_logic_vector(3 downto 0) := "1011";
  constant USBF_T_PID_DATA2	: std_logic_vector(3 downto 0) := "0111";
  constant USBF_T_PID_MDATA	: std_logic_vector(3 downto 0) := "1111";

  constant USBF_T_PID_ACK		: std_logic_vector(3 downto 0) := "0010";
  constant USBF_T_PID_NACK	: std_logic_vector(3 downto 0) := "1010";
  constant USBF_T_PID_STALL	: std_logic_vector(3 downto 0) := "1110";
  constant USBF_T_PID_NYET	: std_logic_vector(3 downto 0) := "0110";

  constant USBF_T_PID_PRE		: std_logic_vector(3 downto 0) := "1100";
  constant USBF_T_PID_ERR		: std_logic_vector(3 downto 0) := "1100";
  constant USBF_T_PID_SPLIT	: std_logic_vector(3 downto 0) := "1000";
  constant USBF_T_PID_PING	: std_logic_vector(3 downto 0) := "0100";
  constant USBF_T_PID_RES		: std_logic_vector(3 downto 0) := "0000";

  function genpid(pidin: std_logic_vector(3 downto 0)) return std_logic_vector;

  function is_token_pid(pidin: std_logic_vector(3 downto 0)) return boolean;
  function is_data_pid(pidin: std_logic_vector(3 downto 0)) return boolean;
  function is_handshake_pid(pidin: std_logic_vector(3 downto 0)) return boolean;
  function needack(pidin: std_logic_vector(3 downto 0)) return boolean;

end package;

package body usbpkg is


  function reverse(value: in std_logic_vector) return std_logic_vector is
    variable ret: std_logic_vector(value'HIGH downto value'LOW);
  begin
    for i in value'LOW to value'HIGH loop
      ret(i) := value(value'HIGH-i);
    end loop;
    return ret;
  end function;

  function genpid(pidin: std_logic_vector(3 downto 0)) return std_logic_vector is
    variable r: std_logic_vector(7 downto 0);
  begin
    r(3 downto 0) := pidin;
    r(7 downto 4) := not pidin;
    return r;
  end function;

  function is_token_pid(pidin: std_logic_vector(3 downto 0)) return boolean is
  begin
    return pidin(1 downto 0)="01";
  end function;

  function is_data_pid(pidin: std_logic_vector(3 downto 0)) return boolean is
  begin
    return pidin(1 downto 0)="11";
  end function;

  function is_handshake_pid(pidin: std_logic_vector(3 downto 0)) return boolean is
  begin
    return pidin(1 downto 0)="00";
  end function;

  function needack(pidin: std_logic_vector(3 downto 0)) return boolean is
    variable r: boolean;
  begin
    r := false;
    case pidin is
      when USBF_T_PID_IN | USBF_T_PID_SETUP | USBF_T_PID_DATA0 | USBF_T_PID_DATA1 =>
        r := true;
      when others =>
        r := false;
    end case;
    return r;
  end if;

end package body;