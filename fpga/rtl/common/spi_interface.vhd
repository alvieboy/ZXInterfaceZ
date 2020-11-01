library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
library work;
use work.zxinterfacepkg.all;


entity spi_interface is
  port (
    clk_i                 : in std_logic;
    arst_i                : in std_logic;

    SCKx_i                : in std_logic;
    CSNx_i                : in std_logic;

    MOSI_i                : in std_logic;
    MISO_o                : out std_logic;

    pc_i                  : in std_logic_vector(15 downto 0);

    vidmem_en_o           : out std_logic;
    vidmem_adr_o          : out std_logic_vector(12 downto 0);
    vidmem_data_i         : in std_logic_vector(7 downto 0);

    rstfifo_o             : out std_logic;
    rstspect_o            : out std_logic;
    intenable_o           : out std_logic;
    capsyncen_o           : out std_logic;
    frameend_o            : out std_logic;
    mode2a_o              : out std_logic;

    vidmode_o             : out std_logic_vector(1 downto 0);
    ulahack_o             : out std_logic;

    forceromcs_trig_on_o  : out std_logic;
    forceromcs_trig_off_o : out std_logic;
    forceromonretn_trig_o : out std_logic; -- single tick, SPI sck
    forcenmi_trig_on_o    : out std_logic; -- single tick, SPI sck
    forcenmi_trig_off_o   : out std_logic; -- single tick, SPI sck
    nmireason_o           : out std_logic_vector(7 downto 0);
    -- Resource FIFO

    resfifo_reset_o       : out std_logic;
    resfifo_wr_o          : out std_logic;
    resfifo_write_o       : out std_logic_vector(7 downto 0);
    resfifo_full_i        : in std_logic_vector(3 downto 0);

    -- TAP player FIFO
    tapfifo_reset_o       : out std_logic;
    tapfifo_wr_o          : out std_logic;
    tapfifo_write_o       : out std_logic_vector(8 downto 0);
    tapfifo_full_i        : in std_logic;
    tapfifo_used_i        : in std_logic_vector(9 downto 0);
    tap_enable_o          : out std_logic;

    -- Command FIFO

    cmdfifo_reset_o       : out std_logic;
    cmdfifo_rd_o          : out std_logic;
    cmdfifo_read_i        : in std_logic_vector(7 downto 0);
    cmdfifo_empty_i       : in std_logic;
    cmdfifo_intack_o      : out std_logic; -- Interrupt acknowledge

    -- External RAM access
    extram_addr_o         : out std_logic_vector(31 downto 0);
    extram_dat_i          : in std_logic_vector(31 downto 0);
    extram_dat_o          : out std_logic_vector(31 downto 0);
    extram_req_o          : out std_logic;
    extram_we_o           : out std_logic;
    --extram_rd_o           : out std_logic;
    extram_valid_i        : in std_logic;

    -- Generic address/data
    generic_addr_o        : out std_logic_vector(10 downto 0);
    generic_dat_o         : out std_logic_vector(7 downto 0);

    -- USB access

    usb_rd_o              : out std_logic;
    usb_wr_o              : out std_logic;
    usb_dat_i             : in std_logic_vector(7 downto 0);
    usb_int_i             : in std_logic; -- USB interrupt

    -- Capture access
    capture_rd_o          : out std_logic;
    capture_wr_o          : out std_logic;
    capture_dat_i         : in std_logic_vector(7 downto 0);

    -- Keyboard manipulation
    kbd_en_o              : out std_logic;
    kbd_force_press_o     : out std_logic_vector(39 downto 0); -- 40 keys.
    -- Joystick data
    joy_en_o              : out std_logic;
    joy_data_o            : out std_logic_vector(4 downto 0);
    -- Mouse
    mouse_en_o            : out std_logic;
    mouse_x_o             : out std_logic_vector(7 downto 0);
    mouse_y_o             : out std_logic_vector(7 downto 0);
    mouse_buttons_o       : out std_logic_vector(1 downto 0);
    -- AY
    ay_en_o               : out std_logic;
    ay_en_reads_o         : out std_logic;
    -- Volume settings
    volume_o              : out std_logic_vector(63 downto 0);
    memromsel_o           : out std_logic_vector(2 downto 0);
    memsel_we_o           : out std_logic;
    romsel_we_o           : out std_logic
  );

end entity spi_interface;

