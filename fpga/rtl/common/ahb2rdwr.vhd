library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
library work;
use work.ahbpkg.all;

entity ahb2rdwr is
  generic (
    AWIDTH: natural := 13;
    DWIDTH: natural := 8
  );
  port (
    clk_i     : in std_logic;
    arst_i    : in std_logic;

    ahb_m2s_i : in AHB_M2S;
    ahb_s2m_o : out AHB_S2M;
    addr_o    : out std_logic_vector(AWIDTH-1 downto 0);
    dat_o     : out std_logic_vector(DWIDTH-1 downto 0);
    dat_i     : in  std_logic_vector(DWIDTH-1 downto 0);
    rd_o      : out std_logic;
    wr_o      : out std_logic
  );
end entity ahb2rdwr;

architecture beh of ahb2rdwr is

  signal ahb_write_r    : std_logic;
  signal ahb_addr_r     : std_logic_vector(AWIDTH-1 downto 0);

begin

  process(arst_i, clk_i)
  begin
    if arst_i='1' then
      ahb_write_r <= '0';
      ahb_addr_r  <= (others => 'X');
    elsif rising_edge(clk_i) then
      if ahb_m2s_i.HTRANS /= C_AHB_TRANS_IDLE and ahb_m2s_i.HWRITE='1' then
        ahb_write_r <= '1';
        ahb_addr_r  <= ahb_m2s_i.HADDR(AWIDTH-1 downto 0);
      else
        ahb_write_r <= '0';
      end if;
    end if;
  end process;

  rd_o    <= '1' when ahb_m2s_i.HTRANS /= C_AHB_TRANS_IDLE and ahb_m2s_i.HWRITE='0' else '0';
  wr_o    <= '1' when ahb_write_r='1' else '0';
  dat_o   <= ahb_m2s_i.HWDATA(DWIDTH-1 downto 0);
  addr_o  <= ahb_m2s_i.HADDR(AWIDTH-1 downto 0) when ahb_write_r='0' else ahb_addr_r;

  ahb_s2m_o.HRDATA(DWIDTH-1 downto 0) <= dat_i;
  ahb_s2m_o.HREADY <= '1';
  ahb_s2m_o.HRESP  <= '0';

end beh;

