LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

LIBRARY work;
use work.bfm_Usbdevice_p.all;
use work.txt_util.all;
use work.usbpkg.all;

entity bfm_Usbdevice is
  port (
    Cmd_i   : in Cmd_Usbdevice_type;
    Data_o  : out Data_Usbdevice_type;
    DM_io   : inout std_logic;
    DP_io   : inout std_logic
  );
end entity bfm_Usbdevice;

architecture sim of bfm_Usbdevice is

  signal attached_s:  boolean := false;
  -- local clock, rst,

  signal clk_s:  std_logic := '0';
  signal rst_s:  std_logic := '0';
  signal rstn_s:  std_logic := '1';
  signal vp_s     : std_logic;
  signal vm_s     : std_logic;
  signal rcv_s     : std_logic;
  signal noe_s     : std_logic;
  signal softcon_s     : std_logic := '0';
  signal speed_s     : std_logic := '0';
  signal vmo_s     : std_logic;
  signal vpo_s     : std_logic;

  signal eop_s      : std_logic;

  signal DataOut_s        : std_logic_vector(7 downto 0);
  signal TxValid_s        : std_logic := '0';
  signal TxReady_s        : std_logic;
  signal DataIn_s         : std_logic_vector(7 downto 0);
  signal RxValid_s        : std_logic;
  signal RxActive_s       : std_logic;
  signal RxError_s        : std_logic;
  signal LineState_s      : std_logic_vector(1 downto 0);

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

		-- Token Information
	signal	token_fadr: std_logic_vector(6 downto 0);
  signal  token_endp: std_logic_vector(3 downto 0);
  signal  token_valid:std_logic;
  signal  crc5_err:   std_logic;
	signal	frame_no:   std_logic_vector(10 downto 0);

		-- Receive Data Output
	signal	rx_data_st:    std_logic_vector(7 downto 0);
  signal  rx_data_valid:    std_logic;
  signal  rx_data_done:    std_logic;
  signal  crc16_err:     std_logic;

		-- Misc.
	signal	seq_err:    std_logic;
  signal  rx_busy:    std_logic;

  signal  usb_rst_s : std_logic;


  signal crc16_out_s  : std_logic_vector(15 downto 0);

  signal sendack      : std_logic:='0';
  signal sendpacket   : std_logic:='0';
  signal vmo_dly_s    : std_logic;
  signal vpo_dly_s    : std_logic;

