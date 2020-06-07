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
    addr_i      : in std_logic_vector(10 downto 0);
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
    pwrflt_i    : in std_logic;
    dbg_o       : out std_logic_vector(7 downto 0)
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

  signal dbg_rx_data_done_s : std_logic;

  constant C_SOF_TIMEOUT: natural   := altsim(48000, 4800); -- 1ms synth, 100us simulation
  constant C_ATTACH_DELAY: natural  := altsim(480000, 4);-- 48000; -- 10 ms
  constant C_NUM_CHANNELS: natural  := 8;
  constant C_RESET_DELAY: natural   := altsim(48000*50,500); -- 50 ms
  constant C_RESET_DELAY_AFTER: natural   := altsim(480, 48); -- 10 us
  constant C_INTERRUPT_HOLDOFF : natural := altsim(4800*5, 480); -- 500us / 10us

  type host_state_type is (
    DETACHED,
    ATTACHED,
    IDLE,
    RESET1,
    RESET2,
    --SUSPEND,
    --RESUME,
    WAIT_SOF,
    IN1,
    OUT1,
    SOF1,
    SETUP1
  );

  type status_reg_type is record
    fulllowspeed    : std_logic; -- '1': FS, '0': LS
    poweron         : std_logic;
    reset           : std_logic;
    suspend         : std_logic;
    overcurrent     : std_logic;
    --connectdetect   : std_logic;
    connected       : std_logic;
  end record;

  type interrupt_reg_type is record
    overcurrent     : std_logic; -- Interrupt on overcurrent
    connectdetect   : std_logic; -- Interrupt on connect event
    disconnectdetect: std_logic; -- Interrupt on disconnect event
    ginten          : std_logic; -- Global USB interrupt enable
  end record;

  constant C_EP_TYPE_INTERRUPT: std_logic_vector(1 downto 0) := "11";

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
    interval        : std_logic_vector(7 downto 0); -- for interrupt endpoints
  end record;

  type channel_trans_reg_type is record
    dpid            : std_logic_vector(1 downto 0);  -- Data PID.;
                                                     -- 11: setup
                                                     -- 00: IN
                                                     -- 01: OUT
    cnt             : std_logic; -- Limited to 1 packet.
    seq             : std_logic;
    size            : unsigned(6 downto 0);
    epaddr          : unsigned(9 downto 0);
    retries         : unsigned(1 downto 0);
    intervalcnt     : unsigned(7 downto 0); -- for interrupt endpoints
    issued          : std_logic;
  end record;                                           

  type channel_interrupt_conf_reg_type is record
    datatogglerror  : std_logic;
    crcerror    : std_logic;
    babble          : std_logic;
    transerror      : std_logic; -- Timeout
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
    sr              : status_reg_type;
    intconfr        : interrupt_reg_type;
    intpendr        : interrupt_reg_type;
    speed           : std_logic;
    attach_count    : natural range 0 to C_ATTACH_DELAY - 1;
    sof_count       : natural range 0 to C_SOF_TIMEOUT - 1;
    frame           : unsigned(10 downto 0);
    channel         : natural range 0 to C_NUM_CHANNELS -1;
    reset_delay     : natural range 0 to C_RESET_DELAY-1;
    ch              : channels_type;
    int_holdoff     : natural range 0 to C_INTERRUPT_HOLDOFF-1;
  end record;

  signal r                    : regs_type;
  --signal frame_crc5_s         : std_logic_vector(4 downto 0);

  signal statusreg_s          : std_logic_vector(7 downto 0);
  --signal intconfreg_s         : std_logic_vector(7 downto 0);
  signal intpendreg_s         : std_logic_vector(7 downto 0);

  signal write_data_s         : std_logic_vector(7 downto 0);
  signal write_address_s      : std_logic_vector(10 downto 0);


  signal vpo_s, vmo_s: std_logic;


  signal wr_sync_s:  std_logic;
  signal rd_sync_s:  std_logic;

  signal address_ep_crc_in_s: std_logic_vector(10 downto 0);
  signal address_ep_crc_out_s: std_logic_vector(4 downto 0);

  signal debug_ch:  channel_type;

  -- Epmem
  signal epmem_read_en_s    : std_logic;
  signal epmem_write_en_s   : std_logic;
  signal epmem_addr_s       : std_logic_vector(9 downto 0);
  signal epmem_data_in_s    : std_logic_vector(7 downto 0);
  signal epmem_data_out_s   : std_logic_vector(7 downto 0);
  signal hep_dat_s          : std_logic_vector(7 downto 0);
  signal hep_rd_s   : std_logic;
  signal hep_wr_s   : std_logic;



  signal int_s                : std_logic;
  signal read_data_s          : std_logic_vector(7 downto 0);
  signal read_data_sync_s     : std_logic_vector(7 downto 0);

  signal trans_addr_s       : std_logic_vector(6 downto 0);
  signal trans_dsize_s      : std_logic_vector(6 downto 0);
  signal trans_dsize_read_s : std_logic_vector(6 downto 0);
  signal trans_daddr_s      : std_logic_vector(9 downto 0);
  signal trans_ep_s         : std_logic_vector(3 downto 0);
  signal trans_status_s     : usb_transaction_status_type;
  signal trans_pid_s        : std_logic_vector(3 downto 0);
  signal trans_strobe_s     : std_logic;
  signal trans_data_seq_s   : std_logic;
  signal trans_seq_valid_s  : std_logic;
  signal phy_txactive_s     : std_logic;
  signal fs_ce_s            : std_logic;
	signal dbg_fs_ce_r		: std_logic;
