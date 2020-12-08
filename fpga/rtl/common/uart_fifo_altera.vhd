LIBRARY ieee;
USE ieee.std_logic_1164.all;

LIBRARY altera_mf;
USE altera_mf.all;

ENTITY uart_fifo_altera IS
	PORT
	(
		aclr		: IN STD_LOGIC ;
		clock		: IN STD_LOGIC ;
		data		: IN STD_LOGIC_VECTOR (7 DOWNTO 0);
		rdreq		: IN STD_LOGIC ;
		sclr		: IN STD_LOGIC ;
		wrreq		: IN STD_LOGIC ;
		empty		: OUT STD_LOGIC ;
		full		: OUT STD_LOGIC ;
		q		: OUT STD_LOGIC_VECTOR (7 DOWNTO 0);
		usedw		: OUT STD_LOGIC_VECTOR (3 DOWNTO 0)
	);
END uart_fifo_altera;


ARCHITECTURE SYN OF uart_fifo_altera IS

	SIGNAL sub_wire0	: STD_LOGIC ;
	SIGNAL sub_wire1	: STD_LOGIC ;
	SIGNAL sub_wire2	: STD_LOGIC_VECTOR (7 DOWNTO 0);
	SIGNAL sub_wire3	: STD_LOGIC_VECTOR (3 DOWNTO 0);



	COMPONENT scfifo
	GENERIC (
		add_ram_output_register		: STRING;
		intended_device_family		: STRING;
		lpm_numwords		: NATURAL;
		lpm_showahead		: STRING;
		lpm_type		: STRING;
		lpm_width		: NATURAL;
		lpm_widthu		: NATURAL;
		overflow_checking		: STRING;
		underflow_checking		: STRING;
		use_eab		: STRING
	);
	PORT (
			aclr	: IN STD_LOGIC ;
			clock	: IN STD_LOGIC ;
			data	: IN STD_LOGIC_VECTOR (7 DOWNTO 0);
			rdreq	: IN STD_LOGIC ;
			sclr	: IN STD_LOGIC ;
			wrreq	: IN STD_LOGIC ;
			empty	: OUT STD_LOGIC ;
			full	: OUT STD_LOGIC ;
			q	: OUT STD_LOGIC_VECTOR (7 DOWNTO 0);
			usedw	: OUT STD_LOGIC_VECTOR (3 DOWNTO 0)
	);
	END COMPONENT;

BEGIN
	empty    <= sub_wire0;
	full    <= sub_wire1;
	q    <= sub_wire2(7 DOWNTO 0);
	usedw    <= sub_wire3(3 DOWNTO 0);

	scfifo_component : scfifo
	GENERIC MAP (
		add_ram_output_register => "OFF",
		intended_device_family => "Cyclone IV E",
		lpm_numwords => 16,
		lpm_showahead => "ON",
		lpm_type => "scfifo",
		lpm_width => 8,
		lpm_widthu => 4,
		overflow_checking => "ON",
		underflow_checking => "ON",
		use_eab => "OFF"
	)
	PORT MAP (
		aclr => aclr,
		clock => clock,
		data => data,
		rdreq => rdreq,
		sclr => sclr,
		wrreq => wrreq,
		empty => sub_wire0,
		full => sub_wire1,
		q => sub_wire2,
		usedw => sub_wire3
	);

END SYN;
