library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
library work;
use work.zxinterfacepkg.all;


entity spi_interface is
  port (
    SCK_i         : in std_logic;
    CSN_i         : in std_logic;
    arst_i        : in std_logic;

    --D_io          : inout std_logic_vector(3 downto 0);
    MOSI_i        : in std_logic;
    MISO_o        : out std_logic;

    fifo_empty_i  : in std_logic;
    fifo_rd_o     : out std_logic;
    fifo_data_i   : in std_logic_vector(31 downto 0);

    vidmem_en_o   : out std_logic;
    vidmem_adr_o  : out std_logic_vector(12 downto 0);
    vidmem_data_i : in std_logic_vector(7 downto 0);

    capmem_en_o   : out std_logic;
    capmem_adr_o  : out std_logic_vector(CAPTURE_MEMWIDTH_BITS-1 downto 0);
    capmem_data_i : in std_logic_vector(35 downto 0);

    -- Synchronous to SCK_i
    capture_run_o : out std_logic;
    capture_clr_o : out std_logic;
    capture_cmp_o : out std_logic;
    capture_len_i : in std_logic_vector(CAPTURE_MEMWIDTH_BITS-1 downto 0);
    capture_trig_i  : in std_logic;
    capture_trig_mask_o : out std_logic_vector(31 downto 0);
    capture_trig_val_o  : out std_logic_vector(31 downto 0);

    rom_en_o      : out std_logic;
    rom_we_o      : out std_logic;
    rom_di_o      : out std_logic_vector(7 downto 0);
    rom_addr_o    : out std_logic_vector(13 downto 0);


    rstfifo_o     : out std_logic;
    rstspect_o    : out std_logic;
    intenable_o   : out std_logic;
    capsyncen_o   : out std_logic;
    frameend_o    : out std_logic;
    forceromcs_o  : out std_logic
  );

end entity spi_interface;

architecture beh of spi_interface is

  signal txdat_s      : std_logic_vector(7 downto 0);
  signal txload_s     : std_logic;
  signal txready_s    : std_logic;
  signal txden_s      : std_logic;
  
  signal dat_s        : std_logic_vector(7 downto 0);
  signal dat_valid_s  : std_logic;
  signal wordindex_r  : unsigned(1 downto 0);
  signal wordindex2_r : unsigned(2 downto 0);

  signal vid_addr_r   : unsigned(12 downto 0);
  signal flags_r      : std_logic_vector(7 downto 0);
  signal capmem_adr_r : unsigned(CAPTURE_MEMWIDTH_BITS-1 downto 0);

  constant NUMREGS32  : natural := 4;

  subtype reg32_type is std_logic_vector(31 downto 0);
  type regs32_type is array(0 to NUMREGS32-1) of reg32_type;

  signal regs32_r     : regs32_type;
  signal current_reg_r: natural range 0 to NUMREGS32-1;

  type state_type is (
    IDLE,
    UNKNOWN,
    READID,
    READSTATUS,
    READDATA,
    RDVIDMEM1,
    RDVIDMEM2,
    RDVIDMEM,
    READCAPTURE,
    READCAPSTATUS,
    SETFLAGS,
    SETREG32_INDEX,
    SETREG32_1,
    SETREG32_2,
    SETREG32_3,
    SETREG32_4,
    WRITEROM1,
    WRITEROM2,
    WRITEROM
  );

  signal state_r      : state_type;

  signal frame_end_r  : std_logic;
  signal rom_addr_r    : unsigned(13 downto 0);


  signal capture_len_sync_s   : std_logic_vector(CAPTURE_MEMWIDTH_BITS-1 downto 0);
  signal capture_trig_sync_s  : std_logic;

  signal wreg_en_r        : std_logic;

