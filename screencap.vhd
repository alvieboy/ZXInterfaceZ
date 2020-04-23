library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
-- synthesis translate_off
library work;
use work.txt_util.all;
-- synthesis translate_on

entity screencap is
  port (
    clk_i         : in std_logic;
    rst_i         : in std_logic;

    fifo_empty_i  : in std_logic;
    fifo_rd_o     : out std_logic;
    fifo_data_i   : in std_logic_vector(31 downto 0);

    -- Video mem access
    vidmem_clk_i  : in std_logic;
    vidmem_en_i   : in std_logic;
    vidmem_adr_i  : in std_logic_vector(12 downto 0);
    vidmem_data_o : out std_logic_vector(7 downto 0);

    capsyncen_i   : in std_logic;
    intr_i        : in std_logic;
    framecmplt_i  : in std_logic;
    border_i      : in std_logic_vector(2 downto 0);

    -- Videogen configuration
    -- Modeline syntax: pclk hdisp hsyncstart hsyncend htotal vdisp vsyncstart vsyncend vtotal [flags] Flags (optional): +HSync, -HSync, +VSync, -VSync, Interlace, DoubleScan, CSync, +CSync, -CSync
    -- Modeline "800x600"x75.0   49.50  800 816 896 1056  600 601 604 625 +hsync +vsync (46.9 kHz e)
    vidmode_i     : in std_logic;
    pixclk_i      : in std_logic;
    pixrst_i      : in std_logic
  );

end entity screencap;

architecture beh of screencap is

  signal buf_idx_r    : std_logic;
  signal ram_en_s     : std_logic;
  signal ram_we_s     : std_logic;
  signal ram_addr_s   : std_logic_vector(12 downto 0);
  signal ram_din_s    : std_logic_vector(7 downto 0);
  signal run_r        : std_logic;

  signal vgen_vaddr_s : std_logic_vector(12 downto 0);
  signal vgen_vdata_s : std_logic_vector(7 downto 0);
  signal vgen_ven_s   : std_logic;
  signal vgen_vbusy_s : std_logic;


  signal ram_addr_mux_s : std_logic_vector(12 downto 0);
begin

  -- Spectrum video memory addressing
  -- Bitmap starts at 0x4000.
  -- Attributes start at 0x5800, len 768 bytes (0x300)


  ram_addr_mux_s <= ram_addr_s when ram_we_s='1' else vgen_vaddr_s;
  vgen_vbusy_s <= ram_we_s;
--  vgen_vdata_s <=

  screen_ram: entity work.generic_dp_ram
  generic map (
    address_bits  => 13, -- 8KB
    data_bits     => 8

  )
  port map (
    clka    => clk_i,
    ena     => '1', --ram_en_s,
    wea     => ram_we_s,
    addra   => ram_addr_mux_s,
    dia     => ram_din_s,
    doa     => vgen_vdata_s,  -- for video gen

    clkb    => vidmem_clk_i,
    enb     => vidmem_en_i,
    web     => '0',
    dib     => x"00",
    addrb   => vidmem_adr_i,
    dob     => vidmem_data_o
  );

  fifo_rd_o <= (not fifo_empty_i) and run_r;

  process(clk_i, rst_i)
  begin
    if rst_i='1' then
      run_r <= '1';
    elsif rising_edge(clk_i) then
      if capsyncen_i='0' then
        run_r <= '1';
      else
        if intr_i='1' then
          run_r <= '0';
        elsif framecmplt_i='1' then
          run_r <= '1';
        end if;
      end if;
    end if;
  end process;


  process(clk_i, rst_i)
    variable addr_v: unsigned(15 downto 0);
    variable data_v: std_logic_vector(7 downto 0);
  begin
    if rst_i='1' then
    elsif rising_edge(clk_i) then
      ram_en_s <='0';
      ram_we_s <='0';
      if fifo_empty_i='0' and run_r='1' then
        -- Process data.
        addr_v := unsigned(fifo_data_i(15 downto 0));
        data_v := fifo_data_i(23 downto 16);

        if fifo_data_i(24)='0' then -- Memory data
          report "Mem data";
          -- 0100 0000 0000 0000
          -- 0101 1100 1111 1111
          if (addr_v >= x"4000") and (addr_v < x"5B00") then
            ram_addr_s <= std_logic_vector(addr_v(12 downto 0));
            ram_en_s  <= '1';
            ram_we_s  <= '1';
            ram_din_s <= data_v;
            report "Video data";
          end if;
        else
          -- synthesis translate_off
          report "IO write, address 0x"  & hstr(fifo_data_i(7 downto 0)) & " value " &
            hstr(data_v);
          -- synthesis translate_on

        end if;
      end if;
    end if;
  end process;


  vgen: entity work.videogen
    port map (
      clk_i         => clk_i,
      rst_i         => rst_i,

      vidmode_i     => vidmode_i,
      pixclk_i      => pixclk_i,
      pixrst_i      => pixrst_i,

      -- Video access
      vaddr_o       => vgen_vaddr_s,
      ven_o         => vgen_ven_s,
      vbusy_i       => vgen_vbusy_s,
      vdata_i       => vgen_vdata_s,
      vborder_i     => border_i,

      -- video out
      hsync_o       => open,
      vsync_o       => open,
      bright_o      => open,
      grb_o         => open
    );

end beh;

