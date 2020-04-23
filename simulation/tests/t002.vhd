use work.tbc_device_p.all;

architecture t002 of tbc_device is

  signal spiPayload_in_s    : spiPayload_type;
  signal spiPayload_out_s   : spiPayload_type;
  signal fifodata           : unsigned(7 downto 0);
  signal expected_fifodata  : unsigned(7 downto 0);
begin

  process
    variable depth: natural := 0;
  begin
    report "T002: Check depth of command FIFO";

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );


    fifodata <= x"00";


    wait for 32 * 10.41 ns;

    Check( "1: CtrlPins27 is 1", CtrlPins_Data.IO27 , '1');

    dcalc: for i in 0 to 64 loop

      Spectrum_Cmd.Address <= x"0007";
      Spectrum_Cmd.Cmd     <= READIO;
      wait for 0 ps;
      wait until Spectrum_Data.Busy = false;
      Spectrum_Cmd.Cmd     <= NONE;
      if Spectrum_Data.Data(0) = '1' then
        exit dcalc;
      end if;

      Spectrum_Cmd.Address  <= x"0009";
      Spectrum_Cmd.Data     <= std_logic_vector(fifodata);
      Spectrum_Cmd.Cmd      <= WRITEIO;
      wait for 0 ps;
      depth := depth + 1;
      wait until Spectrum_Data.Busy = false;
      Spectrum_Cmd.Cmd      <= NONE;
      wait for 1 us;
      fifodata <= fifodata + 1;

      wait for 32 * 10.41 ns;

      Check( "2: CtrlPins27 is 1", CtrlPins_Data.IO27 , '0');

      --Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

    end loop;

    report "FIFO depth " & str(depth);

    Check( "Depth is less than 64", depth <= 64 );

    expected_fifodata <= x"00";

    l2: for i in 0 to depth*2 loop

      -- Now, read back from FIFO.
      spiPayload_in_s(0) <= x"FB";
      spiPayload_in_s(1) <= x"00";
      spiPayload_in_s(2) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

      Check( "FIFO report not empty", spiPayload_out_s(1), x"FE");
      Check( "FIFO data consistent",  spiPayload_out_s(2), std_logic_vector(expected_fifodata));

      expected_fifodata <= expected_fifodata + 1;

      -- Check that interrupt is still asserted
      Check( "3: CtrlPins27 is 1", CtrlPins_Data.IO27 , '0');


      -- Inject more data.
      Spectrum_Cmd.Address <= x"0007";
      Spectrum_Cmd.Cmd     <= READIO;
      wait until Spectrum_Data.Busy = false;
      Spectrum_Cmd.Cmd     <= NONE;
      
      Check("FIFO not full", Spectrum_Data.Data(0) = '0');

      Spectrum_Cmd.Address  <= x"0009";
      Spectrum_Cmd.Data     <= std_logic_vector(fifodata);
      Spectrum_Cmd.Cmd      <= WRITEIO;
      wait until Spectrum_Data.Busy = false;
      Spectrum_Cmd.Cmd      <= NONE;

      Spectrum_Cmd.Address <= x"0007";
      Spectrum_Cmd.Cmd     <= READIO;
      wait until Spectrum_Data.Busy = false;
      Spectrum_Cmd.Cmd     <= NONE;

      wait for 32 * 10.41 ns;
      Check("FIFO is full", Spectrum_Data.Data(0) = '1');


      fifodata <= fifodata + 1;

    end loop l2;

    -- Now, empty fifo. It must have "queue" size elements.

    -- Clear interrupt
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"10";
    spiPayload_in_s(3) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    -- Interrupt must *still* be asserted
    Check( "4: CtrlPins27 is 1", CtrlPins_Data.IO27 , '0');

    -- Do a full burst read (-1).

    l3: for i in 0 to depth-1 loop
      spiPayload_in_s(0) <= x"FB";
      spiPayload_in_s(1) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

      Check( "FIFO report not empty", spiPayload_out_s(1), x"FE");

      Check( "FIFO data consistent", spiPayload_out_s(2), std_logic_vector(expected_fifodata));
      expected_fifodata <= expected_fifodata + 1;

      Check( "5: CtrlPins27 is 1", CtrlPins_Data.IO27 , '0');

    end loop;

    spiPayload_in_s(0) <= x"FB";
    spiPayload_in_s(1) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

    Check( "FIFO report empty", spiPayload_out_s(1), x"FF");

    Check( "6: CtrlPins27 is 1", CtrlPins_Data.IO27 , '0');

    -- Clear interrupt
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"10";
    spiPayload_in_s(3) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);


    wait for 32 * 10.41 ns;
    Check( "7: CtrlPins27 is 1", CtrlPins_Data.IO27 , '1');

    -- Test FIFO reset

    Spectrum_Cmd.Address  <= x"0009";
    Spectrum_Cmd.Data     <= std_logic_vector(fifodata);
    Spectrum_Cmd.Cmd      <= WRITEIO;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;

    wait for 32 * 10.41 ns;
    Check( "8: CtrlPins27 is 0", CtrlPins_Data.IO27 , '0');

    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"20";
    spiPayload_in_s(3) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    -- ACk interrupt
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"10";
    spiPayload_in_s(3) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);


    wait for 32 * 10.41 ns;
    Check( "9: CtrlPins27 is 1", CtrlPins_Data.IO27 , '1');

    spiPayload_in_s(0) <= x"FB";
    spiPayload_in_s(1) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

    Check( "FIFO report empty", spiPayload_out_s(1), x"FF");

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;


end t002;
