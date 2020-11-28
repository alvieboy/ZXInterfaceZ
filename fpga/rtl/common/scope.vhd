library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.STD_LOGIC_misc.all;
use IEEE.numeric_std.all;
library work;
use work.ahbpkg.all;

--
--
-- Example memory layout for WIDTH_BITS=10 (1024 samples)
--
--              11111111110000000000
--              98765432109876543210
--                    0------------   -- Control
--                    10AAAAAAAAAAbb  -- RAM1 (triggerable)
--                    11AAAAAAAAAAbb  -- RAM2 (non-triggerable)

entity scope is
  generic (
    NONTRIGGERABLE_WIDTH  : natural range 1 to 32 := 8;
    TRIGGERABLE_WIDTH     : natural range 1 to 32 := 8;
    WIDTH_BITS            : natural := 10
  );
  port (
    clk_i         : in std_logic;
    arst_i        : in std_logic;

    nontrig_i     : in std_logic_vector(NONTRIGGERABLE_WIDTH-1 downto 0);
    trig_i        : in std_logic_vector(TRIGGERABLE_WIDTH-1 downto 0);

    ahb_m2s_i     : in AHB_M2S;
    ahb_s2m_o     : out AHB_S2M
  );
end scope;


architecture beh of scope is

  signal counter_r      : unsigned(WIDTH_BITS-1 downto 0);
  signal addr_r         : unsigned(WIDTH_BITS-1 downto 0);

  function extend32(l: in std_logic_vector) return std_logic_vector is
    variable r: std_logic_vector(31 downto 0);
  begin
    r:= (others => '0');
    r(l'range) := l;
    return r;
  end function;

  function byteat(l: in std_logic_vector; index: in std_logic_vector(1 downto 0)) return std_logic_vector is
    variable r: std_logic_vector(31 downto 0);
    variable i: std_logic_vector(7 downto 0);
  begin
    r := extend32(l);
    case index is
      when "00" => return r(7 downto 0);
      when "01" => return r(15 downto 8);
      when "10" => return r(23 downto 16);
      when "11" => return r(31 downto 24);
      when others => return "XXXXXXXX";
    end case;

  end function;

  --
  signal clockdiv_r     : unsigned(3 downto 0) := "0000";
  signal clocktick_s    : std_logic;

  -- Main registers
  signal control_r      : std_logic_vector(31 downto 0);
  alias  enable_s       : std_logic                     is control_r(31);
  alias  clockdiv_s     : std_logic_vector(3 downto 0)  is control_r(3 downto 0);

  signal trig_addr_r    : std_logic_vector(WIDTH_BITS-1 downto 0);

  signal trigmask_r     : std_logic_vector(31 downto 0);
  signal trigval_r      : std_logic_vector(31 downto 0);
  signal trigedge_r     : std_logic_vector(31 downto 0);

  signal trig_latched_r : std_logic_vector(TRIGGERABLE_WIDTH-1 downto 0);

  signal trig_match_array_s: std_logic_vector(TRIGGERABLE_WIDTH-1 downto 0);

  signal capdata_s            : std_logic_vector(7 downto 0);
  signal nontrig_data_s       : std_logic_vector(NONTRIGGERABLE_WIDTH-1 downto 0);
  signal trig_data_s          : std_logic_vector(TRIGGERABLE_WIDTH-1 downto 0);

  signal triggered_r        : std_logic;
  signal running_r          : std_logic;
  signal trigger_match_s    : std_logic;
  signal start_counter_s    : std_logic;
  signal capture_active_s   : std_logic;
  signal start_capture_r    : std_logic;
  signal counter_zero_s     : std_logic;

  function max_counter return unsigned is
    variable r      : unsigned(WIDTH_BITS-1 downto 0);
  begin
    r := (others => '1');
    r(3 downto 0) := (others => '0');
    return r;
  end function;

  signal rd_s          : std_logic;
  signal wr_s          : std_logic;
  signal addr_s        : std_logic_vector(13 downto 0);
  signal dat_in_s      : std_logic_vector(7 downto 0);
  signal dat_out_s     : std_logic_vector(7 downto 0);

begin

  ahb2rdwr_inst: entity work.ahb2rdwr
    generic map (
      AWIDTH => 14, DWIDTH => 8
    )
    port map (
      clk_i     => clk_i,
      arst_i    => arst_i,
      ahb_m2s_i => ahb_m2s_i,
      ahb_s2m_o => ahb_s2m_o,

      addr_o    => addr_s,
      dat_o     => dat_in_s,
      dat_i     => dat_out_s,
      rd_o      => rd_s,
      wr_o      => wr_s
    );


  process(clocktick_s, trigmask_r, trigval_r, trigedge_r, trig_i, trig_latched_r)
  begin
    l: for i in 0 to TRIGGERABLE_WIDTH-1 loop
        if trigmask_r(i)='1' then
          if trigedge_r(i)='0' then -- Level trigger
            trig_match_array_s(i) <= not (trig_i(i) xor trigval_r(i));
          else -- Edge triggered
            if trigval_r(i)='1' then -- Rising edge 0->1
              trig_match_array_s(i) <= trig_i(i) and not trig_latched_r(i);
            else  -- Falling edge 1->0
              trig_match_array_s(i) <= not trig_i(i) and trig_latched_r(i);
            end if;
          end if;
        else
           trig_match_array_s(i) <= '1'; -- trigger disabled
        end if;
    end loop;
  end process;

  trigger_match_s <= and_reduce(trig_match_array_s);

--  trigger_match_s <= '1' when clocktick_s='1' and (trig_i and trigmask_r(TRIGGERABLE_WIDTH-1 downto 0))=trigval_r(TRIGGERABLE_WIDTH-1 downto 0) else '0';

  --capture_active_s <='1' when triggered_r='1' and counter_r/=0 else '0';
  capture_active_s <='1' when enable_s='1' and counter_r/=0 else '0';

  process(clk_i)
  begin
    if rising_edge(clk_i) then
      if clockdiv_r=0 then
        clockdiv_r <= unsigned(clockdiv_s);
      else
        clockdiv_r <= clockdiv_r - 1;
      end if;
    end if;
  end process;

  clocktick_s <= '1' when clockdiv_r=0 else '0';

  process(clk_i,arst_i)
  begin
    if arst_i='1' then
      addr_r          <= (others => '0');
      trig_latched_r  <= (others => '0');
    elsif rising_edge(clk_i) then
      if clocktick_s='1' then
        addr_r <= addr_r + 1;
        trig_latched_r <= trig_i;
      end if;
    end if;
  end process;

  process(clk_i,arst_i)
  begin
    if arst_i='1' then
      triggered_r <= '0';
    elsif rising_edge(clk_i) then
      if trigger_match_s='1' and enable_s='1' then
        triggered_r <= '1';
        if triggered_r='0' and counter_zero_s='0' then
          trig_addr_r <= std_logic_vector(addr_r);
        end if;
      end if;
      if start_capture_r='1' or counter_zero_s='1' then
        triggered_r <= '0';
      end if;
    end if;
  end process;

  counter_zero_s <= '1' when counter_r=0 else '0';

  process(clk_i,arst_i)
  begin
    if arst_i='1' then
      counter_r <= max_counter;
    elsif rising_edge(clk_i) then
      if start_capture_r='1' then --start_counter_s='1' then
        counter_r <= max_counter;
      elsif clocktick_s='1' and triggered_r='1' then
        if counter_zero_s='0' then
          counter_r <= counter_r - 1;
        end if;
      end if;
    end if;
  end process;

  process(clk_i,arst_i)
  begin
    if arst_i='1' then
      trigmask_r      <= (others => '1');
      trigval_r       <= (others => '0');
      control_r       <= (others => '0');
      start_capture_r <= '0';
    elsif rising_edge(clk_i) then
      start_capture_r <= '0';

      if wr_s='1' then
        if addr_s(addr_s'high)='0' then
          case addr_s(4 downto 0) is

            when "00000" =>
              trigmask_r(7 downto 0) <= dat_in_s;
            when "00001" =>
              trigmask_r(15 downto 8) <= dat_in_s;
            when "00010" =>
              trigmask_r(23 downto 16) <= dat_in_s;
            when "00011" =>
              trigmask_r(31 downto 24) <= dat_in_s;

            when "00100" =>
              trigval_r(7 downto 0) <= dat_in_s;
            when "00101" =>
              trigval_r(15 downto 8) <= dat_in_s;
            when "00110" =>
              trigval_r(23 downto 16) <= dat_in_s;
            when "00111" =>
              trigval_r(31 downto 24) <= dat_in_s;

            when "01000" =>
              trigedge_r(7 downto 0) <= dat_in_s;
            when "01001" =>
              trigedge_r(15 downto 8) <= dat_in_s;
            when "01010" =>
              trigedge_r(23 downto 16) <= dat_in_s;
            when "01011" =>
              trigedge_r(31 downto 24) <= dat_in_s;



            when "10000" =>
              control_r(7 downto 0) <= dat_in_s;
            when "10001" =>
              control_r(15 downto 8) <= dat_in_s;
            when "10010" =>
              control_r(23 downto 16) <= dat_in_s;
            when "10011" =>
              control_r(31 downto 24) <= dat_in_s;
              if dat_in_s(7)='1' then
                start_capture_r<='1';
              end if;


            when others =>
          end case;
        end if;
      end if;
    end if;
  end process;


  capram_nontrig_inst: entity work.generic_dp_ram
  generic map (
    address_bits => WIDTH_BITS,
    data_bits => NONTRIGGERABLE_WIDTH
  )
  port map (
    clka    => clk_i,
    ena     => capture_active_s,
    wea     => clocktick_s,
    dia     => nontrig_i,
    addra   => std_logic_vector(addr_r),
    doa     => open,
    clkb    => clk_i,
    enb     => '1',
    web     => '0',
    dib     => (others => 'X'),
    addrb   => std_logic_vector(addr_s(WIDTH_BITS+1 downto 2)),
    dob     => nontrig_data_s
  );

  capram_trig_inst: entity work.generic_dp_ram
  generic map (
    address_bits => WIDTH_BITS,
    data_bits => TRIGGERABLE_WIDTH
  )
  port map (
    clka    => clk_i,
    ena     => capture_active_s,
    wea     => clocktick_s,
    dia     => trig_i,
    addra   => std_logic_vector(addr_r),
    doa     => open,
    clkb    => clk_i,
    enb     => '1',
    web     => '0',
    dib     => (others => 'X'),
    addrb   => std_logic_vector(addr_s(WIDTH_BITS+1 downto 2)),
    dob     => trig_data_s
  );

--  capdata_s <= nontrig_data_s & trig_data_s;

  process(addr_s, control_r, counter_r, trig_addr_r, nontrig_data_s, trig_data_s)
  begin
    if addr_s(addr_s'high)='1' then
      if addr_s(addr_s'high-1)='1' then
        dat_out_s <= byteat( extend32(nontrig_data_s), addr_s(1 downto 0) );
      else
        dat_out_s <= byteat( extend32(trig_data_s), addr_s(1 downto 0) );
      end if;
    else
      dat_out_s <= (others => 'X');
      case addr_s(3 downto 2) is
        when "00" =>
          dat_out_s(7 downto 1)<=(others =>'0');
          dat_out_s(0) <= counter_zero_s;

        when "01" =>
          dat_out_s <= byteat(std_logic_vector(counter_r), addr_s(1 downto 0));

        when "10" =>
          dat_out_s <= byteat(trig_addr_r, addr_s(1 downto 0));

        when "11" =>
          -- control
          dat_out_s <= byteat(control_r, addr_s(1 downto 0));
        when others =>
      end case;
    end if;
  end process;

end beh;
