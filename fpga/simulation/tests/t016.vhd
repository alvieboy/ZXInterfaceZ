use work.tbc_device_p.all;
use work.logger.all;
use work.bfm_spectrum_p.all;
library ieee;
use ieee.math_real.all;

architecture t016 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;
  signal stopInjection: boolean := false;

  type controlin_type is record
    name: string(1 to 4);
    pin: Pin_type;
    index: natural;
  end record;

  type ca is array (0 to 7) of controlin_type;
  constant cpins: ca := (
    ( "INT ", PIN_INT, 0 ),
    ( "MREQ", PIN_MREQ, 1 ),
    ( "IORQ", PIN_IORQ, 2 ),
    ( "RD  ", PIN_RD, 3 ),
    ( "WR  ", PIN_WR, 4 ),
    ( "M1  ", PIN_M1, 5 ),
    ( "RFSH", PIN_RFSH, 6 ),
    ( "CLK ", PIN_CLK, 7 )
  );

  signal loopback: boolean := false;

begin

  process
    variable data: std_logic_vector(7 downto 0);
    variable cnt: natural := 0;
    constant zeroone: std_logic_vector(1 downto 0) := "10";

    procedure uartTx(data: in std_logic_vector(7 downto 0)) is
    begin
      waitnotbusy: loop
        spiPayload_in_s(0) <= x"DA"; -- Read UART status
        spiPayload_in_s(1) <= x"00";
        spiPayload_in_s(2) <= x"00";
        Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);
        if spiPayload_out_s(2)(0)='0' then
          exit waitnotbusy;
        end if;
        report "UART busy " & hstr(spiPayload_out_s(2));
      end loop;

      spiPayload_in_s(0) <= x"D8"; -- Write UART data
      spiPayload_in_s(1) <= data;
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 2, spiPayload_in_s, spiPayload_out_s);

    end procedure;

  begin

    logger_start("T016","BIT mode");

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    wait for 100 ns;

    cnt := 0;
    waitforbit: loop
      spiPayload_in_s(0) <= x"DE"; -- Read status
      spiPayload_in_s(1) <= x"00";
      spiPayload_in_s(2) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);
      --report hstr(spiPayload_out_s(2));
      wait for 5 us;
      cnt := cnt + 1;
      if cnt=100 or spiPayload_out_s(2)(7)='1' then
        exit waitforbit;
      end if;
    end loop;

    check("That we entered BIT mode", cnt/=100);
    if cnt/=10 then

      -- Activate BIT mode.
      spiPayload_in_s(0) <= x"EC";
      spiPayload_in_s(1) <= x"00";
      spiPayload_in_s(2) <= x"00";
      spiPayload_in_s(3) <= x"40";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

      loopback <= true;
      wait for 0 ps;

      -- Send debug data via UART
      uartTx(x"AA");
      uartTx(x"BB");
      uartTx(x"CC");

      --wait for 3 ms;

      -- Set up address
      report "Setting address";
      SpectrumSetAddress(Spectrum_Cmd, Spectrum_Data, x"DEAD");
      SpectrumSetData(Spectrum_Cmd, Spectrum_Data, x"BE");

      spiPayload_in_s(0) <= x"D7"; -- Read BIT
      spiPayload_in_s(1) <= x"00";
      spiPayload_in_s(2) <= x"00";
      spiPayload_in_s(3) <= x"00";
      spiPayload_in_s(4) <= x"00";
      spiPayload_in_s(5) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 6, spiPayload_in_s, spiPayload_out_s);

      check("That we can read address LSB OK", spiPayload_out_s(2), x"AD");
      check("That we can read address MSB OK", spiPayload_out_s(3), x"DE");
      check("That we can read data OK", spiPayload_out_s(4), x"BE");

      l: for i in 0 to 7 loop

        l2: for zo in 0 to 1 loop
          SpectrumSetPin(Spectrum_Cmd, Spectrum_Data,
            cpins(i).pin, zeroone(zo));
  
          spiPayload_in_s(0) <= x"D7"; -- Read BIT
          spiPayload_in_s(1) <= x"00";
          spiPayload_in_s(2) <= x"00";
          spiPayload_in_s(3) <= x"00";
          spiPayload_in_s(4) <= x"00";
          spiPayload_in_s(5) <= x"00";
          Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 6, spiPayload_in_s, spiPayload_out_s);
  
          check("That we can read pin "&cpins(i).name&" OK", spiPayload_out_s(5)(cpins(i).index), zeroone(zo));
        end loop l2;
      end loop l;

      -- Test outputs.
      SpectrumSetData(Spectrum_Cmd, Spectrum_Data, "ZZZZZZZZ");
      SpectrumSetPin(Spectrum_Cmd, Spectrum_Data, PIN_INT, 'H');

      spiPayload_in_s(0) <= x"D6"; -- Write BIT
      spiPayload_in_s(1) <= x"CA";
      spiPayload_in_s(2) <= x"01";
      spiPayload_in_s(3) <= x"00";
      spiPayload_in_s(4) <= x"00";
      --spiPayload_in_s(5) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

      wait for 1 us;

      SpectrumSamplePins(Spectrum_Cmd, Spectrum_Data);
  
      Check("For correct data", Spectrum_Data.Data, x"CA");
      Check("For correct data", to_X01(Spectrum_Data.WaitPin), '1');
      Check("Control pins", CtrlPins_Data.ROMCS, '0');
      Check("Control pins", CtrlPins_Data.RESET, '0');
      Check("Control pins", CtrlPins_Data.NMI, '0');
      Check("Control pins", CtrlPins_Data.IORQULA, '0');

      spiPayload_in_s(0) <= x"D6"; -- Write BIT
      spiPayload_in_s(1) <= x"00";
      spiPayload_in_s(2) <= x"FE";
      spiPayload_in_s(3) <= x"00";
      spiPayload_in_s(4) <= x"00";
      --spiPayload_in_s(5) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

      SpectrumSamplePins(Spectrum_Cmd, Spectrum_Data);

      Check("Control pins", CtrlPins_Data.ROMCS, '1');
      Check("Control pins", CtrlPins_Data.RESET, '1');
      Check("Control pins", CtrlPins_Data.NMI, '1');
      Check("Control pins", CtrlPins_Data.IORQULA, '1');
      Check("For correct data", to_X01(Spectrum_Data.WaitPin), '0');

      -- Disable BIT mode.
      spiPayload_in_s(0) <= x"EC";
      spiPayload_in_s(1) <= x"00";
      spiPayload_in_s(2) <= x"00";
      spiPayload_in_s(3) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

      SpectrumSamplePins(Spectrum_Cmd, Spectrum_Data);

      -- Bus is in High-z, but we have pullups
      Check("For correct data", Spectrum_Data.Data, "HHHHHHHH");
      Check("For correct data WAIT", to_X01(Spectrum_Data.WaitPin), '1');
      Check("Control pins", CtrlPins_Data.ROMCS, '0');
      Check("Control pins", CtrlPins_Data.RESET, '0');
      Check("Control pins", CtrlPins_Data.NMI, '0');
      Check("Control pins", CtrlPins_Data.IORQULA, '0');

      wait for 82*2 us;
      spiPayload_in_s(0) <= x"DA"; -- Read UART status
      spiPayload_in_s(1) <= x"00";
      spiPayload_in_s(2) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);
      check("That we have 3 items in UART queue", spiPayload_out_s(2), "10001110");
      -- Read them
      spiPayload_in_s(0) <= x"D9"; -- Read UART data
      spiPayload_in_s(1) <= x"02"; -- Data size to read
      spiPayload_in_s(2) <= x"00";
      spiPayload_in_s(3) <= x"00";
      spiPayload_in_s(4) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 5, spiPayload_in_s, spiPayload_out_s);

      check("items in UART queue", spiPayload_out_s(3), x"AA");
      check("items in UART queue", spiPayload_out_s(4), x"BB");

      spiPayload_in_s(0) <= x"D9"; -- Read UART data
      spiPayload_in_s(1) <= x"01"; -- Data size to read
      spiPayload_in_s(2) <= x"00";
      spiPayload_in_s(3) <= x"00";
      Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);
      check("items in UART queue", spiPayload_out_s(3), x"CC");

      --spiPayload_in_s(0) <= x"D9"; -- Read UART data
      --spiPayload_in_s(1) <= x"00";
      --spiPayload_in_s(2) <= x"00";
      --Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);
      --check("items in UART queue", spiPayload_out_s(2), x"DD");



    end if; -- Entered BIT mode

    stopInjection <= true;

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;
  process
    -- 976.5625Hz
    --constant GEN_PERIOD: time := 1.024 ms;
    constant GEN_PERIOD: time := 104.16667 ns;
  begin
    EXT_io(13) <= '1';
    wait for GEN_PERIOD/2;
    EXT_io(13) <= '0';
    wait for GEN_PERIOD/2;
    if stopInjection then
      wait;
    end if;
  end process;

  process
  begin
    wait until loopback;
    l: loop
      wait on EXT_io(12);
      ext_io(11) <= EXT_io(12);
      if stopInjection then
        wait;
      end if;
    end loop;
  end process;

end t016;
