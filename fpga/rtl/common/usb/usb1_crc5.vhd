library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;
library work;
use work.usbpkg.all;

entity usb1_crc5 is
  generic (
    reverse_input: boolean;
    invert_and_reverse_output: boolean := false
  );
  port (
    crc_in:   in std_logic_vector(4 downto 0);
    din:      in std_logic_vector(10 downto 0);
    crc_out:  out std_logic_vector(4 downto 0)
  );
end entity usb1_crc5;

architecture behave of usb1_crc5 is

  signal computed_crc : std_logic_vector(4 downto 0);
  signal din_s        : std_logic_vector(10 downto 0);

begin

  din_s <= din when reverse_input=false else reverse(din);

  computed_crc(0) <=	din_s(10) xor din_s(9) xor din_s(6) xor din_s(5) xor din_s(3) xor
			din_s(0) xor crc_in(0) xor crc_in(3) xor crc_in(4);

  computed_crc(1) <=	din_s(10) xor din_s(7) xor din_s(6) xor din_s(4) xor din_s(1) xor
			crc_in(0) xor crc_in(1) xor crc_in(4);

  computed_crc(2) <=	din_s(10) xor din_s(9) xor din_s(8) xor din_s(7) xor din_s(6) xor
			din_s(3) xor din_s(2) xor din_s(0) xor crc_in(0) xor crc_in(1) xor
			crc_in(2) xor crc_in(3) xor crc_in(4);

  computed_crc(3) <=	din_s(10) xor din_s(9) xor din_s(8) xor din_s(7) xor din_s(4) xor din_s(3) xor
			din_s(1) xor crc_in(1) xor crc_in(2) xor crc_in(3) xor crc_in(4);

  computed_crc(4) <=	din_s(10) xor din_s(9) xor din_s(8) xor din_s(5) xor din_s(4) xor din_s(2) xor
			crc_in(2) xor crc_in(3) xor crc_in(4);


  process(computed_crc)
  begin
    if invert_and_reverse_output then
      crc_out <= not ( computed_crc(0)&computed_crc(1)&computed_crc(2)&computed_crc(3)&computed_crc(4) );
    else
      crc_out <= computed_crc;
    end if;
  end process;


end behave;