BEGIN

  rstinv      <= not ausbrst_i;

  statusreg_s <= '0' &
    r.sr.fulllowspeed & 
    r.sr.poweron      & 
    r.sr.reset        &
    '0' & --r.sr.suspend       &
    r.sr.overcurrent   &
    '0' &
    r.sr.connected;

  --intconfreg_s <= r.intconfr.ginten & "0000" & r.intconfr.overcurrent & r.intconfr.connectdetect & r.intconfr.disconnectdetect;
  intpendreg_s <= "00000" & r.intpendr.overcurrent & r.intpendr.connectdetect & r.intpendr.disconnectdetect;


  
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
    XcvrSelect_i     => r.speed,        -- i
    fs_ce_o          => fs_ce_s,
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

  phy_txactive_s <= not noe_s;

  process(usbclk_i, ausbrst_i, r, Phy_Linestate, wr_sync_s, write_address_s, write_data_s, statusreg_s, intpendreg_s,
    usb_rst_phy, trans_status_s, trans_dsize_read_s, pwrflt_i)
    variable w  : regs_type;
    variable ch : channel_type;
    variable wch_u : unsigned(2 downto 0);
    variable wch : natural;
    variable channel_handled: boolean;
    variable can_issue_request: boolean;
    variable interrupt_v: std_logic_vector(C_NUM_CHANNELS-1 downto 0);
  begin
    w := r;
    tx_mode_s       <= '1';
    -- Optimizations
    trans_pid_s     <= (others => 'X');
    trans_strobe_s  <= '0';
    trans_addr_s    <= (others => 'X');
    trans_dsize_s   <= (others => 'X');
    trans_daddr_s   <= (others => 'X');
    trans_ep_s      <= (others => 'X');
    trans_data_seq_s<='X';
    --trans_addr_s    <= (others => 'X');
    read_data_s     <= (others => '0');

    -- End optimizations
    if r.sof_count=0 then
      w.sof_count         := C_SOF_TIMEOUT - 1;
      w.frame             := r.frame + 1;
    else
      w.sof_count         := r.sof_count - 1;
    end if;

    chint: for i in 0 to C_NUM_CHANNELS-1 loop
      interrupt_v(i) := '0';
      if  r.ch(i).intpend.datatogglerror = '1' or
          r.ch(i).intpend.crcerror       = '1' or
          r.ch(i).intpend.babble         = '1' or
          r.ch(i).intpend.transerror     = '1' or
          r.ch(i).intpend.ack            = '1' or
          r.ch(i).intpend.nack           = '1' or
          r.ch(i).intpend.stall          = '1' or
          r.ch(i).intpend.cplt           = '1'
         then
        interrupt_v(i) := '1';
      end if;
    end loop;

    if r.int_holdoff=0 then
      int_s <= (or_reduce(interrupt_v) or r.intpendr.connectdetect or r.intpendr.overcurrent) and r.intconfr.ginten;
    else
      int_s <= '0';
      w.int_holdoff := r.int_holdoff - 1;
    end if;

    -- Process writes coming from SPI
    if wr_sync_s='1' then
      if write_address_s(10 downto 7) = "0000" then
        case write_address_s(6 downto 0) is
          when "0000000" =>
            w.sr.poweron               := write_data_s(5);
            if write_data_s(4)='1' then w.sr.reset := '1'; end if;

          when "0000010" =>  -- Interrupt conf reg
            w.intconfr.ginten         := write_data_s(7);
            w.intconfr.disconnectdetect  := write_data_s(0);
            w.intconfr.connectdetect  := write_data_s(1);
            w.intconfr.overcurrent    := write_data_s(2);
          when "0000011" =>
            -- Interrupt clear/ack
            if write_data_s(0)='1' then w.intpendr.disconnectdetect := '0'; end if;
            if write_data_s(1)='1' then w.intpendr.connectdetect := '0'; end if;
            if write_data_s(2)='1' then w.intpendr.overcurrent   := '0'; end if;
            if write_data_s(7)='1' then w.int_holdoff:=C_INTERRUPT_HOLDOFF-1; end if;
    
          when others =>
        end case;
      elsif write_address_s(10 downto 7) = "0001" then
          wch_u := unsigned(write_address_s(6 downto 4));
          wch := to_integer(wch_u);
          case write_address_s(3 downto 0) is
            when "0000" =>
              w.ch(wch).conf.eptype    := write_data_s(7 downto 6);
              w.ch(wch).conf.maxsize   := write_data_s(5 downto 0);
            when "0001" =>
              --w.ch(wch).conf.oddframe  := write_data_s(7);
              --w.ch(wch).conf.lowspeed  := write_data_s(6);
              w.ch(wch).conf.direction := write_data_s(7);
              w.ch(wch).conf.epnum     := write_data_s(3 downto 0);

            when "0010" =>
              w.ch(wch).conf.enabled   := write_data_s(7);
              w.ch(wch).conf.address   := write_data_s(6 downto 0);
            when "0011" => -- Interrupt configuration
              w.ch(wch).intconf.datatogglerror  := write_data_s(7);
              w.ch(wch).intconf.crcerror    := write_data_s(6);
              w.ch(wch).intconf.babble          := write_data_s(5);
              w.ch(wch).intconf.transerror      := write_data_s(4);
              w.ch(wch).intconf.ack             := write_data_s(3);
              w.ch(wch).intconf.nack            := write_data_s(2);
              w.ch(wch).intconf.stall           := write_data_s(1);
              w.ch(wch).intconf.cplt            := write_data_s(0);
            when "0100" => -- Interrupt clear
              if write_data_s(7)='1' then w.ch(wch).intpend.datatogglerror  := '0'; end if;
              if write_data_s(6)='1' then w.ch(wch).intpend.crcerror        := '0'; end if;
              if write_data_s(5)='1' then w.ch(wch).intpend.babble          := '0'; end if;
              if write_data_s(4)='1' then w.ch(wch).intpend.transerror      := '0'; end if;
              if write_data_s(3)='1' then w.ch(wch).intpend.ack             := '0'; end if;
              if write_data_s(2)='1' then w.ch(wch).intpend.nack            := '0'; end if;
              if write_data_s(1)='1' then w.ch(wch).intpend.stall           := '0'; end if;
              if write_data_s(0)='1' then w.ch(wch).intpend.cplt            := '0'; end if;

            when "0101" =>
              w.ch(wch).conf.interval       := write_data_s;
              w.ch(wch).trans.intervalcnt   := unsigned(write_data_s); -- restart counter.
            when "1000" =>
              w.ch(wch).trans.dpid    := write_data_s(1 downto 0);
              w.ch(wch).trans.seq     := write_data_s(2);
              w.ch(wch).trans.epaddr(9 downto 8)  := unsigned(write_data_s(4 downto 3));
              w.ch(wch).trans.retries := unsigned(write_data_s(6 downto 5));

            when "1001" => -- Transaction
              w.ch(wch).trans.epaddr(7 downto 0)   := unsigned(write_data_s);

            when "1010" => -- Transaction
              w.ch(wch).trans.size := unsigned(write_data_s(6 downto 0));
              w.ch(wch).trans.cnt  := write_data_s(7);
            when others =>
          end case;
      end if;
    end if;

    -- read data
    if not is_x(write_address_s) then

    if write_address_s(10 downto 7) = "0000" then
      case write_address_s(6 downto 0) is
        when "0000000" =>
          read_data_s <= statusreg_s;
        when "0000001" =>
          -- channel interrupt pending reg
          read_data_s <= intpendreg_s;
        when "0000010" =>
          --  interrupt status reg
          read_data_s <= interrupt_v;
        when others =>
          case trans_status_s is
            when IDLE       =>  read_data_s <= x"00";
            when BUSY       =>  read_data_s <= x"01";
            when TIMEOUT    =>  read_data_s <= x"02";
            when BABBLE     =>  read_data_s <= x"03";
            when ACK        =>  read_data_s <= x"04";
            when NACK       =>  read_data_s <= x"05";
            when STALL      =>  read_data_s <= x"06";
            when CRCERROR   =>  read_data_s <= x"07";
            when COMPLETED  =>  read_data_s <= x"08";
            when others     =>  read_data_s <= x"FF";
          end case;
      end case;

    elsif write_address_s(10 downto 7) = "0001" then
      wch_u := unsigned(write_address_s(6 downto 4));
      wch := to_integer(wch_u);
      case write_address_s(3 downto 0) is
        when "0000" =>
          read_data_s(7 downto 6) <= r.ch(wch).conf.eptype;
          read_data_s(5 downto 0) <= r.ch(wch).conf.maxsize;
        when "0001" =>
          --read_data_s(7)          <= r.ch(wch).conf.oddframe;
          --read_data_s(6)          <= r.ch(wch).conf.lowspeed;
          read_data_s(7)          <= r.ch(wch).conf.direction;
          read_data_s(3 downto 0) <= r.ch(wch).conf.epnum;
        --
        when "0010" =>
          read_data_s(7)          <= r.ch(wch).conf.enabled;
          read_data_s(6 downto 0) <= r.ch(wch).conf.address;
        when "0011" => -- Interrupt configuration
          read_data_s(7)          <= r.ch(wch).intconf.datatogglerror;
          read_data_s(6)          <= r.ch(wch).intconf.crcerror;
          read_data_s(5)          <= r.ch(wch).intconf.babble;
          read_data_s(4)          <= r.ch(wch).intconf.transerror;
          read_data_s(3)          <= r.ch(wch).intconf.ack;
          read_data_s(2)          <= r.ch(wch).intconf.nack;
          read_data_s(1)          <= r.ch(wch).intconf.stall;
          read_data_s(0)          <= r.ch(wch).intconf.cplt;
        when "0100" => -- Interrupt read
          read_data_s(7)          <= r.ch(wch).intpend.datatogglerror;
          read_data_s(6)          <= r.ch(wch).intpend.crcerror;
          read_data_s(5)          <= r.ch(wch).intpend.babble;
          read_data_s(4)          <= r.ch(wch).intpend.transerror;
          read_data_s(3)          <= r.ch(wch).intpend.ack;
          read_data_s(2)          <= r.ch(wch).intpend.nack;
          read_data_s(1)          <= r.ch(wch).intpend.stall;
          read_data_s(0)          <= r.ch(wch).intpend.cplt;
        when "0101" =>
          read_data_s             <= r.ch(wch).conf.interval;
        when "1000" =>
          read_data_s(1 downto 0) <= r.ch(wch).trans.dpid;
          read_data_s(2)          <= r.ch(wch).trans.seq;
          read_data_s(4 downto 3) <= std_logic_vector(r.ch(wch).trans.epaddr(9 downto 8));
          read_data_s(6 downto 5) <= std_logic_vector(r.ch(wch).trans.retries);
        when "1001" => -- Transaction
          read_data_s             <= std_logic_vector(r.ch(wch).trans.epaddr(7 downto 0));
        when "1010" => -- Transaction
          read_data_s(6 downto 0) <= std_logic_vector(r.ch(wch).trans.size);
          read_data_s(7)          <= r.ch(wch).trans.cnt;
        when others =>
          read_data_s <= (others => 'X');
        end case;
    end if;
    end if;

    ch := r.ch( r.channel );
    debug_ch <= ch;

    case r.host_state is
      when DETACHED =>
        if Phy_Linestate="01" or Phy_Linestate="10" then
          if r.sr.poweron='1' then
          if r.attach_count=0 then
            w.sr.fulllowspeed   := Phy_Linestate(0); -- Set speed according to USB+ pullup
            w.speed := Phy_Linestate(0); -- Set speed according to USB+ pullup
            w.host_state        := ATTACHED;
            --w.sr.connectdetect  :='1';
            w.sr.connected      :='1';
            if r.intconfr.connectdetect='1' then
              w.intpendr.connectdetect:='1';
            end if;
            w.sof_count         := C_SOF_TIMEOUT - 1;
          else
            w.attach_count      := r.attach_count - 1;
          end if;
          end if;
        else
          w.attach_count        := C_ATTACH_DELAY - 1;
          --w.sr.connectdetect    :='0';
          w.sr.connected        :='0';
        end if;

      when ATTACHED | IDLE =>

        channel_handled   := false;
        can_issue_request := false;
        --
        -- sync   pid  +packet  crc
        --
        -- 8    + 8    598[s]  + 2    (616)

        --34 bits time for ack/nack . 38 bits for token.
        if r.speed='1' then
          if r.sof_count > ((616+34+38)*4) then
            can_issue_request := true;
          end if;
        else
          if r.sof_count > ((616+34+38)*4*8) then
            can_issue_request := true;
          end if;
        end if;
        -- synthesis translate_off
          if r.sof_count > 100 then
            can_issue_request := true;
          end if;
        -- synthesis translate_on

        if ch.conf.enabled='1' then
          -- synopsys translate_off
          if rising_edge(usbclk_i) then
            report "Channel "&str(r.channel)&" enabled";
          end if;
          -- synopsys translate_on

          if ch.conf.eptype = C_EP_TYPE_INTERRUPT and (ch.trans.intervalcnt/=0 or ch.trans.issued='1')then
            can_issue_request := false;
          end if;

          if ch.trans.cnt/='0' and can_issue_request then
            if ch.trans.dpid="11" then
              report "Got setup request";
              w.host_state := SETUP1;
              channel_handled := true;
            elsif ch.trans.dpid="00" then
              w.host_state := IN1;
              channel_handled := true;
            elsif ch.trans.dpid="01" then
              w.host_state := OUT1;
              channel_handled := true;
            end if;
          end if;
        end if;

        if r.sr.reset='1' then
          w.host_state := RESET1;
          w.reset_delay := C_RESET_DELAY -1;
        end if;

        if not channel_handled then
          if r.channel /= C_NUM_CHANNELS-1 then
            w.channel := r.channel + 1;
          else
            w.channel := 0;
            if not can_issue_request then
              w.host_state := WAIT_SOF;
            end if;
          end if;
        end if;

        if r.sr.poweron='0' then
          w.host_state := DETACHED;
        end if;

      when WAIT_SOF =>
        if r.sof_count=0 then
          w.host_state := SOF1;
        end if;
        if r.sr.reset='1' then
          w.host_state := RESET1;
          w.reset_delay := C_RESET_DELAY -1;
        end if;

        if r.sr.poweron='0' then
          w.host_state := DETACHED;
        end if;
        --if usb_rst_phy='1' then
          -- Disconnected???
        --  if r.intconfr.disconnectdetect='1' then
        --    w.intpendr.disconnectdetect := '1';
        --  end if;
        --  w.sr.connected := '0';
        --  w.host_state := DETACHED;
        --end if;


      when SOF1 =>
        trans_pid_s     <= USBF_T_PID_SOF;
        trans_strobe_s  <= '1';

        if trans_status_s=COMPLETED then
          -- For all interrupt EPs, decrease counters
          ie: for i in 0 to C_NUM_CHANNELS-1 loop
            w.ch(i).trans.issued := '0';

            --if r.ch(i).conf.eptype = C_EP_TYPE_INTERRUPT then
              if r.ch(i).trans.intervalcnt=0 then
                w.ch(i).trans.intervalcnt := unsigned(r.ch(i).conf.interval);
              else
                w.ch(i).trans.intervalcnt := r.ch(i).trans.intervalcnt - 1;
              end if;
            --end if;
          end loop;

          w.host_state := IDLE;
        end if;

      when SETUP1 =>
        trans_pid_s       <= USBF_T_PID_SETUP;
        trans_strobe_s    <= '1';

        trans_daddr_s     <= std_logic_vector(r.ch(r.channel).trans.epaddr);
        trans_dsize_s     <= std_logic_vector(r.ch(r.channel).trans.size);
        trans_ep_s        <= r.ch(r.channel).conf.epnum;
        trans_addr_s      <= r.ch(r.channel).conf.address;
        trans_data_seq_s  <= r.ch(r.channel).trans.seq;

        case trans_status_s is
          when ACK =>
            -- synthesis translate_off
            if rising_edge(usbclk_i) then report "Got ACK for SETUP"; end if;
            -- synthesis translate_on

            w.ch(r.channel).trans.cnt   := '0';
            w.ch(r.channel).intpend.ack := '1';
            w.ch(r.channel).intpend.cplt:= '1';
            w.host_state := IDLE;
          when NACK =>
            -- Keep trying...
            w.host_state := IDLE;

          when STALL =>
            w.ch(r.channel).intpend.stall := '1';
            w.ch(r.channel).trans.cnt   := '0';
            w.host_state := IDLE;

          when IDLE | BUSY =>
            -- Stay here.
          when others =>
            w.ch(r.channel).trans.cnt   := '0';
            w.ch(r.channel).intpend.transerror := '1';
            w.host_state := IDLE;
        end case;


      when IN1 =>
        trans_pid_s     <= USBF_T_PID_IN;
        trans_strobe_s  <= '1';

        trans_daddr_s   <= std_logic_vector(r.ch(r.channel).trans.epaddr);
        trans_dsize_s   <= std_logic_vector(r.ch(r.channel).trans.size);
        trans_ep_s      <= r.ch(r.channel).conf.epnum;
        trans_addr_s    <= r.ch(r.channel).conf.address;
        trans_data_seq_s  <= r.ch(r.channel).trans.seq;

        case trans_status_s is
          when COMPLETED =>
            w.ch(r.channel).trans.cnt    := '0';
            w.ch(r.channel).intpend.cplt := '1';
            if trans_seq_valid_s='0' then
              w.ch(r.channel).intpend.datatogglerror := '1';
            end if;
            --w.ch(r.channel).intpend.ack  := '1';

            -- TODO: check data sequence!!!

            -- synthesis translate_off
            if rising_edge(usbclk_i) then report "IN completed, len " & hstr(trans_dsize_read_s); end if;
            -- synthesis translate_on
            w.host_state := IDLE;
            w.ch(r.channel).trans.size := unsigned(trans_dsize_read_s);
          when NACK =>
            -- Keep trying...
            w.ch(r.channel).trans.issued := '1';
            -- synthesis translate_off
            if rising_edge(usbclk_i) then report "IN NAK" & hstr(trans_dsize_read_s); end if;
            -- synthesis translate_on
            w.host_state := IDLE;

          when IDLE | BUSY =>

          when TIMEOUT | CRCERROR =>
            -- Retry
            if r.ch(r.channel).trans.retries="00" then
              if trans_status_s=TIMEOUT then
                w.ch(r.channel).intpend.transerror  := '1';
              else
                w.ch(r.channel).intpend.crcerror    := '1';
              end if;
              w.ch(r.channel).trans.cnt   := '0';
            else
              w.ch(r.channel).trans.retries := r.ch(r.channel).trans.retries - 1;
            end if;
            w.host_state := IDLE;

          when BABBLE =>
            w.ch(r.channel).intpend.stall := '1';
            w.ch(r.channel).trans.cnt   := '0';
            w.host_state := IDLE;

          when STALL =>
            w.ch(r.channel).intpend.stall := '1';
            w.ch(r.channel).trans.cnt   := '0';
            w.host_state := IDLE;

          when others =>
            -- synthesis translate_off
            if rising_edge(usbclk_i) then report "Unknown state " & hstr(trans_dsize_read_s); end if;
            -- synthesis translate_on

            w.ch(r.channel).intpend.transerror := '1';
            w.ch(r.channel).trans.cnt   := '0';
            w.host_state := IDLE;
        end case;

      when OUT1 =>
        trans_pid_s     <= USBF_T_PID_OUT;
        trans_strobe_s  <= '1';

        trans_daddr_s     <= std_logic_vector(r.ch(r.channel).trans.epaddr);
        trans_dsize_s     <= std_logic_vector(r.ch(r.channel).trans.size);
        trans_ep_s        <= r.ch(r.channel).conf.epnum;
        trans_addr_s      <= r.ch(r.channel).conf.address;
        trans_data_seq_s  <= r.ch(r.channel).trans.seq;

        case trans_status_s is
          when COMPLETED  | ACK=>
            w.ch(r.channel).trans.cnt    := '0';
            w.ch(r.channel).intpend.cplt := '1';
            w.ch(r.channel).intpend.ack  := '1';

            w.ch(r.channel).trans.seq    := not r.ch(r.channel).trans.seq;
            -- synthesis translate_off
            if rising_edge(usbclk_i) then report "OUT completed, len " & hstr(trans_dsize_read_s); end if;
            -- synthesis translate_on
            w.host_state := IDLE;
            w.ch(r.channel).trans.size := unsigned(trans_dsize_read_s);
          when NACK =>
            -- Keep trying...
            -- synthesis translate_off
            if rising_edge(usbclk_i) then report "OUT NAK" & hstr(trans_dsize_read_s); end if;
            -- synthesis translate_on
            w.host_state := IDLE;

          when STALL =>
            w.ch(r.channel).intpend.stall := '1';
            w.ch(r.channel).trans.cnt   := '0';
            w.host_state := IDLE;
          when IDLE | BUSY =>

          when TIMEOUT =>
            w.ch(r.channel).trans.cnt   := '0';
            w.ch(r.channel).intpend.transerror := '1';
            w.host_state := IDLE;

          when CRCERROR =>
            w.ch(r.channel).trans.cnt   := '0';
            w.ch(r.channel).intpend.crcerror := '1';
            w.host_state := IDLE;

          when others =>
                      -- synthesis translate_off
            if rising_edge(usbclk_i) then report "Unknown state " & hstr(trans_dsize_read_s); end if;
            -- synthesis translate_on

            w.ch(r.channel).trans.cnt   := '0';
            w.ch(r.channel).intpend.transerror := '1';
            w.host_state := IDLE;
        end case;


      when RESET1 =>
        tx_mode_s <= '0'; -- Single-ended
        --Phy_DataOut <= "10101010";
        --Phy_TxValid <= '1';
        if r.reset_delay = 0 then
          w.reset_delay := C_RESET_DELAY_AFTER;
          w.host_state := RESET2;
        else
          w.reset_delay := r.reset_delay-1;
        end if;

      when RESET2 =>
        if r.reset_delay = 0 then
          w.sr.reset := '0';
          w.host_state := IDLE;
        else
          w.reset_delay := r.reset_delay-1;
        end if;

      when others =>
        report "INVALID STATE";

    end case;


    if usb_rst_phy='1' and tx_mode_s='1' then
      -- Disconnected???
      --w.sr.connectdetect := '1';
      --if r.intconfr.disconnectdetect='1' then
      --  w.intpendr.disconnectdetect := '1';
      --end if;

      --w.sr.connected := '0';
      --w.host_state := DETACHED;
    end if;


    w.sr.overcurrent := not pwrflt_i;

    if pwrflt_i='0' and r.intconfr.overcurrent='1' then
      w.intpendr.overcurrent:='1';
    end if;



    if ausbrst_i='1' then
      r.host_state        <= DETACHED;
      r.frame             <= (others => '0');
      r.sof_count         <= C_SOF_TIMEOUT - 1;
      r.attach_count      <= C_ATTACH_DELAY - 1;
      r.sr.poweron        <= '0';
      r.sr.overcurrent    <= '0';
      r.sr.fulllowspeed   <= '0';
      --r.sr.connectdetect  <= '0';
      r.sr.connected      <= '0';
      r.sr.reset          <= '0';

      r.intpendr.connectdetect <= '0';
      r.intpendr.disconnectdetect <= '0';
      r.intpendr.overcurrent   <= '0';
      r.intconfr.ginten        <= '0';
      r.intconfr.connectdetect <= '0';
      r.intconfr.disconnectdetect <= '0';
      r.intconfr.overcurrent   <= '0';
      r.int_holdoff            <= C_INTERRUPT_HOLDOFF-1;


      chc: for i in 0 to C_NUM_CHANNELS-1 loop
        r.ch(i).conf.enabled      <= '0';
        r.ch(i).trans.epaddr      <= (others => '0');
        r.ch(i).trans.intervalcnt <= (others => '0');
        r.ch(i).conf.interval     <= (others => '0');
        w.ch(i).trans.issued := '1';

      end loop;

    elsif rising_edge(usbclk_i) then
      r <= w;
    end if;
  end process;

  -- Pass on the write request
  wrb: block
    signal di_s,do_s: std_logic_vector(18 downto 0);
  begin
    di_s              <= dat_i & addr_i;
    write_address_s   <= do_s(10 downto 0);
    write_data_s      <= do_s(18 downto 11);

  wr_rq_sync: entity work.async_dualpulse_data
    generic map (
      DWIDTH => 8+11,
      WIDTH => 4
    )
    port map (
      clki_i  => clk_i,
      arst_i  => arst_i,
      clko_i  => usbclk_i,
      pulse_i(0) => wr_i,
      pulse_i(1) => rd_i,
      data_i  => di_s,
      pulse_o(0) => wr_sync_s,
      pulse_o(1) => rd_sync_s,
      data_o  => do_s
    );

  end block;

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


  hep_rd_s <= rd_i and addr_i(10);
  hep_wr_s <= wr_i and addr_i(10);

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
      haddr_i   => addr_i(9 downto 0),
      hdata_o   => hep_dat_s,
      hdata_i   => dat_i
  );

  usb_trans_inst: entity work.usb_trans
  port map (
    usbclk_i          => usbclk_i,
    ausbrst_i         => ausbrst_i,
    speed_i           => r.speed,
    fs_ce_i           => fs_ce_s,
    usb_rst_i         => usb_rst_phy,
    -- Transmission
    pid_i             => trans_pid_s,

    -- Address/EP for token packets
    addr_i            => trans_addr_s,
    ep_i              => trans_ep_s,
    -- Frame number for SOF
    frame_i           => std_logic_vector(r.frame),
    --
    dsize_i           => trans_dsize_s,
    dsize_o           => trans_dsize_read_s,
    daddr_i           => trans_daddr_s,

    strobe_i          => trans_strobe_s,
    data_seq_i        => trans_data_seq_s,
    data_seq_valid_o  => trans_seq_valid_s,

    phy_txready_i     => Phy_TxReady,
    phy_txactive_i    => phy_txactive_s, -- from OE
    phy_txdata_o      => Phy_DataOut,
    phy_data_valid_o  => Phy_TxValid,
    phy_rxactive_i    => Phy_RxActive,
    phy_rxvalid_i     => Phy_RxValid,
    phy_rxdata_i      => Phy_DataIn,
    phy_rxerror_i     => Phy_RxError,

    -- Connection to EPMEM

    urd_o             => epmem_read_en_s,
    uwr_o             => epmem_write_en_s,
    uaddr_o           => epmem_addr_s,
    udata_i           => epmem_data_out_s,
    udata_o           => epmem_data_in_s,

    dbg_rx_data_done_o => dbg_rx_data_done_s,
    status_o          => trans_status_s
  );


  --epmem_addr_s <= std_logic_vector(r.epmem_addr);

  speed_o     <= r.speed;
  softcon_o   <= '0';
  noe_o       <= '0' when r.host_state=RESET1 else noe_s;
  vpo_o       <= '0' when r.host_state=RESET1 else vpo_s;
  vmo_o       <= '0' when r.host_state=RESET1 else vmo_s;
  int_async_o <= int_s;
  pwren_o     <= not r.sr.poweron;
  dat_o <= hep_dat_s when addr_i(10)='1' else read_data_sync_s;

  deb: block
    signal fs: std_logic;
  begin
  process(usbclk_i,ausbrst_i)
  begin
    if ausbrst_i='1' then
      dbg_o <= (others => '0');
      fs <= '0';
    elsif rising_edge(usbclk_i) then
        dbg_o(0) <= Phy_RxActive;
        dbg_o(1) <= Phy_TxActive_s;
        dbg_o(2) <= fs;--Phy_RxError;
        dbg_o(3) <= dbg_rx_data_done_s;--Phy_Linestate(0);
        dbg_o(4) <= Phy_Linestate(1);

        case r.host_state is
            when DETACHED | ATTACHED  => dbg_o(7 downto 5) <= "000";
            when IDLE                 => dbg_o(7 downto 5) <= "001";
            when RESET1 | RESET2      => dbg_o(7 downto 5) <= "010";
            when WAIT_SOF             => dbg_o(7 downto 5) <= "011";
            when IN1                  => dbg_o(7 downto 5) <= "100";
            when OUT1                 => dbg_o(7 downto 5) <= "101";
            when SOF1                 => dbg_o(7 downto 5) <= "110";
            when SETUP1               => dbg_o(7 downto 5) <= "111";
        end case; 

        if fs_ce_s='1' then
          fs <= not fs;
        end if;
    end if;
  end process;
  end block;
END rtl;

