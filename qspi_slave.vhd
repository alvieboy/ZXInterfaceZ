library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
use IEEE.STD_logic_misc.all;

entity qspi_slave is
  port (
    SCK_i         : in std_logic;
    CSN_i         : in std_logic;
    --D_io          : inout std_logic_vector(3 downto 0);
    MOSI_i        : in std_logic;
    MISO_o        : out std_logic;
    txdat_i       : in std_logic_vector(7 downto 0);
    txload_i      : in std_logic;
    txready_o     : out std_logic;
    txden_i       : in std_logic;
    qen_i         : in std_logic; -- Quad enabled

    dat_o         : out std_logic_vector(7 downto 0);
    dat_valid_o   : out std_logic
  );

end entity qspi_slave;

architecture beh of qspi_slave is

  signal shreg_r  : std_logic_vector(7 downto 0);
  signal outreg_r : std_logic_vector(7 downto 0);
  --signal nibble_r : std_logic;
  signal cnt_r    : unsigned(2 downto 0);
  --signal nibbleout_r : std_logic;
  signal din_s    : std_logic_vector(7 downto 0);
  signal outen_r  : std_logic;
  signal load_s   : std_logic;
  signal txready_r : std_logic;

  --alias MOSI      : std_logic IS D_io(0);
  --alias MISO      : std_logic IS D_io(1);
  --alias QWP       : std_logic IS D_io(2);
  --alias QHD       : std_logic IS D_io(3);
begin

  process(SCK_i, CSN_i)
  begin
    if CSN_i='1' then
      shreg_r     <= (others => '0');
      cnt_r       <= (others => '0');
      txready_r   <= '1';
    elsif rising_edge(SCK_i) then
      if qen_i='1' then
        --shreg_r   <= shreg_r(3 downto 0) & D_io;
      else
        shreg_r   <= shreg_r(6 downto 0) & MOSI_i;
      end if;
      cnt_r <= cnt_r + 1;
      txready_r <= load_s;
    end if;
  end process;

  process(cnt_r, qen_i)
  begin
    if qen_i='1' then
      load_s <= cnt_r(0);
    else
      load_s <= and_reduce(std_logic_vector(cnt_r));
    end if;
  end process;

  process(SCK_i, CSN_i)
  begin
    if CSN_i='1' then
      outreg_r      <= (others => '0');
      --nibbleout_r   <= '1';
    elsif falling_edge(SCK_i) then
      outen_r <= txden_i;
      if txden_i='1' then
        if txload_i='1' and txready_r='1' then
          outreg_r <= txdat_i;
        else
          if qen_i='1' then
            outreg_r(7 downto 4) <= outreg_r(3 downto 0);
          else
            outreg_r(7 downto 0) <= outreg_r(6 downto 0) & '0';
          end if;
        end if;
      end if;
    end if;
  end process;

  txready_o     <= txready_r;

  process(qen_i, shreg_r, MOSI_i, outen_r, outreg_r)
  begin
    if qen_i='1' then
      --din_s         <= shreg_r(3 downto 0) & D_io;
    else
      din_s         <= shreg_r(6 downto 0) & MOSI_i;
    end if;

    if outen_r='0' then
      MISO_o <= 'Z';--D_io <= (others => 'Z');
    else
      if qen_i='0' then
        --QWP <= 'Z';
        --QHD <= 'Z';
        --MOSI <= 'Z';
        MISO_o <= outreg_r(7);
      else
        --D_io <= outreg_r(7 downto 4);
      end if;
    end if;

  end process;

  dat_o         <= din_s;
  dat_valid_o   <= load_s;


end architecture;