architecture beh of spi_interface is

  signal txdat_s      : std_logic_vector(7 downto 0);
  signal dat_s        : std_logic_vector(7 downto 0);
  signal first_dat_s  : std_logic;
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
    UNKNOWN,
    READID,
    READSTATUS,
    RDVIDMEM1,
    RDVIDMEM2,
    RDVIDMEM,
    READCAPTURE,
    READCAPSTATUS,
    SETFLAGS1,
    SETFLAGS2,
    SETFLAGS3,
    SETMEMROM,
    SETREG32_INDEX,
    SETREG32_1,
    SETREG32_2,
    SETREG32_3,
    SETREG32_4,
    WRRESFIFO,
    WRTAPFIFO,
    RDTAPFIFOUSAGE,
    READFIFOCMD1,
    READFIFOCMDDATA,
    RDPC1,
    EXTRAMADDR1,
    EXTRAMADDR2,
    EXTRAMADDR3,
    EXTRAMDATA,
    GENERICADDR1,
    GENERICADDR2,
    GENERICREADDATA,
    GENERICWRITEDATA
  );

  signal state_r      : state_type;

  signal frame_end_r  : std_logic;
  signal rom_addr_r    : unsigned(13 downto 0);


  --signal capture_len_sync_s   : std_logic_vector(CAPTURE_MEMWIDTH_BITS-1 downto 0);
  --signal capture_trig_sync_s  : std_logic;

  signal wreg_en_r        : std_logic;
  signal tapfifo_used_lsb_r : std_logic_vector(7 downto 0);

  signal extram_addr_r  : std_logic_vector(23 downto 0);
  signal extram_req_r   : std_logic := '0'; -- FIxme: needs areset
  signal extram_we_r    : std_logic := '0';
  signal extram_wdata_r : std_logic_vector(7 downto 0);

  signal generic_addr_wr_r  : std_logic_vector(15 downto 0);
  signal generic_is_write_r : std_logic;

  signal usb_rd_r       : std_logic := '0'; -- FIxme: needs areset
  signal capture_rd_r   : std_logic := '0'; -- FIxme: needs areset
  signal tapcmd_r       : std_logic;

  signal memsel_we_r    : std_logic;
  signal romsel_we_r    : std_logic;
  signal memromsel_r    : std_logic_vector(2 downto 0);

  type generic_access_type is (
    GENERIC_USB,
    GENERIC_CAPTURE
  );
  signal generic_access_r: generic_access_type;

  signal cmdfifo_read_issued_r: std_logic;

  signal txload_s     : std_logic;
  signal txden_s      : std_logic;
  signal rx_rd_s      : std_logic;
  signal tx_full_s    : std_logic;
  signal rx_empty_s   : std_logic;

  signal dat_valid_s  : std_logic;

  signal csn_s        : std_logic;
  signal vidmem_en_r  : std_logic;
  signal vidmem_data_r: std_logic_vector(7 downto 0);
