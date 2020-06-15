use work.tbc_device_p.all;
library ieee;
use ieee.math_real.all;

architecture t011 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
    variable data: std_logic_vector(7 downto 0);
  begin
    report "T006: TZX play";

    Spimaster_Cmd <= Cmd_Spimaster_Defaults;

    PowerUpAndReset(
      SysRst_Cmd,
      SysClk_Cmd,
      SpectRst_Cmd,
      SpectClk_Cmd
    );

    wait for 100 ns;
    Audiocap_Cmd.Enabled <= true;

    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"06"; -- TAP reset + enable

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    spiPayload_in_s(0) <= x"EC";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";
    spiPayload_in_s(3) <= x"04"; -- TAP enable

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 4, spiPayload_in_s, spiPayload_out_s);

    spiPayload_in_s(0) <= x"E5";
    spiPayload_in_s(1) <= x"00";
    spiPayload_in_s(2) <= x"00";

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 2, spiPayload_in_s, spiPayload_out_s);

    Check("SPI fifo MSB is cleared", spiPayload_out_s(1), x"00");
    Check("SPI fifo LSB is cleared", spiPayload_out_s(2), x"00");

    -- Set pulse widths

    spiPayload_in_s(0) <= x"E6"; -- Commands
    spiPayload_in_s(1) <= x"00"; --    pilot
    spiPayload_in_s(2) <= x"07"; --
    spiPayload_in_s(3) <= x"00"; --

    spiPayload_in_s(4) <= x"01"; --    sync0
    spiPayload_in_s(5) <= x"02"; --
    spiPayload_in_s(6) <= x"00"; --

    spiPayload_in_s(7) <= x"02"; --   sync1
    spiPayload_in_s(8) <= x"01"; --
    spiPayload_in_s(9) <= x"00"; --

    spiPayload_in_s(10) <= x"03"; --  logic 0
    spiPayload_in_s(11) <= x"05"; --
    spiPayload_in_s(12) <= x"00"; --

    spiPayload_in_s(13) <= x"04"; --
    spiPayload_in_s(14) <= x"03"; --  logic 1
    spiPayload_in_s(15) <= x"00"; --

    spiPayload_in_s(16) <= x"08"; --  Header pilot pulses
    spiPayload_in_s(17) <= x"08"; --
    spiPayload_in_s(18) <= x"00"; --

    spiPayload_in_s(19) <= x"09"; -- Data pilot pulses
    spiPayload_in_s(20) <= x"08"; --
    spiPayload_in_s(21) <= x"00"; --

    spiPayload_in_s(22) <= x"0A"; -- Gap in ms
    spiPayload_in_s(23) <= x"01"; --
    spiPayload_in_s(24) <= x"00"; --

    spiPayload_in_s(25) <= x"83"; -- TZX mode

    spiPayload_in_s(26) <= x"0B"; -- Datalen LSB
    spiPayload_in_s(27) <= x"02"; --
    spiPayload_in_s(28) <= x"00"; --

    spiPayload_in_s(29) <= x"0C"; -- Datalen MSB, including byte len
    spiPayload_in_s(30) <= x"00"; --
    spiPayload_in_s(31) <= x"02"; -- 6 bits. (8-2 == 6)

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 32, spiPayload_in_s, spiPayload_out_s);

    --spiPayload_in_s(0) <= x"E6"; -- Commands
    --spiPayload_in_s(1) <= x"80"; -- Reset to defaults

    --Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 2, spiPayload_in_s, spiPayload_out_s);



    -- Play

    spiPayload_in_s(0) <= x"E4";
    spiPayload_in_s(1) <= x"00";  -- Type

    spiPayload_in_s(2) <= x"55";
    spiPayload_in_s(3) <= x"AA";
    spiPayload_in_s(4) <= x"F0";  -- Will only send 6 bits out of 8

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 33, spiPayload_in_s, spiPayload_out_s);


    wait for 100 ms;

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;

    process
      variable r: real;
      variable period: time;
    begin
      wait on Audiocap_Data.Trans;

      r := real(Audiocap_Data.Delta / 285.71429 ps) / 1000.0;

      report "EVENT pulse: " & time'image(Audiocap_Data.Delta) & ", T-state pulses " &
        str(integer(round(r))) & " polarity " & str(Audiocap_Data.Polarity);
   end process;



end t011;
