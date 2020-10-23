library IEEE;   
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
LIBRARY altera_mf;
USE altera_mf.all;

entity resource_fifo is
  port (
    -- Write port
    wclk_i    : in std_logic;
    wdata_i   : in std_logic_vector(7 downto 0);
    wen_i     : in std_logic;
    wusedw_o  : out std_logic_vector(9 downto 0);
    wfull_o   : out std_logic;
    wempty_o  : out std_logic;

    wqfull_o  : out std_logic;
    whfull_o  : out std_logic;
    wqqqfull_o: out std_logic;

    -- Read port
    rclk_i    : in std_logic;
    rdata_o   : out std_logic_vector(7 downto 0);
    ren_i     : in std_logic;
    rempty_o  : out std_logic;
    rfull_o   : out std_logic;
    rusedw_o  : out std_logic_vector(9 downto 0);

    -- Others
    aclr_i    : in std_logic
  );
end entity resource_fifo;

architecture beh of resource_fifo is

	COMPONENT dcfifo
	GENERIC (
		intended_device_family		: STRING;
		lpm_numwords		: NATURAL;
		lpm_showahead		: STRING;
		lpm_type		: STRING;
		lpm_width		: NATURAL;
		lpm_widthu		: NATURAL;
		overflow_checking		: STRING;
		rdsync_delaypipe		: NATURAL;
		underflow_checking		: STRING;
		use_eab		: STRING;
		wrsync_delaypipe		: NATURAL;
		read_aclr_synch		: STRING;
		write_aclr_synch		: STRING
	);
	PORT (
			aclr	: IN STD_LOGIC ;
			data	: IN STD_LOGIC_VECTOR (7 DOWNTO 0);
			rdclk	: IN STD_LOGIC ;
			rdreq	: IN STD_LOGIC ;
			wrclk	: IN STD_LOGIC ;
			wrreq	: IN STD_LOGIC ;
			q	: OUT STD_LOGIC_VECTOR (7 DOWNTO 0);
			rdempty	: OUT STD_LOGIC ;
			rdfull	: OUT STD_LOGIC ;
			rdusedw	: OUT STD_LOGIC_VECTOR (9 DOWNTO 0);
			wrempty	: OUT STD_LOGIC ;
			wrfull	: OUT STD_LOGIC ;
			wrusedw	: OUT STD_LOGIC_VECTOR (9 DOWNTO 0)
	);
	END COMPONENT;

  signal rusedw_s:  std_logic_vector(9 downto 0);
  signal wusedw_s:  std_logic_vector(9 downto 0);

begin

	dcfifo_component : dcfifo
	GENERIC MAP (
		intended_device_family => "Cyclone IV E",
		lpm_numwords => 1024,
		lpm_showahead => "ON",
		lpm_type => "dcfifo",
		lpm_width => 8,
		lpm_widthu => 10,
		overflow_checking => "ON",
		rdsync_delaypipe => 5,
		underflow_checking => "ON",
		use_eab => "ON",
		wrsync_delaypipe => 5,
		read_aclr_synch => "ON",
    write_aclr_synch => "ON"
	)
	PORT MAP (
    aclr    => aclr_i,
		data    => wdata_i,
		rdclk   => rclk_i,
		rdreq   => ren_i,
		wrclk   => wclk_i,
		wrreq   => wen_i,
		q       => rdata_o,
		rdempty => rempty_o,
		rdfull  => rfull_o,
		rdusedw => rusedw_s,
		wrempty => wempty_o,
		wrfull  => wfull_o,
		wrusedw => wusedw_s
	);

  wqfull_o    <= '1' when wusedw_s(8)='1' else '0';
  whfull_o    <= '1' when wusedw_s(9)='1' else '0';
  wqqqfull_o  <= '1' when wusedw_s(9 downto 8)="11" else '0';

  wusedw_o  <= wusedw_s;
  rusedw_o  <= rusedw_s;

end beh;

