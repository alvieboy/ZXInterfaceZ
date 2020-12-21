LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;
LIBRARY work;
use work.ahbpkg.all;
-- synthesis translate_off
use work.txt_util.all;
-- synthesis translate_on
LIBRARY altera_mf;
USE altera_mf.altera_mf_components.all;
entity psram is
  port (
    clk_i   : in std_logic;
    arst_i  : in std_logic;

    ahb_i   : in AHB_M2S;
    ahb_o   : out AHB_S2M;

    hp_ahb_i   : in AHB_M2S;
    hp_ahb_o   : out AHB_S2M;

    cs_n_o  : out std_logic := '1';
    clk_o   : out std_logic;
    d_i     : in std_logic_vector(3 downto 0);
    d_o     : out std_logic_vector(3 downto 0);
    oe_o    : out std_logic_vector(3 downto 0)
  );
end entity psram;

architecture beh of psram is

  type state_type is (
    STARTUP,
    INIT1,
    INIT2,
    INIT3,
    INIT4,
    INIT5,
    IDLE,
    CHIPSELECT,
    ADDRESS1,
    ADDRESS2,
    ADDRESS3,
    ADDRESS4,
    ADDRESS5,
    ADDRESS6,
    ADDRESS7,
    RDDLY,
    RDDATA1,
    RDDATA2,
    RDDATA3,
    WRDATA1,
    WRDATA2,
    WRDATA3,
    DESELECT,
    DESELECT_WAIT1,
    DESELECT_WAIT2,
    ABORT_SWITCH_TO_HP,
    ABORT_SWITCH_TO_HP2
  );

  signal clock_enable_s : std_logic;
  signal clock_enable_r : std_logic;
  signal bus_oe_s       : std_logic_vector(3 downto 0);
  signal data_out_r     : std_logic_vector(3 downto 0);
  signal d_neg_r        : std_logic_vector(3 downto 0);
  signal csn_r          : std_logic := '1';
  signal clkgen_s       : std_logic_vector(0 downto 0);
  signal state_r        : state_type;
  signal init_cnt_r     : unsigned(2 downto 0);
  signal std_addr_r         : std_logic_vector(23 downto 0);
  signal std_wrdata_r       : std_logic_vector(7 downto 0);
  signal hp_addr_r         : std_logic_vector(23 downto 0);
  signal hp_wrdata_r       : std_logic_vector(7 downto 0);
  signal data_capture_r : std_logic_vector(7 downto 0);
  signal std_wr_r           : std_logic;
  signal hp_wr_r            : std_logic;

  signal addr_s             : std_logic_vector(23 downto 0);
  signal wrdata_s           : std_logic_vector(7 downto 0);

  -- Enter quad mode: 0x35
  constant CMD_ENTERQUADMODE  : std_logic_vector(7 downto 0) := x"35";--x"35";
  constant CMD_QUADREAD       : std_logic_vector(7 downto 0) := x"EB";--x"35";
  constant CMD_QUADWRITE      : std_logic_vector(7 downto 0) := x"38";--x"35";

  signal clock_disable_s      : std_logic;
  signal master_r             : std_logic; -- '0': regular master, '1': high priority master
  signal hp_request           : std_logic;

  signal write_flag_s         : std_logic;
  signal need_resume_r        : std_logic; -- Last STD transaction was aborted.
  signal abort_transaction_s  : std_logic;
  signal test_ready           : boolean := false;

  constant SELECT_DELAY       : natural := 5;

  signal select_dly_r         : natural range 0 to SELECT_DELAY-1;

