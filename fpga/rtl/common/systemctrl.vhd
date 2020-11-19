library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
library work;
use work.zxinterfacepkg.all;
use work.ahbpkg.all;
-- synthesis translate_off
use work.txt_util.all;
-- synthesis translate_on

entity systemctrl is
  port (
    clk_i                 : in std_logic;
    arst_i                : in std_logic;

    ahb_m2s_i             : in AHB_M2S;
    ahb_s2m_o             : out AHB_S2M;

    pc_i                  : in std_logic_vector(15 downto 0);

    bit_to_cpu_i          : in bit_to_cpu_t;
    bit_from_cpu_o        : out bit_from_cpu_t;

    vidmem_en_o           : out std_logic;
    vidmem_adr_o          : out std_logic_vector(12 downto 0);
    vidmem_data_i         : in std_logic_vector(7 downto 0);

    rstfifo_o             : out std_logic;
    rstspect_o            : out std_logic;
    intenable_o           : out std_logic;
    capsyncen_o           : out std_logic;
    frameend_o            : out std_logic;
    mode2a_o              : out std_logic;

    vidmode_o             : out std_logic_vector(1 downto 0);
    ulahack_o             : out std_logic;

    forceromcs_trig_on_o  : out std_logic;
    forceromcs_trig_off_o : out std_logic;
    forceromonretn_trig_o : out std_logic; -- single tick, SPI sck
    forcenmi_trig_on_o    : out std_logic; -- single tick, SPI sck
    forcenmi_trig_off_o   : out std_logic; -- single tick, SPI sck
    nmireason_o           : out std_logic_vector(7 downto 0);
    -- Resource FIFO

    resfifo_reset_o       : out std_logic;
    resfifo_wr_o          : out std_logic;
    resfifo_write_o       : out std_logic_vector(7 downto 0);
    resfifo_full_i        : in std_logic_vector(3 downto 0);

    -- TAP player FIFO
    tapfifo_reset_o       : out std_logic;
    tapfifo_wr_o          : out std_logic;
    tapfifo_write_o       : out std_logic_vector(8 downto 0);
    tapfifo_full_i        : in std_logic;
    tapfifo_used_i        : in std_logic_vector(9 downto 0);
    tap_enable_o          : out std_logic;

    -- Command FIFO

    cmdfifo_reset_o       : out std_logic;
    cmdfifo_rd_o          : out std_logic;
    cmdfifo_read_i        : in std_logic_vector(7 downto 0);
    cmdfifo_empty_i       : in std_logic;
    cmdfifo_intack_o      : out std_logic; -- Interrupt acknowledge

    -- External RAM access
    extram_addr_o         : out std_logic_vector(31 downto 0);
    extram_dat_i          : in std_logic_vector(31 downto 0);
    extram_dat_o          : out std_logic_vector(31 downto 0);
    extram_req_o          : out std_logic;
    extram_we_o           : out std_logic;
    --extram_rd_o           : out std_logic;
    extram_valid_i        : in std_logic;

    -- Generic address/data
    generic_addr_o        : out std_logic_vector(10 downto 0);
    generic_dat_o         : out std_logic_vector(7 downto 0);

    -- USB access

    usb_rd_o              : out std_logic;
    usb_wr_o              : out std_logic;
    usb_dat_i             : in std_logic_vector(7 downto 0);
    usb_int_i             : in std_logic; -- USB interrupt

    -- Capture access
    capture_rd_o          : out std_logic;
    capture_wr_o          : out std_logic;
    capture_dat_i         : in std_logic_vector(7 downto 0);

    -- Keyboard manipulation
    kbd_en_o              : out std_logic;
    kbd_force_press_o     : out std_logic_vector(39 downto 0); -- 40 keys.
    -- Joystick data
    joy_en_o              : out std_logic;
    joy_data_o            : out std_logic_vector(4 downto 0);
    -- Mouse
    mouse_en_o            : out std_logic;
    mouse_x_o             : out std_logic_vector(7 downto 0);
    mouse_y_o             : out std_logic_vector(7 downto 0);
    mouse_buttons_o       : out std_logic_vector(1 downto 0);
    -- AY
    ay_en_o               : out std_logic;
    ay_en_reads_o         : out std_logic;
    -- Volume settings
    volume_o              : out std_logic_vector(63 downto 0);
    audio_enable_o        : out std_logic;
    memromsel_o           : out std_logic_vector(2 downto 0);
    memsel_we_o           : out std_logic;
    romsel_we_o           : out std_logic
  );
