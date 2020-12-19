use work.tbc_device_p.all;
use work.logger.all;
use work.zxinterfacepkg.all;

architecture t002 of tbc_device is

  signal spiPayload_in_s    : spiPayload_type;
  signal spiPayload_out_s   : spiPayload_type;
  signal fifodata           : unsigned(7 downto 0);
  signal expected_size      : unsigned(2 downto 0);
begin

  process
    variable depth: natural := 0;
    variable used_size_v:  std_logic_vector(2 downto 0);
    variable expected_fifodata  : unsigned(7 downto 0);

    procedure ackFifoInterrupt is
    begin
      -- Clear interrupt
      spiPayload_in_s(0) <= x"A0";
      spiPayload_in_s(1) <= x"01";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 2, spiPayload_in_s, spiPayload_out_s);
    end procedure;

    procedure checkInterrupt(line: in natural) is
    begin
      Check( "CtrlPins27 is 0", CtrlPins_Data.IO27 , '0');
      -- Get interrupt
      spiPayload_in_s(0) <= x"A1";
      spiPayload_in_s(1) <= x"00";
      spiPayload_in_s(2) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

      Check( "Interrupt line " &str(line)&" active", spiPayload_out_s(2)(line),'1');
    end procedure;

  begin
    logger_start("T002","Check depth of command FIFO");

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

      Spectrum_Cmd.Address <= x"00" & SPECT_PORT_CMD_FIFO_STATUS;
      Spectrum_Cmd.Cmd     <= READIO;
      wait for 0 ps;
      wait until Spectrum_Data.Busy = false;
      Spectrum_Cmd.Cmd     <= NONE;
      if Spectrum_Data.Data(0) = '1' then
        exit dcalc;
      end if;

      Spectrum_Cmd.Address  <= x"00" & SPECT_PORT_CMD_FIFO_DATA;
      Spectrum_Cmd.Data     <= std_logic_vector(fifodata);
      Spectrum_Cmd.Cmd      <= WRITEIO;
      wait for 0 ps;
      depth := depth + 1;
      wait until Spectrum_Data.Busy = false;
      Spectrum_Cmd.Cmd      <= NONE;
      wait for 1 us;
      fifodata <= fifodata + 1;

      wait for 32 * 10.41 ns;

      checkInterrupt(0);
      -- Ack interrupt
      ackInterrupt(CtrlPins_Cmd);
      ackFifoInterrupt;


      --Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

    end loop;

    report "FIFO depth " & str(depth);

    Check( "Depth is less than 64", depth <= 64 );

    expected_fifodata := x"00";
    expected_size <= "100";

    l2: for i in 0 to depth*2 loop
      -- Get FIFO used size
      spiPayload_in_s(0) <= x"DE";
      spiPayload_in_s(1) <= x"00";
      spiPayload_in_s(2) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

      report "Fifo used data " & hstr(spiPayload_out_s(2));
      used_size_v := spiPayload_out_s(2)(6 downto 4);
      Check( "Fifo USED is correct", std_logic_vector(expected_size), used_size_v);


      -- Now, read back from FIFO, one byte at a time.
      spiPayload_in_s(0) <= x"FB";
      spiPayload_in_s(1) <= x"01";
      spiPayload_in_s(2) <= x"00";
      spiPayload_in_s(3) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

      --Check( "FIFO report not empty", spiPayload_out_s(2), x"FE");
      Check( "FIFO data consistent",  spiPayload_out_s(3), std_logic_vector(expected_fifodata));

      expected_fifodata := expected_fifodata + 1;

      -- Check that interrupt is still asserted
      checkInterrupt(0);


      -- Inject more data.
      Spectrum_Cmd.Address <= x"00" & SPECT_PORT_CMD_FIFO_STATUS;
      Spectrum_Cmd.Cmd     <= READIO;
      wait until Spectrum_Data.Busy = false;
      Spectrum_Cmd.Cmd     <= NONE;
      
      Check("FIFO not full", Spectrum_Data.Data(0) = '0');

      Spectrum_Cmd.Address  <=  x"00" & SPECT_PORT_CMD_FIFO_DATA;
      Spectrum_Cmd.Data     <= std_logic_vector(fifodata);
      Spectrum_Cmd.Cmd      <= WRITEIO;
      wait until Spectrum_Data.Busy = false;
      Spectrum_Cmd.Cmd      <= NONE;

      Spectrum_Cmd.Address <=  x"00" & SPECT_PORT_CMD_FIFO_STATUS;
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
    checkInterrupt(0);

    -- Do a full burst read (4).

    --l3: for i in 0 to depth-1 loop
      spiPayload_in_s(0) <= x"FB";
      spiPayload_in_s(1) <= x"04";
      spiPayload_in_s(2) <= x"00";
      spiPayload_in_s(3) <= x"00"; --d0
      spiPayload_in_s(4) <= x"00"; --d0
      spiPayload_in_s(5) <= x"00"; --d0
      spiPayload_in_s(6) <= x"00"; --d0
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 7, spiPayload_in_s, spiPayload_out_s);

      --Check( "FIFO report not empty", spiPayload_out_s(2), x"FE");
      l4: for i in 0 to 3 loop

        Check( "FIFO burst data consistent", spiPayload_out_s(3+i), std_logic_vector(expected_fifodata));

        expected_fifodata := expected_fifodata + 1;
      end loop;

      --checkInterrupt(0);

    --end loop;


    -- Get FIFO used size
    spiPayload_in_s(0) <= x"DE";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

    report "Fifo used data " & hstr(spiPayload_out_s(2));
    used_size_v := spiPayload_out_s(2)(6 downto 4);


    Check( "FIFO report empty", used_size_v, "000");

    --checkInterrupt(0);

    -- Clear interrupt
    ackInterrupt(CtrlPins_Cmd);

    wait for 32 * 10.41 ns;
    Check( "7: CtrlPins27 is 1", CtrlPins_Data.IO27 , '1');

    -- Ensure it does not trigger again.

    ackFifoInterrupt;

    wait for 32 * 10.41 ns;
    Check( "7.1: CtrlPins27 is 1", CtrlPins_Data.IO27 , '1');


    -- Test FIFO reset

    Spectrum_Cmd.Address  <=  x"00" & SPECT_PORT_CMD_FIFO_DATA; 
    Spectrum_Cmd.Data     <= std_logic_vector(fifodata);
    Spectrum_Cmd.Cmd      <= WRITEIO;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;

    wait for 32 * 10.41 ns;

    checkInterrupt(0);

    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"20";
    spiPayload_in_s(3) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    ackInterrupt(CtrlPins_Cmd);
    ackFifoInterrupt;

    wait for 32 * 10.41 ns;
    Check( "9: CtrlPins27 is 1", CtrlPins_Data.IO27 , '1');

    -- Get FIFO used size
    spiPayload_in_s(0) <= x"DE";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

    report "Fifo used data " & hstr(spiPayload_out_s(2));
    used_size_v := spiPayload_out_s(2)(6 downto 4);

    Check( "FIFO report empty", used_size_v, "000");

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );
    logger_end;

  end process;


end t002;
