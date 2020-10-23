LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.txt_util.all;
use work.logger.all;
USE std.textio.all;

package bfm_spimaster_p is

  type SPICmd_Type is (
    NONE,
    SELECT_DEVICE,
    DESELECT_DEVICE,
    TRANSCEIVE,
    FLUSH
  );


  type Cmd_Spimaster_type is record
    Cmd       : SPICmd_Type;
    Period    : time;
    Verbose   : boolean;
    Data      : std_logic_vector(7 downto 0);
  end record;

  type Data_Spimaster_type is record
    Busy      : boolean;
    Data      : std_logic_vector(7 downto 0);
  end record;

  constant Cmd_Spimaster_Defaults: Cmd_Spimaster_type := ( None, 100 ns, false, x"00" );

  component bfm_spimaster is
    port (
      Cmd_i     : in Cmd_Spimaster_type;
      Data_o    : out Data_Spimaster_type;
      -- Outputs.
      sck_o     : out std_logic;
      mosi_o    : out std_logic;
      miso_i    : in std_logic;
      csn_o     : out std_logic
    );
  end component bfm_spimaster;

  type spiPayload_type is array(0 to 2047) of std_logic_vector(7 downto 0);

  procedure Spi_Transceive(
      signal Cmd : out Cmd_Spimaster_type;
      signal Data : in Data_Spimaster_type;
      len  : natural range 1 to 2048;
      signal din  : in spiPayload_type;
      signal dout : out spiPayload_type
    );
  procedure Spi_Flush (
      signal Cmd  : out Cmd_Spimaster_type;
      signal Data : in Data_Spimaster_type
    );

end package;

package body bfm_spimaster_p is

  procedure Spi_Transceive(
      signal Cmd  : out Cmd_Spimaster_type;
      signal Data : in Data_Spimaster_type;
      len         : natural range 1 to 2048;
      signal din  : in spiPayload_type;
      signal dout : out spiPayload_type
    ) is
    variable txl: line;
    variable rxl: line;
  begin
    Cmd.Cmd <= SELECT_DEVICE;
    wait for 0 ps;
    wait until Data.Busy = false;

    write(txl, string'("TX:"));
    write(rxl, string'("RX:"));

    l1: for i in 1 to len loop
      Cmd.Data  <= din(i-1);
      write(txl, " " & hstr(din(i-1)));
      Cmd.Cmd   <= TRANSCEIVE;
      wait for 0 ps;
      --report "Sent "& str(i);
      wait until Data.Busy = false;
      dout(i-1) <= Data.Data;
      write(rxl, " " & hstr(Data.Data));
      --report "Dat "& str(i);
      Cmd.Cmd   <= NONE;
      wait for 0 ps;
    end loop;
    if (true) then
      log(string'("SPI Transaction:"));
      log(txl);
      log(rxl);
    end if;

    Cmd.Cmd <= DESELECT_DEVICE;
    wait for 0 ps;
    wait until Data.Busy = false;

  end procedure;

  procedure Spi_Flush (
      signal Cmd  : out Cmd_Spimaster_type;
      signal Data : in Data_Spimaster_type
    ) is
  begin
    Cmd.Cmd <= FLUSH;
    wait for 0 ps;
    wait until Data.Busy = false;
  end procedure;


end package body;
