library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

entity usb_tx_phy is
  port (
    clk_i             : in  std_logic;
    arstn_i           : in  std_logic;

    -- UTMI Interface
    XcvrSelect_i      : in  std_logic; -- '0' - Low speed, '1' - Full speed.
    TermSelect_i      : in  std_logic; -- '0' - Low speed, '1' - Full speed.
    OpMode_i          : in  std_logic; -- '1' - normal mode, '0' - disable bit stuff/SOP/EOP
    DataOut_i         : in  std_logic_vector(7 downto 0);
    TxValid_i         : in  std_logic;
    TxReady_o         : out std_logic;

    -- Transciever Interface
    txdp, txdn, txoen_o : out std_logic
  );
end usb_tx_phy;

architecture beh of usb_tx_phy is

  type state_type is (
    IDLE,
    SOP,
    DATA,
    WAIT1,
    EOP1,
    EOP2,
    EOP3
  );

  
  signal ones_count_r   : unsigned(2 downto 0);
  signal bit_count_r    : unsigned(2 downto 0);
  signal state_r        : state_type;
  signal data_r         : std_logic_vector(7 downto 0);
  signal load_sop_s     : std_logic;
  signal load_data_s    : std_logic;
  signal clock_speed_r  : std_logic;
  signal bit_tick_s     : std_logic;
  signal shift_data_s   : std_logic;
  signal stuff_r        : std_logic;
  signal need_stuff_s   : std_logic;
  signal sd_data        : std_logic;
  signal nrzi_data      : std_logic;
  signal eop_se0        : std_logic;
  signal eop_se0_q      : std_logic;
  signal eop_se0_q_q    : std_logic;
  signal load_eop_s     : std_logic;
  signal clock_reset_s  : std_logic;
  signal send_pre_r     : std_logic;
  signal send_pre_s     : std_logic;
  signal wait_dly_r     : unsigned(1 downto 0);
  constant C_SYNC : std_logic_vector(7 downto 0) := x"80";
  constant C_PRE  : std_logic_vector(7 downto 0) := x"3C";

