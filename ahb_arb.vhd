LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.STD_LOGIC_MISC.ALL;
USE IEEE.NUMERIC_STD.ALL;
LIBRARY WORK;
USE WORK.AHBPKG.ALL;

ENTITY ahb_arb IS
   GENERIC (
      M0_ADDR_AND_MASK: STD_LOGIC_VECTOR(31 DOWNTO 0) := x"FFFFFFFF";
      M0_ADDR_OR_MASK:  STD_LOGIC_VECTOR(31 DOWNTO 0) := x"00000000";
      M1_ADDR_AND_MASK: STD_LOGIC_VECTOR(31 DOWNTO 0) := x"FFFFFFFF";
      M1_ADDR_OR_MASK:  STD_LOGIC_VECTOR(31 DOWNTO 0) := x"00000000";
      M2_ADDR_AND_MASK: STD_LOGIC_VECTOR(31 DOWNTO 0) := x"FFFFFFFF";
      M2_ADDR_OR_MASK:  STD_LOGIC_VECTOR(31 DOWNTO 0) := x"00000000";
      M3_ADDR_AND_MASK: STD_LOGIC_VECTOR(31 DOWNTO 0) := x"FFFFFFFF";
      M3_ADDR_OR_MASK:  STD_LOGIC_VECTOR(31 DOWNTO 0) := x"00000000"
   );
   PORT (
      CLK         : IN STD_LOGIC;
      RST         : IN STD_LOGIC;

      HMAST0_I     : IN  AHB_M2S;
      HMAST0_O     : OUT AHB_S2M;

      HMAST1_I     : IN  AHB_M2S;
      HMAST1_O     : OUT AHB_S2M;

      HMAST2_I     : IN  AHB_M2S;
      HMAST2_O     : OUT AHB_S2M;

      HMAST3_I     : IN  AHB_M2S;
      HMAST3_O     : OUT AHB_S2M;

      HSLAV_I     : IN  AHB_S2M;
      HSLAV_O     : OUT AHB_M2S
   );
END ENTITY ahb_arb;


ARCHITECTURE beh OF ahb_arb IS

   SIGNAL hmaster_r     : INTEGER range 0 to 3;
   SIGNAL hmaster_d_r     : INTEGER range 0 to 3;

   SIGNAL hnextmaster_s : integer range 0 to 3;

   SIGNAL hready_r      : std_logic;	-- needed for two-cycle error response
   SIGNAL hlock_r       : std_logic;
   SIGNAL hmastlock_r   : std_logic;
   SIGNAL htrans_r      : std_logic_vector(1 downto 0);    -- transfer type

   TYPE HMAST_I_Typ is  ARRAY (0 to 3) OF AHB_M2S;
   SIGNAL HMAST_I:     HMAST_I_Typ;
   TYPE HMAST_O_Typ is  ARRAY (0 to 3) OF AHB_S2M;
   SIGNAL HMAST_O:     HMAST_O_Typ;

   SIGNAL data_stage_r  : std_logic;

   TYPE state_TYP IS (IDLE, ADDRESS_PHASE, DATA_PHASE );

   TYPE HMAST_ADDR_Typ is  ARRAY (0 to 3) OF STD_LOGIC_VECTOR(31 DOWNTO 0);

   SIGNAL hmast_addr: HMAST_ADDR_Typ;

   SIGNAL state: state_TYP;

