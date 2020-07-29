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
begin

  ck_o <= clk_i;

  process
  begin
    wr_o      <= '1';
    rd_o      <= '1';
    mreq_o    <= '1';
    ioreq_o   <= '1';
    m1_o      <= '1';
    rfsh_o    <= '1';
    d_io      <= (others => 'Z');

    l1: loop
      --report "Wait cmd";
      wait on Cmd_i.Cmd;
      --report "Got cmd";
      Data_o.Busy <= true;

      case Cmd_i.Cmd is
        when WRITEIO =>
          wait until rising_edge(clk_i);
          a_o <= Cmd_i.Address after C_TDCRA;
          wait until falling_edge(clk_i);
          ioreq_o <= '0' after C_TDCRIORQ;
          d_io    <= Cmd_i.Data;
          wait until falling_edge(clk_i);
          wr_o    <= '0' after C_TDCRWR;
          wait until falling_edge(clk_i);
          wr_o    <= '1';
          ioreq_o <= '1';
          d_io    <= (others => 'Z');

        when READIO =>
          wait until rising_edge(clk_i);
          a_o <= Cmd_i.Address after C_TDCRA;
          wait until falling_edge(clk_i);
          ioreq_o <= '0' after C_TDCRIORQ;
          rd_o    <= '0' after C_TDCRRD;
          --d_io    <= Cmd_i.Data;
          wait until falling_edge(clk_i);
          wait until falling_edge(clk_i);
          Data_o.Data <= d_io;
          rd_o    <= '1';
          ioreq_o <= '1';

        when WRITEMEM =>
          wait until rising_edge(clk_i);
          a_o <= Cmd_i.Address;
          wait until falling_edge(clk_i);
          mreq_o <= '0';
          d_io    <= Cmd_i.Data;
          wait until falling_edge(clk_i);
          wr_o    <= '0';
          wait until falling_edge(clk_i);
          wr_o    <= '1';
          mreq_o <= '1';
          d_io    <= (others => 'Z');

        when READMEM =>
          wait until rising_edge(clk_i);
          a_o <= Cmd_i.Address;
          wait until falling_edge(clk_i);
          mreq_o <= '0';
          rd_o    <= '0';
          --d_io    <= Cmd_i.Data;
          wait until falling_edge(clk_i);
          wait until falling_edge(clk_i);
          Data_o.Data <= d_io;
          rd_o    <= '1';
          mreq_o <= '1';

        when READOPCODE =>
          wait until rising_edge(clk_i);
          a_o <= Cmd_i.Address;
          m1_o    <= '0';
          wait until falling_edge(clk_i);
          mreq_o  <= '0';
          rd_o    <= '0';
          --d_io    <= Cmd_i.Data;
          wait until rising_edge(clk_i);
          wait until rising_edge(clk_i);
          Data_o.Data <= d_io;
          rd_o    <= '1';
          mreq_o  <= '1';
          m1_o    <= '1';
          -- Perform refresh request
          a_o     <= Cmd_i.Refresh;
          rfsh_o  <= '0';
          wait until falling_edge(clk_i);
          mreq_o  <= '0';
          wait until falling_edge(clk_i);
          mreq_o  <= '1';
          --wait until rising_edge(clk_i);
          rfsh_o  <= '1' after 20 ns;

        when others =>
      end case;
      Data_o.Busy <= false;
    end loop;
  end process;
end sim;