begin

  txclkgen: entity work.usb_txclkgen
  port map (
    clk_i     => clk_i,
    arstn_i   => arstn_i,
    speed_i   => clock_speed_r,
    srst_i    => clock_reset_s,
    tick_o    => bit_tick_s
  );


  process(state_r, TxValid_i, OpMode_i, XcvrSelect_i, TermSelect_i, bit_tick_s, DataOut_i, wait_dly_r, eop_se0_q, eop_se0_q_q)
  begin

    load_sop_s <= '0';
    load_eop_s <= '0';
    send_pre_s <= '0';

    case state_r is
      when IDLE =>
        if TxValid_i='1' then
          if OpMode_i='1' then -- Normal operation
            if DataOut_i=x"A5" and XcvrSelect_i='0' then
              -- Low-speed KA
              load_eop_s    <= '1';
            elsif XcvrSelect_i=TermSelect_i then -- Same speed
              load_sop_s    <= '1';
            else -- Different speed. Will need to generate PRE.
              load_sop_s    <= '1';
              send_pre_s    <= '1';
              --report "TBD" severity failure;
            end if;
          else
              -- Disable SOP, EOP, NRZI
          end if;
        end if;
      when WAIT1 =>
        if bit_tick_s='1' and wait_dly_r=0 then
          load_sop_s <= '1';
        end if;
      when others =>
    end case;
    -- If we are still handling EOP, wait
    if eop_se0_q='1' or eop_se0_q_q='1' then
      load_sop_s <= '0';
      load_eop_s <= '0';
      send_pre_s <= '0';

    end if;
  end process;

  clock_reset_s <= load_sop_s;

  process(clk_i,arstn_i)
  begin
    if arstn_i='0' then
      clock_speed_r <='1';
      send_pre_r  <= '0';
    elsif rising_edge(clk_i) then
      if load_sop_s='1' then
        clock_speed_r <= TermSelect_i;
      end if;
      if send_pre_s='1' then
        send_pre_r  <= '1';
      elsif state_r=WAIT1 then
        if wait_dly_r=0 and bit_tick_s='1' then
          clock_speed_r <= '0';--TermSelect_i;
          send_pre_r    <= '0';
        end if;
      end if;
    end if;
  end process;

  process(clk_i,arstn_i)
  begin
    if arstn_i='0' then
      data_r  <= (others => 'X');
    elsif rising_edge(clk_i) then
      if load_sop_s='1' then
        data_r <= C_SYNC;
      elsif load_data_s='1' then
        if send_pre_r='1' then
          data_r <= C_PRE;
        else
          data_r <= DataOut_i;
        end if;
      end if;
    end if;
  end process;


  shift_data_s <= '1' when (state_r=SOP or state_r=DATA)
        and bit_tick_s='1' else '0';

  load_data_s <= '1' when shift_data_s='1' and std_logic_vector(bit_count_r)="111" and stuff_r='0' else '0';

  need_stuff_s <= '1' when std_logic_vector(ones_count_r)="110" else '0';

  TxReady_o <= (load_data_s or load_eop_s) and not send_pre_r;

  process(clk_i,arstn_i)
  begin
    if arstn_i='0' then
      sd_data       <= '1';
      nrzi_data     <= '1';
      ones_count_r  <= "000";
    elsif rising_edge(clk_i) then
      if shift_data_s='1' then
        if need_stuff_s = '1' then
          sd_data       <= '0';
          nrzi_data     <= not nrzi_data;
          ones_count_r  <= "000";
        else
          sd_data <= data_r(conv_integer(bit_count_r));
          if data_r(conv_integer(bit_count_r))='1' then
            ones_count_r  <= ones_count_r + 1;
          else
            nrzi_data     <= not nrzi_data;
            ones_count_r  <= "000";
          end if;
        end if;
      end if;
      if state_r=EOP2 or state_r=WAIT1 then
        if bit_tick_s='1' then
          nrzi_data <= '1';
          sd_data <= '0';
        end if;
      end if;

    end if;
  end process;

  process(clk_i,arstn_i)
  begin
    if arstn_i='0' then
    elsif rising_edge(clk_i) then
      if eop_se0='1' then
        txdp  <= '0';
        txdn  <= '0';
      else
        txdp  <= nrzi_data xor not TermSelect_i;--not clock_speed_r;
        txdn  <= nrzi_data xor TermSelect_i;--clock_speed_r;
      end if;
    end if;
  end process;


  process(clk_i,arstn_i)
  begin
    if arstn_i='0' then
      stuff_r <= '0';
    elsif rising_edge(clk_i) then
      stuff_r <= need_stuff_s;
    end if;
  end process;

  process(clk_i,arstn_i)
  begin
    if arstn_i='0' then
      eop_se0 <= '0';
    elsif rising_edge(clk_i) then
      if bit_tick_s='1' then
        if state_r=EOP1 or state_r=EOP2 then
          eop_se0 <= '1';
        else
          eop_se0 <= '0';
        end if;
        eop_se0_q   <= eop_se0;
        eop_se0_q_q <= eop_se0_q;
      end if;
    end if;
  end process;

  process(clk_i,arstn_i)
  begin
    if arstn_i='0' then
      bit_count_r <= "000";
    elsif rising_edge(clk_i) then
      if load_sop_s='1' or load_data_s='1' then
        bit_count_r <= "000";
      elsif shift_data_s='1' and stuff_r='0' then
        bit_count_r <= bit_count_r + 1;
      end if;
    end if;
  end process;

  -- TXOE

  process(clk_i,arstn_i)
  begin
    if arstn_i='0' then
      txoen_o <= '1';
    elsif rising_edge(clk_i) then
      if state_r=IDLE and (load_sop_s='1' or load_eop_s='1') then
        txoen_o <= '0';
      elsif eop_se0_q_q='1' and bit_tick_s='1' then
        txoen_o <= '1';
      end if;
    end if;
  end process;

  process(clk_i,arstn_i)
  begin
    if arstn_i='0' then
      wait_dly_r <= (others => 'X');
    elsif rising_edge(clk_i) then
      case state_r is
        when DATA =>
          wait_dly_r <= (others => '0');
          if load_data_s='1' then
            if send_pre_r='1' then
              wait_dly_r <= "11";
            end if;
          end if;
        when WAIT1 =>
          if wait_dly_r/=0 and bit_tick_s='1' then
            wait_dly_r <= wait_dly_r - 1;
          end if;
        when others =>
          wait_dly_r <= (others => 'X');
      end case;
    end if;
  end process;
  
  process(clk_i,arstn_i)
  begin
    if arstn_i='0' then
      state_r <= IDLE;
    elsif rising_edge(clk_i) then
      case state_r is
        when IDLE =>
          if load_sop_s='1' then
            state_r <= SOP;
          elsif load_eop_s='1' then
            state_r <= EOP1;
          end if;
        when SOP =>
          -- We might need to send a PRE PID here.

          if load_data_s='1' then
            state_r <= DATA;
          end if;
        when DATA =>
          if load_data_s='1' then
            if send_pre_r='1' then
               state_r <= WAIT1;
            elsif TxValid_i='0' then
              state_r <= EOP1;
            end if;
          end if;
        when EOP1 =>
          if bit_tick_s='1' then
            state_r <= EOP2;
          end if;
        when EOP2 =>
          if bit_tick_s='1' then
            state_r <= EOP3;
          end if;
        when EOP3 =>
          if bit_tick_s='1' then
            state_r <= IDLE;
          end if;
        when WAIT1 =>
          if bit_tick_s='1' and wait_dly_r=0 then
            state_r <= SOP;
          end if;
          

        when others =>
      end case;
    end if;
  end process;

end beh;
