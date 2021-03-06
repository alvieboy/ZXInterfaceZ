library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.STD_LOGIC_misc.all;
use IEEE.numeric_std.all;
library work;
use work.zxinterfacepkg.all;
use work.zxinterfaceports.all;
-- synthesis translate_off
use work.txt_util.all;
-- synthesis translate_on

entity interfacez_io is
  port  (
    clk_i     : in std_logic;
    rst_i     : in std_logic;

    --ioreq_i   : in std_logic;
    --rd_i      : in std_logic;
    wrp_i     : in std_logic;
    rdp_i     : in std_logic;
    rdp_dly_i : in std_logic;
    active_i  : in std_logic;
    ulahack_i : in std_logic;
    mode2a_i  : in std_logic;
    tap_enable_i: in std_logic;

    -- NMI reason
    nmireason_i: in std_logic_vector(7 downto 0);

    adr_i     : in std_logic_vector(15 downto 0);
    dat_i     : in std_logic_vector(7 downto 0);
    dat_o     : out std_logic_vector(7 downto 0);
    enable_o  : out std_logic;

    port_fe_o : out std_logic_vector(5 downto 0);
    audio_i   : in std_logic;
    ear_o     : out std_logic; -- Comes from the cassete player
    mic_o     : out std_logic; -- Goes to cassete player

    forceiorqula_o  : out std_logic;
    keyb_trigger_o  : out std_logic;

    -- Resource request control
    spec_nreq_o : out std_logic; -- Spectrum data request.

    -- Resource FIFO connections

    resfifo_rd_o           : out std_logic;
    resfifo_read_i         : in std_logic_vector(7 downto 0);
    resfifo_empty_i        : in std_logic;
    -- Command FIFO connection

    cmdfifo_wr_o           : out std_logic;
    cmdfifo_write_o        : out std_logic_vector(7 downto 0);
    cmdfifo_full_i         : in std_logic;
    -- Ram
    ram_addr_o             : out std_logic_vector(23 downto 0);
    ram_dat_o              : out std_logic_vector(7 downto 0);
    ram_dat_i              : in std_logic_vector(7 downto 0);
    ram_wr_o               : out std_logic;
    ram_rd_o               : out std_logic;
    ram_ack_i              : in std_logic;

    -- Keyboard manipulation
    kbd_en_i              : in std_logic;
    kbd_force_press_i     : in std_logic_vector(39 downto 0); -- 40 keys.
    -- Joystick data
    joy_en_i              : in std_logic;
    joy_data_i            : in std_logic_vector(5 downto 0);
    -- Mouse
    mouse_en_i            : in std_logic;
    mouse_x_i             : in std_logic_vector(7 downto 0);
    mouse_y_i             : in std_logic_vector(7 downto 0);
    mouse_buttons_i       : in std_logic_vector(1 downto 0);
    -- AY interface
    ay_en_i               : in std_logic; -- Global AY enable
    ay_en_reads_i         : in std_logic; -- AY read enable. Only for models *WITHOUT* AY, otherwise conflicts.
    ay_wr_o               : out std_logic;
    ay_din_i              : in std_logic_vector(7 downto 0);
    ay_adr_o              : out std_logic_vector(3 downto 0);
    ay_dout_o             : out std_logic_vector(7 downto 0);

    -- Paging control
    romsel_o              : out std_logic_vector(1 downto 0);
    memsel_o              : out std_logic_vector(2 downto 0);

    -- Paging override
    romsel_i              : in std_logic_vector(1 downto 0);
    memsel_i              : in std_logic_vector(2 downto 0);
    romsel_we_i           : in std_logic;
    memsel_we_i           : in std_logic;

    page128_pmc_o         : out std_logic_vector(7 downto 0);
    page128_smc_o         : out std_logic_vector(7 downto 0);
  
    miscctrl_i            : in std_logic_vector(7 downto 0);
    miscctrl_o            : out std_logic_vector(7 downto 0);

    trig_force_clearromcsonret_o  : out std_logic;
    disable_romcs_o       : out std_logic;
    im_i                  : in std_logic_vector(1 downto 0); -- Detected interrupt mode
    dbg_o                 : out std_logic_vector(7 downto 0)
  );

