library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

entity qspi_slave is
  port (
    SCK_i         : in std_logic;
    CSN_i         : in std_logic;
    D_io          : inout std_logic_vector(3 downto 0);

    txdat_i       : in std_logic_vector(7 downto 0);
    txload_i      : in std_logic;
    txready_o     : out std_logic;
    txden_i       : in std_logic;

    dat_o         : out std_logic_vector(7 downto 0);
    dat_valid_o   : out std_logic
  );

end entity qspi_slave;

architecture beh of qspi_slave is

  signal shreg_r  : std_logic_vector(7 downto 0);
  signal outreg_r : std_logic_vector(7 downto 0);
  signal nibble_r : std_logic;
  signal nibbleout_r : std_logic;
  signal din_s    : std_logic_vector(7 downto 0);
  signal outen_r  : std_logic;

begin

  process(SCK_i, CSN_i)
  begin
    if CSN_i='1' then
      shreg_r   <= (others => '0');
      nibble_r  <= '0';
    elsif rising_edge(SCK_i) then
      shreg_r   <= shreg_r(3 downto 0) & D_io;
      nibble_r  <= not nibble_r;
    end if;
  end process;


  process(SCK_i, CSN_i)
  begin
    if CSN_i='1' then
      outreg_r   <= (others => '0');
      nibbleout_r  <= '1';
    elsif falling_edge(SCK_i) then
      outen_r <= txden_i;
      if txden_i='1' then
        nibbleout_r <= not nibbleout_r;
        if txload_i='1' and nibbleout_r='1' then
          outreg_r <= txdat_i;
        else
          outreg_r(7 downto 4) <= outreg_r(3 downto 0);
        end if;
      else
        nibbleout_r <= '1';
      end if;
    end if;
  end process;

  txready_o <= nibbleout_r;

  din_s <= shreg_r(3 downto 0) & D_io;

  dat_o         <= din_s;
  dat_valid_o   <= nibble_r;

  D_io <= (others => 'Z') when outen_r='0' else outreg_r(7 downto 4);

end architecture;

