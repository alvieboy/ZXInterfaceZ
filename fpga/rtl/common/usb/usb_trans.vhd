library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.numeric_std.all;
library work;
use work.usbpkg.all;
-- synthesis translate_off
use work.txt_util.all;
-- synthesis translate_on

ENTITY usb_trans IS
  PORT (
    usbclk_i    : in std_logic;
    ausbrst_i   : in std_logic;

    -- Transmission
    pid_i       : in std_logic_vector(3 downto 0);
    speed_i     : in std_logic;
    fs_ce_i     : in std_logic;
    usb_rst_i   : in std_logic;

    -- Address/EP for token packets
    addr_i      : in std_logic_vector(6 downto 0);
    ep_i        : in std_logic_vector(3 downto 0);
    -- Frame number for SOF
    frame_i     : in std_logic_vector(10 downto 0);
    --
    dsize_i     : in std_logic_vector(6 downto 0); -- 0 to 127
    dsize_o     : out std_logic_vector(6 downto 0); -- 0 to 127
    daddr_i     : in std_logic_vector(9 downto 0); -- EPmem address
    strobe_i    : in std_logic;
    data_seq_i  : in std_logic;
    data_seq_valid_o : out std_logic;

    phy_txready_i     : in std_logic;
    phy_txactive_i    : in std_logic;
    phy_txdata_o      : out std_logic_vector(7 downto 0);
    phy_data_valid_o  : out std_logic;
    phy_rxactive_i    : in std_logic;
    phy_rxvalid_i     : in std_logic;
    phy_rxdata_i      : in std_logic_vector(7 downto 0);
    phy_rxerror_i     : in std_logic;

    -- Connection to EPMEM

    urd_o       : out std_logic;
    uwr_o       : out std_logic;
    uaddr_o     : out std_logic_vector(9 downto 0);
    udata_i     : in  std_logic_vector(7 downto 0);
    udata_o     : out std_logic_vector(7 downto 0);

    dbg_rx_data_done_o: out std_logic;
    dbg_state_o  : out std_logic_vector(4 downto 0);

    status_o    : out usb_transaction_status_type;

    cnt_ack_o       : out std_logic_vector(7 downto 0);
    cnt_nack_o      : out std_logic_vector(7 downto 0);
    cnt_babble_o    : out std_logic_vector(7 downto 0);
    cnt_stall_o     : out std_logic_vector(7 downto 0);
    cnt_crcerror_o  : out std_logic_vector(7 downto 0);
    cnt_timeout_o   : out std_logic_vector(7 downto 0);
    cnt_errorpid_o  : out std_logic_vector(7 downto 0);
    cnt_cplt_o      : out std_logic_vector(7 downto 0)

  );

end entity usb_trans;

architecture beh of usb_trans is

  type state_type is (
    IDLE,
    SENDPID,
    TOKEN1,
    TOKEN2,
    DATA1,
    CRC1,
    CRC2,
    FLUSH,
    ERRORPID,
    BABBLE,
    TIMEOUT,
    WAIT_RX,
    WAIT_DATA,
    WAIT_ACK_NACK,
    ACK,
    NACK,
    STALL,
    SEND_ACK,
    SEND_NACK,
    CRCERROR,
    COMPLETE
  );

  -- Debug counters

  constant C_DEFAULT_ITG : natural := 4; --((3)*4); -- 3 bit times
  constant C_RX_TIMEOUT  : natural := 7+8+8+8; --((7+8)*4); -- 7+8 bit times

  type regs_type is record
    token_data  : std_logic_vector(10 downto 0); -- Frame or Addr/EP pair
    pid         : std_logic_vector(3 downto 0);
    txsize      : std_logic_vector(6 downto 0);
    addr        : unsigned(9 downto 0);
    state       : state_type;
    txcrc16     : std_logic_vector(15 downto 0);
    rxtimeout   : natural range 0 to C_RX_TIMEOUT-1;
    seq         : std_logic;
    seq_valid     : std_logic;
    pd_resetn     : std_logic;
    cnt_ack       : unsigned(7 downto 0);
    cnt_nack      : unsigned(7 downto 0);
    cnt_babble    : unsigned(7 downto 0);
    cnt_stall     : unsigned(7 downto 0);
    cnt_crcerror  : unsigned(7 downto 0);
    cnt_timeout   : unsigned(7 downto 0);
    cnt_errorpid  : unsigned(7 downto 0);
    cnt_cplt      : unsigned(7 downto 0);
 
  end record;

  signal  r             : regs_type;
  signal  crc5_out_s    : std_logic_vector(4 downto 0);
	signal	pid_OUT       : std_logic;
  signal  pid_IN:     std_logic;
  signal  pid_SOF:    std_logic;
  signal  pid_SETUP:  std_logic;
	signal	pid_DATA0:  std_logic;
  signal  pid_DATA1:  std_logic;
  signal  pid_DATA2:  std_logic;
  signal  pid_MDATA:  std_logic;
	signal	pid_ACK:    std_logic;
  signal  pid_NACK:   std_logic;
  signal  pid_STALL:  std_logic;
  signal  pid_NYET:   std_logic;
	signal	pid_PRE:    std_logic;
  signal  pid_ERR:    std_logic;
  signal  pid_SPLIT:  std_logic;
  signal  pid_PING:   std_logic;
	signal	pid_cks_err:std_logic;

  signal itg_r            : natural range 0 to C_DEFAULT_ITG-1;
  signal itg_zero_s       : boolean;

	signal rx_data_st       : std_logic_vector(7 downto 0);
  signal rx_data_valid    : std_logic;
  signal rx_data_done     : std_logic;
  signal crc16_err        : std_logic;
  signal seq_err          : std_logic;
  signal rx_busy          : std_logic;
  signal token_valid_s    : std_logic;

  signal crc16_in_s       : std_logic_vector(7 downto 0);
  signal crc16_out_s      : std_logic_vector(15 downto 0);

