LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.STD_LOGIC_MISC.ALL;
USE IEEE.NUMERIC_STD.ALL;
LIBRARY WORK;
USE WORK.AHBPKG.ALL;
-- synthesis translate_off
USE work.txt_util.all;
-- synthesis translate_on

ENTITY ahb_intercon4 IS
   GENERIC (
      S0_ADDR_MASK  : STD_LOGIC_VECTOR(31 DOWNTO 0) := x"FFFFFFFF";
      S0_ADDR_VALUE : STD_LOGIC_VECTOR(31 DOWNTO 0) := x"00000000";
      S1_ADDR_MASK  : STD_LOGIC_VECTOR(31 DOWNTO 0) := x"FFFFFFFF";
      S1_ADDR_VALUE : STD_LOGIC_VECTOR(31 DOWNTO 0) := x"00000000";
      S2_ADDR_MASK  : STD_LOGIC_VECTOR(31 DOWNTO 0) := x"FFFFFFFF";
      S2_ADDR_VALUE : STD_LOGIC_VECTOR(31 DOWNTO 0) := x"00000000";
      S3_ADDR_MASK  : STD_LOGIC_VECTOR(31 DOWNTO 0) := x"FFFFFFFF";
      S3_ADDR_VALUE : STD_LOGIC_VECTOR(31 DOWNTO 0) := x"00000000"
   );
   PORT (
      clk_i         : IN STD_LOGIC;
      arst_i         : IN STD_LOGIC;

      HMAST_I     : IN  AHB_M2S;
      HMAST_O     : OUT AHB_S2M;

      HSLAV0_I    : IN  AHB_S2M;
      HSLAV0_O    : OUT AHB_M2S;
      HSLAV1_I    : IN  AHB_S2M;
      HSLAV1_O    : OUT AHB_M2S;
      HSLAV2_I    : IN  AHB_S2M;
      HSLAV2_O    : OUT AHB_M2S;
      HSLAV3_I    : IN  AHB_S2M;
      HSLAV3_O    : OUT AHB_M2S
   );
END ENTITY ahb_intercon4;


ARCHITECTURE beh OF ahb_intercon4 IS

  SIGNAL hslave_r       : INTEGER range 0 to 3;
  SIGNAL next_slave_s   : INTEGER range 0 to 3;
  SIGNAL data_stage_r   : std_logic;
  SIGNAL slave_sel_s    : std_logic_vector(3 downto 0);

  type si_type is ARRAY(0 to 3) of AHB_S2M;
  type so_type is ARRAY(0 to 3) of AHB_M2S;

  signal slave_s2m    : si_type;
  signal slave_m2s    : so_type;

  -- Latched request signals

  signal latch_m2s_r  : AHB_M2S;

  signal MASKED_S0_ADDR : std_logic_vector(31 downto 0);
  signal MASKED_S1_ADDR : std_logic_vector(31 downto 0);
  signal MASKED_S2_ADDR : std_logic_vector(31 downto 0);
  signal MASKED_S3_ADDR : std_logic_vector(31 downto 0);

  type state_type is (
    IDLE,
    ADDRESS,
    DATA
  );
  signal state_r      : state_type;
 
