library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
use IEEE.std_logic_misc.all;

entity logiccapture is
  generic (
    COMPRESS_BITS: natural := 8;
    MEMWIDTH_BITS: natural := 14
  );
  port (
    clk_i       : in std_logic;
    arst_i      : in std_logic;
    stick_i     : in std_logic;

    clr_i       : in std_logic;
    run_i       : in std_logic;
    din_i       : in std_logic_vector(35-COMPRESS_BITS downto 0);
    compress_i  : in std_logic;
    trig_mask_i : in std_logic_vector(35-COMPRESS_BITS downto 0);
    trig_val_i  : in std_logic_vector(35-COMPRESS_BITS downto 0);

    clen_o      : out std_logic_vector(MEMWIDTH_BITS-1 downto 0);

    ramclk_i    : in std_logic;
    ramen_i     : in std_logic;
    ramaddr_i   : in std_logic_vector(MEMWIDTH_BITS-1 downto 0);
    ramdo_o     : out std_logic_vector(35 downto 0)
    
  );
end entity logiccapture;


architecture beh of logiccapture is

  signal sreg_r     : unsigned(MEMWIDTH_BITS-1 downto 0);
  signal cap_r      : std_logic_vector(35-COMPRESS_BITS downto 0);


  signal count_r    : unsigned(COMPRESS_BITS-1 downto 0);
  signal triggered_r: std_logic;

  signal memen_s    : std_logic;
  signal memaddr_s  : std_logic_vector(MEMWIDTH_BITS-1 downto 0);
  signal memdi_s    : std_logic_vector(35 downto 0);
  signal memdo_s    : std_logic_vector(35 downto 0);

  signal full_s     : std_logic;
  signal match_s    : std_logic;

  constant MASK     : std_logic_vector(27 downto 0) := x"FFFFFFF";

begin

  full_s  <= and_reduce(std_logic_vector(sreg_r));

  ram_inst: entity work.generic_dp_ram
  generic map (
    address_bits  => MEMWIDTH_BITS,
    data_bits     => 36
  )
  port map (
    clka      => clk_i,
    ena       => memen_s,
    wea       => '1',
    addra     => memaddr_s,
    dia       => memdi_s,
    doa       => memdo_s,

    clkb      => ramclk_i,
    enb       => ramen_i,
    web       => '0',
    addrb     => ramaddr_i,
    dib       => (others => '0'),
    dob       => ramdo_o
  );

  --memen_s <= run_i and not full_s;

  process(din_i, cap_r)
    variable c1_v, c2_v: std_logic_vector(35-COMPRESS_BITS downto 0);
  begin
    c1_v := din_i;-- AND MASK;
    c2_v := cap_r;-- AND MASK;
    if c1_v=c2_v then
      match_s <= '1';
    else
      match_s <= '0';
    end if;
  end process;

  memdi_s <= std_logic_vector(count_r) & cap_r;
  memaddr_s <= std_logic_vector(sreg_r);

  process(run_i, full_s, stick_i, match_s, count_r)
  begin
    if run_i='1' and full_s='0' and stick_i='1' then
      memen_s <= '1';
    else
      memen_s <= '0';
    end if;
  end process;

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      cap_r <= (others => '0');
      count_r <= (others => '0');
      sreg_r  <= (others => '0');
      triggered_r <= '0';

    elsif rising_edge(clk_i) then
      if clr_i='1' then
        sreg_r <= (others => '0');
        cap_r <= (others => '0');
        count_r <= (others => '0');
        triggered_r <= '0';
      elsif stick_i='1' then
        cap_r <= din_i;
        if triggered_r='1' then
          if compress_i='0' then
            if full_s='0' then
              sreg_r    <= sreg_r + 1;
            else
              -- synthesis translate_off
              report "Capture buffer full";
              -- synthesis translate_on
            end if;
            count_r   <= (others => '0');
          else
            if match_s='1' and full_s='0' then
              count_r <= count_r + 1;
            else
              if full_s='0' then
                sreg_r    <= sreg_r + 1;
                count_r   <= (others => '0');
              end if;
            end if;
          end if;
        else
          if (din_i and trig_mask_i) = (trig_val_i and trig_mask_i) then
            triggered_r<='1';
          end if;
        end if;
      end if;
    end if;
  end process;

end beh;
