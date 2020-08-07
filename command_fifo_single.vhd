library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

entity command_fifo_single is
  port (
    clk_i     : in std_logic;
    arst_i    : in std_logic;
    reset_i   : in std_logic;
    wr_i      : in std_logic;
    rd_i      : in std_logic;
    wD_i      : in std_logic_vector(7 downto 0);
    rQ_o      : out std_logic_vector(7 downto 0);
    full_o    : out std_logic;
    empty_o   : out std_logic
  );
end entity command_fifo_single;


architecture beh of command_fifo_single is

  component smallfifo IS
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
		usedw		: OUT STD_LOGIC_VECTOR (1 DOWNTO 0)
	);
  END component smallfifo;

begin

  fifo_inst: smallfifo
	PORT MAP
	(
		aclr		=> arst_i,
		clock		=> clk_i,
		data		=> wD_i,
		rdreq		=> rd_i,
		sclr		=> reset_i,
		wrreq		=> wr_i,
		empty		=> empty_o,
		full		=> full_o,
		q		    => rQ_o,
		usedw		=> open
	);

end beh;
