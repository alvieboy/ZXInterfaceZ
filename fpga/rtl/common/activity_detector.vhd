library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

entity activity_detector is
  generic (
    PRESCALE  :  natural := 1
  );
  port (
    clk_i     : in std_logic;
    arst_i    : in std_logic;
    tick_i    : in std_logic;

    dat_i     : in std_logic;
    idlecnt_o : out std_logic_vector(7 downto 0)
  );
end entity activity_detector;

architecture beh of activity_detector is

  signal idlecnt_r  : unsigned(7 downto 0);
  signal prescale_r : natural range 0 to PRESCALE-1;
  signal dat_r      : std_logic;
  signal event_r    : std_logic;

begin

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      dat_r <= '0';
    elsif rising_edge(clk_i) then
      dat_r <= dat_i;
    end if;
  end process;

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      event_r     <= '0';
      idlecnt_r   <= (others => '0');
      prescale_r  <= PRESCALE-1;
    elsif rising_edge(clk_i) then
      if dat_r /= dat_i then
        -- Activity
        idlecnt_r <= (others => '0');
      else
        if tick_i='1' then
          if prescale_r=0 then
            if idlecnt_r/=x"FF" then
              idlecnt_r <= idlecnt_r + 1;
            end if;
            prescale_r <= PRESCALE-1;
          else
            prescale_r <= prescale_r - 1;
          end if;
        end if;
      end if;
    end if;
  end process;

  idlecnt_o <= std_logic_vector(idlecnt_r);

end beh;