BEGIN

  slave_s2m(0)  <= HSLAV0_I;
  slave_s2m(1)  <= HSLAV1_I;
  slave_s2m(2)  <= HSLAV2_I;
  slave_s2m(3)  <= HSLAV3_I;
  HSLAV0_O      <= slave_m2s(0);
  HSLAV1_O      <= slave_m2s(1);
  HSLAV2_O      <= slave_m2s(2);
  HSLAV3_O      <= slave_m2s(3);

  MASKED_S0_ADDR <= latch_m2s_r.HADDR and S0_ADDR_MASK;
  MASKED_S1_ADDR <= latch_m2s_r.HADDR and S1_ADDR_MASK;
  MASKED_S2_ADDR <= latch_m2s_r.HADDR and S2_ADDR_MASK;
  MASKED_S3_ADDR <= latch_m2s_r.HADDR and S3_ADDR_MASK;

  slave_sel_s(0) <= '1' when MASKED_S0_ADDR = S0_ADDR_VALUE else '0';
  slave_sel_s(1) <= '1' when MASKED_S1_ADDR = S1_ADDR_VALUE else '0';
  slave_sel_s(2) <= '1' when MASKED_S2_ADDR = S2_ADDR_VALUE else '0';
  slave_sel_s(3) <= '1' when MASKED_S3_ADDR = S3_ADDR_VALUE else '0';

  slave_out_gen: for i in 0 to 3 generate

    slave_m2s(i).HTRANS     <= latch_m2s_r.HTRANS when (slave_sel_s(i)='1' and state_r=ADDRESS) else C_AHB_TRANS_IDLE;
    slave_m2s(i).HPROT      <= latch_m2s_r.HPROT;
    slave_m2s(i).HSIZE      <= latch_m2s_r.HSIZE;
    slave_m2s(i).HWRITE     <= latch_m2s_r.HWRITE;

    slave_m2s(i).HWDATA     <= HMAST_I.HWDATA;

    slave_m2s(i).HBURST     <= latch_m2s_r.HBURST;
    slave_m2s(i).HMASTLOCK  <= latch_m2s_r.HMASTLOCK;
    slave_m2s(i).HADDR      <= latch_m2s_r.HADDR;
    --slave_m2s(i).HWDATA     <= latch_m2s_r.HWDATA;

  end generate;

  process(state_r, slave_s2m, slave_sel_s)
  begin
    case state_r is
    when IDLE =>
      HMAST_O.HREADY <= '1';
      HMAST_O.HRESP  <= '0';
      HMAST_O.HRDATA <= (others => 'X');
    when ADDRESS =>
      -- At this point we are still waiting for acknowledge from
      -- slave side.
      HMAST_O.HREADY <= '0';
      HMAST_O.HRESP  <= '0';
      HMAST_O.HRDATA <= (others => 'X');
    when DATA =>
      case slave_sel_s is
        when "0001" => HMAST_O.HREADY <= slave_s2m(0).HREADY; HMAST_O.HRESP <= slave_s2m(0).HRESP; HMAST_O.HRDATA  <= slave_s2m(0).HRDATA;
        when "0010" => HMAST_O.HREADY <= slave_s2m(1).HREADY; HMAST_O.HRESP <= slave_s2m(1).HRESP; HMAST_O.HRDATA  <= slave_s2m(1).HRDATA;
        when "0100" => HMAST_O.HREADY <= slave_s2m(2).HREADY; HMAST_O.HRESP <= slave_s2m(2).HRESP; HMAST_O.HRDATA  <= slave_s2m(2).HRDATA;
        when "1000" => HMAST_O.HREADY <= slave_s2m(3).HREADY; HMAST_O.HRESP <= slave_s2m(3).HRESP; HMAST_O.HRDATA  <= slave_s2m(3).HRDATA;
        when others => HMAST_O.HREADY <= '0'; HMAST_O.HRESP <= '1'; HMAST_O.HRDATA <= (others => 'X');
      end case;
    end case;
  end process;

  process(clk_i, arst_i)
    variable ready_v: std_logic;
  begin
    if arst_i='1' then
      state_r   <= IDLE;
      hslave_r  <= 0;
    elsif rising_edge(clk_i) then

      case slave_sel_s is
        when "0001" => ready_v := slave_s2m(0).HREADY;
        when "0010" => ready_v := slave_s2m(1).HREADY;
        when "0100" => ready_v := slave_s2m(2).HREADY;
        when "1000" => ready_v := slave_s2m(3).HREADY;
        when others => ready_v := '0';-- Error
      end case;

      case state_r is

        when IDLE =>
          -- Always latch data
          latch_m2s_r <= HMAST_I;
          if HMAST_I.HTRANS/=C_AHB_TRANS_IDLE then
            state_r <= ADDRESS;
          end if;

        when ADDRESS =>

          if ready_v='1' then
            state_r <= DATA;
          end if;

       when DATA =>

          if ready_v='1' then
            state_r <= IDLE;
          end if;


      end case;
    end if;
  end process;



END beh;