end systemctrl;


architecture beh of systemctrl is

  constant NUMREGS32  : natural := 8;

  subtype reg32_type is std_logic_vector(31 downto 0);
  type regs32_type is array(0 to NUMREGS32-1) of reg32_type;

  signal regs32_r       : regs32_type := (others => (others => '0'));
  signal tempreg_r      : std_logic_vector(23 downto 0);
  signal pc_latch_r     : std_logic_vector(7 downto 0);

  signal flags_r      : std_logic_vector(15 downto 0);

  -- For AHB interconnection
  signal rd_s           : std_logic;
  signal wr_s           : std_logic;
  signal addr_s         : std_logic_vector(5 downto 0);
  signal dat_in_s       : std_logic_vector(7 downto 0);
  signal dat_out_s      : std_logic_vector(7 downto 0);

  --
  signal bit_we_s       : std_logic;
  signal bit_dout_s     : std_logic_vector(7 downto 0);

  signal memsel_we_r    : std_logic;
  signal romsel_we_r    : std_logic;
  signal memromsel_r    : std_logic_vector(2 downto 0);

  signal do_read_fifo_r : std_logic;
  signal tapfifo_used_lsb_r : std_logic_vector(7 downto 0);

begin

  ahb2rdwr_inst: entity work.ahb2rdwr
    generic map (
      AWIDTH => 6, DWIDTH => 8
    )
    port map (
      clk_i     => clk_i,
      arst_i    => arst_i,
      ahb_m2s_i => ahb_m2s_i,
      ahb_s2m_o => ahb_s2m_o,

      addr_o    => addr_s,
      dat_o     => dat_in_s,
      dat_i     => dat_out_s,
      rd_o      => rd_s,
      wr_o      => wr_s
    );

  bit_ctrl_inst: entity work.bit_ctrl
    port map (
      clk_i         => clk_i,
      arst_i        => arst_i,
      bit_enable_i  => flags_r(14),
      bit_data_i    => bit_to_cpu_i.bit_data,
      bit_data_o    => bit_from_cpu_o.bit_data,
      bit_we_i      => bit_we_s,
      bit_index_i   => unsigned(addr_s(1 downto 0)),
      bit_din_i     => dat_in_s,
      bit_dout_o    => bit_dout_s
    );

  bit_we_s <= '1' when wr_s='1' and addr_s(5 downto 2)="0010" else '0';


  -- Read multiplexing
  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      dat_out_s <= (others => 'X');
      cmdfifo_rd_o    <= '0';
      do_read_fifo_r  <= '0';
      bit_from_cpu_o.rx_read <= '0';
    elsif rising_edge(clk_i) then

      cmdfifo_rd_o <= '0';
      bit_from_cpu_o.rx_read <= '0';

      if rd_s='0' then
        dat_out_s <= (others => 'X');
      else
        dat_out_s <= (others => 'X');

        case addr_s(5 downto 0) is
          when "000000" =>
            dat_out_s <= C_FPGAID0;
          when "000001" =>
            dat_out_s <= C_FPGAID1;
          when "000010" =>
            dat_out_s <= C_FPGAID2;
          when "000011" => null;
          when "000100" =>
            dat_out_s <= bit_to_cpu_i.bit_request & '0' & cmdfifo_empty_i & resfifo_full_i & '0';
          when "000101" => null; -- Write-only
          when "000110" => null; -- Write-only
          when "000111" => null; -- Write-only
          when "001000" | "001001" |"001010" | "001011" => null; -- Write-only
          when "001100" | "001101" |"001110" | "001111" => -- BIT data
            dat_out_s <= bit_dout_s;
          when "010000" => -- PC LSB
            pc_latch_r  <= pc_i(15 downto 8);
            dat_out_s   <= pc_i(7 downto 0);
          when "010001" => -- PC MSB
            dat_out_s   <= pc_latch_r;
          when "010010" => null;-- Write-only
          when "010011" => null;-- Write-only
          when "010100" => null;-- Write-only
          -- CMD fifo.
          when "010101" =>
            if cmdfifo_empty_i='1' then
              dat_out_s <= x"FF";
              do_read_fifo_r <= '0';
            else
              dat_out_s <= x"FE";
              do_read_fifo_r <= '1';
            end if;

          when "010110" =>
            cmdfifo_rd_o  <= do_read_fifo_r;
            dat_out_s     <= cmdfifo_read_i;
          when "010111" => null; -- Unused
          when "011000" | "011001"=> null; -- TAP Fifo/Command write;

          when "011010" =>
            dat_out_s <= tapfifo_full_i & "00000" & tapfifo_used_i(9 downto 8);
            tapfifo_used_lsb_r <= tapfifo_used_i(7 downto 0);

          when "011011" =>
            dat_out_s <= tapfifo_used_lsb_r;

          when "011100" => -- Test UART status
            dat_out_s   <= "10" &
                  bit_to_cpu_i.rx_avail_size &
                  bit_to_cpu_i.rx_avail &
                  bit_to_cpu_i.tx_busy;
          when "011101" => -- Test UART data
            dat_out_s   <= bit_to_cpu_i.rx_data;
            bit_from_cpu_o.rx_read <= '1';




          when others =>
        end case;
      end if;
    end if;
  end process;

  process(clk_i, arst_i)
  begin
    if arst_i='1' then

      flags_r               <= (others => '0');
      resfifo_reset_o       <= '0';
      forceromonretn_trig_o <= '0';
      forceromcs_trig_on_o  <= '0';
      forceromcs_trig_off_o <= '0';
      forcenmi_trig_on_o    <= '0';
      forcenmi_trig_off_o   <= '0';
      cmdfifo_intack_o      <= '0';
      cmdfifo_reset_o       <= '0';

      memsel_we_r           <= '0';
      romsel_we_r           <= '0';
      memromsel_r           <= (others => 'X');
      --bit_from_cpu_o.tx_data_valid <= '0';
      --bit_from_cpu_o.tx_data  <= (others => 'X');

    elsif rising_edge(clk_i) then

      resfifo_reset_o       <= '0';
      forceromonretn_trig_o <= '0';
      forceromcs_trig_on_o  <= '0';
      forceromcs_trig_off_o <= '0';
      forcenmi_trig_on_o    <= '0';
      forcenmi_trig_off_o   <= '0';
      cmdfifo_intack_o      <= '0';
      cmdfifo_reset_o       <= '0';
      memsel_we_r           <= '0';
      romsel_we_r           <= '0';
      memromsel_r           <= (others => 'X');
      --bit_from_cpu_o.tx_data_valid <= '0';
      --bit_from_cpu_o.tx_data  <= (others => 'X');
      

      if wr_s='1' then
        case addr_s is
          when "000000" => null;
          when "000001" => null;
          when "000010" => null;
          when "000011" => null;
          when "000100" => null;
          when "000101" =>
            flags_r(7 downto 0) <= dat_in_s;
          when "000110" =>
            resfifo_reset_o       <= dat_in_s(0);
            forceromonretn_trig_o <= dat_in_s(1);
            forceromcs_trig_on_o  <= dat_in_s(2);
            forceromcs_trig_off_o <= dat_in_s(3);
            -- Command FIFO interrupt acknowledge
            cmdfifo_intack_o      <= dat_in_s(4);
            cmdfifo_reset_o       <= dat_in_s(5);
            forcenmi_trig_on_o    <= dat_in_s(6);
            forcenmi_trig_off_o   <= dat_in_s(7); -- this might not be necessary.
          when "000111" =>
            flags_r(15 downto 8) <= dat_in_s;
          when "001000" | "001001" | "001010" | "001011" => null; -- BIT is handled by BIT mode
          when "001100" | "001101" | "001110" | "001111" => null; -- BIT is handled by BIT mode
          when "010000" | "010001" => null; -- Last PC not writeable
          when "010010" =>
            memromsel_r <= dat_in_s(2 downto 0);
            memsel_we_r <= dat_in_s(7);
            romsel_we_r <= not dat_in_s(7);
          when "010011" => null; -- Frame EOF not implemented
          when "010100" => null; -- Resource FIFO write
          when "010101" | "010111" => null; -- Command FIFO read
          when "011000" => null; -- TAP fifo write
          when "011001" => null; -- TAP command fifo write
          when "011010" | "011011"=> null; -- TAP command fifo usage read
          when "011100" => null; -- Test UART status read
          when "011101" => null; -- Test UART data
             --TestUARTData  0XXXXXXXXXX10XXXXXX011101 1
            --bit_from_cpu_o.tx_data_valid <= '1';
            --bit_from_cpu_o.tx_data  <= dat_in_s;

          when "1---00" =>
            tempreg_r(23 downto 16) <= dat_in_s;
          when "1---01" =>
            tempreg_r(15 downto 8) <= dat_in_s;
          when "1---10" =>
            tempreg_r(7 downto 0) <= dat_in_s;
          when "1---11" =>
            regs32_r( to_integer(unsigned(addr_s(5 downto 2)))) <= tempreg_r & dat_in_s;


          when others =>
          -- synthesis translate_off
            report "TBD write addr " & str(addr_s) severity failure;
          -- synthesis translate_on
        end case;
      end if;

    end if;
  end process;

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      mode2a_o <= '0';
    elsif rising_edge(clk_i) then
      if flags_r(13)='1' then
        mode2a_o <= '1';
      end if;
    end if;
  end process;



  rstfifo_o             <= flags_r(0);
  rstspect_o            <= flags_r(1);
  intenable_o           <= flags_r(5); -- Interrupt enable
  capsyncen_o           <= flags_r(6); -- Capture sync

  ulahack_o             <= flags_r(8); --
  tapfifo_reset_o       <= flags_r(9); --
  tap_enable_o          <= flags_r(10); --

  vidmode_o(0)          <= flags_r(11); --
  vidmode_o(1)          <= flags_r(12); --

  bit_from_cpu_o.bit_enable <= flags_r(14); -- BIT enabled
  audio_enable_o        <= flags_r(15);

  resfifo_wr_o          <= '1' when wr_s='1' and addr_s="010100" else '0';
  resfifo_write_o       <= dat_in_s;

  -- Last address bit determines wheteher is command or data
  tapfifo_wr_o          <= '1' when wr_s='1' and addr_s(5 downto 1)="01100" else '0';
  tapfifo_write_o       <= addr_s(0) & dat_in_s;

  bit_from_cpu_o.tx_data_valid <= '1' when wr_s='1' and addr_s="011101" else '0';
  bit_from_cpu_o.tx_data  <= dat_in_s;

  kbd_en_o              <= regs32_r(2)(0);
  joy_en_o              <= regs32_r(2)(1);
  mouse_en_o            <= regs32_r(2)(2);
  ay_en_o               <= regs32_r(2)(3);
  ay_en_reads_o         <= regs32_r(2)(4);

  kbd_force_press_o     <= regs32_r(4)(7 downto 0) & regs32_r(3);

  -- Joystick and mouse data

  mouse_x_o             <= regs32_r(5)(7 downto 0);
  mouse_y_o             <= regs32_r(5)(15 downto 8);
  mouse_buttons_o       <= regs32_r(5)(17 downto 16);
  joy_data_o            <= regs32_r(5)(22 downto 18);

  -- Volumes
  volume_o              <= regs32_r(7) & regs32_r(6);
  -- NMI
  nmireason_o           <= regs32_r(0)(7 downto 0);

  memromsel_o           <= memromsel_r;
  memsel_we_o           <= memsel_we_r;
  romsel_we_o           <= romsel_we_r;





end beh;


