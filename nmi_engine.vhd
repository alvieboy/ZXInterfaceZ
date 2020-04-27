library IEEE;   
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

entity nmi_engine is
  port (
    clk_i         : in std_logic;
    arst_i        : in std_logic;

    busy_o        : out std_logic;
    reset_i       : in  std_logic;

    nmi_req_i     : in std_logic; -- Request assertion of NMI
    nmi_forcerom_i: in std_logic; -- Force ROM on assertion of NMI

    nmi_det_i     : in std_logic; -- from INSN detector

    romcs_force_o : out std_logic;
    nmi_o         : out std_logic;
  );
end entity nmi_engine;

architecture beh of nmi_engine is

  signal nmi_requested_r  : std_logic;
  signal nmi_force_r      : std_logic;
  signal nmi_rom_active_r : std_logic;
begin

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      nmi_requested_r <= '0';
    elsif rising_edge(clk_i) then
      if reset_i='1' then
        nmi_requested_r   <= '0';
        nmi_rom_active_r  <= '0';
      else
        if nmi_req_i='1' then
          nmi_requested_r <= '1';
          nmi_force_r     <= nmi_forcerom_i;
        end if;

        if nmi_requested_r='1' and nmi_det_i='1' then
          nmi_requested_r <= '0';
          if nmi_force_r='1' then
            nmi_rom_active_r <= '1';
          end if;
        end if;
      end if;
    end if;
  end process;

  busy_o        <= nmi_requested_r;
  nmi_o         <= nmi_requested_r;
  romcs_force_o <= nmi_rom_active_r;

end beh;