begin

  vidmem_adr_o <= std_logic_vector(vid_addr_r);
  frameend_o <= frame_end_r;

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
    -- RX fifo
    rx_rd_i       => rx_rd_s,
    rx_rdata_o(8)   => first_dat_s,
    rx_rdata_o(7 downto 0)    => dat_s,
    rx_empty_o    => rx_empty_s,
    csn_o         => csn_s
  );

  process(clk_i, arst_i)
  begin
    if arst_i='1' then

      flags_r               <= (others => '0');
      resfifo_reset_o       <= '0';
      forceromonretn_trig_o <= '0';
      forceromcs_trig_on_o  <= '0';
      forceromcs_trig_off_o <= '0';
      forcenmi_trig_on_o    <= '0';
      forcenmi_trig_off_o   <= '0';
      cmdfifo_intack_o      <= '0';
      cmdfifo_reset_o       <= '0';
    elsif rising_edge(clk_i) then

      resfifo_reset_o       <= '0';
      forceromonretn_trig_o <= '0';
      forceromcs_trig_on_o  <= '0';
      forceromcs_trig_off_o <= '0';
      forcenmi_trig_on_o    <= '0';
      forcenmi_trig_off_o   <= '0';
      cmdfifo_intack_o      <= '0';
      cmdfifo_reset_o       <= '0';

      if state_r=SETFLAGS1 then
        if dat_valid_s='1' then
          flags_r(7 downto 0) <= dat_s;
        end if;
      elsif state_r=SETFLAGS2 then
        if dat_valid_s='1' then
          resfifo_reset_o       <= dat_s(0);
          forceromonretn_trig_o <= dat_s(1);
          forceromcs_trig_on_o  <= dat_s(2);
          forceromcs_trig_off_o <= dat_s(3);
          -- Command FIFO interrupt acknowledge
          cmdfifo_intack_o      <= dat_s(4);
          cmdfifo_reset_o       <= dat_s(5);
          forcenmi_trig_on_o    <= dat_s(6);
          forcenmi_trig_off_o   <= dat_s(7); -- this might not be necessary.
        end if;
      elsif state_r=SETFLAGS3 then
        if dat_valid_s='1' then
          flags_r(15 downto 8) <= dat_s;
        end if;
      end if;

    end if;
  end process;


  process(clk_i, arst_i)
    variable caplen_v:  std_logic_vector(31 downto 0);
  begin
    if arst_i='1' or csn_s='1' then
      state_r       <= IDLE;
      txden_s       <= '0';
      txload_s      <= '0';
      cmdfifo_rd_o  <= '0';
      vidmem_en_o   <= '0';
      frame_end_r   <= '0';
      --capmem_adr_r  <= (others=>'0');
      wreg_en_r     <= '0';
      usb_rd_r      <= '0';
      capture_rd_r  <= '0';
      --usb_wr_r      <= '0';
      memsel_we_r   <= '0';
      romsel_we_r   <= '0';
      cmdfifo_read_issued_r <= '0';

      memromsel_r   <= (others => 'X');

    elsif rising_edge(clk_i) then

      cmdfifo_rd_o  <= '0';
      usb_rd_r      <= '0';
      capture_rd_r  <= '0';
      extram_req_r  <= '0';
      memsel_we_r   <= '0';
      romsel_we_r   <= '0';
      memromsel_r   <= (others => 'X');
      vidmem_en_r   <= '0';

      txload_s      <= '0';

      if rx_empty_s='0' and first_dat_s='1' then
        -- Forcibly handle commands.
        --capmem_adr_r <= (others => '0');

        case dat_s is
          when x"DE" => -- Read status
            txload_s  <= '1';
            txdat_s   <= "00" & cmdfifo_empty_i & resfifo_full_i & '0';--fifo_empty_i;
            state_r   <= READSTATUS;

          when x"DF" => -- Read video memory
            txload_s <= '1';
            txdat_s <= "00000000";
            state_r <= RDVIDMEM1;

          when x"40" => -- Read last PC
            pc_latch_r <= pc_i(7 downto 0);
            txload_s <= '1';
            txdat_s <= pc_i(15 downto 8);
            state_r <= RDPC1;

          when x"50" => -- Read external RAM
            txload_s <= '1';
            txdat_s <= "00000000";
            extram_we_r <= '0';
            state_r <= EXTRAMADDR1;

          when x"51" => -- Write external RAM
            txload_s <= '1';
            txdat_s <= "00000000";
            extram_we_r <= '1';
            state_r <= EXTRAMADDR1;

          when x"60" => -- USB read
            txload_s <= '1';
            txdat_s <= "00000000";
            generic_is_write_r <= '0';
            generic_access_r <= GENERIC_USB;
            state_r <= GENERICADDR1;

          when x"61" => -- USB write
            txload_s <= '1';
            txdat_s <= "00000000";
            generic_is_write_r <= '1';
            generic_access_r <= GENERIC_USB;
            state_r <= GENERICADDR1;

          when x"70" => -- Capture read
            if C_CAPTURE_ENABLED then
              txload_s <= '1';
              txdat_s <= "00000000";
              generic_is_write_r <= '0';
              generic_access_r <= GENERIC_CAPTURE;
              state_r <= GENERICADDR1;
            end if;

          when x"71" => -- Capture write
            if C_CAPTURE_ENABLED then
              txload_s <= '1';
              txdat_s <= "00000000";
              generic_is_write_r <= '1';
              generic_access_r <= GENERIC_CAPTURE;
              state_r <= GENERICADDR1;
            end if;

          when x"E3" => -- Write Resource FIFO contents
            txload_s <= '1';
            txdat_s <= "00000000";
            state_r <= WRRESFIFO;

          when x"E4" => -- Write TAP FIFO contents
            txload_s <= '1';
            txdat_s <= "00000000";
            state_r <= WRTAPFIFO;
            tapcmd_r <= '0';

          when x"E6" => -- Write TAP command FIFO contents
            txload_s <= '1';
            txdat_s <= "00000000";
            state_r <= WRTAPFIFO;
            tapcmd_r <= '1';

          when x"E5" => -- Get TAP FIFO usage
            txload_s <= '1';
            txdat_s <= tapfifo_full_i & "00000" & tapfifo_used_i(9 downto 8);
            txdat_s <= "000000" & tapfifo_used_i(9 downto 8);
            tapfifo_used_lsb_r <= tapfifo_used_i(7 downto 0);
            state_r <= RDTAPFIFOUSAGE;

          when x"EB" => -- Set mem/rom
            txden_s <= '1';
            txload_s <= '1';
            txdat_s <= "00000000";
            state_r <= SETMEMROM;

          when x"EC" => -- Set flags
            txden_s <= '1';
            txload_s <= '1';
            txdat_s <= "00000000";
            state_r <= SETFLAGS1;

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

          when x"FB" => -- Read FIFO command data
            txden_s     <= '1';
            txload_s    <= '1';
            wordindex_r <= "00";
            cmdfifo_read_issued_r <= '0';

            if cmdfifo_empty_i='1' then
              txdat_s   <= x"FF";
              state_r   <= UNKNOWN;
            else
              txdat_s   <= x"FE";

              state_r   <= READFIFOCMD1;
            end if;

          when x"9E" | x"9F" => -- Read ID
            txden_s <= '1';
            txload_s <= '1';
            wordindex_r <= "00";
            txdat_s <= C_FPGAID0;
            state_r <= READID;
            
          when others =>
            -- Unknown command
            state_r <= UNKNOWN;
        end case;
      end if;


      -- Non command data
      case state_r is
        when IDLE =>

        when RDPC1 =>

          txload_s <= dat_valid_s;
          txdat_s <= pc_latch_r;
          if dat_valid_s='1' then
            state_r <= UNKNOWN;
          end if;

        when RDTAPFIFOUSAGE =>
          txden_s <= '1';
          txload_s <= dat_valid_s;
          txdat_s <= tapfifo_used_lsb_r;
          if dat_valid_s='1' then
            state_r <= UNKNOWN;
          end if;

        when SETREG32_INDEX =>
          txload_s <= '1';
          txload_s <= dat_valid_s;
          if dat_valid_s='1' then
            current_reg_r <= to_integer(unsigned(dat_s));
            state_r <= SETREG32_1;
          end if;

        when SETREG32_1 =>
          txload_s <= dat_valid_s;
          txden_s <= '1';
          txdat_s <= regs32_r(current_reg_r)(31 downto 24);
          if dat_valid_s='1' then
            if wreg_en_r='1' then
              --regs32_r(current_reg_r)(31 downto 24) <= dat_s;
              tempreg_r(23 downto 16) <= dat_s;
            end if;
            state_r <= SETREG32_2;
          end if;

        when SETREG32_2 =>
          txload_s <= dat_valid_s;
          txden_s <= '1';
          txdat_s <= regs32_r(current_reg_r)(23 downto 16);
          if dat_valid_s='1' then
            if wreg_en_r='1' then
              --regs32_r(current_reg_r)(23 downto 16) <= dat_s;
              tempreg_r(15 downto 8) <= dat_s;
            end if;
            state_r <= SETREG32_3;
          end if;

        when SETREG32_3 =>
          txload_s <= dat_valid_s;
          txden_s <= '1';
          txdat_s <= regs32_r(current_reg_r)(15 downto 8);
          if dat_valid_s='1' then
            if wreg_en_r='1' then
              tempreg_r(7 downto 0) <= dat_s;
            end if;
            state_r <= SETREG32_4;
          end if;

        when SETREG32_4 =>
          txload_s <= dat_valid_s;
          txden_s <= '1';
          txdat_s <= regs32_r(current_reg_r)(7 downto 0);
          if dat_valid_s='1' then
            if wreg_en_r='1' then
              regs32_r(current_reg_r) <= tempreg_r & dat_s;
            end if;
            state_r <= UNKNOWN;
          end if;

        when SETMEMROM =>
          memromsel_r <= dat_s(2 downto 0);

          if dat_valid_s='1' then
            memsel_we_r <= dat_s(7);
            romsel_we_r <= not dat_s(7);
            state_r <= UNKNOWN;
          end if;


        when WRRESFIFO =>
        when WRTAPFIFO =>
          
        when READSTATUS =>
          txload_s <= dat_valid_s;
          txden_s <= '1';

        when RDVIDMEM1 =>
          txload_s <= dat_valid_s;
          if dat_valid_s='1' then
            vid_addr_r(12 downto 8) <= unsigned(dat_s(4 downto 0));
            state_r <= RDVIDMEM2;
          end if;

        when RDVIDMEM2 =>
          txload_s <= dat_valid_s;
          if dat_valid_s='1' then
            vid_addr_r(7 downto 0) <= unsigned(dat_s);
            state_r <= RDVIDMEM;
            vidmem_en_o <= '1';
          end if;

        when RDVIDMEM =>
          txload_s <= '1';
          txden_s <= '1';
          txdat_s <= vidmem_data_i;


          if dat_valid_s='1' then
            vidmem_en_r <= '1';
            --vid_addr_r <= vid_addr_r + 1;
          end if;

        when SETFLAGS1 =>

          if dat_valid_s='1' then
            state_r <= SETFLAGS2;
          end if;

        when SETFLAGS2 =>
          
          if dat_valid_s='1' then
            state_r <= SETFLAGS3;
          end if;

        when SETFLAGS3 =>
          
          if dat_valid_s='1' then
            state_r <= UNKNOWN;
          end if;

        when READFIFOCMD1 =>
          --txload_s  <= '1';
          txload_s <= dat_valid_s;
          txdat_s   <= cmdfifo_read_i;

          if dat_valid_s ='1' then
            state_r <= READFIFOCMDDATA;
          end if;

        when READFIFOCMDDATA =>
          --txload_s  <= '1';
          --txden_s   <= '1';
          txload_s <= dat_valid_s;
          txdat_s   <= cmdfifo_read_i;

          if dat_valid_s ='1' then
            cmdfifo_read_issued_r<='1';
          end if;

          if dat_valid_s ='1' and cmdfifo_read_issued_r='1' then
            cmdfifo_rd_o <= '1';
          end if;

        when READID =>
          --txload_s <= '1';
          txload_s <= dat_valid_s;

          txden_s <= '1';
          if dat_valid_s='1' then
            wordindex_r <= wordindex_r + 1;
          end if;
          case wordindex_r is
            when "00" => txdat_s <= C_FPGAID1;
            when "01" => txdat_s <= C_FPGAID2;
            when "10" =>
              txdat_s <= (others => '0');

              if C_SCREENCAP_ENABLED  then txdat_s(0) <= '1'; end if;
              txdat_s(1) <= '1';
              if C_CAPTURE_ENABLED    then txdat_s(2) <= '1'; end if;

            when others =>
          end case;

        when EXTRAMADDR1 =>
          txload_s <= '1';
          txden_s <= '1';
          if dat_valid_s='1' then
            extram_addr_r(23 downto 16) <= dat_s;
            state_r <= EXTRAMADDR2;
          end if;

        when EXTRAMADDR2 =>
          txload_s <= '1';
          txden_s <= '1';
          if dat_valid_s='1' then
            extram_addr_r(15 downto 8) <= dat_s;
            state_r <= EXTRAMADDR3;
          end if;

        when EXTRAMADDR3 =>
          txload_s <= '1';
          txden_s <= '1';
          if dat_valid_s='1' then
            extram_addr_r(7 downto 0) <= dat_s;
            -- Request
            if extram_we_r='0' then
              extram_req_r <= '1';
            end if;
            state_r <= EXTRAMDATA;
          end if;

        when EXTRAMDATA =>
          txload_s <= '1';
          txden_s <= '1';
          txdat_s <= extram_dat_i(7 downto 0);

          if dat_valid_s='1' then
            extram_wdata_r <= dat_s;
            extram_req_r <= '1';
          end if;

        when GENERICADDR1 =>
          txload_s <= '1';
          txden_s <= '1';
          if dat_valid_s='1' then
            generic_addr_wr_r(15 downto 8) <= dat_s;
            state_r <= GENERICADDR2;
          end if;

        when GENERICADDR2 =>
          txload_s <= '1';
          txden_s <= '1';
          if dat_valid_s='1' then
            generic_addr_wr_r(7 downto 0) <= dat_s;
            if generic_is_write_r='1' then
              state_r <= GENERICWRITEDATA;
            else
              state_r <= GENERICREADDATA;
              --
              if generic_access_r=GENERIC_USB then
                usb_rd_r      <= '1';
              end if;
              if generic_access_r=GENERIC_CAPTURE then
                capture_rd_r  <= '1';
              end if;
            end if;
          end if;

        when GENERICREADDATA =>
          txload_s <= '1';
          txden_s   <= '1';
          case generic_access_r is
            when GENERIC_USB =>
              txdat_s   <= usb_dat_i;
            when GENERIC_CAPTURE =>
              txdat_s   <= capture_dat_i;
          end case;

          if dat_valid_s='1' then
            case generic_access_r is
              when GENERIC_USB =>
                usb_rd_r <= '1';
              when GENERIC_CAPTURE =>
                capture_rd_r <= '1';
            end case;
            state_r <= GENERICREADDATA;
          end if;

        when GENERICWRITEDATA =>
          txload_s <= '1';
          txden_s   <= '1';
          case generic_access_r is
            when GENERIC_USB =>
              txdat_s   <= usb_dat_i;
            when GENERIC_CAPTURE =>
              txdat_s   <= capture_dat_i;
          end case;

        when UNKNOWN =>
          -- Leave TXDEN.

        when others =>
          txload_s <= '0';
          txden_s <= '0';
      end case;

      case state_r is
        when EXTRAMDATA =>
          if extram_valid_i='1' then
            extram_addr_r <= std_logic_vector(unsigned(extram_addr_r) + 1);
          end if;
        when GENERICREADDATA | GENERICWRITEDATA =>
          if dat_valid_s='1' then
            generic_addr_wr_r <= std_logic_vector(unsigned(generic_addr_wr_r) +1);
          end if;
        when others =>
      end case;

      if vidmem_en_r='1' then
        --vidmem_data_r <= vidmem_data_i;
        vid_addr_r <= vid_addr_r + 1;
      end if;

    end if; -- rising_edge
  end process;

  -- Force mode2a to stay ON once asserted
  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      mode2a_o <= '0';
    elsif rising_edge(clk_i) then
      if flags_r(13)='1' then
        mode2a_o <= '1';
      end if;
    end if;
  end process;

  rstfifo_o             <= flags_r(0);
  rstspect_o            <= flags_r(1);
  intenable_o           <= flags_r(5); -- Interrupt enable
  capsyncen_o           <= flags_r(6); -- Capture sync

  ulahack_o             <= flags_r(8); --
  tapfifo_reset_o       <= flags_r(9); --
  tap_enable_o          <= flags_r(10); --

  vidmode_o(0)          <= flags_r(11); --
  vidmode_o(1)          <= flags_r(12); --
  --mode2a_o              <= flags_r(13); --

  resfifo_wr_o          <= '1' when state_r=WRRESFIFO and dat_valid_s='1' else '0';
  resfifo_write_o       <= dat_s;

  tapfifo_wr_o          <= '1' when state_r=WRTAPFIFO and dat_valid_s='1' else '0';
  tapfifo_write_o       <= tapcmd_r & dat_s;
  usb_rd_o              <= usb_rd_r;

  usb_wr_o              <= '1' when state_r=GENERICWRITEDATA and dat_valid_s='1' and generic_access_r=GENERIC_USB else '0';
  capture_wr_o          <= '1' when state_r=GENERICWRITEDATA and dat_valid_s='1' and generic_access_r=GENERIC_CAPTURE else '0';
  generic_dat_o         <= dat_s;

  extram_addr_o         <= x"00" & extram_addr_r;
  extram_req_o          <= extram_req_r;
  extram_dat_o          <= x"000000" & extram_wdata_r;
  extram_we_o           <= extram_we_r;

  generic_addr_o        <= generic_addr_wr_r(10 downto 0);
  capture_rd_o          <= capture_rd_r;

  kbd_en_o              <= regs32_r(2)(0);
  joy_en_o              <= regs32_r(2)(1);
  mouse_en_o            <= regs32_r(2)(2);
  ay_en_o               <= regs32_r(2)(3);
  ay_en_reads_o         <= regs32_r(2)(4);

  kbd_force_press_o     <= regs32_r(4)(7 downto 0) & regs32_r(3);

  -- Joystick and mouse data

  mouse_x_o             <= regs32_r(5)(7 downto 0);
  mouse_y_o             <= regs32_r(5)(15 downto 8);
  mouse_buttons_o       <= regs32_r(5)(17 downto 16);
  joy_data_o            <= regs32_r(5)(22 downto 18);

  -- Volumes
  volume_o              <= regs32_r(7) & regs32_r(6);
  -- NMI
  nmireason_o           <= regs32_r(0)(7 downto 0);

  memromsel_o           <= memromsel_r;
  memsel_we_o           <= memsel_we_r;
  romsel_we_o           <= romsel_we_r;


end beh;
