process
    procedure genRead( a: std_logic_vector(15 downto 0) ) is
    begin
      XCK_s <= '1';
      ZX_A_s  <= a;
      wait for ZXPERIOD/4;
      XCK_s <= '0';
      XMREQ_s <='0';
      XRD_s <='0';
      wait for ZXPERIOD/4;
      XCK_s <= '1';
      wait for ZXPERIOD/4;
      XCK_s <= '0';
      wait for ZXPERIOD/4;
      XCK_s <= '1';
      XMREQ_s <= '1';
      XRD_s   <= '1';
      wait for ZXPERIOD/4;
      XCK_s <= '0';
    end procedure;
  begin
    XCK_s     <= '0';
    XMREQ_s   <= '1';
    XIORQ_s   <= '1';
    XRD_s     <= '1';
    XWR_s     <= '1';
    XM1_s     <= '0';
    wait for 1 us;
    genRead( x"DEAD" );
    wait;
  end process;
