LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;
LIBRARY work;
use work.zxinterfacepkg.all;

entity bit_ctrl is
  port (
    clk_i         : in std_logic;
    arst_i        : in std_logic;

    bit_enable_i  : in std_logic;

    bit_data_i    : in std_logic_vector(31 downto 0);
    bit_data_o    : out std_logic_vector(31 downto 0);

    -- read/write
    bit_we_i      : in std_logic;
    bit_index_i   : in unsigned(1 downto 0);
    bit_din_i     : in std_logic_vector(7 downto 0);
    bit_dout_o    : out std_logic_vector(7 downto 0)
  );

end entity bit_ctrl;

architecture beh of bit_ctrl is

  signal dout_r       : std_logic_vector(31 downto 0);
  signal dout_temp_r  : std_logic_vector(23 downto 0);

begin

  bit_dout_o <= bit_data_i(((1+to_integer(to_01(bit_index_i)))*8)-1 downto (8*(to_integer(to_01(bit_index_i)))));

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      dout_r  <= (others => '0');
    elsif rising_edge(clk_i) then
      if bit_enable_i='0' then
        dout_r  <= (others => '0');
      else
        if bit_we_i='1' and bit_index_i=3 then
          dout_r <= bit_din_i & dout_temp_r;
        end if;
      end if;
    end if;
  end process;

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      dout_temp_r  <= (others => '0');
    elsif rising_edge(clk_i) then
      if bit_we_i='1' then
        case bit_index_i is
          when "00" =>
            dout_temp_r(7 downto 0) <= bit_din_i;
          when "01" =>
            dout_temp_r(15 downto 8) <= bit_din_i;
          when "10" =>
            dout_temp_r(23 downto 16) <= bit_din_i;
          --when 3 =>
            --dout_temp_r(31 downto 24) <= bit_din_i;
          when others =>
        end case;
      end if;
    end if;
  end process;

  bit_data_o <= dout_r;

end beh;
