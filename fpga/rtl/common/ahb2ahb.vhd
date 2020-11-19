LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.NUMERIC_STD.ALL;
LIBRARY work;
use work.ahbpkg.all;

entity ahb2ahb is
  generic (
    AWIDTH: natural := 13;
    DWIDTH: natural := 8
  );
  port (
    -- Master
    mclk_i  : in std_logic;
    arst_i : in std_logic;
    m2s_i   : in AHB_M2S;
    s2m_o   : out AHB_S2M;
    -- Slave
    sclk_i  : in std_logic;
    m2s_o   : out AHB_M2S;
    s2m_i   : in AHB_S2M
  );
end entity ahb2ahb;

architecture beh of ahb2ahb is

  constant REQWIDTH: natural := AWIDTH + DWIDTH + 1;
  constant RESPWIDTH: natural := DWIDTH + 1;

  signal request_din_s      : std_logic_vector(REQWIDTH-1 downto 0);
  signal request_dout_s     : std_logic_vector(REQWIDTH-1 downto 0);
  signal request_queued_s   : std_logic;
  signal request_rdempty_s  : std_logic;
  signal request_rdreq_s    : std_logic;
  signal request_wrreq_s    : std_logic;
  signal request_wrfull_s   : std_logic;
  signal request_wdata_r    : std_logic_vector(DWIDTH-1 downto 0);
  signal request_addr_r     : std_logic_vector(AWIDTH-1 downto 0);

  signal response_din_s      : std_logic_vector(RESPWIDTH-1 downto 0);
  signal response_dout_s     : std_logic_vector(RESPWIDTH-1 downto 0);
  signal response_rdempty_s  : std_logic;
  signal response_rdreq_s    : std_logic;
  signal response_wrreq_s    : std_logic;
  signal response_wrfull_s   : std_logic;


  type state_type is (
    IDLE,
    ADDRESS,
    DATA
   );

  type reqstate_type is (
    IDLE,
    DATA,
    BUSY
   );

  signal state_r: state_type;
  signal reqstate_r: reqstate_type;
  --signal request_queued_r  : std_logic;

begin

  request_din_s <= request_addr_r &
                   m2s_i.HWDATA(DWIDTH-1 downto 0) &
                   m2s_i.HWRITE;

  request_fifo_inst: ENTITY work.swfifo
    GENERIC map (
      WIDTH => REQWIDTH
    )
    PORT map
    (
      aclr		=> arst_i,
      data		=> request_din_s,
      rdclk		=> sclk_i,
      rdreq		=> request_rdreq_s,
      wrclk		=> mclk_i,
      wrreq		=> request_wrreq_s,
      q		    => request_dout_s,
      rdempty	=> request_rdempty_s,
      wrfull	=> request_wrfull_s
    );

  response_fifo_inst: ENTITY work.swfifo
    GENERIC map (
      WIDTH => RESPWIDTH
    )
    PORT map
    (
      aclr		=> arst_i,
      data		=> response_din_s,
      rdclk		=> mclk_i,
      rdreq		=> response_rdreq_s,
      wrclk		=> sclk_i,
      wrreq		=> response_wrreq_s,
      q		    => response_dout_s,
      rdempty	=> response_rdempty_s,
      wrfull	=> response_wrfull_s
    );


  request_wrreq_s <= '1' when reqstate_r=DATA else '0';
  response_rdreq_s <='1' when reqstate_r=BUSY and  response_rdempty_s='0' else '0';

  process(mclk_i,arst_i)
  begin
    if arst_i='1' then
      reqstate_r<=IDLE;
    elsif rising_edge(mclk_i) then
      case reqstate_r is
        when IDLE =>
          if m2s_i.HTRANS = C_AHB_TRANS_NONSEQ or m2s_i.HTRANS=C_AHB_TRANS_SEQ then
            reqstate_r <= DATA;
            request_addr_r <= m2s_i.HADDR(AWIDTH-1 downto 0);
          end if;
        when DATA =>
           reqstate_r <= BUSY;
        when BUSY =>
          if response_rdempty_s='0' then
            if m2s_i.HTRANS /= C_AHB_TRANS_NONSEQ then
              reqstate_r <= IDLE;
            else
              reqstate_r <= DATA;
              request_addr_r <= m2s_i.HADDR(AWIDTH-1 downto 0);
            end if;
          end if;
      end case;
    end if;
  end process;


  s2m_o.HREADY <= '1' when reqstate_r=IDLE or response_rdempty_s='0' else '0';
  s2m_o.HRDATA(DWIDTH-1 downto 0) <= response_dout_s(RESPWIDTH-1 downto 1);
  s2m_o.HRESP  <= response_dout_s(0);

  -- TBD
  request_rdreq_s <= not request_rdempty_s when state_r=IDLE else '0';

  m2s_o.HADDR(AWIDTH-1 downto 0)   <= request_dout_s(REQWIDTH-1 downto REQWIDTH-AWIDTH);

  m2s_o.HWDATA(DWIDTH-1 downto 0)  <= request_wdata_r;

  

  m2s_o.HWRITE  <= request_dout_s(0);
  --m2s_o.HADDR   <= request_dout_s(0);
  m2s_o.HSIZE  <= C_AHB_SIZE_BYTE;
  m2s_o.HPROT  <= "0000";
  m2s_o.HBURST <= C_AHB_BURST_SINGLE;

  response_wrreq_s <= '1' when state_r=DATA and s2m_i.HREADY='1' else '0';
  response_din_s <= s2m_i.HRDATA(DWIDTH-1 downto 0) & s2m_i.HRESP;

  process(sclk_i,arst_i)
  begin
    if arst_i='1' then
      m2s_o.HTRANS <= C_AHB_TRANS_IDLE;
      state_r <= IDLE;
    elsif rising_edge(sclk_i) then
      case state_r is
        when IDLE =>
          if request_rdempty_s='0' then
            m2s_o.HTRANS <= C_AHB_TRANS_NONSEQ;
            state_r <= ADDRESS;
          end if;
        when ADDRESS =>
          if s2m_i.HREADY='1' then
            request_wdata_r <= request_dout_s(REQWIDTH-AWIDTH-1 downto REQWIDTH-AWIDTH-DWIDTH);
            state_r <= DATA;
            m2s_o.HTRANS <= C_AHB_TRANS_IDLE;
          end if;
        when DATA =>
          if s2m_i.HREADY='1' then
            state_r <= IDLE;
          end if;
      end case;
    end if;
  end process;
  

end beh;
