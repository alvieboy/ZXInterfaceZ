library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

entity spi_interface is
  port (
    SCK_i         : in std_logic;
    CSN_i         : in std_logic;

    --D_io          : inout std_logic_vector(3 downto 0);
    MOSI_i        : in std_logic;
    MISO_o        : out std_logic;

    fifo_empty_i  : in std_logic;
    fifo_rd_o     : out std_logic;
    fifo_data_i   : in std_logic_vector(31 downto 0)
  );

end entity spi_interface;

architecture beh of spi_interface is

  signal txdat_s      : std_logic_vector(7 downto 0);
  signal txload_s     : std_logic;
  signal txready_s    : std_logic;
  signal txden_s      : std_logic;
  
  signal dat_s        : std_logic_vector(7 downto 0);
  signal dat_valid_s  : std_logic;
  signal wordindex_r  : unsigned(1 downto 0);


  type state_type is (
    IDLE,
    UNKNOWN,
    READID,
    READSTATUS,
    READDATA
  );

  signal state_r      : state_type;
begin

  spi_inst: entity work.qspi_slave
  port map (
    SCK_i         => SCK_i,
    CSN_i         => CSN_i,
    --D_io          => D_io,
    MOSI_i        => MOSI_i,
    MISO_o        => MISO_o,
    txdat_i       => txdat_s,
    txload_i      => txload_s,
    txready_o     => txready_s,
    txden_i       => txden_s,
    qen_i         => '0',

    dat_o         => dat_s,
    dat_valid_o   => dat_valid_s
  );

  process(SCK_i, CSN_i)
  begin
    if CSN_i='1' then
      state_r <= IDLE;
      txden_s <= '0';
      txload_s <= '0';
      fifo_rd_o <= '0';
    elsif rising_edge(SCK_i) then
      fifo_rd_o <= '0';
      case state_r is
        when IDLE =>
          if dat_valid_s='1' then
            case dat_s is
              when x"DE" => -- Read status
                txden_s <= '1';
                txload_s <= '1';
                txdat_s <= "0000000" & fifo_empty_i;
                state_r <= READSTATUS;
              when x"FC" => -- Read data
                txden_s <= '1';
                txload_s <= '1';
                wordindex_r <= "00";
                if fifo_empty_i='1' then
                  txdat_s <= x"FF";
                  state_r <= UNKNOWN;
                else
                  txdat_s <= x"FE";
                  state_r <= READDATA;
                end if;

              when x"9E" | x"9F" => -- Read ID
                txden_s <= '1';
                txload_s <= '1';
                wordindex_r <= "00";
                txdat_s <= x"A5";
                state_r <= READID;
                
              when others =>
                -- Unknown command
                state_r <= UNKNOWN;
            end case;
          end if;
        when READSTATUS =>
          txload_s <= '1';
          txden_s <= '1';

        when READDATA =>
          txload_s <= '1';
          txden_s <= '1';
          if txready_s='1' then
            wordindex_r <= wordindex_r + 1;
          end if;
          case wordindex_r is
            when "00" => txdat_s <= fifo_data_i(7 downto 0);
            when "01" => txdat_s <= fifo_data_i(15 downto 8);
            when "10" => txdat_s <= fifo_data_i(23 downto 16);
            when "11" => txdat_s <= fifo_data_i(31 downto 24);
            when others =>
          end case;

          if wordindex_r="11" and txready_s='1' then
            fifo_rd_o <= '1';
          end if;

        when READID =>
          txload_s <= '1';
          txden_s <= '1';
          if txready_s='1' then
            wordindex_r <= wordindex_r + 1;
          end if;
          case wordindex_r is
            when "00" => txdat_s <= x"A5";
            when "01" => txdat_s <= x"10";
            when "10" => txdat_s <= x"00";
            when "11" => txdat_s <= x"01";
            when others =>
          end case;

          if wordindex_r="11" and txready_s='1' then
            fifo_rd_o <= '1';
          end if;

        when others =>
          txload_s <= '0';
          txden_s <= '0';
      end case;
    end if;
  end process;

end beh;
