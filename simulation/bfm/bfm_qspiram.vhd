LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

LIBRARY work;
use work.bfm_qspiram_p.all;
use work.txt_util.all;


entity bfm_qspiram is
  port (
    Cmd_i   : in Cmd_QSPIRam_type;
    --Data_o  : out Data_QSPIRam_type;
    SCK_i   : in std_logic;
    D_io    : inout std_logic_vector(3 downto 0);
    CSn_i   : in std_logic
  );
end entity bfm_qspiram;

architecture sim of bfm_qspiram is

  subtype memword_type is std_logic_vector(7 downto 0);
  type mem_type is array(0 to 65535) of memword_type;
  shared variable qspiram: mem_type := (others => (others => '0'));
  signal data_out_s :  std_logic_vector(3 downto 0);
  signal oe_s       :  std_logic_vector(3 downto 0) := (others => '0');
  signal qpi_r      : std_logic  := '0';
  signal index_r    : natural range 0 to 23;
  signal rindex_r   : natural range 0 to 23;
  type state_type is (
    CMD,
    ENDLESS,
    ADDRESS,
    DELAYREAD,
    READ1,
    READ2,
    WRITE1,
    WRITE2
    );

  signal state: state_type := CMD;
  signal iswrite: std_logic;
  signal wdata_r: std_logic_vector(3 downto 0);

  constant tACK: time := 6 ns;
  constant tKOH: time := 1.5 ns;


  
begin

  t: for i in 0 to 3 generate
    D_io(i) <= 'Z' when Cmd_i.Enabled=false or oe_s(i)='0' else data_out_s(i);
  end generate;

  process(CSn_i, SCK_i, Cmd_i.Enabled)
    variable din_byte_v : std_logic_vector(7 downto 0);
    variable write_v : std_logic_vector(7 downto 0);
    variable rval_v : std_logic_vector(7 downto 0);
    variable address_v : std_logic_vector(23 downto 0);
    variable code_complete_v: boolean;
  begin
    if Cmd_i.Enabled then
      if CSn_i='1' then
        state <= CMD;
        index_r <= 7;
        oe_s <= "0000";
      else
        if rising_edge(SCK_i) then
          code_complete_v := false;
          case state is
            when CMD =>
                oe_s <= "0000";
                if qpi_r='0' then
                din_byte_v(index_r) := D_io(0);
                if index_r=0 then
                  code_complete_v := true;
                else
                  index_r <= index_r - 1;
                end if;
              else
                -- QPI mode
                din_byte_v(index_r downto index_r-3) := D_io;
                if index_r=3 then
                  code_complete_v := true;
                else
                  index_r <= index_r - 4;
                end if;
              end if;

              if code_complete_v then
                  case din_byte_v is
                    when x"35" =>
                      -- qio enable
                      qpi_r <= '1';
                    when x"EB" => -- QIO read
                      index_r <= 23;
                      state <= ADDRESS;
                      iswrite<='0';
                    when x"38" => -- QIO write
                      index_r <= 23;
                      state <= ADDRESS;
                      iswrite<='1';

                    when others =>
                      report "Unsupported SPI CODE 0x" & hstr(din_byte_v) severity failure;
                  end case;
              end if;

            when ADDRESS =>
              if qpi_r='0' then
                address_v(index_r) := D_io(0);
                if index_r=0 then
                  code_complete_v := true;
                else
                  index_r <= index_r - 1;
                end if;
              else
                address_v(index_r downto index_r-3) := D_io(3 downto 0);
                if index_r=3 then
                  code_complete_v := true;
                else
                  index_r <= index_r - 4;
                end if;
              end if;
              if code_complete_v then
                if iswrite='1' then
                  report "Write address: 0x" & hstr(address_v);
                  state <= WRITE1;
                else
                  report "Read address: 0x" & hstr(address_v);
                  state <= DELAYREAD;
                  index_r <= 5; -- delay
                end if;
              end if;

            when DELAYREAD =>
                if index_r=0 then
                  state <= READ1;
                  if qpi_r='1' then
                    oe_s <= (others => '1');
                  else
                    oe_s(3) <= '1';
                  end if;
                else
                  index_r <= index_r - 1;
                end if;
              when WRITE1 =>
                wdata_r <= D_io(3 downto 0);
                state <= WRITE2;
              when WRITE2 =>
                write_v := wdata_r & D_io;
                report "  > " &hstr(address_v) & " Data " & hstr(write_v);
                qspiram( to_integer( unsigned(address_v(15 downto 0))  ) ) := write_v;
                state <= WRITE1;
              when READ1 | READ2 =>
                oe_s <= "1111";

            when others =>

          end case;
        end if;

        if falling_edge(SCK_i) then
          case state is
            when READ1 =>
              if qpi_r='1' then

                  rval_v := qspiram( to_integer( unsigned(address_v(15 downto 0))  ) );
                if rindex_r=3 then

                  report "  < " &hstr(address_v) & " Data " & hstr(rval_v);

                  data_out_s <= "XXXX" after tKOH,  rval_v(3 downto 0) after tACK;
                  rindex_r <= 7;
                else
                  data_out_s <= "XXXX" after tKOH,  rval_v(7 downto 4) after tACK;
                  rindex_r <= rindex_r - 4;
                end if;
              else
                if rindex_r=0 then
                  data_out_s(3) <= 'X' after tKOH,  '0' after tACK;
                  rindex_r <= 7;
                else
                  data_out_s(3) <= 'X' after tKOH,  '1' after tACK;
                  rindex_r <= rindex_r - 1;
                end if;

              end if;
            when others =>
              rindex_r <= 7;
          end case;
        end if;


      end if;
    end if;
  end process;

  -- Timing checks
  process
    variable lastclk: time;
    variable lastsel: time;
    variable delta: time;
  begin
    wait on SCK_i, CSn_i;
    if rising_edge(SCK_i) then
      lastclk := now;

      delta := now - lastsel;
      if delta < 2.5 ns then
        report "TIMING VIOLATION: tCSP<2.5ns failed, tCSP=" & time'image(delta) severity failure;
      end if;

    end if;
    if rising_edge(CSn_i) then
      delta := now - lastclk;
      if delta < 20 ns then
        report "TIMING VIOLATION: tCHD<20ns failed, tCHD=" & time'image(delta) severity failure;
      end if;
    end if;
    if falling_edge(CSn_i) then
      lastsel := now;
    end if;
  end process;

end sim;
