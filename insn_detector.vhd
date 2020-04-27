library IEEE;   
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;

entity insn_detector is
  port (
    clk_i       : in std_logic;
    arst_i      : in std_logic;

    valid_i     : in std_logic;

    a_i         : in std_logic_vector(15 downto 0);
    d_i         : in std_logic_vector(7 downto 0);
    m1_i        : in std_logic;

    pc_o        : out std_logic_vector(15 downto 0);
    pc_valid_o  : out std_logic;

    retn_det_o  : out std_logic  -- RETN detected
  );
end entity insn_detector;

architecture beh of insn_detector is

  signal retn_detected_r  :  std_logic;
  signal retn_q_r         :  std_logic;

begin

  process(clk_i, arst_i)
  begin
    if arst_i='1' then

      retn_q_r        <= '0';
      retn_detected_r <= '0';

    elsif rising_edge(clk_i) then
      retn_detected_r <= '0';
      pc_valid_o      <= '0';
      if valid_i='1' then

        if m1_i='0' then
          pc_valid_o  <= '1';
          pc_o        <= a_i;
        end if;

        
        if a_i(15)='0' and a_i(14)='0' then
          if retn_q_r='1' then
            retn_q_r<='0';
            if (d_i=x"45") then
              retn_detected_r<='1';
            end if;
          else
            if d_i=x"ED" then
              retn_q_r<='1';
            end if;
          end if;
        end if; -- not ROM address
      else -- not valid
        --retn_q_r <= '0';
      end if;
    end if;
  end process;

  retn_det_o <= retn_detected_r;

end beh;

