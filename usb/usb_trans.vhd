ENTITY usb_trans IS
  PORT (
    usbclk_i    : in std_logic;
    ausbrst_i   : in std_logic;

    -- Transmission
    pid_i       : in std_logic_vector(3 downto 0);

    -- Address/EP for token packets
    addr_i      : in std_logic_vector(6 downto 0);
    ep_i        : in std_logic_vector(3 downto 0);
    -- Frame number for SOF
    frame_i     : in std_logic_vector(10 downto 0);
    --
    dsize_i     : in std_logic_vector(6 downto 0); -- 0 to 127

    strobe_i    : in std_logic;

    phy_txready_i     : in std_logic;
    phy_txactive_i    : in std_logic;
    phy_txdata_o      : out std_logic_vector(7 downto 0);
    phy_data_valid_o  : out std_logic;
    phy_rxactive_i    : in std_logic;
    phy_rxvalid_i    : in std_logic;
    phy_rxdata_i      : in std_logic_vector(7 downto 0);
    phy_rxerror_i     : in std_logic_vector(7 downto 0);

    -- Connection to EPMEM

    urd_o       : out std_logic;
    uwr_o       : out std_logic;
    uaddr_o     : out std_logic_vector(10 downto 0);
    udata_i     : in  std_logic_vector(7 downto 0);
    udata_o     : out std_logic_vector(7 downto 0);


    status_o    : out usb_transaction_status_type
  );

end entity usb_trans;

architecture beh of usb_trans is

  type state_type is (
    IDLE,
    SENDPID
  );

  type regs_type is record
    token_data  : std_logic_vector(10 downto 0); -- Frame or Addr/EP pair
    pid         : std_logic_vector(3 downto 0);
    txsize      : std_logic_vector(6 downto 0);
    addr        : unsigned(10 downto 0);
    state       : state_type;
  end record;

  signal r            : regs_type;
  signal crc5_out_s   : std_logic_vector(4 downto 0);

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

  process(usbclk_i, r)
    variable w: regs_type;
  begin
    w := r;
    phy_data_o        <= (others => 'X');
    phy_data_valid_o  <= '0';
    udata_o           <= (others => 'X');
    urd_o             <= '0';
    uwr_o             <= '0';
    uaddr_o           <= r.addr;

    case r.state is
      when IDLE =>
        w.pid         := pid_i;
        w.token_data  := (others => 'X');
        w.addr        := addr_i;
        w.txsize      := dsize_i;

        if strobe_i='1' then
          if is_token_pid(pid_i) then
            if pid_i=USBF_T_PID_SOF then
              w.token_data  := frame_i;
            else
              w.token_data  := addr_i & ep_i;
            end if;
          else
            w.token_data  := (others => 'X');
          end if;
          w.state := SENDPID;
        end if;

      when SENDPID =>
        phy_data_o        <= genpid(r.pid);
        phy_data_valid_o  <= '1';

        if phy_txready_i='1' then
          if is_token_pid(pid) then
            w.state       := TOKEN1;
          elsif is_data_pid(pid) then
            w.state       := DATA_1;
          elsif is_handshake_pid(pid) then
            w.state       := FLUSH;
          else
            w.state       := ERRORPID;
          end if;
        end if;

        if phy_rxactive_i='1' then
          w.state := BABBLE;
        end if;

      when TOKEN1 =>
        phy_data_o        <= r.token_data(7 downto 0);
        phy_data_valid_o  <= '1';
        if phy_txready_i='1' then
          w.state := TOKEN2;
        end if;

      when TOKEN2 =>
        phy_data_o        <= crc5_out_s & r.token_data(10 downto 8);
        phy_data_valid_o  <= '1';
        if phy_txready_i='1' then
          w.state := FLUSH;
        end if;

      when FLUSH =>
        if phy_txactive_i='0' then
          -- Do we need ack ?
          if needack(pid) then
            w.state := WAIT_ACK_NACK;
          else
            w.state := IDLE;
          end if;
        else
        end if;

    end case;
    

    if ausbrst_i='1' then
      r.state       <= IDLE;
      r.token_data  <= (others => 'X');

    elsif rising_edge(usbclk_i) then
      r <= w;
    end if;

  end process;

end beh;


