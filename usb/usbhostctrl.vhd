--
--  USB Host FS controller
--
--  Copyright 2016-2020 Alvaro Lopes <alvieboy@alvie.com>
--
--  The FreeBSD license
--
--  Redistribution and use in source and binary forms, with or without
--  modification, are permitted provided that the following conditions
--  are met:
--
--  1. Redistributions of source code must retain the above copyright
--     notice, this list of conditions and the following disclaimer.
--  2. Redistributions in binary form must reproduce the above
--     copyright notice, this list of conditions and the following
--     disclaimer in the documentation and/or other materials
--     provided with the distribution.
--
--  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
--  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
--  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
--  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
--  ZPU PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
--  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
--  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
--  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
--  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
--  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
--  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
--  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--
--


library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_misc.all;
use ieee.numeric_std.all;
library work;
use work.usbpkg.all;
-- synopsys translate_off
use work.txt_util.all;
-- synopsys translate_on

ENTITY usbhostctrl IS
  PORT (
    usbclk_i    : in std_logic;
    ausbrst_i   : in std_logic;

    -- Comms to external world
    clk_i       : in std_logic;
    arst_i      : in std_logic;
    rd_i        : in std_logic;
    wr_i        : in std_logic;
    addr_i      : in std_logic_vector(11 downto 0);
    dat_i       : in std_logic_vector(7 downto 0);
    dat_o       : out std_logic_vector(7 downto 0);

    int_o       : out std_logic; -- sync to clk_i
    int_async_o : out std_logic; -- sync to usb clock
    -- Interface to transceiver
    softcon_o   : out std_logic;
    noe_o       : out std_logic;
    speed_o     : out std_logic;
    vpo_o       : out std_logic;
    vmo_o       : out std_logic;

    rcv_i       : in std_logic;
    vp_i        : in  std_logic;
    vm_i        : in  std_logic;
    pwren_o     : out std_logic;
    pwrflt_i    : in std_logic
  );
END entity usbhostctrl;

