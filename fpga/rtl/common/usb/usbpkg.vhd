library ieee;
use ieee.std_logic_1164.all;

package usbpkg is

  constant C_USB_TRANS_USE_COUNTERS: boolean := false;
  constant C_USB_TRANS_DEBUG: boolean := false;

  type usb_transaction_status_type is (
    IDLE,
    BUSY,
    TIMEOUT,
    BABBLE,
    ACK,
    NACK,
    STALL,
    CRCERROR,
    COMPLETED
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
  function is_pre_pid(pidin: std_logic_vector(3 downto 0)) return boolean;

  function needack(pidin: std_logic_vector(3 downto 0)) return boolean;
  function needdata(pidin: std_logic_vector(3 downto 0)) return boolean;
  function inv(value: in std_logic_vector) return std_logic_vector;
  function usb_crc16(
    crc_in:   in std_logic_vector(15 downto 0);
    din:      in std_logic_vector(7 downto 0))
    return std_logic_vector;

  function altsim(synth: natural; simulation: natural) return natural;

end package;

package body usbpkg is


  function reverse(value: in std_logic_vector) return std_logic_vector is
    variable ret: std_logic_vector(value'RANGE);--HIGH downto value'LOW);
  begin
    for i in value'LOW to value'HIGH loop
      ret(i) := value(value'HIGH+value'LOW-i);
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

  function is_pre_pid(pidin: std_logic_vector(3 downto 0)) return boolean is
  begin
    return pidin = USBF_T_PID_PRE;
  end function;

  function is_data_pid(pidin: std_logic_vector(3 downto 0)) return boolean is
  begin
    return pidin(1 downto 0)="11";
  end function;

  function is_handshake_pid(pidin: std_logic_vector(3 downto 0)) return boolean is
  begin
    return pidin(1 downto 0)="10";
  end function;

  function needack(pidin: std_logic_vector(3 downto 0)) return boolean is
    variable r: boolean;
  begin
    r := false;
    case pidin is
      when USBF_T_PID_IN | USBF_T_PID_DATA0 | USBF_T_PID_DATA1 =>
        r := true;
      when others =>
        r := false;
    end case;
    return r;
  end function;

  function needdata(pidin: std_logic_vector(3 downto 0)) return boolean is
    variable r: boolean;
  begin
    r := false;
    case pidin is
      when USBF_T_PID_IN =>
        r := true;
      when others =>
        r := false;
    end case;
    return r;
  end function;

  function inv(value: in std_logic_vector) return std_logic_vector is
    variable ret: std_logic_vector(value'HIGH downto value'LOW);
  begin
    --l1: for i in value'LOW to value'HIGH loop
    --  ret(i) := not value(i);
    --end loop;
    return not value;--ret;
  end function;


  function usb_crc16(
    crc_in:   in std_logic_vector(15 downto 0);
    din:      in std_logic_vector(7 downto 0))
    return std_logic_vector is

    variable crc_out: std_logic_vector(15 downto 0);
  begin

    crc_out(0) :=	din(7) xor din(6) xor din(5) xor din(4) xor din(3) xor
         din(2) xor din(1) xor din(0) xor crc_in(8) xor crc_in(9) xor
         crc_in(10) xor crc_in(11) xor crc_in(12) xor crc_in(13) xor
         crc_in(14) xor crc_in(15);
    crc_out(1) :=	din(7) xor din(6) xor din(5) xor din(4) xor din(3) xor din(2) xor
         din(1) xor crc_in(9) xor crc_in(10) xor crc_in(11) xor
         crc_in(12) xor crc_in(13) xor crc_in(14) xor crc_in(15);
    crc_out(2) :=	din(1) xor din(0) xor crc_in(8) xor crc_in(9);
    crc_out(3) :=	din(2) xor din(1) xor crc_in(9) xor crc_in(10);
    crc_out(4) :=	din(3) xor din(2) xor crc_in(10) xor crc_in(11);
    crc_out(5) :=	din(4) xor din(3) xor crc_in(11) xor crc_in(12);
    crc_out(6) :=	din(5) xor din(4) xor crc_in(12) xor crc_in(13);
    crc_out(7) :=	din(6) xor din(5) xor crc_in(13) xor crc_in(14);
    crc_out(8) :=	din(7) xor din(6) xor crc_in(0) xor crc_in(14) xor crc_in(15);
    crc_out(9) :=	din(7) xor crc_in(1) xor crc_in(15);
    crc_out(10) :=	crc_in(2);
    crc_out(11) :=	crc_in(3);
    crc_out(12) :=	crc_in(4);
    crc_out(13) :=	crc_in(5);
    crc_out(14) :=	crc_in(6);
    crc_out(15) :=	din(7) xor din(6) xor din(5) xor din(4) xor din(3) xor din(2) xor
         din(1) xor din(0) xor crc_in(7) xor crc_in(8) xor crc_in(9) xor
         crc_in(10) xor crc_in(11) xor crc_in(12) xor crc_in(13) xor
         crc_in(14) xor crc_in(15);
    return crc_out;
  end function;

  function altsim(synth: natural; simulation: natural) return natural is
    variable r : natural;
  begin
    r := synth;
    -- synthesis translate_off
    r := simulation;
    -- synthesis translate_on
    return r;
  end altsim;

end package body;
