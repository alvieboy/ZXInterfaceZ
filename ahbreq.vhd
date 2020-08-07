LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;
LIBRARY work;
use work.ahbpkg.all;

entity ahbreq is
  generic (
    MODE: string -- "LEVEL" or "TOGGLE"
  );
  port (
    clk_i     : in std_logic;
    arst_i    : in std_logic;

    addr_i    : in std_logic_vector(31 downto 0);
    trans_i   : in std_logic;
    we_i      : in std_logic;
    valid_o   : out std_logic;
    data_o    : out std_logic_vector(31 downto 0);
    data_i    : in std_logic_vector(31 downto 0);

    m_i       : in AHB_S2M;
    m_o       : out AHB_M2S
  );
end entity ahbreq;

architecture beh of ahbreq is


  type state_type is ( IDLE, WAITREADY, WAITCOMPLETE );

  type regs_type is record
    state     : state_type;
    trans     : std_logic;
    data      : std_logic_vector(31 downto 0);
    wdata     : std_logic_vector(31 downto 0);
    busy      : std_logic;
    valid     : std_logic;
    write     : std_logic;
  end record;

  signal r        : regs_type;
  signal trans_s  : std_logic;

begin

  trans_s   <= trans_i xor r.trans when mode="TOGGLE" else trans_i;

  process(clk_i,arst_i, addr_i, m_i, trans_s, r, trans_i, we_i, data_i)
    variable w: regs_type;
  begin
    w         := r;
    if mode="TOGGLE" then
      w.trans   := trans_i;
    end if;
    w.valid   := '0';
    -- TBD: we mihgt need to delay this due to resyncs
    m_o.HTRANS  <= C_AHB_TRANS_IDLE;
    m_o.HADDR   <= addr_i;
    m_o.HWRITE  <= 'X';
    m_o.HWDATA  <= (others => 'X');
    case r.state is
      when IDLE =>
        if trans_s = '1' then
          m_o.HTRANS <= C_AHB_TRANS_SEQ;
          m_o.HWRITE <= we_i;
          m_o.HWDATA  <= data_i;
          w.busy := '1';
          w.write := we_i;
          --w.wdata := data_i;
          if m_i.HREADY='0' then
            w.state := WAITREADY;
          else
            w.state := WAITCOMPLETE;
          end if;
        end if;

      when WAITREADY =>
        m_o.HTRANS <= C_AHB_TRANS_SEQ;
        m_o.HWRITE <= r.write;
        m_o.HWDATA  <= data_i; --r.wdata;
        if m_i.HREADY='1' then
          w.state := WAITCOMPLETE;
        end if;

      when WAITCOMPLETE =>
        m_o.HWRITE <= r.write;
        m_o.HWDATA  <= data_i;
        if m_i.HREADY='1' then
          w.state := IDLE;
          w.busy  := '0';
          w.data  := m_i.HRDATA;
          w.valid := '1';
        end if;
      when others =>
    end case;

    if arst_i='1' then
      r.state <= IDLE;
      r.busy  <= '0';
      r.valid  <= '0';
      if mode="TOGGLE" then
        r.trans  <= '0';
      end if;
    elsif rising_edge(clk_i) then
      r <= w;
    end if;
  end process;

  data_o    <= r.data;
  valid_o   <= r.valid;
  m_o.HSIZE   <= C_AHB_SIZE_BYTE;
  m_o.HBURST  <= C_AHB_BURST_SINGLE;
  m_o.HPROT   <= "0000";
  m_o.HMASTLOCK <= '0';
end beh;