end entity interfacez_io;

architecture beh of interfacez_io is

  signal port_fe_r        : std_logic_vector(5 downto 0);
  signal addr_match_s     : std_logic;
  signal enable_s         : std_logic;
  signal dataread_r       : std_logic_vector(7 downto 0);
  signal testdata_r       : unsigned(7 downto 0);
  signal d_write_r        : std_logic_vector(7 downto 0);
  signal cmdfifo_wr_r     : std_logic;
  signal uladata_r        : std_logic_vector(7 downto 0);
  signal forceoutput_s    : std_logic;
  signal ram_wr_r         : std_logic;
  signal ram_rd_r         : std_logic;
  signal ram_addr_r       : unsigned(23 downto 0);

  signal scratch0_r       : std_logic_vector(7 downto 0);
  signal scratch1_r       : std_logic_vector(7 downto 0);

  signal keyb_data_s      : std_logic_vector(4 downto 0);

  constant ULA_HACK_DLY   : natural := 4;

  signal uladly_r         : std_logic_vector(ULA_HACK_DLY-1 downto 0);

  signal has_key_inject_s : std_logic;

  signal internal_reg_sel_s     : std_logic; -- Internal register access
  signal kempston_joy_sel_s     : std_logic;
  signal kempston_mousex_sel_s  : std_logic;
  signal kempston_mousey_sel_s  : std_logic;
  signal kempston_mouseb_sel_s  : std_logic;
  signal kempston_mouse_sel_s   : std_logic;

  signal ay_register_sel_s      : std_logic;
  signal ay_data_sel_s          : std_logic;
  signal ay_sel_s               : std_logic;
  signal ay_read_active_s       : std_logic;
  signal ayreg_r                : std_logic_vector(3 downto 0);

  signal romsel_r               : std_logic_vector(1 downto 0);
  signal memsel_r               : std_logic_vector(2 downto 0);

  signal page128_sel_s          : std_logic;
  signal page2a_pmc_sel_s       : std_logic;
  signal page2a_smc_sel_s       : std_logic;

  signal page128_pmc_r          : std_logic_vector(7 downto 0); -- Reused in 128K mode
  signal page128_smc_r          : std_logic_vector(7 downto 0);
  signal trig_force_clearromcsonret_r : std_logic;
  signal pseudo_audio_s         : std_logic_vector(7 downto 0);