--  constant txdata: txdata_type := (
--    x"12",
--    x"01",
--    x"00",
------    x"02", -- version
--    x"00", -- class
--    x"00", -- subclass
--    x"00", -- protocol
--    x"40", -- max pack size
--    x"dd", -- vend1
--    x"27", -- vend2
--    x"c0",
--    x"16", --prod
--    x"00",
------    x"02",
--    x"01", --man
--    x"02", -- prod
--    x"03", -- serial,
--    x"01" -- num conf.
--  );
begin



  process
  begin
    if Cmd_i.Enabled then
      rst_s <= '1' after 40 ns;
      g: loop
        clk_s <= '1';
        wait for 500 ns / 48;
        clk_s <= '0';
        wait for 500 ns / 48;
        if not Cmd_i.Enabled then
          exit g;
        end if;
      end loop;
    else
      rst_s <= '0';
      clk_s <= '0';
      report "Stopping USB clock";
      wait on Cmd_i.Enabled;
    end if;
  end process;

  phy: entity work.usb_phy
  generic map (
    usb_rst_det   => true,
    CLOCK         => "48"
  )
  port map (
    clk              => clk_s,
    rst              => rst_s,
    phy_tx_mode      => '1',--: in  std_logic;  -- HIGH level for differential io mode (else single-ended)
    usb_rst          => usb_rst_s,

    -- Transciever Interface
    rxd             => rcv_s,
    rxdp            => vp_s,
    rxdn            => vm_s,
    txdp            => vpo_s,
    txdn            => vmo_s,
    txoe            => noe_s,
    eop_o           => eop_s,

    -- UTMI Interface
    XcvrSelect_i     => speed_s,
    HostXcvrSelect_i => speed_s,
    DataOut_i        => DataOut_s,
    TxValid_i        => TxValid_s,
    TxReady_o        => TxReady_s,
    DataIn_o         => DataIn_s,
    RxValid_o        => RxValid_s,
    RxActive_o       => RxActive_s,
    RxError_o        => RxError_s,
    LineState_o      => LineState_s
  );

  xcvr: entity work.usbxcvr_sim
  port map (
    DP        => DP_io,
    DM        => DM_io,
    VP_o      => vp_s,
    VM_o      => vm_s,
    RCV_o     => rcv_s,
    OE_i      => noe_s,
    SOFTCON_i => softcon_s,
    SPEED_i   => speed_s,
    VMO_i     => vmo_dly_s,
    VPO_i     => vpo_dly_s
  );


  pd: entity work.usb1_pd
  port map (
    clk       => clk_s,
    rst       => rst_s,
    rx_data   => DataIn_s,
    rx_valid  => RxValid_s,
    rx_active => RxActive_s,
    rx_err    => RxError_s,

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
		token_fadr  => token_fadr,
    token_endp  => token_endp,
    token_valid => token_valid,
    crc5_err    => crc5_err,
		frame_no    => frame_no,

		-- Receive Data Output
		rx_data_st  => rx_data_st,
    rx_data_valid => rx_data_valid,
    rx_data_done  => rx_data_done,
    crc16_err     => crc16_err,

		-- Misc.
		seq_err       => seq_err,
    rx_busy       => rx_busy
  );

  rstn_s <= not rst_s;

  process
  begin
    softcon_s <= '0';
    speed_s <= '0';
    attachloop: loop
    wait on Cmd_i.Cmd, Cmd_i.Enabled;
    if Cmd_i.Cmd=ATTACH then
      report "USB: Attaching device";
      if Cmd_i.FullSpeed then
        speed_s   <= '1';
      else
        speed_s   <= '0';
      end if;

      softcon_s <= '1';
    end if;
    if Cmd_i.Enabled=false then
      softcon_s <= '0';
    end if;
    end loop;
  end process;

  process
  begin
    if Cmd_i.Enabled then
      wait until RxActive_s='1';
      report "RX start";
      wait until RxActive_s='0';
      report "RX stop";
      
    else
      wait on Cmd_i.Enabled;
    end if;
  end process;

  process
  begin
    wait until token_valid='1';
    if crc5_err='1' then
      report "CRC5 error detected!!!!" severity failure;
    end if;
  end process;

  process
  begin
    wait on token_valid, eop_s;
    if eop_s'event and eop_s='1' then
      if speed_s='0' then
        report "******* KA (low-speed) ********";
        Data_o.SOFStamp <= now;
      end if;
    elsif token_valid'event and token_valid='1' then
    report "******* PID ********";
    if pid_SOF='1' then
      Data_o.SOFStamp <= now;
      report "******* SOF ******** frame=" & str(to_integer(unsigned(frame_no)));
    end if;
    if pid_SETUP='1' then
      report "******* SETUP token ********";
    end if;
    if pid_IN='1' then
      report "******* IN token ********";
    end if;
    if pid_OUT='1' then
      report "******* OUT token ********";
    end if;
    if pid_ACK='1' then
      report "******* ACK token ********";
    end if;
    if pid_PRE='1' then
      report "******* PRE token ********";
    end if;
    end if;
  end process;

  process
  begin
    wait until rising_edge(usb_rst_s);
    Data_o.ResetStamp <= now;
    report "******* USB RESET DETECTED ********";
  end process;

  process
  begin
    wait on crc16_err, rx_data_valid, rx_data_done, token_valid;
    if crc16_err='1' then
      report "CRC16 error detected!!!!";
    else
      if rx_data_valid='1' then
        report "USB data " &hstr(rx_data_st);
      end if;
      if rx_data_done='1' then
        report "USB packet complete";
        sendack <= not sendack after 500 ns;
      end if;
      if pid_IN='1' and token_valid='1' then
        report "USB IN requested, nacking";
        if Cmd_i.AckMode=Nack then
            sendack <= not sendack; 
        else                          
          --   Send packet
            sendpacket <= not sendpacket;
        end if;
      end if;
    end if;
  end process;


  process
    procedure transmit(d: std_logic_vector(7 downto 0)) is
    begin
      TxValid_s <= '1';
      DataOut_s <= d;

      l: loop
        wait until rising_edge(clk_s);
        if TxReady_s'delayed='1' then
          exit l;
        end if;
      end loop;
      TxValid_s <= '0';
    end transmit;

    variable d_v: std_logic_vector(7 downto 0);
    variable c_v  : std_logic_vector(15 downto 0);
    variable txcrc: std_logic_vector(15 downto 0);

  begin
    wait on sendack, sendpacket;

    if sendack'event then
      report "Sending ack/nack/timeout";
--      wait for 500 ns; -- 6-bit times
     -- wait until rx_data_done='1';

      if speed_s='1' then
        wait for 500 ns;
      else
        wait for 8*500 ns;
      end if;


      case Cmd_i.AckMode is
        when Ack =>
          d_v  := genpid(USBF_T_PID_ACK);
        when Nack =>
          d_v  := genpid(USBF_T_PID_NACK);
        when Timeout =>
          null;
      end case;
  
      if Cmd_i.AckMode/=Timeout then
  --      wait for 500 ns; -- 6-bit times
        transmit(d_v);
      end if;
    end if;

    if sendpacket'event then

      report "Transmitting data packet";
      transmit(genpid(USBF_T_PID_DATA1));

      txcrc := x"FFFF";
      wait for 0 ps;
      --report "CRC: " & hstr(txcrc);
      if Cmd_i.TxDataLen > 0  then
      m: for i in 0 to Cmd_i.TxDataLen-1 loop
        transmit( Cmd_i.TxData(i) );
        txcrc := usb_crc16(txcrc, reverse(Cmd_i.TxData(i)) );
        --report "CRC: " & hstr(txcrc) & " data " & hstr(txdata(i));
      end loop;
      end if;
      --report "Final CRC: " & hstr(txcrc);
      c_v := reverse(not(txcrc));
      report "Reversed final CRC: " & hstr(c_v);

      transmit(c_v(7 downto 0));
      transmit(c_v(15 downto 8));

    end if;

  end process;

  vmo_dly_s <= transport vmo_s after 20 ns;
  vpo_dly_s <= vpo_s;

end sim;
