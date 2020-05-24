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


    status_o    : out usb_transaction_status_type
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

  constant C_DEFAULT_ITG : natural := 3; --((3)*4); -- 3 bit times
  constant C_RX_TIMEOUT  : natural := 7+8+8; --((7+8)*4); -- 7+8 bit times

  type regs_type is record
    token_data  : std_logic_vector(10 downto 0); -- Frame or Addr/EP pair
    pid         : std_logic_vector(3 downto 0);
    txsize      : std_logic_vector(6 downto 0);
    addr        : unsigned(9 downto 0);
    state       : state_type;
    txcrc16     : std_logic_vector(15 downto 0);
    itg         : natural range 0 to C_DEFAULT_ITG-1;
    rxtimeout   : natural range 0 to C_RX_TIMEOUT-1;
    seq         : std_logic;
  end record;

  signal r            : regs_type;
  signal crc5_out_s   : std_logic_vector(4 downto 0);
  signal pd_resetn_s  : std_logic;
	signal	pid_OUT:    std_logic;
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
    rst       => pd_resetn_s,
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
    rx_data_done,pid_ACK,pid_NACK, ausbrst_i)
    variable w: regs_type;
  begin
    w := r;

    phy_txdata_o      <= (others => 'X');
    phy_data_valid_o  <= '0';
    urd_o             <= '0';
    uwr_o             <= '0';
    pd_resetn_s       <= '1';
    status_o          <= BUSY;
    crc16_in_s        <= (others => 'X');

    w.itg             := C_DEFAULT_ITG-1;

    case r.state is
      when IDLE =>
        status_o          <= IDLE;
        w.pid         := pid_i;
        w.token_data  := (others => 'X');
        w.addr        := unsigned(daddr_i);
        w.txsize      := dsize_i;
        w.txcrc16     := (others => 'X');
        w.seq         := data_seq_i;

        if strobe_i='1' then
          if is_token_pid(pid_i) then
            if pid_i=USBF_T_PID_SOF then
              w.token_data  := frame_i;
            else
              w.token_data  := ep_i & addr_i;
            end if;
          else
            w.token_data  := (others => 'X');
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

        pd_resetn_s       <= '0'; -- Reset packet decoder

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
          if r.itg=0 then
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
          else
            w.itg       := r.itg - 1;
          end if;
        else
          if fs_ce_i='1' then
            w.itg := C_DEFAULT_ITG - 1;
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

      when ERRORPID =>

      when BABBLE =>
        status_o          <= BABBLE;
        w.state := IDLE;

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
        w.itg         := C_DEFAULT_ITG - 1;

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
            w.state := SEND_ACK;
          end if;
        end if;

        if pid_STALL='1' then
          w.state := STALL;
        end if;

        if rx_data_done='0' and phy_rxactive_i='0' then
          w.state := TIMEOUT;
        end if;

      when SEND_ACK =>
        status_o          <= BUSY;
        w.pid       := USBF_T_PID_ACK;

        if r.itg=0 then
          w.state   := SENDPID;
        else
          w.itg := r.itg - 1;
        end if;



      when WAIT_ACK_NACK =>
        status_o          <= BUSY;
        w.itg := C_DEFAULT_ITG - 1;

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

        if r.itg=0 then
          status_o          <= ACK;
          w.state   := IDLE;
        else
          w.itg := r.itg - 1;
        end if;

      when STALL =>
        status_o          <= BUSY;

        if r.itg=0 then
          status_o          <= STALL;
          w.state   := IDLE;
        else
          w.itg := r.itg - 1;
        end if;

      when NACK =>
        status_o          <= BUSY;

        if r.itg=0 then
          status_o          <= ACK;
          w.state   := IDLE;
        else
          w.itg := r.itg - 1;
        end if;

      when CRCERROR =>
        status_o          <= BUSY;

        if r.itg=0 then
          status_o          <= ACK;
          w.state   := IDLE;
        else
          w.itg := r.itg - 1;
        end if;

      when COMPLETE =>
        status_o          <= BUSY;

        if r.itg=0 then
          status_o          <= COMPLETED;
          w.state   := IDLE;
        else
          w.itg := r.itg - 1;
        end if;

      when SEND_NACK=>

    end case;
    

    if ausbrst_i='1' then
      r.state       <= IDLE;
      r.token_data  <= (others => 'X');
      pd_resetn_s   <= '0';
    elsif rising_edge(usbclk_i) then
      r <= w;
    end if;

  end process;



end beh;


