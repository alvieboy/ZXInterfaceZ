library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

package zxinterfaceports is

  TYPE zxaddrb IS ( 'x', 'X', '0', '1' );
  TYPE zxaddrb_vector IS ARRAY (NATURAL RANGE <>) of zxaddrb;
  FUNCTION to_stdlogicvector(b: zxaddrb_vector) RETURN std_logic_vector;
  FUNCTION z80_address_match(a: std_logic_vector; b: zxaddrb_vector) RETURN BOOLEAN;
  FUNCTION z80_address_match_std_logic(a: std_logic_vector; b: zxaddrb_vector) RETURN std_logic;

  -- Internal registers. All internal registers use the following mask.
  -- 0 x 1 x x x 1 1

  constant SPECT_PORT_INTERNAL_REGISTER       : zxaddrb_vector(15 downto 0) := "xxxxxxxx" & "0x1xxx11";

  constant SPECT_PORT_SCRATCH0                : std_logic_vector(7 downto 0) := "00100011";   -- x"23"
  constant SPECT_PORT_MISCCTRL                : std_logic_vector(7 downto 0) := "00100111";   -- x"27"
  constant SPECT_PORT_CMD_FIFO_STATUS         : std_logic_vector(7 downto 0) := "00101011";   -- x"2B"
  constant SPECT_PORT_RESOURCE_FIFO_STATUS    : std_logic_vector(7 downto 0) := "00101111";   -- x"2F"
  constant SPECT_PORT_RESOURCE_FIFO_DATA      : std_logic_vector(7 downto 0) := "00110011";   -- x"33"
  constant SPECT_PORT_RAM_ADDR_LOW            : std_logic_vector(7 downto 0) := "00110111";   -- x"37"
  constant SPECT_PORT_RAM_ADDR_MIDDLE         : std_logic_vector(7 downto 0) := "00111011";   -- x"3B"
  constant SPECT_PORT_RAM_ADDR_HIGH           : std_logic_vector(7 downto 0) := "00111111";   -- x"3F"
  constant SPECT_PORT_RAM_DATA                : std_logic_vector(7 downto 0) := "01100011";   -- x"63"
  constant SPECT_PORT_CMD_FIFO_DATA           : std_logic_vector(7 downto 0) := "01100111";   -- x"67"
  constant SPECT_PORT_MEMSEL                  : std_logic_vector(7 downto 0) := "01101011";   -- x"6B"
  constant SPECT_PORT_NMIREASON               : std_logic_vector(7 downto 0) := "01101111";   -- x"6F"
  constant SPECT_PORT_PSEUDO_AUDIO_DATA       : std_logic_vector(7 downto 0) := "01110011";   -- x"73"
  -- UNUSED: "01110111"; -- x"77"
  -- UNUSED: "01111011"; -- x"7b"
  -- UNUSED: "01111111"; -- x"7f"

  constant SPECT_PORT_KEMPSTON_JOYSTICK       : zxaddrb_vector(15 downto 0) := "xxxxxxxx" & "000xxxx1";
  constant SPECT_PORT_KEMPSTON_MOUSEX         : zxaddrb_vector(15 downto 0) := "xxxxx011" & "xx0xxxx1";
  constant SPECT_PORT_KEMPSTON_MOUSEY         : zxaddrb_vector(15 downto 0) := "xxxxx111" & "xx0xxxx1";
  constant SPECT_PORT_KEMPSTON_MOUSEB         : zxaddrb_vector(15 downto 0) := "xxxxxx10" & "xx0xxxx1";
  constant SPECT_PORT_AY_REGISTER             : zxaddrb_vector(15 downto 0) := "11xxxxxx" & "xxxxxx01";
  constant SPECT_PORT_AY_DATA                 : zxaddrb_vector(15 downto 0) := "10xxxxxx" & "xxxxxx01";
  constant SPECT_PORT_128PAGE_REGISTER        : zxaddrb_vector(15 downto 0) := "0xxxxxxx" & "xxxxxx01"; -- AKA 7ffd port
  constant SPECT_PORT_2A_PMC_REGISTER         : zxaddrb_vector(15 downto 0) := "01xxxxxx" & "xxxxxx01";  -- Conflicts with 128K page
  constant SPECT_PORT_2A_SMC_REGISTER         : zxaddrb_vector(15 downto 0) := "0001xxxx" & "xxxxxx01"; -- AKA 1ffd port

end package;

package body zxinterfaceports is

    FUNCTION To_StdLogicVector  (b:zxaddrb_vector) RETURN std_logic_vector IS
        ALIAS bv : zxaddrb_vector ( b'LENGTH-1 DOWNTO 0 ) IS b;
        VARIABLE result : std_logic_vector ( b'LENGTH-1 DOWNTO 0 );
    BEGIN
        FOR i IN result'RANGE LOOP
            CASE bv(i) IS
                WHEN '0' => result(i) := '0';
                WHEN '1' => result(i) := '1';
                WHEN others => result(i) := 'X';
            END CASE;
        END LOOP;
        RETURN result;
    END;

    FUNCTION z80_address_match(a: std_logic_vector; b: zxaddrb_vector) RETURN BOOLEAN IS
      ALIAS bv : zxaddrb_vector ( b'LENGTH-1 DOWNTO 0 ) IS b;
      variable result: boolean;
    BEGIN
      result := true;
      FOR i in a'range LOOP
        case bv(i) is
          when '0' => if a(i)/='0' then result:=false; end if;
          when '1' => if a(i)/='1' then result:=false; end if;
          when others => null; -- Ignore
        end case;
      end loop;
      return result;
    end function;

  FUNCTION z80_address_match_std_logic(a: std_logic_vector; b: zxaddrb_vector) RETURN std_logic IS
    variable result: std_logic;
  BEGIN
    IF z80_address_match(a,b) THEN
      result := '1';
    ELSE
      result := '0';
    END IF;
    return result;
  END FUNCTION;


end package body;
