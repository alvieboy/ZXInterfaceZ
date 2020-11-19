LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.bfm_spectrum_p.all;


entity bfm_spectrum is
  port (
    Cmd_i   : in Cmd_Spectrum_type;
    Data_o  : out Data_Spectrum_type;

    clk_i   : in std_logic;
    ck_o    : out std_logic;
    wr_o    : out std_logic;
    rd_o    : out std_logic;
    mreq_o  : out std_logic;
    m1_o    : out std_logic;
    rfsh_o  : out std_logic;
    ioreq_o : out std_logic;
    int_o   : out std_logic;
    wait_i  : in  std_logic;
    nmi_i   : in  std_logic;
    a_o     : out std_logic_vector(15 downto 0);
    d_io    : inout std_logic_vector(7 downto 0)
  );
end entity bfm_spectrum;

architecture sim of bfm_spectrum is

  signal a_s: std_logic_vector(15 downto 0);

  constant C_TDCRA    : time  := 110 ns;
  constant C_TDCRRD   : time  := 85 ns;
  constant C_TDCRWR   : time  := 65 ns;
  constant C_TDCRIORQ : time  := 75 ns;
  constant C_TSDCF    : time  := 50 ns;


	signal SPECT_RESET_n         : std_logic;
  signal SPECT_CLK_n           : std_logic;
  signal SPECT_INT_n           : std_logic;
  signal SPECT_NMI_n           : std_logic;
  signal SPECT_M1_n            : std_logic;
  signal SPECT_MREQ_n          : std_logic;
  signal SPECT_IORQ_n          : std_logic;
  signal SPECT_RD_n            : std_logic;
  signal SPECT_WR_n            : std_logic;
  signal SPECT_RFSH_n          : std_logic;
  signal SPECT_A               : std_logic_vector(15 downto 0);

  signal local_INT_n           : std_logic := '1';
  signal local_NMI_n           : std_logic;
  signal local_M1_n            : std_logic;
  signal local_MREQ_n          : std_logic;
  signal local_IORQ_n          : std_logic;
  signal local_RD_n            : std_logic;
  signal local_WR_n            : std_logic;
  signal local_RFSH_n          : std_logic;
  signal local_CLK             : std_logic := '1';
  signal local_A               : std_logic_vector(15 downto 0);

  signal device_sel           : std_logic := '1';
  signal z80_enable           : std_logic := '0';