ARCHITECTURE rtl OF usbhostctrl is

  SIGNAL  Phy_DataIn     : STD_LOGIC_VECTOR(7 DOWNTO 0);
  SIGNAL  Phy_DataOut    : STD_LOGIC_VECTOR(7 DOWNTO 0);
  SIGNAL  Phy_Linestate  : STD_LOGIC_VECTOR(1 DOWNTO 0);
  SIGNAL  Phy_Opmode     : STD_LOGIC_VECTOR(1 DOWNTO 0);
  SIGNAL  Phy_RxActive   : STD_LOGIC;
  SIGNAL  Phy_RxError    : STD_LOGIC;
  SIGNAL  Phy_RxValid    : STD_LOGIC;
  SIGNAL  Phy_Termselect : STD_LOGIC := 'L';
  SIGNAL  Phy_TxReady    : STD_LOGIC;
  SIGNAL  Phy_TxValid    : STD_LOGIC;
  SIGNAL  Phy_XcvrSelect : STD_LOGIC := 'L';
  SIGNAL  usb_rst_phy    : STD_LOGIC;
  SIGNAL  usb_rst_slv    : STD_LOGIC;

  signal rstinv         : std_logic;
  signal tx_mode_s      : std_logic := '1';
  signal rst_event_q    : std_logic;
  signal rst_event      : std_logic;
  signal noe_s          : std_logic;

  constant C_SOF_TIMEOUT: natural := 4800; -- 1ms

  constant C_ACK_TIMEOUT: natural := 125;-- 2600ns;

  constant C_DEFAULT_ITG : natural := ((3+5)*4); -- 3 bit times, plus the time we need to flush.

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

  type host_state_type is (
    DETACHED,
    ATTACHED,
    IDLE,
    RESET,
    SETTLE,
    SUSPEND,
    RESUME,
    WAIT_SOF,
    IN1,
    IN2,
    IN3,
    SOF1,
    SOF2,
    SOF3,
    CRC1,
    CRC2,
    ITG,
    SETUP1,
    SETUP2,
    SETUP3,
    DATA1,
    DATA2,
    WAIT_ACK_NACK,
    GOT_ACK,
    GOT_NACK,
    ACK_TIMEOUT,
    INVALIDDATA,
    WAIT_DATA,
    CHECKCRC
  );

  type status_reg_type is record
    fulllowspeed    : std_logic; -- '1': FS, '0': LS
    poweron         : std_logic;
    reset           : std_logic;
    suspend         : std_logic;
    overcurrent     : std_logic;
    connectdetect   : std_logic;
    connected       : std_logic;
  end record;


  constant C_ATTACH_DELAY: natural := 4;--480; -- 10 ms
  constant C_NUM_CHANNELS: natural := 8;
  constant C_RESET_DELAY: natural := 500; -- TBD


  type channel_conf_reg_type is record
    enabled         : std_logic;
    oddframe        : std_logic;
    lowspeed        : std_logic;
    direction       : std_logic;
    epnum           : std_logic_vector(3 downto 0);
    address         : std_logic_vector(6 downto 0);
    eptype          : std_logic_vector(1 downto 0); -- 00: Control
                                                    -- 01: Isochronous
                                                    -- 10: Bulk
                                                    -- 11: Interrupt

    maxsize         : std_logic_vector(5 downto 0); -- Max 64 bytes.
  end record;

  type channel_trans_reg_type is record
    dpid            : std_logic_vector(1 downto 0);  -- Data PID.;
                                                     -- 11: setup
                                                     -- 00: IN
    cnt             : unsigned(1 downto 0); -- Limited to 1 packet.
    size            : unsigned(5 downto 0);
    epaddr          : unsigned(10 downto 0);
  end record;                                           

  type channel_interrupt_conf_reg_type is record
    datatogglerror  : std_logic;
    frameoverrun    : std_logic;
    babble          : std_logic;
    transerror      : std_logic; -- CRC check failure
                                 -- Timeout
                                 -- Bit stuff error
                                 -- False EOP
    ack             : std_logic;
    nack            : std_logic;
    stall           : std_logic;
    cplt            : std_logic; -- Completed
  end record;


  type channel_type is record
    conf      : channel_conf_reg_type;
    trans     : channel_trans_reg_type;
    intconf   : channel_interrupt_conf_reg_type;
    intpend   : channel_interrupt_conf_reg_type;
  end record;

  type channels_type is array (0 to C_NUM_CHANNELS-1) of channel_type;

  type regs_type is record
    host_state      : host_state_type;
    itg_next_state  : host_state_type;
    sr              : status_reg_type;
    speed           : std_logic;
    attach_count    : natural range 0 to C_ATTACH_DELAY - 1;
    sof_count       : natural range 0 to C_SOF_TIMEOUT - 1;
    frame           : unsigned(10 downto 0);
    txcrc           : std_logic_vector(15 downto 0);