begin

  len_sync: entity work.syncv generic map (RESET => '0', WIDTH => CAPTURE_MEMWIDTH_BITS )
      port map ( clk_i => SCK_i, arst_i => arst_i, din_i => capture_len_i, dout_o => capture_len_sync_s );
  trig_sync: entity work.sync generic map (RESET => '0')
      port map ( clk_i => SCK_i, arst_i => arst_i, din_i => capture_trig_i, dout_o => capture_trig_sync_s );


  vidmem_adr_o <= std_logic_vector(vid_addr_r);
  capmem_adr_o <= std_logic_vector(capmem_adr_r);
  capture_trig_val_o  <= regs32_r(1);
  capture_trig_mask_o <= regs32_r(0);
  frameend_o <= frame_end_r;

  spi_inst: entity work.qspi_slave
  port map (
    SCK_i         => SCK_i,
    CSN_i         => CSN_i,
    --D_io          => D_io,
    MOSI_i        => MOSI_i,
    MISO_o        => MISO_o,
    txdat_i       => txdat_s,
    txload_i      => txload_s,
    txready_o     => txready_s,
    txden_i       => txden_s,
    qen_i         => '0',

    dat_o         => dat_s,
    dat_valid_o   => dat_valid_s
  );

  process(SCK_i, arst_i)
  begin
    if arst_i='1' then
      flags_r <= "00000000";
    elsif rising_edge(SCK_i) then
      if state_r=SETFLAGS then
        if dat_valid_s='1' then
          flags_r <= dat_s;
        end if;
      end if;
    end if;
  end process;

  rstfifo_o     <= flags_r(0);
  rstspect_o    <= flags_r(1);
  capture_clr_o <= flags_r(2);
  capture_run_o <= flags_r(3);
  capture_cmp_o <= flags_r(4); -- Compress
  intenable_o   <= flags_r(5); -- Interrupt enable
  capsyncen_o   <= flags_r(6); -- Capture sync
  forceromcs_o  <= flags_r(7);


  rom_en_o <= '1' when state_r=WRITEROM else '0';
  rom_we_o <= dat_valid_s;
  rom_di_o <= dat_s;
  rom_addr_o <= std_logic_vector(rom_addr_r);

  process(SCK_i, CSN_i)
    variable caplen_v:  std_logic_vector(31 downto 0);
  begin
    if CSN_i='1' then
      state_r <= IDLE;
      txden_s <= '0';
      txload_s <= '0';
      fifo_rd_o <= '0';
      vidmem_en_o <= '0';
      frame_end_r <= '0';
      capmem_adr_r <= (others=>'0');
      wreg_en_r <= '0';
      regs32_r(2) <= x"deadbeef";
      regs32_r(3) <= x"cafe1234";
    elsif rising_edge(SCK_i) then

      fifo_rd_o <= '0';
      capmem_en_o <= '0';

      case state_r is
        when IDLE =>
          capmem_adr_r <= (others => '0');

          if dat_valid_s='1' then
            case dat_s is
              when x"DE" => -- Read status
                txden_s <= '1';
                txload_s <= '1';
                txdat_s <= "0000000" & fifo_empty_i;
                state_r <= READSTATUS;

              when x"DF" => -- Read video memory
                txden_s <= '1';
                txload_s <= '1';
                txdat_s <= "00000000";
                state_r <= RDVIDMEM1;

              when x"E0" => -- Read capture memory
                txden_s <= '1';
                txload_s <= '1';
                txdat_s <= "00000000";
                capmem_en_o <= '1'; -- Address should be zero.
                wordindex2_r <= "000";
                --capmem_adr_r <= capmem_adr_r + 1;
                state_r <= READCAPTURE;

              when x"E2" => -- Read capture status
                txden_s <= '1';
                txload_s <= '1';
                txdat_s <= "00000000";
                wordindex_r <= "00";
                state_r <= READCAPSTATUS;

              when x"E1" => -- Write ROM contents
                txden_s <= '1';
                txload_s <= '1';
                txdat_s <= "00000000";
                --capmem_en_o <= '1'; -- Address should be zero.
                --wordindex2_r <= "000";
                rom_addr_r <= (others => '0');
                state_r <= WRITEROM1;

              when x"EC" => -- Set flags
                txden_s <= '1';
                txload_s <= '1';
                txdat_s <= "00000000";
                state_r <= SETFLAGS;

              when x"ED" | x"EE" => -- Set/get regs 32
                txden_s <= '1';
                txload_s <= '1';
                txdat_s <= "00000000";
                wreg_en_r <= dat_s(0);
                state_r <= SETREG32_INDEX;

              when x"EF" => -- Mark end of frame
                txden_s <= '1';
                txload_s <= '1';
                txdat_s <= "00000000";
                frame_end_r <= '1';

              when x"FC" => -- Read data
                txden_s <= '1';
                txload_s <= '1';
                wordindex_r <= "00";
                if fifo_empty_i='1' then
                  txdat_s <= x"FF";
                  state_r <= UNKNOWN;
                else
                  txdat_s <= x"FE";
                  state_r <= READDATA;
                end if;

              when x"9E" | x"9F" => -- Read ID
                txden_s <= '1';
                txload_s <= '1';
                wordindex_r <= "00";
                txdat_s <= x"A5";
                state_r <= READID;
                
              when others =>
                -- Unknown command
                state_r <= UNKNOWN;
            end case;
          end if;

        when SETREG32_INDEX =>
          txload_s <= '1';
          txden_s <= '1';
          if dat_valid_s='1' then
            current_reg_r <= to_integer(unsigned(dat_s));
            state_r <= SETREG32_1;
          end if;

        when SETREG32_1 =>
          txload_s <= '1';
          txden_s <= '1';
          txdat_s <= regs32_r(current_reg_r)(31 downto 24);
          if dat_valid_s='1' then
            if wreg_en_r='1' then
              regs32_r(current_reg_r)(31 downto 24) <= dat_s;
            end if;
            state_r <= SETREG32_2;
          end if;

        when SETREG32_2 =>
          txload_s <= '1';
          txden_s <= '1';
          txdat_s <= regs32_r(current_reg_r)(23 downto 16);
          if dat_valid_s='1' then
            if wreg_en_r='1' then
              regs32_r(current_reg_r)(23 downto 16) <= dat_s;
            end if;
            state_r <= SETREG32_3;
          end if;

        when SETREG32_3 =>
          txload_s <= '1';
          txden_s <= '1';
          txdat_s <= regs32_r(current_reg_r)(15 downto 8);
          if dat_valid_s='1' then
            if wreg_en_r='1' then
              regs32_r(current_reg_r)(15 downto 8) <= dat_s;
            end if;
            state_r <= SETREG32_4;
          end if;

        when SETREG32_4 =>
          txload_s <= '1';
          txden_s <= '1';
          txdat_s <= regs32_r(current_reg_r)(7 downto 0);
          if dat_valid_s='1' then
            if wreg_en_r='1' then
              regs32_r(current_reg_r)(7 downto 0) <= dat_s;
            end if;
            state_r <= UNKNOWN;
          end if;

        when WRITEROM1 =>
          txload_s <= '1';
          txden_s <= '1';
          if dat_valid_s='1' then
            rom_addr_r(13 downto 8) <= unsigned(dat_s(5 downto 0));
            state_r <= WRITEROM2;
          end if;

        when WRITEROM2 =>
          txload_s <= '1';
          txden_s <= '1';
          if dat_valid_s='1' then
            rom_addr_r(7 downto 0) <= unsigned(dat_s);
            state_r <= WRITEROM;
          end if;

        when WRITEROM =>
          txload_s <= '1';
          txden_s <= '1';
          if dat_valid_s='1' then
            rom_addr_r <= rom_addr_r + 1;
          end if;

        when READSTATUS =>
          txload_s <= '1';
          txden_s <= '1';

        when RDVIDMEM1 =>
          if dat_valid_s='1' then
            vid_addr_r(12 downto 8) <= unsigned(dat_s(4 downto 0));
            state_r <= RDVIDMEM2;
          end if;

        when RDVIDMEM2 =>
          if dat_valid_s='1' then
            vid_addr_r(7 downto 0) <= unsigned(dat_s);
            state_r <= RDVIDMEM;
            vidmem_en_o <= '1';
          end if;

        when RDVIDMEM =>

          if dat_valid_s='1' then
            txden_s <= '1';
            txload_s <= '1';
            txdat_s <= vidmem_data_i;
            vid_addr_r <= vid_addr_r + 1;
            --if vid_addr_r=x"1B00" then
              -- Mark end of framebuffer.
            --  framecmplt_r <= '1';
            --end if;
          end if;

        when SETFLAGS =>

          if dat_valid_s='1' then
            txden_s <= '1';
            txload_s <= '1';
            txdat_s <= flags_r;
            --flags_r <= dat_s;
          end if;


        when READDATA =>
          txload_s <= '1';
          txden_s <= '1';
          if dat_valid_s='1' then
            wordindex_r <= wordindex_r + 1;
          end if;
          case wordindex_r is
            when "11" => txdat_s <= fifo_data_i(7 downto 0);
            when "10" => txdat_s <= fifo_data_i(15 downto 8);
            when "01" => txdat_s <= fifo_data_i(23 downto 16);
            when "00" => txdat_s <= fifo_empty_i & fifo_data_i(30 downto 24);
            when others =>
          end case;

          if wordindex_r="11" and txready_s='1' then
            fifo_rd_o <= '1';
          end if;

        when READCAPSTATUS =>
          txload_s <= '1';
          txden_s <= '1';
          if dat_valid_s='1' then
            wordindex_r <= wordindex_r + 1;
          end if;

          caplen_v(31) := capture_trig_sync_s;
          caplen_v(30 downto CAPTURE_MEMWIDTH_BITS) := (others => '0');
          caplen_v(CAPTURE_MEMWIDTH_BITS-1 downto 0)  := capture_len_sync_s;

          case wordindex_r is
            when "11" => txdat_s <= caplen_v(7 downto 0);
            when "10" => txdat_s <= caplen_v(15 downto 8);
            when "01" => txdat_s <= caplen_v(23 downto 16);
            when "00" => txdat_s <= caplen_v(31 downto 24);
            when others =>
          end case;

          if wordindex_r="11" and txready_s='1' then
            fifo_rd_o <= '1';
          end if;

        when READCAPTURE =>
          txload_s <= '1';
          txden_s <= '1';
          capmem_en_o <= '0';

          if dat_valid_s='1' then
            if wordindex2_r = "100" then
              wordindex2_r <= "000";
            else
              wordindex2_r <= wordindex2_r + 1;
            end if;
          end if;
          case wordindex2_r is
            when "100" => txdat_s <= capmem_data_i(7 downto 0);
            when "011" => txdat_s <= capmem_data_i(15 downto 8);
            when "010" => txdat_s <= capmem_data_i(23 downto 16);
            when "001" => txdat_s <= capmem_data_i(31 downto 24);
            when "000" => txdat_s <= "0000" & capmem_data_i(35 downto 32);

            when others =>
          end case;

          if wordindex2_r="100" and dat_valid_s='1' then
            capmem_en_o <= '1';
            capmem_adr_r <= capmem_adr_r + 1;
          end if;

        when READID =>
          txload_s <= '1';
          txden_s <= '1';
          if txready_s='1' then
            wordindex_r <= wordindex_r + 1;
          end if;
          case wordindex_r is
            when "00" => txdat_s <= FPGAID0;
            when "01" => txdat_s <= FPGAID1;
            when "10" => txdat_s <= FPGAID2;
            when "11" =>
              txdat_s <= (others => '0');

              if SCREENCAP_ENABLED  then txdat_s(0) <= '1'; end if;
              if ROM_ENABLED        then txdat_s(1) <= '1'; end if;
              if CAPTURE_ENABLED    then txdat_s(2) <= '1'; end if;

            when others =>
          end case;

        when others =>
          txload_s <= '0';
          txden_s <= '0';
      end case;
    end if;
  end process;

end beh;
