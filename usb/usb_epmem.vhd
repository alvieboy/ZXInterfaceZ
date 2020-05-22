library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

entity usb_epmem is
  port (
      uclk_i    : in std_logic;
      urd_i     : in std_logic;
      uwr_i     : in std_logic;
      uaddr_i   : in std_logic_vector(9 downto 0);
      udata_o   : out std_logic_vector(7 downto 0);
      udata_i   : in std_logic_vector(7 downto 0);

      hclk_i    : in std_logic;
      hrd_i     : in std_logic;
      hwr_i     : in std_logic;
      haddr_i   : in std_logic_vector(9 downto 0);
      hdata_o   : out std_logic_vector(7 downto 0);
      hdata_i   : in std_logic_vector(7 downto 0)
  );
end entity usb_epmem;

architecture beh of usb_epmem is


begin

  epram_inst : entity work.epram
  PORT MAP (
		address_a	 => uaddr_i,
		clock_a	 => uclk_i,
		data_a	 => udata_i,
		rden_a	 => urd_i,
		wren_a	 => uwr_i,
		q_a	 => udata_o,

		q_b	 => hdata_o,
		wren_b	 => hwr_i,
		rden_b	 => hrd_i,
		data_b	 => hdata_i,
		clock_b	 => hclk_i,
		address_b	 => haddr_i
	);

end beh;

