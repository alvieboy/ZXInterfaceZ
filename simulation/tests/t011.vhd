use work.tbc_device_p.all;
use work.tappkg.all;
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

    --Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 2, spiPayload_in_s, spiPayload_out_s);

    Check("SPI fifo MSB is cleared", spiPayload_out_s(1), x"00");
    Check("SPI fifo LSB is cleared", spiPayload_out_s(2), x"00");

    -- Set pulse widths

    spiPayload_in_s(0) <= x"E6"; -- Commands

    -- Play pure tone pulse

    spiPayload_in_s(1) <= TAPCMD_SET_REPEAT;
    spiPayload_in_s(2) <= x"03"; --
    spiPayload_in_s(3) <= x"00"; -- 4 pulses

    spiPayload_in_s(4) <= TAPCMD_PLAY_PULSE; -- Pilot
    spiPayload_in_s(5) <= x"09"; -- 10 T-states
    spiPayload_in_s(6) <= x"00"; --

    spiPayload_in_s(7) <= TAPCMD_SET_REPEAT;
    spiPayload_in_s(8) <= x"00"; --
    spiPayload_in_s(9) <= x"00"; -- 1 pulses

    spiPayload_in_s(10) <= TAPCMD_PLAY_PULSE; -- Pilot
    spiPayload_in_s(11) <= x"06"; -- 7 T-states
    spiPayload_in_s(12) <= x"00"; --
    spiPayload_in_s(13) <= TAPCMD_PLAY_PULSE; -- Pilot
    spiPayload_in_s(14) <= x"05"; -- 6 T-states
    spiPayload_in_s(15) <= x"00"; --

    -- Data block.

    spiPayload_in_s(16) <= TAPCMD_LOGIC0_SIZE;
    spiPayload_in_s(17) <= x"03"; --
    spiPayload_in_s(18) <= x"00"; -- 4 T-states

    spiPayload_in_s(19) <= TAPCMD_LOGIC1_SIZE;
    spiPayload_in_s(20) <= x"04"; --
    spiPayload_in_s(21) <= x"00"; -- 5 T-states

    spiPayload_in_s(22) <= TAPCMD_CHUNK_SIZE_LSB;
    spiPayload_in_s(23) <= x"01"; --
    spiPayload_in_s(24) <= x"00"; -- 8 bytes

    spiPayload_in_s(25) <= TAPCMD_CHUNK_SIZE_MSB;
    spiPayload_in_s(26) <= x"00"; --
    spiPayload_in_s(27) <= x"07";

    --spiPayload_in_s(22) <= TAPCMD_FLUSH; --

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 28, spiPayload_in_s, spiPayload_out_s);


    spiPayload_in_s(0) <= x"E4"; -- Data
    spiPayload_in_s(1) <= x"0F"; -- Data
    spiPayload_in_s(2) <= x"00"; -- Data
--    spiPayload_in_s(2) <= x"00"; -- Data
--    spiPayload_in_s(3) <= x"FF"; -- Data
--    spiPayload_in_s(4) <= x"AA"; -- Data
--    spiPayload_in_s(5) <= x"55"; -- Data
--    spiPayload_in_s(6) <= x"AA"; -- Data
--    spiPayload_in_s(7) <= x"AA"; -- Data
--    spiPayload_in_s(8) <= x"AA"; -- Data

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 3, spiPayload_in_s, spiPayload_out_s);

    Spi_Flush( Spimaster_Cmd, Spimaster_Data);

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
