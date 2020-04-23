LIBRARY ieee;
USE ieee.std_logic_1164.all;
LIBRARY work;
use work.bfm_spimaster_p.all;

entity bfm_spimaster is
  port (
    Cmd_i     : in Cmd_Spimaster_type;
    Data_o    : out Data_Spimaster_type;
    -- Outputs.
    sck_o     : out std_logic;
    mosi_o    : out std_logic;
    miso_i    : in std_logic;
    csn_o     : out std_logic
  );
end entity bfm_spimaster;


architecture sim of bfm_spimaster is

  signal data_r : std_logic_vector(7 downto 0);

begin

  Data_o.Data <= data_r;

  process
  begin
    Data_o.Busy <= false;
    csn_o       <= '1';
    sck_o       <= '1';

    report "SPI: Set up.";
    mloop: loop
      --report "Wait cmd";
      wait on Cmd_i.Cmd;
      Data_o.Busy <= true;
      --report "CMD";
  
      case Cmd_i.Cmd is
        when SELECT_DEVICE    =>
          wait for Cmd_i.Period/2;
          csn_o <= '0';
          wait for Cmd_i.Period/2;
        when DESELECT_DEVICE  =>
          wait for Cmd_i.Period/2;
          csn_o <= '1';
          wait for Cmd_i.Period/2;
        when TRANSCEIVE =>
          l1: for i in 7 downto 0 loop
            mosi_o <= Cmd_i.Data(i);
            sck_o  <= '0';
            wait for Cmd_i.Period / 2;
            sck_o  <= '1';
            data_r(i) <= miso_i;
            wait for Cmd_i.Period / 2;
          end loop;
  
        when others =>
          null;
      end case;
  
      Data_o.Busy <= false;
    end loop;
  end process;

end sim;
