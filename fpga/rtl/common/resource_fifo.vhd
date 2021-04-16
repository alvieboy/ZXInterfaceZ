library IEEE;   
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
LIBRARY altera_mf;
USE altera_mf.all;

entity resource_fifo is
  port (
    -- Write port
    clk_i     : in std_logic;
    wdata_i   : in std_logic_vector(7 downto 0);
    wen_i     : in std_logic;
    usedw_o  : out std_logic_vector(9 downto 0);
    full_o   : out std_logic;
    empty_o  : out std_logic;

    qfull_o  : out std_logic;
    hfull_o  : out std_logic;
    qqqfull_o: out std_logic;

    -- Read port
    rdata_o   : out std_logic_vector(7 downto 0);
    ren_i     : in std_logic;

    -- Others
    aclr_i    : in std_logic
  );
end entity resource_fifo;

architecture beh of resource_fifo is

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
			usedw	: OUT STD_LOGIC_VECTOR (9 DOWNTO 0)
	);
	END COMPONENT;

  signal usedw_s:  std_logic_vector(9 downto 0);

begin

 	scfifo_component : scfifo
	GENERIC MAP (
		intended_device_family => "Cyclone IV E",
		lpm_numwords => 1024,
		lpm_showahead => "ON",
		lpm_type => "scfifo",
		lpm_width => 8,
		lpm_widthu => 10,
		overflow_checking => "ON",
		--rdsync_delaypipe => 5,
		underflow_checking => "ON",
		use_eab => "ON",
		add_ram_output_register => "OFF"
		--wrsync_delaypipe => 5,
		--read_aclr_synch => "ON",
    --write_aclr_synch => "ON"
	)
	PORT MAP (
    aclr    => aclr_i,
    sclr    => '0',
		data    => wdata_i,
		clock   => clk_i,
		rdreq   => ren_i,
		wrreq   => wen_i,
		q       => rdata_o,
		empty   => empty_o,
		full    => full_o,
		usedw   => usedw_s
	);

  qfull_o    <= '1' when usedw_s(8)='1' else '0';
  hfull_o    <= '1' when usedw_s(9)='1' else '0';
  qqqfull_o  <= '1' when usedw_s(9 downto 8)="11" else '0';

  usedw_o  <= usedw_s;

end beh;