begin

  --
  -- Port matching flags. Set to '1' if address matches the defined "range" for the device.
  -- See zxinterfaceports.vhd for a list of supported ports.
  --

  -- Internal register access
  internal_reg_sel_s    <= z80_address_match_std_logic(adr_i, SPECT_PORT_INTERNAL_REGISTER);

  -- Kempston joystick
  kempston_joy_sel_s    <= z80_address_match_std_logic(adr_i, SPECT_PORT_KEMPSTON_JOYSTICK);

  -- Kempston mouse
  kempston_mousex_sel_s <= z80_address_match_std_logic(adr_i, SPECT_PORT_KEMPSTON_MOUSEX);
  kempston_mousey_sel_s <= z80_address_match_std_logic(adr_i, SPECT_PORT_KEMPSTON_MOUSEY);
  kempston_mouseb_sel_s <= z80_address_match_std_logic(adr_i, SPECT_PORT_KEMPSTON_MOUSEB);
  -- Aggregate mouse match (x, y or button)
  kempston_mouse_sel_s  <= kempston_mousex_sel_s or kempston_mousey_sel_s or kempston_mouseb_sel_s;

  -- AY
  ay_register_sel_s     <= z80_address_match_std_logic(adr_i, SPECT_PORT_AY_REGISTER);
  ay_data_sel_s         <= z80_address_match_std_logic(adr_i, SPECT_PORT_AY_DATA);
  -- Aggregate AY select (register or data)
  ay_sel_s              <= ay_register_sel_s OR ay_data_sel_s;
  -- AY read active will also depend if AY is enabled and if AY reads are also enabled.
  ay_read_active_s      <= ay_sel_s AND ay_en_reads_i AND ay_en_i;

  -- Page registers selection, depends also if we are using +2A mode or not.
  page128_sel_s         <= z80_address_match_std_logic(adr_i, SPECT_PORT_128PAGE_REGISTER) AND NOT mode2a_i;
  page2a_pmc_sel_s      <= z80_address_match_std_logic(adr_i, SPECT_PORT_2A_PMC_REGISTER) AND mode2a_i;
  page2a_smc_sel_s      <= z80_address_match_std_logic(adr_i, SPECT_PORT_2A_SMC_REGISTER) AND mode2a_i;

  -- Address match for reads

  addr_match_s <= internal_reg_sel_s
    OR kempston_joy_sel_s
    OR kempston_mouse_sel_s
    OR ay_read_active_s;

	enable_s  <=  active_i and addr_match_s;

  process(clk_i, rst_i)
  begin
    if rst_i='1' then

      port_fe_r     <= (others => '0');
      dataread_r    <= (others => 'X');
      resfifo_rd_o  <= '0';
      testdata_r    <= (others => '0');
      cmdfifo_wr_r  <= '0';
      ram_wr_r      <= '0';
      ram_rd_r      <= '0';
      ay_wr_o       <= '0';
      romsel_r      <= "00";
      memsel_r      <= "000";
      trig_force_clearromcsonret_r<='0';
      page128_pmc_r <= (others => '0');
      page128_smc_r <= (others => '0');
      ay_dout_o     <= (others => '0');
      ayreg_r       <= (others => '0');
      d_write_r     <= (others => 'X');
      ram_addr_r    <= (others => 'X');
      scratch0_r    <= (others => '0');
      scratch1_r    <= (others => '0');
      disable_romcs_o <= '0';
      ram_addr_r    <= (others => 'X');

    elsif rising_edge(clk_i) then

      resfifo_rd_o <= '0';
      cmdfifo_wr_r <= '0';
      ay_wr_o      <= '0';
      trig_force_clearromcsonret_r <= '0';

      if wrp_i='1' and adr_i(0)='0' then -- ULA write
        port_fe_r <= dat_i(5 downto 0);
        -- synthesis translate_off
        report "SET BORDER: "&hstr(dat_i);
        -- synthesis translate_on
      end if;

      -- WRITE REQUEST from Spectrum.

      if wrp_i='1' then
        case adr_i(7 downto 0) is
          when SPECT_PORT_SCRATCH0 =>
            scratch0_r <= dat_i;
          when SPECT_PORT_MISCCTRL =>
            scratch1_r <= dat_i;
          when SPECT_PORT_CMD_FIFO_DATA => -- Command FIFO write.
            d_write_r <= dat_i;
            if cmdfifo_full_i='0' then
              cmdfifo_wr_r    <= '1';
            end if;
          when SPECT_PORT_RAM_ADDR_LOW =>
            ram_addr_r(7 downto 0) <= unsigned(dat_i);
          when SPECT_PORT_RAM_ADDR_MIDDLE =>
            ram_addr_r(15 downto 8) <= unsigned(dat_i);

          when SPECT_PORT_RAM_ADDR_HIGH =>
            ram_addr_r(23 downto 16) <= unsigned(dat_i);

          when SPECT_PORT_RAM_DATA  => -- RAM write.
            d_write_r     <= dat_i;
            ram_wr_r      <= '1';

          when SPECT_PORT_MEMSEL =>
            memsel_r      <= dat_i(2 downto 0);

          when SPECT_PORT_NMIREASON =>
            -- We reuse this address to force ROMCS clear on RET
            trig_force_clearromcsonret_r <= dat_i(0);
            -- We also use it to disable ROMCS temporarly to perform ROM checksums
            disable_romcs_o <= dat_i(1);

          when others =>
        end case;

        if ay_register_sel_s='1' then
          ayreg_r <= dat_i(3 downto 0); -- AY register
        end if;

        if ay_data_sel_s='1' then
          ay_wr_o   <= ay_en_i; --'1';
          ay_dout_o <= dat_i;
        end if;

        if page128_sel_s='1' or page2a_pmc_sel_s='1' then
          page128_pmc_r <= dat_i;
        end if;

        if page2a_smc_sel_s='1' then
          page128_smc_r <= dat_i;
        end if;

      end if;

      if rdp_i='1' then
        case adr_i(7 downto 0) is

          when SPECT_PORT_SCRATCH0 =>
            dataread_r <= scratch0_r;

          when SPECT_PORT_MISCCTRL =>
            dataread_r <= miscctrl_i;--scratch1_r;

          when SPECT_PORT_CMD_FIFO_STATUS => -- Command FIFO status / IM read
            dataread_r <= "00000" & im_i & cmdfifo_full_i;

          when SPECT_PORT_RESOURCE_FIFO_STATUS => -- Resource FIFO status read
            dataread_r <= "0000000" & resfifo_empty_i;

          when SPECT_PORT_RESOURCE_FIFO_DATA => -- FIFO read
            dataread_r <= resfifo_read_i;
            -- Pop data on next clock cycle
            resfifo_rd_o <= '1';
            --testdata_r <= testdata_r + 1;

          when SPECT_PORT_RAM_ADDR_LOW =>
            dataread_r <= std_logic_vector(ram_addr_r(7 downto 0));

          when SPECT_PORT_RAM_ADDR_MIDDLE =>
            dataread_r <= std_logic_vector(ram_addr_r(15 downto 8));

          when SPECT_PORT_RAM_ADDR_HIGH =>
            dataread_r <= std_logic_vector(ram_addr_r(23 downto 16));

          when SPECT_PORT_RAM_DATA => -- RAM read
            ram_rd_r <= '1';
            --dataread_r <= (others => 'X');
          when SPECT_PORT_NMIREASON =>
            dataread_r <= nmireason_i;

          when SPECT_PORT_PSEUDO_AUDIO_DATA =>
            dataread_r <= pseudo_audio_s;

          when others =>
            dataread_r <= (others => '1');
        end case;

        if kempston_joy_sel_s='1' then
          if joy_en_i='1' then
            dataread_r <= joy_data_i(5) & "00" & joy_data_i(4 downto 0);
          else
            dataread_r <= x"FF";
          end if;
        end if;

        if kempston_mousex_sel_s='1' then
          if mouse_en_i='1' then
            dataread_r <= mouse_x_i;
          else
            dataread_r <= x"FF";
          end if;
        end if;

        if kempston_mousey_sel_s='1' then
          if mouse_en_i='1' then
            dataread_r <= mouse_y_i;
          else
            dataread_r <= x"FF";
          end if;
        end if;

        if kempston_mouseb_sel_s='1' then
          if mouse_en_i='1' then
            dataread_r <= "111111" & not mouse_buttons_i(1) & not mouse_buttons_i(0);
          else
            dataread_r <= x"FF";
          end if;
        end if;

        if ay_register_sel_s='1' or ay_data_sel_s='1' then
          dataread_r <= ay_din_i;
        end if;

      end if;

      if ram_ack_i='1' then
        if ram_rd_r='1' then
          -- data
          dataread_r <= ram_dat_i;
        end if;
        ram_wr_r <= '0';
        ram_rd_r <= '0';
        ram_addr_r <= ram_addr_r + 1;
      end if;

      if romsel_we_i='1' then
        romsel_r <= romsel_i;
      end if;

      if memsel_we_i='1' then
        memsel_r <= memsel_i;
      end if;
    end if;
  end process;

  -- Keyboard manipulation

  process(adr_i, dat_i, kbd_force_press_i)
    variable rowk_v   : std_logic_vector(4 downto 0);
    variable kdata_v  : std_logic_vector(4 downto 0);
  begin

    has_key_inject_s <= '0';

    if adr_i(0)='0' then
      -- Decode lines as a normal spectrum would
      kdata_v := dat_i(4 downto 0);

      l: for i in 0 to 7 loop
        if adr_i(8+i)='0' then
          -- Line selected.
          rowk_v  := kbd_force_press_i(((i+1)*5)-1 downto (i*5));
          has_key_inject_s <= or_reduce(rowk_v);

          kdata_v := NOT rowk_v;

        end if;
      end loop;

      keyb_data_s <= kdata_v;

    else
      keyb_data_s <= dat_i(4 downto 0);
    end if;
  end process;

  --has_key_inject_s <= or_reduce(kbd_force_press_i);

  dbg_o(0) <= has_key_inject_s;

  process(clk_i, rst_i)
  begin
    if rst_i='1' then
      forceiorqula_o  <= '0';
      forceoutput_s <= '0';
      keyb_trigger_o <= '0';
      uladata_r     <= (others => '0');
    --
    elsif rising_edge(clk_i) then
      keyb_trigger_o <= '0';

      uladly_r <= uladly_r(ULA_HACK_DLY-2 downto 0) & rdp_dly_i;

      -- Do not allow ULA hack if we are in mode2a.
      if rdp_dly_i='1' and (ulahack_i='1' and mode2a_i='0') and adr_i(0)='0' then
        -- ULA read. Capture ULA data
        uladata_r <= dat_i(7) & audio_i & dat_i(5) & dat_i(4 downto 0);--keyb_data_s;
        -- Start delay. Force IRQULA immediatly
        forceiorqula_o <= '1';
        forceoutput_s <= '1';

      elsif rdp_dly_i='1' and has_key_inject_s='1' and kbd_en_i='1' and adr_i(0)='0' then

        uladata_r <= '1' & audio_i & '1' & keyb_data_s;

        forceiorqula_o <= '1';
        forceoutput_s <= '1';

      end if;

      if active_i='0' then
        forceiorqula_o <= '0';
        forceoutput_s <='0';
      end if;

      if rdp_dly_i='1' and adr_i=x"7FFE" then
        if dat_i(4 downto 0)="11100" then
          keyb_trigger_o <= '1';
        end if;
      end if;
    end if;
  end process;

  process(tap_enable_i, audio_i)
  begin
    if tap_enable_i='0' then
      pseudo_audio_s <= (others => '0');
    else
      if audio_i='1' then
        pseudo_audio_s <= "00001011";
      else
        pseudo_audio_s <= "00000100";
      end if;
    end if;
  end process;

  dat_o           <= dataread_r when forceoutput_s='0' else uladata_r;
  port_fe_o       <= port_fe_r;
  enable_o        <= enable_s or forceoutput_s;
  cmdfifo_write_o <= d_write_r;
  ram_dat_o       <= d_write_r;
  cmdfifo_wr_o    <= cmdfifo_wr_r;
  ram_wr_o        <= ram_wr_r;
  ram_rd_o        <= ram_rd_r;
  ram_addr_o      <= std_logic_vector(ram_addr_r);
  mic_o           <= audio_i xor (port_fe_r(3) or port_fe_r(4));
  romsel_o        <= romsel_r;
  memsel_o        <= memsel_r;
  page128_pmc_o   <= page128_pmc_r;
  page128_smc_o   <= page128_smc_r;
  trig_force_clearromcsonret_o <= trig_force_clearromcsonret_r;
  miscctrl_o      <= scratch1_r;
  ay_adr_o        <= ayreg_r;

end beh;
