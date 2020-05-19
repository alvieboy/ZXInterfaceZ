library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

entity usb_epmem is
  port (
      uclk_i    : in std_logic;
      urd_i     : in std_logic;
      uwr_i     : in std_logic;
      uaddr_i   : in std_logic_vector(10 downto 0);
      udata_o   : out std_logic_vector(7 downto 0);
      udata_i   : in std_logic_vector(7 downto 0);

      hclk_i    : in std_logic;
      hrd_i     : in std_logic;
      hwr_i     : in std_logic;
      haddr_i   : in std_logic_vector(10 downto 0);
      hdata_o   : out std_logic_vector(7 downto 0);
      hdata_i   : in std_logic_vector(7 downto 0)
  );
end entity usb_epmem;

architecture beh of usb_epmem is


begin

  mem_inst: entity work.generic_dp_ram2
    generic map (
      address_bits    => 11,
      data_bits       => 8
    )
    port map (
      clka    => uclk_i,
      rda     => urd_i,
      wea     => uwr_i,
      addra   => uaddr_i,
      dia     => udata_i,
      doa     => udata_o,
      clkb    => hclk_i,
      rdb     => hrd_i,
      web     => hwr_i,
      addrb   => haddr_i,
      dib     => hdata_i,
      dob     => hdata_o
    );

end beh;

