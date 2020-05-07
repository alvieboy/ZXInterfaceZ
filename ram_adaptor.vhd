LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;
LIBRARY work;
use work.ahbpkg.all;

entity ram_adaptor is
  port (
    clk_i   : in std_logic;
    arst_i  : in std_logic;

    ahb_o   : out AHB_M2S;
    ahb_i   : in AHB_S2M;

    ram_addr_i             : in std_logic_vector(15 downto 0);
    ram_dat_i              : in std_logic_vector(7 downto 0);
    ram_dat_o              : out std_logic_vector(7 downto 0);
    ram_wr_i               : in std_logic;
    ram_rd_i               : in std_logic;
    ram_ack_o              : out std_logic

  );
end entity ram_adaptor;

architecture beh of ram_adaptor is

  type state_type is (
    IDLE,
    READ,
    WAITACK
  );
  type regs_type is record
    state: state_type;
  end record;

  signal r: regs_type;
begin

  process(clk_i, arst_i, ram_wr_i)
    variable w: regs_type;
  begin
    w := r;

    ahb_o.HWRITE <= 'X';
    ahb_o.HTRANS <= C_AHB_TRANS_IDLE;
    ram_ack_o <= '0';

    case r.state is
      when IDLE =>
        if ram_wr_i='1' then
          ahb_o.HTRANS <= C_AHB_TRANS_SEQ;
          ahb_o.HWRITE <= '1';
          if ahb_i.HREADY='1' then
            w.state := WAITACK;
          end if;
        elsif ram_rd_i='1' then
          ahb_o.HTRANS <= C_AHB_TRANS_SEQ;
          ahb_o.HWRITE <= '0';
          if ahb_i.HREADY='1' then
            w.state := WAITACK;
          end if;
        end if;
      when WAITACK =>
        ram_ack_o <= ahb_i.HREADY;
        if ahb_i.HREADY='1' then
          --ram_ack_o<='1';
          w.state := IDLE;
        end if;

      when others =>
    end case;

    if arst_i='1' then
      r.state <= IDLE;
    elsif rising_edge(clk_i) then
      r <= w;
    end if;
  end process;

  ram_dat_o                 <= ahb_i.HRDATA(7 downto 0);

  ahb_o.HSIZE               <= C_AHB_SIZE_BYTE;
  ahb_o.HBURST              <= C_AHB_BURST_INCR;
  ahb_o.HMASTLOCK           <= '0';
  ahb_o.HWDATA(7 downto 0)  <= ram_dat_i;
  ahb_o.HADDR(15 downto 0)  <= ram_addr_i;
  ahb_o.HADDR(23 downto 16) <= (others =>'0');


end beh;
