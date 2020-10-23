library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.STD_LOGIC_misc.all;
use IEEE.numeric_std.all;

entity scope is
  generic (
    NONTRIGGERABLE_WIDTH  : natural := 8;
    TRIGGERABLE_WIDTH     : natural := 8;
    WIDTH_BITS            : natural := 10
  );
  port (
    clk_i         : in std_logic;
    arst_i        : in std_logic;

    nontrig_i     : in std_logic_vector(NONTRIGGERABLE_WIDTH-1 downto 0);
    trig_i        : in std_logic_vector(TRIGGERABLE_WIDTH-1 downto 0);

    rd_i          : in std_logic;
    wr_i          : in std_logic;
    addr_i        : in std_logic_vector(10 downto 0);
    din_i         : in std_logic_vector(7 downto 0);
    dout_o        : out std_logic_vector(7 downto 0)

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

  signal capdata_s          : std_logic_vector(7 downto 0);
  signal nontrig_data_s      : std_logic_vector(NONTRIGGERABLE_WIDTH-1 downto 0);

  signal triggered_r        : std_logic;
  signal trigger_match_s     : std_logic;
  signal start_counter_s    : std_logic;
  signal capture_active_s   : std_logic;
  signal start_capture_r    : std_logic;

  function max_counter return unsigned is
    variable r      : unsigned(WIDTH_BITS-1 downto 0);
  begin
    r := (others => '1');
    r(3 downto 0) := (others => '0');
    return r;
  end function;

begin

  trigger_match_s <= '1' when clocktick_s='1' and (trig_i and trigmask_r(TRIGGERABLE_WIDTH-1 downto 0))=trigval_r(TRIGGERABLE_WIDTH-1 downto 0) else '0';

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
      addr_r <= (others => '0');
    elsif rising_edge(clk_i) then
      if clocktick_s='1' then
        addr_r <= addr_r + 1;
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
        if triggered_r='0' then
          trig_addr_r <= std_logic_vector(addr_r);
        end if;
      end if;
      if start_capture_r='1' then
        triggered_r <= '0';
      end if;
    end if;
  end process;

  process(clk_i,arst_i)
  begin
    if arst_i='1' then
      counter_r <= max_counter;
    elsif rising_edge(clk_i) then
      if start_counter_s='1' then
        counter_r <= max_counter;
      elsif clocktick_s='1' and triggered_r='1' then
        if counter_r/=0 then
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

      if wr_i='1' then
        if addr_i(addr_i'high)='0' then
          case addr_i(3 downto 0) is
            when "1100" =>
              control_r(7 downto 0) <= din_i;
            when "1101" =>
              control_r(15 downto 8) <= din_i;
            when "1110" =>
              control_r(23 downto 16) <= din_i;
            when "1111" =>
              control_r(31 downto 24) <= din_i;
              if din_i(7)='1' then
                start_capture_r<='1';
              end if;

            when "0100" =>
              trigmask_r(7 downto 0) <= din_i;
            when "0101" =>
              trigmask_r(15 downto 8) <= din_i;
            when "0110" =>
              trigmask_r(23 downto 16) <= din_i;
            when "0111" =>
              trigmask_r(31 downto 24) <= din_i;

            when "1000" =>
              trigval_r(7 downto 0) <= din_i;
            when "1001" =>
              trigval_r(15 downto 8) <= din_i;
            when "1010" =>
              trigval_r(23 downto 16) <= din_i;
            when "1011" =>
              trigval_r(31 downto 24) <= din_i;
            when others =>
          end case;
        end if;
      end if;
    end if;
  end process;


  capram: entity work.generic_dp_ram
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
    addrb   => std_logic_vector(addr_i(WIDTH_BITS-1 downto 0)),
    dob     => nontrig_data_s
  );

  --capdata_s <= nontrig_data_s & trig_data_s;

  process(addr_i, control_r, counter_r, trig_addr_r)
  begin
    if addr_i(addr_i'high)='1' then
      dout_o <= capdata_s;
    else
      dout_o <= (others => 'X');
      case addr_i(3 downto 2) is
        when "11" =>
          -- control
          dout_o <= byteat(control_r, addr_i(1 downto 0));
        when "10" =>
        when "01" =>
          dout_o <= byteat(std_logic_vector(counter_r), addr_i(1 downto 0));
        when "00" =>
          dout_o <= byteat(trig_addr_r, addr_i(1 downto 0));
        when others =>
      end case;
    end if;
  end process;

  process(clk_i, arst_i)
  begin


  end process;



end beh;
