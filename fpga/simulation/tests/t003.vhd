use work.tbc_device_p.all;
use work.zxinterfacepkg.all;
--use work.zxinterfaceports.all;
use work.logger.all;

architecture t003 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

  type mtype is array(0 to 15) of  std_logic_Vector(7 downto 0);
  type xorstype is array(0 to 7) of  std_logic_Vector(7 downto 0);
  constant mdata: mtype := (
    x"DE", x"AD", x"BE", x"EF",
    x"01", x"10", x"02", x"20",
    x"03", x"30", x"04", x"40",
    x"05", x"50", x"06", x"60");

  constant ramdata: mtype := (
    x"EE", x"ED", x"E6", x"EF",
    x"E1", x"1E", x"E2", x"2E",
    x"E3", x"3E", x"E4", x"4E",
    x"E5", x"5E", x"E6", x"6E");

  constant pagexors: xorstype := (
    x"01", x"02", x"04", x"08",
    x"10", x"20", x"40", x"80"
  );

begin

  process
    variable addr: unsigned(15 downto 0);
    variable saddr: unsigned(23 downto 0);
    variable data: std_logic_vector(7 downto 0);
    variable dw: std_logic_vector(7 downto 0);
  begin
    logger_start("T003","Just run");

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    QSPIRam0_Cmd.Enabled <= true;


    -- Activate ROMCS
    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= "00000100"; -- ForceROMCS
    spiPayload_in_s(3) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);


    setup1: for i in 0 to 15 loop
      qspiramWrite(QSPIRam0_Cmd, i, mdata(i));
    end loop;

    addr := x"0000";
    log("Reading memory with ROM address / opcode read");
    rl: for i in 0 to 15 loop
      log("Reading address " & hstr(std_logic_vector(addr)));
      Spectrum_Cmd.Address  <= std_logic_vector(addr);
      Spectrum_Cmd.Data     <= x"00";
      Spectrum_Cmd.Cmd      <= READOPCODE;
      wait until Spectrum_Data.Busy = false;
      Spectrum_Cmd.Cmd      <= NONE;
      wait for 0 ps;
      Check("Correct data from mem", Spectrum_Data.Data, mdata(i));
      addr := addr + 1;
    end loop;

    -- Ensure we cannot write it

    log("Attempting write on top of ROM");

    addr := x"0000";
    rl2: for i in 0 to 15 loop
      Spectrum_Cmd.Address  <= std_logic_vector(addr);
      Spectrum_Cmd.Data     <= x"F7";
      Spectrum_Cmd.Cmd      <= WRITEMEM;
      wait until Spectrum_Data.Busy = false;
      Spectrum_Cmd.Cmd      <= NONE;
      wait for 0 ps;
      addr := addr + 1;
    end loop;

    log("Reading memory with ROM address / opcode read to ensure write did not succeed");
    addr := x"0000";
    rl3: for i in 0 to 15 loop
      Spectrum_Cmd.Address  <= std_logic_vector(addr);
      Spectrum_Cmd.Data     <= x"00";
      Spectrum_Cmd.Cmd      <= READOPCODE;
      wait until Spectrum_Data.Busy = false;
      Spectrum_Cmd.Cmd      <= NONE;
      wait for 0 ps;
      Check("Correct data from mem", Spectrum_Data.Data, mdata(i));
      addr := addr + 1;
    end loop;

    -- Now, process RAM pages
    log("Processing RAM pages");

    pg: for page in 0 to 7 loop
      -- Select page via spectrum
      log("Testing page " & str(page));
      SpectrumWriteIO(Spectrum_Cmd, Spectrum_Data, x"00" & SPECT_PORT_MEMSEL, std_logic_vector(to_unsigned(page,8)));

      addr := x"2000";
      rl4: for i in 0 to 15 loop
        Spectrum_Cmd.Address  <= std_logic_vector(addr);
        Spectrum_Cmd.Data     <= ramdata(i) xor pagexors(page);
        Spectrum_Cmd.Cmd      <= WRITEMEM;
        wait until Spectrum_Data.Busy = false;
        Spectrum_Cmd.Cmd      <= NONE;
        wait for 0 ps;
        addr := addr + 1;
      end loop;

      addr := x"2000";
      rl5: for i in 0 to 15 loop
        Spectrum_Cmd.Address  <= std_logic_vector(addr);
        Spectrum_Cmd.Data     <= ramdata(i);
        Spectrum_Cmd.Cmd      <= READMEM;
        wait until Spectrum_Data.Busy = false;
        Spectrum_Cmd.Cmd      <= NONE;
        wait for 0 ps;
        dw :=  ramdata(i) xor pagexors(page);
        Check("Correct data from RAM", Spectrum_Data.Data, dw);
        addr := addr + 1;
      end loop;

      saddr := x"010000" + (page*8192);

      log("Checking write at address base " & hstr(std_logic_vector(saddr)));
      rl6: for i in 0 to 15 loop
        qspiramRead(QSPIRam0_Cmd, QSPIRam0_Data, std_logic_vector(saddr+i), data);
        dw :=  ramdata(i) xor pagexors(page);
        Check("Correct data from RAM", data, dw);
      end loop;
  
    end loop;



    Spectrum_Cmd.Address  <= x"00FE";
    Spectrum_Cmd.Data     <= x"07";
    Spectrum_Cmd.Cmd      <= WRITEIO;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;


    -- Write a simple pattern on screen.

    Spectrum_Cmd.Address  <= x"4000";
    Spectrum_Cmd.Data     <= x"AA";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;
    Spectrum_Cmd.Address  <= x"4001";
    Spectrum_Cmd.Data     <= x"55";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;
    Spectrum_Cmd.Address  <= x"401F";
    Spectrum_Cmd.Data     <= x"55";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;
    Spectrum_Cmd.Address  <= x"4100";
    Spectrum_Cmd.Data     <= x"0F";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;

    Spectrum_Cmd.Address  <= x"5800";
    Spectrum_Cmd.Data     <= x"78";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;

    Spectrum_Cmd.Address  <= x"5801";
    Spectrum_Cmd.Data     <= "01101010";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;

    Spectrum_Cmd.Address  <= x"581F";
    Spectrum_Cmd.Data     <= "01101010";
    Spectrum_Cmd.Cmd      <= WRITEMEM;
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;

    Spectrum_Cmd.Address  <= x"0066";
    Spectrum_Cmd.Cmd      <= READOPCODE;
    Spectrum_Cmd.Refresh  <= x"0000";
    wait until Spectrum_Data.Busy = false;
    Spectrum_Cmd.Cmd      <= NONE;
    wait for 20 ns;

    
    wait for 10 us;

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );
    logger_end;

  end process;


end t003;