begin

  SPECT_NMI_n <= to_X01(nmi_i);

  realz80_inst: entity work.z80_top
  port map (
		RESET_n         => SPECT_RESET_n,
		CLK_n           => SPECT_CLK_n,
		INT_n           => SPECT_INT_n,
		NMI_n           => SPECT_NMI_n,
		M1_n            => SPECT_M1_n,
		MREQ_n          => SPECT_MREQ_n,
		IORQ_n          => SPECT_IORQ_n,
		RD_n            => SPECT_RD_n,
		WR_n            => SPECT_WR_n,
		RFSH_n          => SPECT_RFSH_n,
    WAIT_n          => wait_i,
		A               => SPECT_A,
		D               => d_io
  );


    wr_o    <= SPECT_WR_n     when device_sel='0' else local_WR_n;
    rd_o    <= SPECT_RD_n     when device_sel='0' else local_RD_n;
    mreq_o  <= SPECT_MREQ_n   when device_sel='0' else local_MREQ_n;
    m1_o    <= SPECT_M1_n     when device_sel='0' else local_M1_n;
    rfsh_o  <= SPECT_RFSH_n   when device_sel='0' else local_RFSH_n;
    ioreq_o <= SPECT_IORQ_n   when device_sel='0' else local_IORQ_n;
    a_o     <= SPECT_A        when device_sel='0' else local_A;
    ck_o    <= SPECT_CLK_n    when device_sel='0' else local_CLK;

    SPECT_CLK_n <= not clk_i when device_sel='0' else '1';

    int_o   <= local_INT_n;

  process
  begin
    wait on z80_enable;
    if z80_enable='1' then
      device_sel<='0';
      SPECT_RESET_n <= '0';
      wait for 1 us;
      SPECT_RESET_n <= '1';
    else
      device_sel <= '1';
    end if;
  end process;



  process
  begin
    local_WR_n      <= '1';
    local_RD_n      <= '1';
    local_MREQ_n    <= '1';
    local_IORQ_n    <= '1';
    local_M1_n      <= '1';
    local_RFSH_n    <= '1';
    d_io      <= (others => 'Z');




    l1: loop
      --report "Wait cmd";

      wait on Cmd_i.Cmd;
      --report "Got cmd";
      Data_o.Busy <= true;

      case Cmd_i.Cmd is
        when WRITEIO =>
          z80_enable <= '0';
          wait until rising_edge(clk_i);
          local_A <= Cmd_i.Address after C_TDCRA;
          wait until falling_edge(clk_i);
          local_IORQ_n <= '0' after C_TDCRIORQ;
          d_io    <= Cmd_i.Data;
          wait until falling_edge(clk_i);
          local_WR_n    <= '0' after C_TDCRWR;
          wait until falling_edge(clk_i);
          local_WR_n    <= '1';
          local_IORQ_n <= '1';
          d_io    <= (others => 'Z');

        when READIO =>
          z80_enable <= '0';
          wait until rising_edge(clk_i);
          local_A <= Cmd_i.Address after C_TDCRA;
          wait until falling_edge(clk_i);
          local_IORQ_n <= '0' after C_TDCRIORQ;
          local_RD_n    <= '0' after C_TDCRRD;
          --d_io    <= Cmd_i.Data;
          wait until falling_edge(clk_i);
          wait until falling_edge(clk_i);
          Data_o.Data <= d_io;
          local_RD_n    <= '1';
          local_IORQ_n <= '1';

        when WRITEMEM =>
          z80_enable <= '0';
          wait until rising_edge(clk_i);
          local_A <= Cmd_i.Address;
          wait until falling_edge(clk_i);
          local_MREQ_n <= '0';
          d_io    <= Cmd_i.Data;
          wait until falling_edge(clk_i);
          local_WR_n    <= '0';
          wait until falling_edge(clk_i);
          local_WR_n    <= '1';
          local_MREQ_n <= '1';
          d_io    <= (others => 'Z');

        when READMEM =>
          z80_enable <= '0';
          wait until rising_edge(clk_i);
          local_A <= Cmd_i.Address;
          wait until falling_edge(clk_i);
          local_MREQ_n <= '0';
          local_RD_n    <= '0';
          --d_io    <= Cmd_i.Data;
          wait until falling_edge(clk_i);
          wait until falling_edge(clk_i);
          Data_o.Data <= d_io;
          local_RD_n    <= '1';
          local_MREQ_n <= '1';

        when READOPCODE =>
          z80_enable <= '0';
          wait until rising_edge(clk_i);
          local_A <= Cmd_i.Address;
          local_M1_n    <= '0';
          wait until falling_edge(clk_i);
          local_MREQ_n  <= '0';
          local_RD_n    <= '0';
          --d_io    <= Cmd_i.Data;
          wait until rising_edge(clk_i);
          wait until rising_edge(clk_i);
          Data_o.Data <= d_io;
          local_RD_n    <= '1';
          local_MREQ_n  <= '1';
          local_M1_n    <= '1';
          -- Perform refresh request
          local_A     <= Cmd_i.Refresh;
          local_RFSH_n  <= '0';
          wait until falling_edge(clk_i);
          local_MREQ_n  <= '0';
          wait until falling_edge(clk_i);
          local_MREQ_n  <= '1';
          --wait until rising_edge(clk_i);
          local_RFSH_n  <= '1' after 20 ns;
        when RUNZ80 =>
          z80_enable <= '1';
          wait for 0 ps;
        when STOPZ80 =>
          z80_enable <= '0';
          wait for 0 ps;

        when SETPIN =>
          z80_enable <= '0';
          case Cmd_i.Pin is
            when PIN_ADDRESS  => local_A      <= Cmd_i.Address;
            when PIN_DATA     => d_io         <= Cmd_i.Data;
            when PIN_CLK      => local_CLK    <= Cmd_i.Data(0);
            when PIN_M1       => local_M1_n   <= Cmd_i.Data(0);
            when PIN_MREQ     => local_MREQ_n <= Cmd_i.Data(0);
            when PIN_IORQ     => local_IORQ_n <= Cmd_i.Data(0);
            when PIN_RD       => local_RD_n   <= Cmd_i.Data(0);
            when PIN_WR       => local_WR_n   <= Cmd_i.Data(0);
            when PIN_INT      => local_INT_n  <= Cmd_i.Data(0);
            when PIN_RFSH     => local_RFSH_n <= Cmd_i.Data(0);
          end case;
          wait for 0 ps;
        when SAMPLEPINS =>
          Data_o.Data <= d_io;
          Data_o.WaitPin <= wait_i;
          wait for 0 ps;

        when others =>
      end case;
      Data_o.Busy <= false;
    end loop;
  end process;
end sim;
