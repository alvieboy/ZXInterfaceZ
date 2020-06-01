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

    nmi_access_o: out std_logic;
    rst8_det_o  : out std_logic;
    retn_det_o  : out std_logic  -- RETN detected
  );
end entity insn_detector;

architecture beh of insn_detector is

  signal retn_detected_r  :  std_logic;
  signal rst8_detected_r  :  std_logic;
  signal nmi_access_r     :  std_logic;

  type decode_state_type is (
    NO_PREFIX,
    PREFIX_DD,
    PREFIX_FD,
    PREFIX_CB,
    PREFIX_DD_CB,
    PREFIX_FD_CB,
    PREFIX_ED
  );

  signal decode_state: decode_state_type;

begin


  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      decode_state    <= NO_PREFIX;
    elsif rising_edge(clk_i) then
      if valid_i='1' then
        if m1_i='0' then -- M1 cycle
          case decode_state is
            when NO_PREFIX =>
              case d_i is
                when x"DD"  => decode_state <= PREFIX_DD;
                when x"FD"  => decode_state <= PREFIX_FD;
                when x"CB"  => decode_state <= PREFIX_CB;
                when x"ED"  => decode_state <= PREFIX_ED;
                when others => decode_state <= NO_PREFIX;
              end case;

            when PREFIX_DD =>
              case d_i is
                when x"CB"  => decode_state <= PREFIX_DD_CB;
                when others => decode_state <= NO_PREFIX;
              end case;

            when PREFIX_FD =>
              case d_i is
                when x"CB"  => decode_state <= PREFIX_FD_CB;
                when others => decode_state <= NO_PREFIX;
              end case;
              
            when others =>
              decode_state <= NO_PREFIX;
          end case;
        else -- Not M1 cycle
          decode_state    <= NO_PREFIX;
        end if;
      end if;
    end if;
  end process;



  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      retn_detected_r <= '0';
      nmi_access_r    <= '0';

    elsif rising_edge(clk_i) then
      retn_detected_r <= '0';
      rst8_detected_r <= '0';
      nmi_access_r    <= '0';
      pc_valid_o      <= '0';

      if valid_i='1' then

        if m1_i='0' then
          pc_valid_o  <= '1';
          pc_o        <= a_i;

          if decode_state=PREFIX_ED and d_i=x"45" then
            retn_detected_r <= '1';
          end if;

          if decode_state=NO_PREFIX and d_i=x"CF" then
            rst8_detected_r <= '1';
          end if;

        end if;


        -- Detect entry in NMI handler
        if a_i=x"0066" and m1_i='0' then
          nmi_access_r <= '1';
        end if;

      end if;

    end if;
  end process;

  retn_det_o    <= retn_detected_r;
  nmi_access_o  <= nmi_access_r;
  rst8_det_o    <= rst8_detected_r;

end beh;

