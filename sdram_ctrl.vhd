library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.std_logic_unsigned.all;

use work.wishbonepkg.all;

entity sdram_ctrl is
  port (
    wb_clk_i: in std_logic;
	 	wb_rst_i: in std_logic;

    wb_dat_o: out std_logic_vector(7 downto 0);
    wb_dat_i: in std_logic_vector(7 downto 0);
    wb_adr_i: in std_logic_vector(23 downto 0);
    wb_we_i:  in std_logic;
    wb_cyc_i: in std_logic;
    wb_stb_i: in std_logic;
    --wb_sel_i: in std_logic_vector(3 downto 0);
    wb_ack_o: out std_logic;
    wb_stall_o: out std_logic;

    -- extra clocking
    clk_off_3ns: in std_logic;

    -- SDRAM signals
     DRAM_ADDR   : OUT   STD_LOGIC_VECTOR (12 downto 0);
     DRAM_BA      : OUT   STD_LOGIC_VECTOR (1 downto 0);
     DRAM_CAS_N   : OUT   STD_LOGIC;
     DRAM_CKE      : OUT   STD_LOGIC;
     DRAM_CLK      : OUT   STD_LOGIC;
     DRAM_CS_N   : OUT   STD_LOGIC;
     DRAM_DQ      : INOUT STD_LOGIC_VECTOR(7 downto 0);
     DRAM_DQM      : OUT   STD_LOGIC;--_VECTOR(1 downto 0);
     DRAM_RAS_N   : OUT   STD_LOGIC;
     DRAM_WE_N    : OUT   STD_LOGIC
  
  );

end entity sdram_ctrl;

architecture behave of sdram_ctrl is

  signal sdr_address:    STD_LOGIC_VECTOR (23 downto 0);
  signal sdr_req_read      :     STD_LOGIC;
  signal sdr_req_write   :      STD_LOGIC;
  signal sdr_data_out      :      STD_LOGIC_VECTOR (7 downto 0);
  signal sdr_data_out_valid :      STD_LOGIC;
  signal sdr_data_in      : STD_LOGIC_VECTOR( 7 downto 0);

  --signal sdr_data_mask: std_logic_vector(3 downto 0);

  signal pending: std_logic;

begin

  ctrl_inst: entity work.sdram_controller
    generic map (
      HIGH_BIT => 23,
      ADDRESS_BITS => 13
   )
   port map (
    clock_100   => wb_clk_i,
    clock_100_delayed_3ns => clk_off_3ns,
    rst => wb_rst_i,

    DRAM_ADDR   => DRAM_ADDR,
    DRAM_BA     => DRAM_BA,
    DRAM_CAS_N  => DRAM_CAS_N,
    DRAM_CKE    => DRAM_CKE,
    DRAM_CLK    => DRAM_CLK,
    DRAM_CS_N   => DRAM_CS_N,
    DRAM_DQ     => DRAM_DQ,
    DRAM_DQM    => DRAM_DQM,
    DRAM_RAS_N  => DRAM_RAS_N,
    DRAM_WE_N   => DRAM_WE_N,

    pending     => pending,
    address     => sdr_address,
    req_read    => sdr_req_read,
    req_write   => sdr_req_write,
    data_out    => sdr_data_out,
    data_out_valid => sdr_data_out_valid,
    data_in      => sdr_data_in,
    data_mask   => '1'--sdr_data_mask
   );


  sdr_address(23 downto 0) <= wb_adr_i(23 downto 0);

  sdr_req_read<='1' when wb_cyc_i='1' and wb_stb_i='1' and wb_we_i='0' else '0';
  sdr_req_write<='1' when wb_cyc_i='1' and wb_stb_i='1' and wb_we_i='1' else '0';

  sdr_data_in <= wb_dat_i;
  --sdr_data_mask(0) <= wb_sel_i;

  wb_stall_o <= '1' when pending='1' else '0';

  process(wb_clk_i)
  begin
    if rising_edge(wb_clk_i) then

     wb_ack_o <= sdr_data_out_valid;
     wb_dat_o <= sdr_data_out;
    end if;
  end process;

end behave;