--    rxcrc           : std_logic_vector(15 downto 0);
    frame_lsb       : std_logic_vector(7 downto 0); -- for SOF packets
    itg             : natural;
    channel         : natural range 0 to C_NUM_CHANNELS -1;
    reset_delay     : natural range 0 to C_RESET_DELAY-1;
    ch              : channels_type;
    size            : unsigned(5 downto 0);
    epmem_addr      : unsigned(10 downto 0);
    rx_timeout      : natural;
  end record;


  signal r                    : regs_type;
  signal attach_event_s       : std_logic;
  signal frame_crc5_s         : std_logic_vector(4 downto 0);
  signal frame_rev_s          : std_logic_vector(10 downto 0);

  signal crc16_in_s           : std_logic_vector(7 downto 0);
  signal crc16_out_s          : std_logic_vector(15 downto 0);
  --signal rxcrc16_out_s        : std_logic_vector(15 downto 0);

  signal statusreg_s          : std_logic_vector(7 downto 0);
  signal statusreg_sync_s     : std_logic_vector(7 downto 0);
  signal write_data_s         : std_logic_vector(7 downto 0);
  signal write_address_s      : std_logic_vector(11 downto 0);

  function inv(value: in std_logic_vector(7 downto 0)) return std_logic_vector is
    variable ret: std_logic_vector(7 downto 0);
  begin
    ret := value xor x"FF";
    return ret;
  end function;

  function reverse(value: in std_logic_vector) return std_logic_vector is
    variable ret: std_logic_vector(value'HIGH downto value'LOW);
  begin
    for i in value'LOW to value'HIGH loop
      ret(i) := value(value'HIGH-i);
    end loop;
    return ret;
  end function;

  signal clr_connectevent_async_s: std_logic;
  signal clr_connectevent_s: std_logic;

  signal set_reset_async_s: std_logic;
  signal set_reset_s: std_logic;

  signal vpo_s, vmo_s: std_logic;


  signal wr_sync_s:  std_logic;

  signal address_ep_crc_in_s: std_logic_vector(10 downto 0);
  signal address_ep_crc_out_s: std_logic_vector(4 downto 0);

  signal debug_ch:  channel_type;

  -- Epmem
  signal epmem_read_en_s    : std_logic;
  signal epmem_write_en_s   : std_logic;
  signal epmem_addr_s       : std_logic_vector(10 downto 0);
  signal epmem_data_in_s    : std_logic_vector(7 downto 0);
  signal epmem_data_out_s   : std_logic_vector(7 downto 0);
  signal hep_dat_s          : std_logic_vector(7 downto 0);
  signal hep_rd_s   : std_logic;
  signal hep_wr_s   : std_logic;


	signal rx_data_st: std_logic_vector(7 downto 0);
    signal rx_data_valid    : std_logic;
    signal rx_data_done     : std_logic;
    signal crc16_err        : std_logic;
		signal seq_err          : std_logic;
    signal rx_busy          : std_logic;
    signal token_valid_s    : std_logic;

  signal pd_resetn_s    :   std_logic;
  signal int_s          : std_logic;
  signal read_data_s          : std_logic_vector(7 downto 0);
  signal read_data_sync_s          : std_logic_vector(7 downto 0);

BEGIN

  rstinv      <= not ausbrst_i;
  
  usb_phy_inst : ENTITY work.usb_phy       --Open Cores USB Phy, designed by Rudolf Usselmanns
  GENERIC MAP (
    usb_rst_det      => TRUE,
    CLOCK            => "48"
  )
  PORT MAP (
    clk              => usbclk_i,       -- i
    rst              => rstinv,         -- i
    phy_tx_mode      => tx_mode_s,      -- i
    usb_rst          => usb_rst_phy,    -- o
    txdp             => vpo_s,          -- o
    txdn             => vmo_s,          -- o
    txoe             => noe_s,          -- o
    rxd              => rcv_i,          -- i
    rxdp             => vp_i,           -- i
    rxdn             => vm_i,           -- i
    DataOut_i        => Phy_DataOut,    -- i (7 downto 0);
    TxValid_i        => Phy_TxValid,    -- i
    TxReady_o        => Phy_TxReady,    -- o
    DataIn_o         => Phy_DataIn,     -- o (7 downto 0);
    RxValid_o        => Phy_RxValid,    -- o
    RxActive_o       => Phy_RxActive,   -- o
    RxError_o        => Phy_RxError,    -- o
    LineState_o      => Phy_LineState   -- o (1 downto 0). (0) is P, (1) is N
  );


  process(usbclk_i, ausbrst_i, r, Phy_Linestate)
    variable w  : regs_type;
    variable ch : channel_type;
    variable wch_u : unsigned(2 downto 0);
    variable wch : natural;
    variable channel_handled: boolean;
    variable can_issue_request: boolean;
    variable interrupt_v: std_logic_vector(C_NUM_CHANNELS-1 downto 0);
  begin
    w := r;

    attach_event_s  <= '0';
    Phy_TxValid     <= '0';
    Phy_DataOut     <= (others => 'X');
    tx_mode_s       <= '1';
    address_ep_crc_in_s <= (others => 'X');
    epmem_read_en_s <= '0';
    epmem_write_en_s <= '0';
    crc16_in_s      <= (others => 'X');
    pd_resetn_s     <= '0';
    -- Optimizations
    -- End optimizations
    if r.sof_count=0 then
      w.sof_count         := C_SOF_TIMEOUT - 1;
      w.frame             := r.frame + 1;
    else
      w.sof_count         := r.sof_count - 1;
    end if;

    if clr_connectevent_s='1' then
      w.sr.connectdetect := '0';
    end if;

    if set_reset_s='1' then
      w.sr.reset := '1';
    end if;

    chint: for i in 0 to C_NUM_CHANNELS-1 loop
      interrupt_v(i) := '0';
      if r.ch(i).intpend.ack='1' then
        interrupt_v(i) := '1';
      end if;
    end loop;

    int_s <= or_reduce(interrupt_v);

    -- Process writes coming from SPI
    if wr_sync_s='1' then
      if write_address_s(11 downto 6) = "000001" then
          wch_u := unsigned(write_address_s(5 downto 3));
          wch := to_integer(wch_u);
          case write_address_s(2 downto 0) is
            when "000" =>
              w.ch(wch).conf.eptype    := write_data_s(7 downto 6);
              w.ch(wch).conf.maxsize   := write_data_s(5 downto 0);
            when "001" =>
              w.ch(wch).conf.oddframe  := write_data_s(7);      
              w.ch(wch).conf.lowspeed  := write_data_s(6);
              w.ch(wch).conf.direction := write_data_s(5);
              w.ch(wch).conf.epnum     := write_data_s(3 downto 0);

            when "010" =>
              w.ch(wch).conf.enabled   := write_data_s(7);
              w.ch(wch).conf.address   := write_data_s(6 downto 0);
            when "011" => -- Interrupt configuration
              w.ch(wch).intconf.datatogglerror  := write_data_s(7);
              w.ch(wch).intconf.frameoverrun    := write_data_s(6);
              w.ch(wch).intconf.babble          := write_data_s(5);
              w.ch(wch).intconf.transerror      := write_data_s(4);
              w.ch(wch).intconf.ack             := write_data_s(3);
              w.ch(wch).intconf.nack            := write_data_s(2);
              w.ch(wch).intconf.stall           := write_data_s(1);
              w.ch(wch).intconf.cplt            := write_data_s(0);
            when "100" => -- Interrupt clear
              if write_data_s(7)='1' then w.ch(wch).intpend.datatogglerror  := '0'; end if;
              if write_data_s(6)='1' then w.ch(wch).intpend.frameoverrun    := '0'; end if;
              if write_data_s(5)='1' then w.ch(wch).intpend.babble          := '0'; end if;
              if write_data_s(4)='1' then w.ch(wch).intpend.transerror      := '0'; end if;
              if write_data_s(3)='1' then w.ch(wch).intpend.ack             := '0'; end if;
              if write_data_s(2)='1' then w.ch(wch).intpend.nack            := '0'; end if;
              if write_data_s(1)='1' then w.ch(wch).intpend.stall           := '0'; end if;
              if write_data_s(0)='1' then w.ch(wch).intpend.cplt            := '0'; end if;
            when "101" =>
              w.ch(wch).trans.dpid := write_data_s(1 downto 0);
            when "110" => -- Transaction
              w.ch(wch).trans.size := unsigned(write_data_s(5 downto 0));
              w.ch(wch).trans.cnt  := unsigned(write_data_s(7 downto 6));
            when others =>
          end case;
       end if;
    end if;

    -- read data
    if write_address_s(11 downto 6) = "000001" then
      wch_u := unsigned(write_address_s(5 downto 3));
      wch := to_integer(wch_u);
      case write_address_s(2 downto 0) is
        --when "000" =>
        --  w.ch(wch).conf.eptype    := write_data_s(7 downto 6);
        --  w.ch(wch).conf.maxsize   := write_data_s(5 downto 0);
        --when "001" =>
        --  w.ch(wch).conf.oddframe  := write_data_s(7);      
        --  w.ch(wch).conf.lowspeed  := write_data_s(6);
        --  w.ch(wch).conf.direction := write_data_s(5);
        --  w.ch(wch).conf.epnum     := write_data_s(3 downto 0);
        --
        --when "010" =>
        --  w.ch(wch).conf.enabled   := write_data_s(7);
        --  w.ch(wch).conf.address   := write_data_s(6 downto 0);
        --when "011" => -- Interrupt configuration
        --  w.ch(wch).intconf.datatogglerror  := write_data_s(7);
        --  w.ch(wch).intconf.frameoverrun    := write_data_s(6);
        --  w.ch(wch).intconf.babble          := write_data_s(5);
        --  w.ch(wch).intconf.transerror      := write_data_s(4);
        --  w.ch(wch).intconf.ack             := write_data_s(3);
        --  w.ch(wch).intconf.nack            := write_data_s(2);
        --  w.ch(wch).intconf.stall           := write_data_s(1);
        --  w.ch(wch).intconf.cplt            := write_data_s(0);
        when "100" => -- Interrupt read
          read_data_s(7)  <= r.ch(wch).intpend.datatogglerror;
          read_data_s(6)  <= r.ch(wch).intpend.frameoverrun;
          read_data_s(5)  <= r.ch(wch).intpend.babble;
          read_data_s(4)  <= r.ch(wch).intpend.transerror;
          read_data_s(3)  <= r.ch(wch).intpend.ack;
          read_data_s(2)  <= r.ch(wch).intpend.nack;
          read_data_s(1)  <= r.ch(wch).intpend.stall;
          read_data_s(0)  <= r.ch(wch).intpend.cplt;
        when others =>
         read_data_s <= statusreg_s;
      end case;
    else
         read_data_s <= statusreg_s;
    end if;

    ch := r.ch( r.channel );
    debug_ch <= ch;

    case r.host_state is
      when DETACHED =>
        if Phy_Linestate="01" or Phy_Linestate="10" then
          if r.attach_count=0 then
            w.sr.fulllowspeed   := Phy_Linestate(0); -- Set speed according to USB+ pullup
            w.host_state        := ATTACHED;
            attach_event_s      <= '1';
            w.sr.connectdetect  :='1';
            w.sr.connected      :='1';
            w.sof_count         := C_SOF_TIMEOUT - 1;
          else
            w.attach_count      := r.attach_count - 1;
          end if;
        else
          w.attach_count        := C_ATTACH_DELAY - 1;
          w.sr.connectdetect    :='0';
          w.sr.connected        :='0';
        end if;

      when ATTACHED | IDLE =>

        channel_handled   := false;
        can_issue_request := true;
        --
        -- sync   pid  +packet  crc
        --
        -- 8    + 8    598[s]  + 2    (616)

        --34 bits time for ack/nack . 38 bits for token.
        if r.sof_count > ((616+34+38)*4) then
          can_issue_request := true;
        end if;

        if ch.conf.enabled='1' then
          -- synopsys translate_off
          if rising_edge(usbclk_i) then
            report "Channel "&str(r.channel)&" enabled";
            if ch.trans.cnt/="00" and can_issue_request then
              if ch.trans.dpid="11" then
                w.host_state := SETUP1;
                channel_handled := true;
              elsif ch.trans.dpid="00" then
                w.host_state := IN1;
                channel_handled := true;
              end if;
            end if;
          end if;
          -- synopsys translate_on
        end if;

        if r.sr.reset='1' then
          w.host_state := RESET;
          w.reset_delay := C_RESET_DELAY -1;
        end if;
        if usb_rst_phy='1' then
          -- Disconnected???
          w.sr.connectdetect := '1';
          w.sr.connected := '0';
          w.host_state := DETACHED;
        end if;

        if not channel_handled then
          if r.channel /= C_NUM_CHANNELS-1 then
            w.channel := r.channel + 1;
          else
            w.channel := 0;
            w.host_state := WAIT_SOF;
          end if;
        end if;

      when WAIT_SOF =>
        if r.sof_count=0 then
          w.host_state := SOF1;
        end if;
        if r.sr.reset='1' then
          w.host_state := RESET;
          w.reset_delay := C_RESET_DELAY -1;
        end if;
        if usb_rst_phy='1' then
          -- Disconnected???
          w.sr.connectdetect := '1';
          w.sr.connected := '0';
          w.host_state := DETACHED;
        end if;


      when SOF1 =>
        Phy_TxValid   <= '1';
        Phy_DataOut   <= "10100101";

        if Phy_TxReady='1' then
          w.host_state := SOF2;
        end if;

      when SOF2 =>
        Phy_TxValid   <= '1';
        Phy_DataOut   <= std_logic_vector(r.frame(7 downto 0));

        if Phy_TxReady='1' then
          w.host_state := SOF3;
        end if;

      when SOF3 =>
        Phy_TxValid   <= '1';
        Phy_DataOut   <= r.frame_lsb;

        if Phy_TxReady='1' then
          w.itg_next_state := IDLE;
          w.host_state := ITG;
        end if;

      when RESET =>
        tx_mode_s <= '0'; -- Single-ended
        --Phy_DataOut <= "10101010";
        --Phy_TxValid <= '1';
        if r.reset_delay = 0 then
          w.sr.reset := '0';
          w.itg := 3;
          w.itg_next_state := IDLE;
          w.host_state := ITG;
        else
          w.reset_delay := r.reset_delay-1;
        end if;

      when SETTLE =>
        w.host_state := IDLE;

      when CRC1 =>
        Phy_DataOut <= reverse(inv(r.txcrc(15 downto 8)));
        Phy_TxValid <='1';
        if Phy_TxReady='1' then
          w.host_state := CRC2;
        end if;

      when CRC2 =>
        Phy_DataOut <= reverse(inv(r.txcrc(7 downto 0)));
        Phy_TxValid <='1';
        if Phy_TxReady='1' then
          w.itg := 3;
          --w.itg_next_state := IDLE;
          w.rx_timeout := C_ACK_TIMEOUT;
          w.host_state := WAIT_ACK_NACK;
        end if;

      when ITG  =>
        if noe_s='0' then
          w.itg := 3;
        else
          if r.itg=0 then
            w.host_state := r.itg_next_state;
          else
            w.itg := w.itg - 1;
          end if;
        end if;

      when SETUP1 =>
       -- Send SETUP token.
        Phy_TxValid   <= '1';
        Phy_DataOut   <= genpid(USBF_T_PID_SETUP);
        if Phy_TxReady='1' then
          w.host_state := SETUP2;
        end if;

      when IN1 =>
       -- Send SETUP token.
        Phy_TxValid   <= '1';
        Phy_DataOut   <= genpid(USBF_T_PID_IN);
        if Phy_TxReady='1' then
          w.host_state := IN2;
        end if;

      when IN2 =>
        Phy_TxValid   <= '1';
        Phy_DataOut   <= ch.conf.epnum(0) & ch.conf.address(6 downto 0);

        if Phy_TxReady='1' then
          w.host_state := IN3;
        end if;

      when IN3 =>
        Phy_TxValid   <= '1';
        address_ep_crc_in_s <= reverse(  ch.conf.address & ch.conf.epnum );

        Phy_DataOut   <= address_ep_crc_out_s & ch.conf.epnum(3 downto 1) ;

        if Phy_TxReady='1' then
          w.host_state := ITG;
          w.itg := 3;
          w.size := ch.trans.size;
          w.rx_timeout := C_ACK_TIMEOUT;
          w.itg_next_state := WAIT_DATA; -- Wrong.
          w.epmem_addr  := ch.trans.epaddr;
        end if;

      when SETUP2 =>
        Phy_TxValid   <= '1';
        Phy_DataOut   <= ch.conf.epnum(0) & ch.conf.address(6 downto 0);

        if Phy_TxReady='1' then
          w.host_state := SETUP3;
        end if;

      when SETUP3 =>
        Phy_TxValid   <= '1';
        address_ep_crc_in_s <= reverse(  ch.conf.address & ch.conf.epnum );

        Phy_DataOut   <= address_ep_crc_out_s & ch.conf.epnum(3 downto 1) ;

        if Phy_TxReady='1' then
          w.host_state := ITG;
          w.itg := 3;
          w.size := ch.trans.size;
          w.itg_next_state := DATA1;
          w.epmem_addr  := ch.trans.epaddr;
        end if;

      when DATA1 =>
        Phy_DataOut   <= genpid(USBF_T_PID_DATA0); -- TBD: Data0/1
        Phy_TxValid   <= '1';
        w.txcrc       := x"FFFF";

        if Phy_TxReady='1' then
          w.host_state := DATA2;
          epmem_read_en_s <= '1';
          w.epmem_addr := r.epmem_addr +1;
        end if;

      when DATA2 =>
        Phy_DataOut   <= epmem_data_out_s;
        crc16_in_s    <= reverse(epmem_data_out_s);

        Phy_TxValid   <= '1';

        if Phy_TxReady='1' then
          w.txcrc       := crc16_out_s;
          if r.size=0 then
            w.host_state := CRC1;
            --w.itg_next_state := WAIT_ACK;
          else
            w.size := r.size - 1;
            w.host_state := DATA2;
            epmem_read_en_s <= '1';
            w.epmem_addr := r.epmem_addr +1;
          end if;
        end if;

      when WAIT_ACK_NACK =>
        pd_resetn_s     <= '1';
        if Phy_RxActive='0' then
          if r.rx_timeout=0 then
            -- timeout
            w.host_state := ACK_TIMEOUT;
          else
            w.rx_timeout := r.rx_timeout-1;
          end if;
        end if;

        if token_valid_s='1' then
          if pid_ACK='1' then
            w.host_state := GOT_ACK;
          elsif pid_NACK='1' then
            w.host_state := GOT_NACK;
          else
            w.host_state := INVALIDDATA;
          end if;
        end if;
        if Phy_RxError='1' then
          w.host_state := INVALIDDATA;
        end if;

      when GOT_ACK =>
        -- synthesis translate_off
        if rising_edge(usbclk_i) then
          report "got ACK";
        end if;
        -- synthesis translate_on
        w.ch(r.channel).trans.cnt := "00";
        w.itg         := C_DEFAULT_ITG;
        w.host_state  := ITG;
        w.itg_next_state := IDLE;
        w.ch(r.channel).intpend.ack   := '1';
        w.ch(r.channel).intpend.cplt  := '1';

      when GOT_NACK =>
        -- synthesis translate_off
        if rising_edge(usbclk_i) then
          report "got NACK";
        end if;
        -- synthesis translate_on
        w.itg_next_state  := IDLE;
        w.itg             := C_DEFAULT_ITG;
        w.host_state      := ITG; -- Retry

      when ACK_TIMEOUT =>
        -- synthesis translate_off
        if rising_edge(usbclk_i) then
          report "Timeout waiting for data";
        end if;
        -- synthesis translate_on
        w.ch(r.channel).trans.cnt := "00";
        w.host_state := IDLE;
        -- TBD: we need a counter!



      when WAIT_DATA =>
        pd_resetn_s     <= '1';

        --w.size        := (others => '0'); --ch.trans.size;
        w.ch(r.channel).trans.size := (others => '0');--w.ch(r.channel).trans.size +1;
        w.epmem_addr  := ch.trans.epaddr;

        if Phy_RxActive='0' then
          if r.rx_timeout=0 then
            -- timeout
            w.host_state := ACK_TIMEOUT;
          else
            w.rx_timeout := r.rx_timeout-1;
          end if;
        end if;

        if token_valid_s='1' then
          if pid_NACK='1' then
            w.host_state := GOT_NACK;
          else
            w.host_state := INVALIDDATA;
          end if;
        end if;

        if rx_data_done='1' then
          w.host_state := CHECKCRC;
        else
          if rx_data_valid='1' then
            w.ch(r.channel).trans.size := w.ch(r.channel).trans.size +1;
          end if;
        end if;

      when CHECKCRC =>
        if crc16_err='1' then
          report "CRC16 error!!!!";
        else
        end if;

      when others =>
        report "INVALID STATE";

    end case;

    w.frame_lsb   := frame_crc5_s & r.frame(10) & r.frame(9) & r.frame(8)  ;



    if ausbrst_i='1' then
      --Phy_TxValid         <= '0';
      r.host_state        <= DETACHED;
      r.frame             <= (others => '0');
      r.sof_count         <= C_SOF_TIMEOUT - 1;
      r.attach_count      <= C_ATTACH_DELAY - 1;
      r.sr.poweron        <= '0';
      r.sr.overcurrent    <= '0';
      r.sr.connectdetect  <= '0';
      r.sr.connected      <= '0';
      r.sr.reset          <= '0';

      chc: for i in 0 to C_NUM_CHANNELS-1 loop
        r.ch(i).conf.enabled <= '0';
        r.ch(i).trans.epaddr <= (others => '0');
      end loop;

    elsif rising_edge(usbclk_i) then
      r <= w;
    end if;
  end process;

  frame_rev_s <=reverse( std_logic_vector(r.frame) );


  frame_crc_inst: entity work.usb1_crc5
  generic map (
    invert_and_reverse_output => true
  )
  port map (
	  crc_in => "11111",
	  din => frame_rev_s,
	  crc_out => frame_crc5_s
  );

  address_ep_inst: entity work.usb1_crc5
  generic map (
    invert_and_reverse => true
  )
  port map (
	  crc_in => "11111",
	  din => address_ep_crc_in_s,
	  crc_out => address_ep_crc_out_s
  );

  data_crc_inst: entity work.usb1_crc16
  port map (
	  crc_in  => r.txcrc,
	  din     => crc16_in_s,
	  crc_out => crc16_out_s
  );

  --data_crc_inst: entity work.usb1_crc16
  --port map (
	--  crc_in  => r.rxcrc,
	--  din     => Phy_DataIn,
	--  crc_out => rxcrc16_out_s
  --);

  -- Status reg
  statusreg_s <= '0' &
    r.sr.fulllowspeed & 
    r.sr.poweron      & 
    r.sr.reset        &
    '0' & --r.sr.suspend       &
    r.sr.overcurrent   &
    r.sr.connectdetect &
    r.sr.connected;

  --sr_sync: entity work.syncv
  --  generic map ( WIDTH => 8, RESET => '0' )
  --  port map (
  --    clk_i   => clk_i,
  --    arst_i  => arst_i,
  --    din_i   => statusreg_s,
  --    dout_o  => statusreg_sync_s
  --  );

  clr_connectevent_async_s  <= '1' when addr_i="0000000000" and wr_i='1' and dat_i(1)='1' else '0';
  set_reset_async_s  <= '1' when addr_i="0000000000" and wr_i='1' and dat_i(4)='1' else '0';

  -- Pass on the write request
  wr_rq_sync: entity work.async_pulse2
    generic map (
      WIDTH => 4
    )
    port map (
      clki_i  => clk_i,
      arst_i  => arst_i,
      clko_i  => usbclk_i,
      pulse_i => wr_i,
      pulse_o => wr_sync_s
    );
  -- Pass on write data
  data_sync: entity work.syncv
    generic map (
      WIDTH => 12,
      RESET => 'X'
    )
    port map (
      arst_i  => arst_i,
      clk_i   => usbclk_i,
      din_i   => addr_i,
      dout_o  => write_address_s
    );

  -- Pass on data write from SPI
  addr_sync: entity work.syncv
    generic map (
      WIDTH => 8,
      RESET => 'X'
    )
    port map (
      arst_i  => arst_i,
      clk_i   => usbclk_i,
      din_i   => dat_i,
      dout_o  => write_data_s
    );

  -- Pass on data read (to SPI)
  dread_sync: entity work.syncv
    generic map (
      WIDTH => 8,
      RESET => 'X'
    )
    port map (
      arst_i  => arst_i,
      clk_i   => clk_i,
      din_i   => read_data_s,
      dout_o  => read_data_sync_s
    );


  int_sync: entity work.sync
    generic map (
      RESET => '0'
    )
    port map (
      arst_i  => arst_i,
      clk_i   => clk_i,
      din_i   => int_s,
      dout_o  => int_o
    );

  clr_connectevent: entity work.async_pulse2
    port map (
      clki_i  => clk_i,
      arst_i  => arst_i,
      clko_i  => usbclk_i,
      pulse_i => clr_connectevent_async_s,
      pulse_o => clr_connectevent_s
    );

  set_reset_sync: entity work.async_pulse2
    port map (
      clki_i  => clk_i,
      arst_i  => arst_i,
      clko_i  => usbclk_i,
      pulse_i => set_reset_async_s,
      pulse_o => set_reset_s
    );

  hep_rd_s <= rd_i and addr_i(11);
  hep_wr_s <= wr_i and addr_i(11);

  epmem_inst: entity work.usb_epmem
    port map (
      uclk_i    => usbclk_i,
      urd_i     => epmem_read_en_s,
      uwr_i     => epmem_write_en_s,
      uaddr_i   => epmem_addr_s,
      udata_o   => epmem_data_out_s,
      udata_i   => epmem_data_in_s,

      hclk_i    => clk_i,
      hrd_i     => hep_rd_s,
      hwr_i     => hep_wr_s,
      haddr_i   => addr_i(10 downto 0),
      hdata_o   => hep_dat_s,
      hdata_i   => dat_i
  );

  pd: entity work.usb1_pd
  port map (
    clk       => usbclk_i,
    rst       => pd_resetn_s,
    rx_data   => Phy_DataIn,
    rx_valid  => Phy_RxValid,
    rx_active => Phy_RxActive,
    rx_err    => Phy_RxError,

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



  epmem_addr_s <= std_logic_vector(r.epmem_addr);

  speed_o     <= r.speed;
  softcon_o   <= '0';
  noe_o       <= '0' when r.host_state=RESET else noe_s;
  vpo_o       <= '0' when r.host_state=RESET else vpo_s;
  vmo_o       <= '0' when r.host_state=RESET else vmo_s;
  int_async_o <= int_s;

  dat_o <= hep_dat_s when addr_i(11)='1' else read_data_s;

END rtl;