BEGIN

   HMAST_I(0) <= HMAST0_I;
   HMAST_I(1) <= HMAST1_I;
   HMAST_I(2) <= HMAST2_I;
   HMAST_I(3) <= HMAST3_I;
   HMAST0_O <= HMAST_O(0);
   HMAST1_O <= HMAST_O(1);
   HMAST2_O <= HMAST_O(2);
   HMAST3_O <= HMAST_O(3);

   hmast_addr(0) <= (HMAST_I(0).HADDR AND M0_ADDR_AND_MASK) OR M0_ADDR_OR_MASK;
   hmast_addr(1) <= (HMAST_I(1).HADDR AND M1_ADDR_AND_MASK) OR M1_ADDR_OR_MASK;
   hmast_addr(2) <= (HMAST_I(2).HADDR AND M2_ADDR_AND_MASK) OR M2_ADDR_OR_MASK;
   hmast_addr(3) <= (HMAST_I(3).HADDR AND M3_ADDR_AND_MASK) OR M3_ADDR_OR_MASK;
   -- Muxers

   HSLAV_O.HADDR       <= hmast_addr(hmaster_r);--HMAST_I(hmaster_r).HADDR;
   HSLAV_O.HBURST      <= HMAST_I(hmaster_r).HBURST;
   HSLAV_O.HMASTLOCK   <= hmastlock_r; --HMAST_I(hmaster_r).HMASTLOCK;  -- TBD
   HSLAV_O.HPROT       <= HMAST_I(hmaster_r).HPROT;
   HSLAV_O.HSIZE       <= HMAST_I(hmaster_r).HSIZE;
   HSLAV_O.HWDATA      <= HMAST_I(hmaster_d_r).HWDATA;
   HSLAV_O.HWRITE      <= HMAST_I(hmaster_r).HWRITE;
   HSLAV_O.HTRANS      <= HMAST_I(hmaster_r).HTRANS;

   gen1: FOR N IN 0 TO 3 GENERATE
      HMAST_O(N).HREADY     <= HSLAV_I.HREADY WHEN hmaster_r=N ELSE '0';
      HMAST_O(N).HRDATA     <= HSLAV_I.HRDATA;
      HMAST_O(N).HRESP      <= HSLAV_I.HRESP  WHEN hmaster_r=N ELSE '0';
   END GENERATE;

   PROCESS(HMAST_I(0).HTRANS,
           HMAST_I(1).HTRANS,
           HMAST_I(2).HTRANS,
           HMAST_I(3).HTRANS, hmaster_r)
   BEGIN
      CASE hmaster_r IS
         WHEN 0 =>
            IF HMAST_I(1).HTRANS/=C_AHB_TRANS_IDLE THEN
               hnextmaster_s<=1;
            ELSIF HMAST_I(2).HTRANS/=C_AHB_TRANS_IDLE THEN
               hnextmaster_s<=2;
            ELSIF HMAST_I(3).HTRANS/=C_AHB_TRANS_IDLE THEN
               hnextmaster_s<=3;
            ELSE
               hnextmaster_s<=0;
            END IF;
         WHEN 1 =>
            IF HMAST_I(2).HTRANS/=C_AHB_TRANS_IDLE THEN
               hnextmaster_s<=2;
            ELSIF HMAST_I(3).HTRANS/=C_AHB_TRANS_IDLE THEN
               hnextmaster_s<=3;
            ELSIF HMAST_I(0).HTRANS/=C_AHB_TRANS_IDLE THEN
               hnextmaster_s<=0;
            ELSE
               hnextmaster_s<=1;
            END IF;
         WHEN 2 =>
            IF HMAST_I(3).HTRANS/=C_AHB_TRANS_IDLE THEN
               hnextmaster_s<=3;
            ELSIF HMAST_I(0).HTRANS/=C_AHB_TRANS_IDLE THEN
               hnextmaster_s<=0;
            ELSIF HMAST_I(1).HTRANS/=C_AHB_TRANS_IDLE THEN
               hnextmaster_s<=1;
            ELSE
               hnextmaster_s<=2;
            END IF;
         WHEN 3 =>
            IF HMAST_I(0).HTRANS/=C_AHB_TRANS_IDLE THEN
               hnextmaster_s<=0;
            ELSIF HMAST_I(1).HTRANS/=C_AHB_TRANS_IDLE THEN
               hnextmaster_s<=1;
            ELSIF HMAST_I(2).HTRANS/=C_AHB_TRANS_IDLE THEN
               hnextmaster_s<=2;
            ELSE
               hnextmaster_s<=3;
            END IF;
         WHEN OTHERS =>
      END CASE;
   END PROCESS;


   PROCESS(CLK)
      VARIABLE next_master_v: INTEGER;
   BEGIN
      IF RISING_EDGE(CLK) THEN
         IF RST='1' THEN
            htrans_r    <= "00";
            hmaster_r   <= 0;
            hmaster_d_r <= 0;
            hmastlock_r <= '0';
         ELSE

            next_master_v := hmaster_r;

            IF HMAST_I(hmaster_r).HMASTLOCK='0' THEN -- Don't switch if master locked bus
               IF HMAST_I(hmaster_r).HTRANS = C_AHB_TRANS_IDLE OR
                  ( HMAST_I(hmaster_r).HTRANS = C_AHB_TRANS_NONSEQ AND
                    HMAST_I(hmaster_r).HBURST = C_AHB_BURST_SINGLE
                  ) THEN
                  next_master_v := hnextmaster_s;
               END IF;
            END IF;

            hlock_r <= HMAST_I(next_master_v).HMASTLOCK;

            IF HSLAV_I.HREADY = '1' THEN
               hmaster_r   <= next_master_v;
               hmaster_d_r <= hmaster_r;
               htrans_r    <= HMAST_I(hmaster_r).HTRANS;
               hmastlock_r <= hlock_r;
            END IF;

         END IF;

      END IF;
   END PROCESS;

END beh;

