LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;
LIBRARY work;
use work.ahbpkg.all;

entity ram_adaptor is
  port (
    clk_i   : in std_logic;
    arst_i  : in std_logic;

    ahb_o   : out AHB_M2S;
    ahb_i   : in AHB_S2M;

    ram_addr_i             : in std_logic_vector(23 downto 0);
    ram_dat_i              : in std_logic_vector(7 downto 0);
    ram_dat_o              : out std_logic_vector(7 downto 0);
    ram_wr_i               : in std_logic;
    ram_rd_i               : in std_logic;
    ram_ack_o              : out std_logic;

    -- Spectrum interface
    spect_addr_i          : in std_logic_vector(15 downto 0);
    spect_data_i          : in std_logic_vector(7 downto 0);
    spect_data_o          : out std_logic_vector(7 downto 0);
    rom_active_i          : in std_logic;
    spect_clk_rise_i      : in std_logic;
    spect_clk_fall_i      : in std_logic;
    spect_wait_o          : out std_logic;
    -- Ticks
    spect_mem_rd_p_i      : in std_logic;
    spect_mem_wr_p_i      : in std_logic;
    romsel_i              : in std_logic_vector(1 downto 0);
    memsel_i              : in std_logic_vector(2 downto 0)

  );
end entity ram_adaptor;

architecture beh of ram_adaptor is

  type state_type is (
    IDLE,
    READ,
    WAITACK
  );
  type regs_type is record
    state   :  state_type;
    master  : std_logic;   -- '0': normal RAM interface, '1': spectrum interface
    srd     : std_logic;
    swr     : std_logic;
    ack     : std_logic;
    spectdata : std_logic_vector(7 downto 0);
    ramdata   : std_logic_vector(7 downto 0);
  end record;

  signal r: regs_type;
--
-- +---------------------+                                  +---+---+ +---------------------+
-- | External Addr Space |                                  | R | M | | Spectrum Addr Space |
-- |   Start  |   End    | Size |                           | S | S | | Start    | End      |
-- +----------+----------+------+---------------------------+---+---+-+----------+----------+
-- | 0x000000 | 0x001FFF |  8kB | Main InterfaceZ ROM       | 0 | X | |   0x0000 |   0x1FFF |
-- | 0x002000 | 0x003FFF |  8kB | Alternate small ROM       | 1 | X | |   0x0000 |   0x1FFF |
-- | 0x004000 | 0x007FFF | 16kB | Alternate big 16 ROM      | 2 | X | |   0x0000 |   0x3FFF |
-- | 0x008000 | 0x00FFFF | 32kB | Alternate big 32 ROM      | 3 | X | |   0x0000 |   0x7FFF | * only for +2A/+3 models, TBD
-- | 0x010000 | 0x011FFF |  8kB | RAM bank 0                |0,1| 0 | |   0x2000 |   0x3FFF |
-- | 0x012000 | 0x013FFF |  8kB | RAM bank 1                |0,1| 1 | |   0x2000 |   0x3FFF |
-- | 0x014000 | 0x015FFF |  8kB | RAM bank 2                |0,1| 2 | |   0x2000 |   0x3FFF |
-- | 0x016000 | 0x017FFF |  8kB | RAM bank 3                |0,1| 3 | |   0x2000 |   0x3FFF |
-- | 0x018000 | 0x019FFF |  8kB | RAM bank 4                |0,1| 4 | |   0x2000 |   0x3FFF |
-- | 0x01A000 | 0x01BFFF |  8kB | RAM bank 5                |0,1| 5 | |   0x2000 |   0x3FFF |
-- | 0x01C000 | 0x01DFFF |  8kB | RAM bank 6                |0,1| 6 | |   0x2000 |   0x3FFF |
-- | 0x01E000 | 0x01FFFF |  8kB | RAM bank 7                |0,1| 7 | |   0x2000 |   0x3FFF |
-- | 0x020000 | 0x7FFFFF |      | Unused                    | - | - | |   -      |   -      |
-- +----------+----------+------+---------------------------+---+---+-+----------+----------+


  function valid_write(addr: in std_logic_vector(15 downto 0);
      romsel: in std_logic_vector(1 downto 0);
      memsel: in std_logic_vector(2 downto 0)) return std_logic is
  begin
    case romsel(1 downto 0) is
      when "00" | "01" =>
        if addr(13)='1' then
          return '1';
        else
          return '0';
        end if;
      when others =>
        return '0';
    end case;
  end function;

  function translate_address(addr: in std_logic_vector(15 downto 0);
      romsel: in std_logic_vector(1 downto 0);
      memsel: in std_logic_vector(2 downto 0)) return std_logic_vector is

    variable aout: std_logic_vector(23 downto 0);
  begin
    aout := (others => '0');

    case romsel(1 downto 0) is
      when "00" | "01" =>
        if addr(13)='0' then
          -- ROM area
          aout(13) := romsel(0); -- Select main or alternate rom.
          aout(12 downto 0) := addr(12 downto 0);
        else
          -- Memory area
          aout(16) := '1';
          aout(15 downto 13) := memsel(2 downto 0);
          aout(12 downto 0) := addr(12 downto 0);

        end if;
      when "10" =>
        aout(14) := '1';
        aout(13 downto 0) := addr(13 downto 0);
      when "11" =>
        aout(15) := '1';
        aout(14 downto 0) := addr(14 downto 0);
      when others =>
    end case;
    return aout;
  end function;

  signal read_access_s      : std_logic;
  signal write_access_s     : std_logic;

  signal request_delayed    : std_logic;

