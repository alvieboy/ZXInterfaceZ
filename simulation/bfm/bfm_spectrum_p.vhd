LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.txt_util.all;

package bfm_spectrum_p is

  type SpectrumCmd is (
    NONE,
    WRITEIO,
    READIO,
    WRITEMEM,
    READMEM
  );

  type Data_Spectrum_type is record
    Busy      : boolean;
    Data      : std_logic_vector(7 downto 0);
  end record;

  type Cmd_Spectrum_type is record
    Cmd       : SpectrumCmd;
    Address   : std_logic_vector(15 downto 0);
    Data      : std_logic_vector(7 downto 0);
  end record;

  component bfm_spectrum is
    port (
      Cmd_i   : in Cmd_Spectrum_type;
      Data_o  : out Data_Spectrum_type;

      ck_o    : out std_logic;
      wr_o    : out std_logic;
      rd_o    : out std_logic;
      mreq_o  : out std_logic;
      ioreq_o : out std_logic;
      a_o     : out std_logic_vector(15 downto 0);
      d_io    : inout std_logic_vector(7 downto 0)
    );
  end component bfm_spectrum;

  

end package;