begin

  cs_n_o          <= csn_r;
  oe_o            <= bus_oe_s;
  d_o             <= data_out_r;
  clk_o           <= clkgen_s(0);
  test_ready      <= true after 2 us;
  clock_disable_s <= not clock_enable_r;

  write_flag_s    <= std_wr_r when master_r='0'     else hp_wr_r;
  addr_s          <= std_addr_r when master_r='0'   else hp_addr_r;
  wrdata_s        <= ahb_i.HWDATA(7 downto 0) when master_r='0' else hp_ahb_i.HWDATA(7 downto 0);

  clk_inst : ALTDDIO_OUT
  GENERIC MAP (
    extend_oe_disable => "OFF",
    intended_device_family => "Cyclone IV E",
    invert_output => "ON",
    lpm_hint => "UNUSED",
    lpm_type => "altddio_out",
    oe_reg => "UNREGISTERED",
    power_up_high => "ON",
    width => 1
  )
  PORT MAP (
    aclr      => clock_disable_s,
    datain_h  => "1",
    datain_l  => "0",
    outclock  => clk_i,
    dataout   => clkgen_s,
    oe_out    => open
  );

  -- Generate clock enable signal
  process(state_r)
  begin
    case state_r is
      when STARTUP | INIT3 | INIT4 | INIT5 | IDLE |
           DESELECT_WAIT1 | DESELECT_WAIT2 | RDDATA2 |
           RDDATA3 | DESELECT | ABORT_SWITCH_TO_HP | ABORT_SWITCH_TO_HP2 =>
        clock_enable_s <= '0';
      when others   =>
        clock_enable_s  <= '1';
    end case;
  end process;


  hp_request <= '1' when hp_ahb_i.HTRANS/=C_AHB_TRANS_IDLE else '0';

  abort_transaction_s <= '0'; --'1' when master_r='0' and hp_request='1'
  --  else '0';

  process(state_r, hp_request, select_dly_r)
  begin
    ahb_o.HREADY    <= '0';
    hp_ahb_o.HREADY    <= '0';
    case state_r is
      when IDLE    =>
        ahb_o.HREADY      <= not hp_request;
        hp_ahb_o.HREADY   <= '1';
        -- Except when busy
        if select_dly_r/=0 then
          ahb_o.HREADY      <= '0';
          hp_ahb_o.HREADY   <= '0';
        end if;
      when DESELECT_WAIT1 =>
        ahb_o.HREADY      <= not master_r; -- ack only ours
        hp_ahb_o.HREADY   <= master_r; -- ack only ours
      when ABORT_SWITCH_TO_HP =>
        ahb_o.HREADY      <= not hp_request;
        hp_ahb_o.HREADY   <= '1';
        if select_dly_r/=0 then
          ahb_o.HREADY      <= '0';
          hp_ahb_o.HREADY   <= '0';
        end if;
      when others =>
    end case;
  end process;

  counters: process(clk_i, arst_i)
  begin
    if arst_i='1' then
      init_cnt_r    <= "111";
    elsif rising_edge(clk_i) then
      case state_r is
        when STARTUP =>
          -- Hold reset value
          init_cnt_r  <= init_cnt_r;

        when INIT1 =>
          init_cnt_r  <= init_cnt_r - 1;

        when ADDRESS7 =>
          init_cnt_r  <= "110";

        when RDDLY =>
          init_cnt_r  <= init_cnt_r - 1;

        when others =>
          init_cnt_r  <= (others => 'X');
      end case;
          
    end if;
  end process;




  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      state_r       <= STARTUP;
      bus_oe_s      <= (others => '0');
      csn_r         <= '1';
      std_addr_r    <= (others => 'X');
      hp_addr_r     <= (others => 'X');
      std_wrdata_r  <= (others => 'X');
      hp_wrdata_r   <= (others => 'X');
      hp_addr_r     <= (others => 'X');
      std_wr_r      <= 'X';
      hp_wr_r       <= 'X';
      need_resume_r <= '0';
      clock_enable_r<='0';
      data_out_r    <= (others => 'X');
      bus_oe_s      <= (others => 'X');
      master_r      <= 'X';
      select_dly_r  <= 0;
      data_capture_r <= (others => 'X');
    elsif rising_edge(clk_i) then

      if select_dly_r/=0 then
        select_dly_r <= select_dly_r - 1;
      end if;

      case state_r is
        when STARTUP =>
          if test_ready then
            state_r     <= INIT1;
            clock_enable_r <= '1';
          else
            clock_enable_r <= '0';
          end if;
        when INIT1 =>
          data_out_r(0)  <= CMD_ENTERQUADMODE( to_integer(init_cnt_r) );
          data_out_r(1)  <= '1';
          data_out_r(2)  <= '1';
          bus_oe_s    <= "0111"; -- Only MOSI/WP/HOLD
          csn_r      <= '0';
          if init_cnt_r=0 then
            state_r   <= INIT2;
          end if;
        when INIT2 =>
          data_out_r  <= (others => 'X');
          state_r         <= INIT3;
          clock_enable_r  <='0';
        when INIT3  =>
          data_out_r  <= (others => 'X');
          state_r         <= INIT4;
        when INIT4  =>
          data_out_r  <= (others => 'X');
          csn_r           <= '1';
          state_r         <= INIT5;

        when INIT5 =>
          data_out_r  <= (others => 'X');

          state_r <= IDLE;

        when IDLE =>

        data_out_r  <= (others => 'X');

          need_resume_r <= '0';
          std_wr_r        <= ahb_i.HWRITE;
          std_addr_r      <= ahb_i.HADDR(23 downto 0);
          hp_wr_r         <= hp_ahb_i.HWRITE;
          hp_addr_r       <= hp_ahb_i.HADDR(23 downto 0);

          if select_dly_r=0 then
            if ahb_i.HTRANS/=C_AHB_TRANS_IDLE then

                state_r         <= CHIPSELECT;
                bus_oe_s        <= "1111";
                csn_r           <= '0';
                clock_enable_r  <= '1';
                master_r        <= '0';

            end if;

            if hp_ahb_i.HTRANS/=C_AHB_TRANS_IDLE then
                state_r         <= CHIPSELECT;
                bus_oe_s        <= "1111";
                csn_r           <= '0';
                clock_enable_r  <= '1';
                master_r        <= '1';
            end if;
          end if;

        when CHIPSELECT =>
          if write_flag_s='1' then
            data_out_r(3 downto 0) <= CMD_QUADWRITE(7 downto 4);
          else
            data_out_r(3 downto 0) <= CMD_QUADREAD(7 downto 4);
          end if;

          state_r <= ADDRESS1;
          -- If HP master interrupted, deselect
          if abort_transaction_s='1' then
            state_r         <= ABORT_SWITCH_TO_HP;
            --csn_r           <= '1';
            clock_enable_r  <= '0';
          end if;

        when ABORT_SWITCH_TO_HP =>
          hp_wr_r         <= hp_ahb_i.HWRITE;
          hp_addr_r       <= hp_ahb_i.HADDR(23 downto 0);
          --hp_wrdata_r     <= hp_ahb_i.HWDATA(7 downto 0);
          master_r        <= '1';
          need_resume_r   <= '1';
          csn_r           <= '1';
          select_dly_r    <= SELECT_DELAY-1;
          state_r         <= ABORT_SWITCH_TO_HP2;

        when ABORT_SWITCH_TO_HP2 =>
          if select_dly_r=0 then
            csn_r           <= '0';
            clock_enable_r  <= '1';
            state_r         <= CHIPSELECT;
            bus_oe_s        <= "1111";
          end if;

        when ADDRESS1 =>
          state_r <= ADDRESS2;
          bus_oe_s    <= "1111";
          csn_r <= '0';
          if write_flag_s='1' then
            data_out_r(3 downto 0) <= CMD_QUADWRITE(3 downto 0);
          else
            data_out_r(3 downto 0) <= CMD_QUADREAD(3 downto 0);
          end if;
          if abort_transaction_s='1' then
            state_r         <= ABORT_SWITCH_TO_HP;
            --csn_r           <= '1';
            clock_enable_r  <= '0';
          end if;

        when ADDRESS2 =>
          state_r <= ADDRESS3;
          bus_oe_s    <= "1111";
          csn_r <= '0';
          data_out_r(3 downto 0) <= addr_s(23 downto 20);
          if abort_transaction_s='1' then
            state_r         <= ABORT_SWITCH_TO_HP;
            --csn_r           <= '1';
            clock_enable_r  <= '0';
          end if;

        when ADDRESS3 =>
          state_r <= ADDRESS4;
          bus_oe_s    <= "1111";
          csn_r <= '0';
          data_out_r(3 downto 0) <= addr_s(19 downto 16);
          if abort_transaction_s='1' then
            state_r         <= ABORT_SWITCH_TO_HP;
            --csn_r           <= '1';
            clock_enable_r  <= '0';
          end if;

        when ADDRESS4 =>
          state_r <= ADDRESS5;
          bus_oe_s    <= "1111";
          csn_r <= '0';
          data_out_r(3 downto 0) <= addr_s(15 downto 12);
          if abort_transaction_s='1' then
            state_r         <= ABORT_SWITCH_TO_HP;
            --csn_r           <= '1';
            clock_enable_r  <= '0';
          end if;

        when ADDRESS5 =>
          state_r <= ADDRESS6;
          bus_oe_s    <= "1111";
          csn_r <= '0';
          data_out_r(3 downto 0) <= addr_s(11 downto 8);
          if abort_transaction_s='1' then
            state_r         <= ABORT_SWITCH_TO_HP;
            --csn_r           <= '1';
            clock_enable_r  <= '0';
          end if;

        when ADDRESS6 =>
          state_r <= ADDRESS7;
          bus_oe_s    <= "1111";
          csn_r <= '0';
          data_out_r(3 downto 0) <= addr_s(7 downto 4);
          if abort_transaction_s='1' then
            state_r         <= ABORT_SWITCH_TO_HP;
            --csn_r           <= '1';
            clock_enable_r  <= '0';
          end if;

        when ADDRESS7 =>
          if write_flag_s='0' then
            state_r <= RDDLY;
          else
            state_r <= WRDATA1;
          end if;
          csn_r <= '0';
          bus_oe_s    <= "1111";
          data_out_r(3 downto 0) <= addr_s(3 downto 0);

          if write_flag_s='0' then
            if abort_transaction_s='1' then
              state_r         <= ABORT_SWITCH_TO_HP;
              --csn_r           <= '1';
              clock_enable_r  <= '0';
            end if;
          end if;


        when RDDLY =>
          bus_oe_s    <= "0000";
          if init_cnt_r=0 then
            state_r   <= RDDATA1;
          end if;

          if abort_transaction_s='1' then
            state_r         <= ABORT_SWITCH_TO_HP;
            --csn_r           <= '1';
            clock_enable_r  <= '0';
          end if;

        when RDDATA1 =>
          bus_oe_s        <= "0000";
          state_r         <= RDDATA2;
          clock_enable_r  <='0';

        when RDDATA2 =>
          bus_oe_s    <= "0000";
          data_capture_r(3 downto 0) <= d_neg_r;
          state_r <= RDDATA3;--RDDATA3;

        when RDDATA3 =>
          bus_oe_s    <= "0000";
          csn_r <= '1'; -- Looks like early deselect - is not.
          data_capture_r(7 downto 4) <= d_neg_r;
          clock_enable_r <= '0';
          state_r <= DESELECT_WAIT1;

        when WRDATA1 =>
          bus_oe_s    <= "1111";
          data_out_r(3 downto 0) <= wrdata_s(3 downto 0);
          state_r <= WRDATA2;

        when WRDATA2 =>
          bus_oe_s    <= "1111";
          data_out_r(3 downto 0) <= wrdata_s(7 downto 4);
          state_r <= WRDATA3;

        when WRDATA3 =>
          bus_oe_s    <= "1111";
          data_out_r(3 downto 0) <= (others => 'X');--wrdata_r(7 downto 4);
          state_r <= DESELECT_WAIT1;
          clock_enable_r <= '0';

        when DESELECT_WAIT1 =>
          state_r <= DESELECT_WAIT2;

        when DESELECT_WAIT2 =>
          csn_r <= '1';
          select_dly_r<=SELECT_DELAY-1;
          state_r <= DESELECT;

        when DESELECT =>
          if need_resume_r='1' then
            if select_dly_r=0 then
              state_r         <= CHIPSELECT;
              master_r        <= '0';
              clock_enable_r  <= '1';
              csn_r           <= '0';
              bus_oe_s        <= "1111";
              need_resume_r <= '0';
            end if;
          else
            state_r <= IDLE;
            csn_r <= '1';
            need_resume_r <= '0';
          end if;

      when others =>
      end case;
          
    end if;
  end process;

  process(clk_i)
  begin
    if falling_edge(clk_i) then
      d_neg_r   <= d_i;
    end if;
  end process;

  ahb_o.HRDATA(7 downto 0)  <= data_capture_r;
  ahb_o.HRDATA(31 downto 8)  <= (others => '0');
  ahb_o.HRESP   <= '0';

  hp_ahb_o.HRDATA(7 downto 0)  <= data_capture_r;
  hp_ahb_o.HRDATA(31 downto 8)  <= (others => '0');
  hp_ahb_o.HRESP   <= '0';

-- synthesis translate_off
  process(clk_i)
  begin
    if rising_edge(clk_i) then
      if clock_enable_r/=clock_enable_s then
        report "ERROR clock enable " & str(clock_enable_r) & " expected " & str(clock_enable_s);-- severity failure;
      end if;
    end if;
  end process;
-- synthesis translate_on


end beh;