begin

  read_access_s   <= rom_active_i  and spect_mem_rd_p_i;
  write_access_s  <= rom_active_i  and spect_mem_wr_p_i;

  process(clk_i, arst_i, ram_wr_i, r, ahb_i, ram_rd_i, spect_addr_i, spect_data_i, spect_mem_rd_p_i, spect_mem_wr_p_i,
    write_access_s, romsel_i, memsel_i, ram_dat_i, ram_addr_i, read_access_s, write_access_s)
    variable w: regs_type;
  begin
    w := r;

    ahb_o.HWRITE <= 'X';
    ahb_o.HTRANS <= C_AHB_TRANS_IDLE;
    ahb_o.HWDATA <= (others => 'X');
    ahb_o.HADDR  <= (others => 'X');
  
    --ram_ack_o <= '0';
    w.ack := '0';
    request_delayed <= '0';

    case r.state is

      when IDLE =>
        if ram_wr_i='1' or write_access_s='1' or r.swr='1' then
          ahb_o.HTRANS <= C_AHB_TRANS_SEQ;
          ahb_o.HWRITE <= ram_wr_i or ( (write_access_s or r.swr) and valid_write(spect_addr_i,romsel_i,memsel_i));

          if ahb_i.HREADY='1' then
            w.state := WAITACK;
          else
            if write_access_s='1' then
              w.swr := '1';  -- Latch write request
              request_delayed <= '1';
            end if;
          end if;
          if ram_wr_i='1' then
            w.master := '1';
            ahb_o.HWDATA(7 downto 0)  <= ram_dat_i;
            ahb_o.HADDR(23 downto 0)  <= ram_addr_i;
          else
            w.master := '0';
            ahb_o.HWDATA(7 downto 0)  <= spect_data_i;
            ahb_o.HADDR(23 downto 0)  <= translate_address(spect_addr_i, romsel_i, memsel_i);
          end if;
        elsif ram_rd_i='1' or read_access_s='1' or r.srd='1' then
          ahb_o.HTRANS <= C_AHB_TRANS_SEQ;
          ahb_o.HWRITE <= '0';
          if ahb_i.HREADY='1' then
            w.state := WAITACK;
          else
            if read_access_s='1' then
              w.srd := '1';  -- Latch read request
              request_delayed <= '1';
            end if;
          end if;
          if ram_rd_i='1' then
            w.master := '1';
            ahb_o.HWDATA(7 downto 0)  <= ram_dat_i;
            ahb_o.HADDR(23 downto 0)  <= ram_addr_i;
          else
            w.master := '0';
            ahb_o.HWDATA(7 downto 0)  <= spect_data_i;
            ahb_o.HADDR(23 downto 0)  <= translate_address(spect_addr_i, romsel_i, memsel_i);
          end if;
        end if;

      when WAITACK =>
        w.srd := '0';
        w.swr := '0';

        if r.master='1' then
          ahb_o.HWDATA(7 downto 0)  <= ram_dat_i;
          ahb_o.HADDR(23 downto 0)  <= ram_addr_i;
        else
          ahb_o.HWDATA(7 downto 0)  <= spect_data_i;
          ahb_o.HADDR(23 downto 0)  <= x"00" & spect_addr_i;
        end if;

        --ram_ack_o <= ahb_i.HREADY and r.master;  -- Ack only for correct master
        w.ack := ahb_i.HREADY and r.master;  -- Ack only for correct master

        if ahb_i.HREADY='1' then
          if r.master='0' then
            w.spectdata := ahb_i.HRDATA(7 downto 0);
          else
            w.ramdata   := ahb_i.HRDATA(7 downto 0);
          end if;
          --ram_ack_o<='1';
          w.state := IDLE;
        end if;

      when others =>
    end case;

    if arst_i='1' then
      r.state   <= IDLE;
      r.srd     <= '0';
      r.swr     <= '0';
      r.master  <= '0';
      r.ack     <= '0';
      r.spectdata  <= (others => '0');
      r.ramdata    <= (others => '0');
    elsif rising_edge(clk_i) then
      r <= w;
    end if;
  end process;

  waitgen: if false generate

  process(clk_i, arst_i)
  begin
    if arst_i='1' then
      spect_wait_o <= '0';
    elsif rising_edge(clk_i) then
      if request_delayed='1' then
        spect_wait_o <= '1';
      elsif spect_clk_fall_i='1' then
        spect_wait_o <= '0';
      end if;
    end if;
  end process;

  end generate;

  waitgen2: if true generate
    wg2: block
      signal delay: integer range 0 to 13;
    begin

      process(clk_i, arst_i)
      begin
        if arst_i='1' then
          spect_wait_o <= '0';
          delay <= 0;
        elsif rising_edge(clk_i) then
          if request_delayed='1' then
            spect_wait_o <= '1';
            delay <= 13;
          else
            if delay=0 then
              spect_wait_o <= '0';
            else
              delay <= delay - 1;
            end if;
          end if;
        end if;
      end process;
    end block;

  end generate;


  ahb_o.HSIZE               <= C_AHB_SIZE_BYTE;
  ahb_o.HBURST              <= C_AHB_BURST_INCR;
  ahb_o.HMASTLOCK           <= '0';
  ahb_o.HPROT               <= "0000";

  spect_data_o              <= r.spectdata;
  ram_dat_o                 <= r.ramdata;
  ram_ack_o                 <= r.ack;
end beh;