begin

  frame_crc_inst: entity work.usb1_crc5
  generic map (
    reverse_input => true,
    invert_and_reverse_output => true
  )
  port map (
	  crc_in  => "11111",
	  din     => r.token_data,
	  crc_out => crc5_out_s
  );

  pd: entity work.usb1_pd
  port map (
    clk       => usbclk_i,
    rst       => r.pd_resetn,
    rx_data   => phy_rxdata_i,
    rx_valid  => Phy_RxValid_i,
    rx_active => Phy_RxActive_i,
    rx_err    => Phy_RxError_i,

  		-- PID Information
		pid_OUT   => pid_OUT,
    pid_IN    => pid_IN,
    pid_SOF   => pid_SOF,
    pid_SETUP => pid_SETUP,
		pid_DATA0 => pid_DATA0,
    pid_DATA1 => pid_DATA1,
    pid_DATA2 => pid_DATA2,
    pid_MDATA => pid_MDATA,
		pid_ACK   => pid_ACK,
    pid_NACK  => pid_NACK,
    pid_STALL => pid_STALL,
    pid_NYET  => pid_NYET,
		pid_PRE   => pid_PRE,
    pid_ERR   => pid_ERR,
    pid_SPLIT => pid_SPLIT,
    pid_PING  => pid_PING,
		pid_cks_err => pid_cks_err,

		-- Token Information
		--token_fadr  => token_fadr,
    --token_endp  => token_endp,
    token_valid   => token_valid_s,
    --crc5_err    => crc5_err,
		--frame_no    => frame_no,

		-- Receive Data Output
		rx_data_st    => rx_data_st,
    rx_data_valid => rx_data_valid,
    rx_data_done  => rx_data_done,
    crc16_err     => crc16_err,

		-- Misc.
		seq_err       => seq_err,
    rx_busy       => rx_busy
  );

  data_crc_inst: entity work.usb1_crc16
  port map (
	  crc_in  => r.txcrc16,
	  din     => crc16_in_s,
	  crc_out => crc16_out_s
  );



  uaddr_o <= std_logic_vector(r.addr);
  udata_o <= rx_data_st;
  dsize_o <= r.txsize; -- Reused for RX

  process(usbclk_i, r, pid_i, daddr_i, dsize_i,data_seq_i,strobe_i,frame_i, addr_i, ep_i,
    phy_txactive_i, phy_txready_i, phy_rxactive_i, crc5_out_s, udata_i, crc16_out_s, rx_data_valid,
    rx_data_done,pid_ACK,pid_NACK, ausbrst_i, speed_i, fs_ce_i,crc16_err,seq_err,pid_STALL,
    itg_zero_s)
    variable w: regs_type;
  begin
    w := r;

    phy_txdata_o      <= (others => 'X');
    phy_data_valid_o  <= '0';
    urd_o             <= '0';
    uwr_o             <= '0';
    status_o          <= BUSY;
    crc16_in_s        <= (others => 'X');

    w.pd_resetn       := '1'; -- Default out of reset

    case r.state is
      when IDLE =>
        status_o          <= IDLE;
        w.pid         := pid_i;
        w.token_data  := ep_i & addr_i;
        w.addr        := unsigned(daddr_i);
        w.txsize      := dsize_i;
        w.txcrc16     := (others => 'X');
        w.seq         := data_seq_i;

        if strobe_i='1' then
          if pid_i=USBF_T_PID_SOF then
            w.token_data  := frame_i;
          end if;
          w.state := SENDPID;
        end if;

      when SENDPID =>
        phy_txdata_o      <= genpid(r.pid);
        phy_data_valid_o  <= '1';
        w.txcrc16         := (others => '1');
        status_o          <= BUSY;
        if phy_txready_i='1' then
          if is_token_pid(r.pid) then
            w.state       := TOKEN1;
          elsif is_data_pid(r.pid) then
            if r.txsize/=0 then
              urd_o         <= '1';
              w.txsize      := r.txsize - 1;
              w.addr        := r.addr + 1;
              w.state       := DATA1;
            else
              w.state       := CRC1;
            end if;
          elsif is_handshake_pid(r.pid) then
            w.state       := FLUSH;
          else
            w.state       := ERRORPID;
          end if;

          -- Special case EOP generation
          if r.pid=USBF_T_PID_SOF and speed_i='0' then
            w.state := FLUSH;
          end if;

        end if;

        --if phy_rxactive_i='1' then
        --  w.state := BABBLE;
        --end if;

        w.pd_resetn       := '0'; -- Reset packet decoder

      when TOKEN1 =>
        status_o          <= BUSY;
        phy_txdata_o      <= r.token_data(7 downto 0);
        phy_data_valid_o  <= '1';
        if phy_txready_i='1' then
          w.state := TOKEN2;
        end if;

      when TOKEN2 =>
        status_o          <= BUSY;
        phy_txdata_o      <= crc5_out_s & r.token_data(10 downto 8);
        phy_data_valid_o  <= '1';
        if phy_txready_i='1' then
          w.state := FLUSH;
        end if;

      when FLUSH =>
        status_o          <= BUSY;
        if phy_txactive_i='0' then

          if itg_zero_s then
            -- Do we need ack ?
            -- synthesis translate_off
            if rising_edge(usbclk_i) then report "Flushed transmission"; end if;
            -- synthesis translate_on
            if needack(r.pid) then
              -- synthesis translate_off
              if rising_edge(usbclk_i) then report "Moving to data RX stage"; end if;
              -- synthesis translate_on
              w.rxtimeout   := C_RX_TIMEOUT - 1;
              w.state       := WAIT_RX;
            elsif r.pid=USBF_T_PID_SETUP or r.pid=USBF_T_PID_OUT then
              -- Move to data stage.
              if r.seq='0' then
                w.pid         := USBF_T_PID_DATA0;
              else
                w.pid         := USBF_T_PID_DATA1;
              end if;
              w.state       := SENDPID;
              -- synthesis translate_off
              if rising_edge(usbclk_i) then report "Set up data stage"; end if;
              -- synthesis translate_on
  
            else
              -- synthesis translate_off
              if rising_edge(usbclk_i) then report "Transaction completed"; end if;
              -- synthesis translate_on

              status_o <= COMPLETED;
              w.state := IDLE;
            end if;
          end if;
        end if;

      when DATA1 =>
        phy_txdata_o      <= udata_i;
        crc16_in_s        <= reverse(udata_i);
        status_o          <= BUSY;

        phy_data_valid_o  <= '1';
        if phy_txready_i='1' then
          -- Size, address.
          w.txcrc16       := crc16_out_s;
          if r.txsize=0 then
            w.addr        := (others => 'X');
            w.state       := CRC1;
          else
            w.addr        := r.addr + 1;
            w.txsize      := r.txsize - 1;
            urd_o         <= '1';
          end if;
        else
          w.txsize      := r.txsize;
          w.addr        := r.addr;
        end if;

      when CRC1  =>
        status_o          <= BUSY;
        phy_txdata_o      <= reverse(not(r.txcrc16(15 downto 8)));
        phy_data_valid_o  <='1';
        if phy_txready_i='1' then
          w.state := CRC2;
        end if;

      when CRC2 =>
        status_o          <= BUSY;
        phy_txdata_o      <= reverse(not(r.txcrc16(7 downto 0)));
        phy_data_valid_o  <= '1';

        if phy_txready_i='1' then
          w.state := FLUSH;
        end if;

      when TIMEOUT =>
        -- synthesis translate_off
        if rising_edge(usbclk_i) then report "Timeout"; end if;
        -- synthesis translate_on
        status_o          <= TIMEOUT;
        w.state := IDLE;

      when WAIT_RX =>
        status_o          <= BUSY;

        if phy_rxactive_i='0' then
          if r.rxtimeout=0 then
            -- synthesis translate_off
            if rising_edge(usbclk_i) then report "Timed out waiting for reply (no RXActive)"; end if;
            -- synthesis translate_on
            w.state := TIMEOUT;
          else
            if fs_ce_i='1' then
              w.rxtimeout := r.rxtimeout - 1;
            end if;
          end if;
        else
          if needdata(r.pid) then
            -- synthesis translate_off
            if rising_edge(usbclk_i) then report "PID needs data"; end if;
            -- synthesis translate_on
            w.state := WAIT_DATA;
            w.txsize := (others => '0');
          elsif needack(r.pid) then

            -- synthesis translate_off
            if rising_edge(usbclk_i) then report "PID needs ACK/NACK"; end if;
            -- synthesis translate_on

            w.state := WAIT_ACK_NACK;
          end if;

        end if;

      when WAIT_DATA =>
        status_o      <= BUSY;

        if rx_data_valid='1' then
          uwr_o <= '1';
          -- synthesis translate_off
          if rising_edge(usbclk_i) then report "Got DATA: " &hstr(rx_data_st); end if;
          -- synthesis translate_on
          w.addr := r.addr + 1;
          w.txsize := r.txsize+ 1;
        elsif rx_data_done='1' then

          -- synthesis translate_off
          if rising_edge(usbclk_i) then report "RX data done idx " &hstr(r.txsize); end if;
          -- synthesis translate_on
          if crc16_err='1' or seq_err='1' then
            -- synthesis translate_off
            if rising_edge(usbclk_i) then report "CRC16 error"; end if;
            -- synthesis translate_on
            w.state := CRCERROR;
          else
            -- Check data sequence validity
            if pid_DATA0='1' and data_seq_i='0' then
              w.seq_valid := '1';
            elsif pid_DATA1='1' and data_seq_i='1' then
              w.seq_valid := '1';
            else
              w.seq_valid := '0';
            end if;

            w.state := SEND_ACK;
          end if;
        end if;

        if pid_STALL='1' then
          w.state := STALL;
        end if;

        if pid_NACK='1' then
          w.state := NACK;
        end if;

        if rx_data_done='0' and phy_rxactive_i='0' then
          w.state := TIMEOUT;
        end if;

      when SEND_ACK =>
        status_o          <= BUSY;
        w.pid       := USBF_T_PID_ACK;

        if itg_zero_s then
          w.state   := SENDPID;
        end if;



      when WAIT_ACK_NACK =>
        status_o          <= BUSY;

        if pid_ACK='1' then
          -- synthesis translate_off
          if rising_edge(usbclk_i) then report "Got ACK"; end if;
          -- synthesis translate_on
          w.state := ACK;
        elsif pid_NACK='1' then
          -- synthesis translate_off
          if rising_edge(usbclk_i) then report "Got NACK"; end if;
          -- synthesis translate_on
          w.state := NACK;
        elsif pid_STALL='1' then
          -- synthesis translate_off
          if rising_edge(usbclk_i) then report "Got STALL"; end if;
          -- synthesis translate_on
          w.state := STALL;
        elsif phy_rxactive_i='0' then
          -- synthesis translate_off
          if rising_edge(usbclk_i) then report "Timeout waiting for ACK/NACK"; end if;
            -- synthesis translate_on
  -- Something went wrong?
          w.state := TIMEOUT;
        end if;

      when ACK =>
        status_o          <= BUSY;

        if itg_zero_s then
          status_o          <= ACK;
          w.state   := IDLE;
          w.cnt_ack   := r.cnt_ack + 1;
        end if;

      when STALL =>
        status_o          <= BUSY;

        if itg_zero_s then
          status_o          <= STALL;
          w.state   := IDLE;
          w.cnt_stall   := r.cnt_stall + 1;
        end if;

      when NACK =>
        status_o          <= BUSY;

        if itg_zero_s then
          status_o          <= NACK;
          w.state   := IDLE;
          w.cnt_nack   := r.cnt_nack + 1;

        end if;

      when CRCERROR =>
        status_o          <= BUSY;

        if itg_zero_s then
          status_o          <= CRCERROR;
          w.state   := IDLE;
          w.cnt_crcerror   := r.cnt_crcerror + 1;

        end if;

      when COMPLETE =>
        status_o          <= BUSY;

        if itg_zero_s then
          status_o          <= COMPLETED;
          w.state   := IDLE;
          w.cnt_cplt   := r.cnt_cplt + 1;

        end if;

      when SEND_NACK=>
      when ERRORPID =>
        w.cnt_errorpid   := r.cnt_errorpid + 1;
          w.state := IDLE;


      when BABBLE =>
        if itg_zero_s then
          status_o          <= BABBLE;
          w.state := IDLE;
          w.cnt_babble   := r.cnt_babble + 1;

        end if;

    end case;
    

    if ausbrst_i='1' then --or usb_rst_i='1' then
      r.state       <= IDLE;
      r.token_data  <= (others => 'X');
      r.pd_resetn   <= '0';
      r.seq_valid   <= '0';

      r.cnt_ack         <= (others => '0');
      r.cnt_nack        <= (others => '0');
      r.cnt_stall       <= (others => '0');
      r.cnt_crcerror    <= (others => '0');
      r.cnt_timeout     <= (others => '0');
      r.cnt_errorpid    <= (others => '0');
  

    elsif rising_edge(usbclk_i) then
      r <= w;
    end if;

  end process;

  itg_zero_s <= true when itg_r=0 else false;

  process(usbclk_i, ausbrst_i)
  begin
    if ausbrst_i='1' then
      itg_r   <= C_DEFAULT_ITG-1;
    elsif rising_edge(usbclk_i) then
      case r.state is

        when COMPLETE | CRCERROR | NACK | STALL | ACK | SEND_ACK | BABBLE =>
          if fs_ce_i='1' then
            if not itg_zero_s then
              itg_r   <= itg_r - 1;
            end if;
          end if;

        when FLUSH =>
          if phy_txactive_i='0' then
            if fs_ce_i='1' then
              if not itg_zero_s then
                itg_r   <= itg_r - 1;
              end if;
            end if;
          end if;

        when others =>
          itg_r   <= C_DEFAULT_ITG-1;
      end case;
    end if;
  end process;

  dbg_rx_data_done_o <= rx_data_done;
  data_seq_valid_o <= r.seq_valid;

  process(r.state)
  begin
    case r.state is
      when IDLE =>           dbg_state_o <= "00000";
      when SENDPID =>        dbg_state_o <= "00001";
      when TOKEN1 =>         dbg_state_o <= "00010";
      when TOKEN2 =>         dbg_state_o <= "00011";
      when DATA1 =>          dbg_state_o <= "00100";
      when CRC1 =>           dbg_state_o <= "00101";
      when CRC2 =>           dbg_state_o <= "00110";
      when FLUSH =>          dbg_state_o <= "00111";
      when ERRORPID =>       dbg_state_o <= "01000";
      when BABBLE =>         dbg_state_o <= "01001";
      when TIMEOUT =>        dbg_state_o <= "01010";
      when WAIT_RX =>        dbg_state_o <= "01011";
      when WAIT_DATA =>      dbg_state_o <= "01100";
      when WAIT_ACK_NACK =>  dbg_state_o <= "01101";
      when ACK =>            dbg_state_o <= "01111";
      when NACK =>           dbg_state_o <= "10000";
      when STALL =>          dbg_state_o <= "10001";
      when SEND_ACK =>       dbg_state_o <= "10010";
      when SEND_NACK =>      dbg_state_o <= "10011";
      when CRCERROR =>       dbg_state_o <= "10100";
      when COMPLETE =>       dbg_state_o <= "10101";
      when others =>         dbg_state_o <= "10110";
    end case;
  end process;

  cnt_ack_o      <= std_logic_vector(r.cnt_ack);
  cnt_nack_o     <= std_logic_vector(r.cnt_nack);
  cnt_babble_o   <= std_logic_vector(r.cnt_babble);
  cnt_stall_o    <= std_logic_vector(r.cnt_stall);
  cnt_crcerror_o <= std_logic_vector(r.cnt_crcerror);
  cnt_timeout_o  <= std_logic_vector(r.cnt_timeout);
  cnt_errorpid_o <= std_logic_vector(r.cnt_errorpid);
  cnt_cplt_o     <= std_logic_vector(r.cnt_cplt);

end beh;