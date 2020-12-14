library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
library work;
use work.zxinterfacepkg.all;
use work.ahbpkg.all;


entity spi_interface is
  port (
    clk_i                 : in std_logic;
    arst_i                : in std_logic;

    SCKx_i                : in std_logic;
    CSNx_i                : in std_logic;

    MOSI_i                : in std_logic;
    MISO_o                : out std_logic;

    -- AHB bus master
    ahb_m2s_o             : out AHB_M2S;
    ahb_s2m_i             : in AHB_S2M
  );

end entity spi_interface;

architecture beh of spi_interface is
  signal txdat_s      : std_logic_vector(7 downto 0);
  signal dat_s        : std_logic_vector(7 downto 0);
  signal first_dat_s  : std_logic;
  signal accept_s     : std_logic;
  signal wordindex_r  : unsigned(1 downto 0);

  signal vid_addr_r   : unsigned(12 downto 0);
  signal flags_r      : std_logic_vector(15 downto 0);
  --signal capmem_adr_r : unsigned(CAPTURE_MEMWIDTH_BITS-1 downto 0);

  constant NUMREGS32  : natural := 8;

  subtype reg32_type is std_logic_vector(31 downto 0);
  type regs32_type is array(0 to NUMREGS32-1) of reg32_type;

  signal regs32_r       : regs32_type := (others => (others => '0'));
  signal tempreg_r      : std_logic_vector(23 downto 0);
  signal current_reg_r  : natural range 0 to NUMREGS32-1;
  signal pc_latch_r     : std_logic_vector(7 downto 0);

  type state_type is (
    IDLE,
    RDWR,
    WRINC, -- This state waits for 1st byte of data
    RDWR_2,
    RDWR_3,
    RDWR_4,
    RDBLOCK,
    INDEXED_3,
    INDEXED_12,
    INDEXED_13,
    INDEXED_24,
    INDEXED_8,
    INDEXED_16
  );



  signal state_r      : state_type;
  signal txload_s     : std_logic;
  signal txden_s      : std_logic;
  signal rx_rd_s      : std_logic;
  signal tx_full_s    : std_logic;
  signal rx_empty_s   : std_logic;

  signal dat_valid_s  : std_logic;

  signal csn_s        : std_logic;

  signal ahb_address_r: std_logic_vector(24 downto 0);
  signal ahb_write_r  : std_logic;
  signal ahb_inc_r    : std_logic;

  signal blocksize_active_r : std_logic;
  signal blocksize_r  : unsigned(7 downto 0);

  function to_01(a: in std_logic_vector) return std_logic_vector is
    variable l: std_logic_vector(a'range);
  begin
    l:=a;
    -- synthesis translate_off
    l1: for i in a'low to a'high loop
      if a(i)='H' or a(i)='1' then
        l(i):='1';
      else
        l(i):='0';
      end if;
    end loop;
    -- synthesis translate_on
    return l;
  end function;

begin

  -- FORCIBLY read all from RX fifo. We don't stall (yet)
  rx_rd_s     <= not rx_empty_s;
  dat_valid_s <= not rx_empty_s;


  spi_inst: entity work.spi_slave_fifo
  port map (
    clk_i         => clk_i,
    arst_i        => arst_i,

    SCK_i         => SCKx_i,
    CSN_i         => CSNx_i,
    MOSI_i        => MOSI_i,
    MISO_o        => MISO_o,

    -- TX fifo
    tx_we_i       => txload_s,
    tx_wdata_i    => txdat_s,
    tx_full_o     => tx_full_s,
    tx_accept_o   => accept_s,
    -- RX fifo
    rx_rd_i                 => rx_rd_s,
    rx_rdata_o(8)           => first_dat_s,
    rx_rdata_o(7 downto 0)  => dat_s,
    rx_empty_o    => rx_empty_s,
    csn_o         => csn_s
  );


  process(clk_i, arst_i)
  begin
    if arst_i='1' or csn_s='1' then

      ahb_m2s_o.HTRANS  <= C_AHB_TRANS_IDLE;
      state_r           <= IDLE;
      ahb_address_r     <= (others => 'X');
      ahb_write_r       <= 'X';
      ahb_inc_r         <= 'X';
      blocksize_active_r<= '0';
      blocksize_r       <= (others => 'X');

    elsif rising_edge(clk_i) then

      if rx_empty_s='0' and first_dat_s='1' then
        blocksize_active_r <= '0';
        case dat_s is
          when x"DA" => -- Read Test UART status
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX011100"); ahb_write_r <= '0'; ahb_inc_r <= '0';
            state_r         <= RDWR;
          when x"D8" => -- Write UART data
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX011101"); ahb_write_r <= '1'; ahb_inc_r <= '0';
            state_r         <= WRINC;
          when x"D9" => -- Read UART data
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX011101"); ahb_write_r <= '0'; ahb_inc_r <= '0';
            state_r         <= RDBLOCK;
          when x"D7" => -- Read BIT
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX001100"); ahb_write_r <= '0'; ahb_inc_r <= '1';
            state_r         <= RDWR;
          when x"D6" => -- Write BIT
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX001000"); ahb_write_r <= '1'; ahb_inc_r <= '1';
            state_r         <= WRINC;
          when x"DE" => -- Read status
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX000100"); ahb_write_r <= '0'; ahb_inc_r <= '0';
            state_r         <= RDWR;
          when x"40" => -- Read last PC
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX010000"); ahb_write_r <= '0'; ahb_inc_r <= '1';
            state_r         <= RDWR;
          when x"E3" => -- Write Resource FIFO contents
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX010100"); ahb_write_r <= '1'; ahb_inc_r <= '0';
            state_r         <= WRINC;
          when x"E4" => -- Write TAP FIFO contents
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX011000"); ahb_write_r <= '1'; ahb_inc_r <= '0';
            state_r         <= WRINC;
          when x"E6" => -- Write TAP command FIFO contents
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX011001"); ahb_write_r <= '1'; ahb_inc_r <= '0';
            state_r         <= WRINC;
          when x"E5" => -- Get TAP FIFO usage
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX011010"); ahb_write_r <= '0'; ahb_inc_r <= '1';
            state_r         <= RDWR; -- This might read UART status, but it's OK
          when x"EB" => -- Set mem/rom
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX010010"); ahb_write_r <= '1'; ahb_inc_r <= '0';
            state_r         <= WRINC;
          when x"EC" => -- Set flags
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX000101"); ahb_write_r <= '1'; ahb_inc_r <= '1';
            state_r         <= WRINC;
          when x"ED" => -- Get regs32
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX1XXX00"); ahb_write_r <= '0'; ahb_inc_r <= '1';
            state_r         <= INDEXED_3;
          when x"EE" => -- Set regs32
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX1XXX00"); ahb_write_r <= '1'; ahb_inc_r <= '1';
            state_r         <= INDEXED_3;
          when x"FB" => -- Read FIFO command data
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX010101"); ahb_write_r <= '0'; ahb_inc_r <= '0';
            state_r         <= RDBLOCK;
          when x"9E" | x"9F" => -- Read ID
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXX000000"); ahb_write_r <= '0'; ahb_inc_r <= '1';
            state_r         <= RDWR;
          when x"DF" => -- Read video memory
            ahb_address_r   <= to_01("00XXXXXXXXX0XXXXXXXXXXXXX"); ahb_write_r <= '0'; ahb_inc_r <= '1';
            state_r         <= INDEXED_13;
          when x"50" => -- Read external RAM
            ahb_address_r   <= to_01("1XXXXXXXXXXXXXXXXXXXXXXXX"); ahb_write_r <= '0'; ahb_inc_r <= '1';
            state_r         <= INDEXED_24;
          when x"51" => -- Write external RAM
            ahb_address_r   <= to_01("1XXXXXXXXXXXXXXXXXXXXXXXX"); ahb_write_r <= '1'; ahb_inc_r <= '1';
            state_r         <= INDEXED_24;
          when x"60" => -- USB read
            ahb_address_r   <= to_01("00XXXXXXXXX11XXXXXXXXXXXX"); ahb_write_r <= '0'; ahb_inc_r <= '1';
            state_r         <= INDEXED_12;
          when x"61" => -- USB write
            ahb_address_r   <= to_01("00XXXXXXXXX11XXXXXXXXXXXX"); ahb_write_r <= '1'; ahb_inc_r <= '1';
            state_r         <= INDEXED_12;
          when x"62" => -- Capture read
            ahb_address_r   <= to_01("01XXXXXXXXXXXXXXXXXXXXXXX"); ahb_write_r <= '0'; ahb_inc_r <= '1';
            state_r         <= INDEXED_16;
          when x"63" => -- Capture write
            ahb_address_r   <= to_01("01XXXXXXXXXXXXXXXXXXXXXXX"); ahb_write_r <= '1'; ahb_inc_r <= '1';
            state_r         <= INDEXED_16;

          when x"64" => -- System controller generic read
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXXXXXXXX"); ahb_write_r <= '0'; ahb_inc_r <= '1';
            state_r         <= INDEXED_12;

          when x"65" => -- System controller generic write
            ahb_address_r   <= to_01("00XXXXXXXXX10XXXXXXXXXXXX"); ahb_write_r <= '1'; ahb_inc_r <= '1';
            state_r         <= INDEXED_12;

          when others =>
            -- Default: read status
            ahb_address_r   <= to_01("0XXXXXXXXXX10XXXXXX000100"); ahb_write_r <= '0'; ahb_inc_r <= '0';
            state_r         <= RDWR;
            null;
        end case;
      else
      end if;

      -- Command processing
      case state_r is
        when IDLE =>
        when INDEXED_12 =>
          if dat_valid_s='1' then
            ahb_address_r(11 downto 8) <= dat_s(3 downto 0);
            state_r <= INDEXED_8;
          end if;

        when INDEXED_13 =>
          if dat_valid_s='1' then
            ahb_address_r(12 downto 8) <= dat_s(4 downto 0);
            state_r <= INDEXED_8;
          end if;

        when INDEXED_24 =>
          if dat_valid_s='1' then
            ahb_address_r(23 downto 16) <= dat_s;
            state_r <= INDEXED_16;
          end if;

        when INDEXED_3 =>
          if dat_valid_s='1' then
            ahb_address_r(2 downto 0) <= dat_s(2 downto 0);
            if ahb_write_r='1' then
              state_r <= WRINC;
            else
              state_r <= RDWR;
            end if;
          end if;

        when INDEXED_16 =>
          if dat_valid_s='1' then
            ahb_address_r(15 downto 8) <= dat_s;
            state_r <= INDEXED_8;
          end if;

        -- OK !!!!
        when INDEXED_8 =>
          if dat_valid_s='1' then
            ahb_address_r(7 downto 0) <= dat_s;
            if ahb_write_r='1' then
              state_r <= WRINC;
            else
              state_r <= RDWR;
            end if;
          end if;

        when RDBLOCK =>
          blocksize_active_r <= '1';
          if dat_valid_s='1' then
            blocksize_r  <= unsigned(dat_s);
            state_r <= RDWR;
          end if;

        when RDWR =>
          ahb_m2s_o.HWRITE  <= ahb_write_r;
          ahb_m2s_o.HADDR(ahb_address_r'range) <= to_01(ahb_address_r);
          -- For BLOCK reads, we only proceed if we have room
          if blocksize_active_r='0' or blocksize_r/=0 then
            state_r <= RDWR_2;
            ahb_m2s_o.HTRANS  <= C_AHB_TRANS_SEQ;
          end if;

        when WRINC =>
          ahb_m2s_o.HWRITE  <= ahb_write_r;
          ahb_m2s_o.HADDR(ahb_address_r'range) <= to_01(ahb_address_r);
          if dat_valid_s='1' then
            ahb_m2s_o.HTRANS  <= C_AHB_TRANS_SEQ;
            ahb_m2s_o.HWDATA(7 downto 0)  <= dat_s;
            state_r <= RDWR_2;
          end if;

        when RDWR_2 =>
          if ahb_s2m_i.HREADY='1' then
            ahb_m2s_o.HTRANS  <= C_AHB_TRANS_IDLE;
            ahb_m2s_o.HWRITE  <= 'X';
            ahb_m2s_o.HADDR   <= (others =>'X');
            state_r <= RDWR_3;
          end if;

        when RDWR_3 =>
          if ahb_s2m_i.HREADY='1' then
            txload_s  <= '1';
            txdat_s   <= ahb_s2m_i.HRDATA(7 downto 0);
            state_r <= RDWR_4;
          end if;

        when RDWR_4 =>
          if dat_valid_s='1' then
            if ahb_inc_r='1' then
              ahb_address_r <= std_logic_vector( unsigned(ahb_address_r) + 1 );
            end if;
            if blocksize_active_r='1' then
              blocksize_r <= blocksize_r - 1;
            end if;
            ahb_m2s_o.HWDATA(7 downto 0)  <= dat_s;
            state_r <= RDWR;
          end if;

        when others =>
          report "TBD " & state_type'image(state_r) severity failure;
      end case;




    end if;
  end process;





end beh;
