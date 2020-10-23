LIBRARY ieee;
USE ieee.std_logic_1164.all;

ENTITY videoram IS
	PORT
	(
		address_a		: IN STD_LOGIC_VECTOR (12 DOWNTO 0);
		address_b		: IN STD_LOGIC_VECTOR (12 DOWNTO 0);
		clock_a		: IN STD_LOGIC  := '1';
		clock_b		: IN STD_LOGIC ;
		data_a		: IN STD_LOGIC_VECTOR (7 DOWNTO 0);
		data_b		: IN STD_LOGIC_VECTOR (7 DOWNTO 0);
		rden_a		: IN STD_LOGIC  := '1';
		rden_b		: IN STD_LOGIC  := '1';
		wren_a		: IN STD_LOGIC  := '0';
		wren_b		: IN STD_LOGIC  := '0';
		q_a		: OUT STD_LOGIC_VECTOR (7 DOWNTO 0);
		q_b		: OUT STD_LOGIC_VECTOR (7 DOWNTO 0)
	);
END videoram;

architecture sim of videoram is

begin
  screen_ram: entity work.generic_dp_ram2
  generic map (
    address_bits  => 13, -- 8KB
    data_bits     => 8

  )
   port map (
    clka    => clock_a,
    rda     => rden_a,
    wea     => wren_a,
    addra   => address_a,
    dia     => data_a,
    doa     => q_a,

    clkb    => clock_b,
    rdb     => rden_b,
    web     => wren_b,
    dib     => data_b,
    addrb   => address_b,
    dob     => q_b
  );

end sim;
