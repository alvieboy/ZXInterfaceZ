use work.tbc_device_p.all;
library ieee;
use ieee.math_real.all;

architecture t006 of tbc_device is

  signal spiPayload_in_s  : spiPayload_type;
  signal spiPayload_out_s : spiPayload_type;

begin

  process
    variable data: std_logic_vector(7 downto 0);
  begin
    report "T006: TAP play";

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

    spiPayload_in_s(0 to 27) <= (
    x"E6",
    x"80", x"57", x"03",
    x"81", x"ae", x"06",
    x"83", x"13", x"00",
    x"84", x"00", x"00",
    --x"85", x"7f", x"1f",
    x"85", x"04", x"00",
    x"86", x"78", x"08",
    x"85", x"00", x"00",
    x"86", x"9b", x"02",
    x"86", x"df", x"02"
 );

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 28, spiPayload_in_s, spiPayload_out_s);

    spiPayload_in_s(0 to 19) <= (
    x"E4",
    x"00", x"00", x"73", x"6b",
    x"6f", x"6f", x"6c", x"64",
    x"61", x"7a", x"65", x"20",
    x"1b", x"00", x"0a", x"00",
    x"1b", x"00", x"44"
 );

    Spi_Transceive( Spimaster_Cmd, Spimaster_Data, 20, spiPayload_in_s, spiPayload_out_s);

    Spi_Flush(Spimaster_Cmd, Spimaster_Data);

    wait for 200 ms;

    FinishTest(
      SysClk_Cmd,
      SpectClk_Cmd
    );

  end process;

    process
      variable r: real;
      variable period: time;
      variable pulses: integer;
      variable last: integer := 0;
      variable bitindex: integer := 7;
      variable byte: std_logic_vector(7 downto 0);
      variable bitv: std_logic;
    begin
      wait on Audiocap_Data.Trans;

      r := real(Audiocap_Data.Delta / 285.71429 ps) / 1000.0;
      pulses := integer(round(r));
      report "EVENT pulse: " & time'image(Audiocap_Data.Delta) & ", T-state pulses " &
        str(pulses) & " polarity " & str(Audiocap_Data.Polarity);

      if last=pulses then
        last := 0;
        bitv := 'X';
        if pulses>842 and pulses<846 then
          bitv := '0';
        elsif pulses>1681 and pulses<1687 then
          bitv := '1';
        else
          bitindex := 7;
        end if;

        if not is_x(bitv) then
          byte(bitindex) := bitv;
          if bitindex=0 then
            bitindex := 7;
            report "BYTE: " & hstr(byte);
          else
            bitindex := bitindex - 1;
          end if;
        end if;
      else
        last := pulses;
      end if;
   end process;

end t006;
