library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.numeric_std.all;
-- synthesis translate_off
library work;
use work.txt_util.all;
-- synthesis translate_on

entity videogen is
  port (
    clk_i         : in std_logic;
    rst_i         : in std_logic;

    vidmode_i     : in std_logic;
    pixclk_i      : in std_logic;
    pixrst_i      : in std_logic;

    -- Video access
    vaddr_o       : out std_logic_vector(12 downto 0);
    ven_o         : out std_logic;
    vbusy_i       : in std_logic;
    vdata_i       : in std_logic_vector(7 downto 0); -- Bitmap data
    vborder_i     : in std_logic_vector(2 downto 0);

    -- video out
    hsync_o       : out std_logic;
    vsync_o       : out std_logic;
    bright_o      : out std_logic;
    grb_o         : out std_logic_vector(2 downto 0)
  );
end entity videogen;

architecture beh of videogen is

  type hvtiming_type is record
    ddisplay      : natural range 0 to 2047; -- H Display
    dfront        : natural range 0 to 2047; -- H Display + Front porch
    dsync         : natural range 0 to 2047; -- H Display + Front porch + Sync
    dtotal        : natural range 0 to 2047; -- H Total (Display+Front porch+Sync+Back porch)
    ddisplaystart : natural range 0 to 2047; -- Actual spectrum display start
    ddisplayend   : natural range 0 to 2047; -- Actual spectrum display end
  end record;

  type videoconf_type is record
    h         : hvtiming_type;
    v         : hvtiming_type;
    dbltriple : std_logic;
    hsync     : std_logic;
    vsync     : std_logic;
  end record videoconf_type;

  -- Modeline "800x600"x60.3   40.00  800 840 968 1056  600 601 605 628 +hsync +vsync (37.9 kHz e)
  constant v0: videoconf_type := (
    h => (
      ddisplay      => 800,
      dfront        => 840,
      dsync         => 968,
      dtotal        => 1056,
      ddisplaystart => 16,     -- (800-(256*3))/2
      ddisplayend   => 16+786
    ),
    v => (
      ddisplay      => 600,
      dfront        => 601,
      dsync         => 605,
      dtotal        => 628,
      ddisplaystart => 8,
      ddisplayend   => 8+384
    ),
    dbltriple =>  '1', -- Triplicate pixels
    hsync     =>  '1',
    vsync     =>  '1'
  );

  -- Modeline "720x400"x70.1   28.32  720 738 846 900  400 412 414 449 -hsync +vsync (31.5 kHz e)
  constant v1: videoconf_type := (
    h => (
      ddisplay      => 720,
      dfront        => 738,
      dsync         => 846,
      dtotal        => 900,
      ddisplaystart => 95,     -- (720-(256*2))/2
      ddisplayend   => 95+512
    ),
    v => (
      ddisplay      => 400,
      dfront        => 412,
      dsync         => 414,
      dtotal        => 449,
      ddisplaystart => 8,
      ddisplayend   => 8+384
    ),
    dbltriple => '0',
    hsync     =>  '0',
    vsync     =>  '1'
  );

  constant test0: videoconf_type := (
    h => (
      ddisplay      => 514,
      dfront        => 515,
      dsync         => 516,
      dtotal        => 517,
      ddisplaystart => 1,     -- (720-(256*2))/2
      ddisplayend   => 1+512
    ),
    v => (
      ddisplay      => 386,
      dfront        => 387,
      dsync         => 383,
      dtotal        => 389,
      ddisplaystart => 1,
      ddisplayend   => 1+384
    ),
    dbltriple => '0',
    hsync     =>  '1',
    vsync     =>  '1'
  );


  function videoparam(mode: in std_logic) return videoconf_type is
    variable vp_v: videoconf_type;
  begin
    if mode='0' then
      vp_v := v0;
    else
      vp_v := v1;
    end if;
    -- synthesis translate_off
    vp_v := test0;
    -- synthesis translate_on
    return vp_v;
  end function;

  signal fifo_rd_s  : std_logic;

  signal hcounter   : natural range 0 to 2047;
  signal vcounter   : natural range 0 to 2047;
  signal vtick_s    : std_logic;

  signal hoverflow  : std_logic;
  signal voverflow  : std_logic;
  signal blank_s    : std_logic;
  signal hblank_s   : std_logic;
  signal vblank_s   : std_logic;

  signal hdisplay_s : std_logic;
  signal vdisplay_s : std_logic;
  signal display_s  : std_logic;

  signal pixel_s    : std_logic_vector(7 downto 0);
  signal attr_s     : std_logic_vector(7 downto 0);
  signal poffset_r  : unsigned(2 downto 0);
  signal prepeat_r  : unsigned(1 downto 0);
  signal invert_s   : std_logic;
  signal vsync_s    : std_logic;

begin

  hoverflow <= '1' when hcounter>videoparam(vidmode_i).h.dtotal else '0';
  voverflow <= '1' when vcounter>videoparam(vidmode_i).v.dtotal else '0';

  -- Blanking
  hblank_s <= '1' when hcounter>=videoparam(vidmode_i).h.ddisplay else '0';
  vblank_s <= '1' when vcounter>=videoparam(vidmode_i).v.ddisplay else '0';
  blank_s <= hblank_s or vblank_s;

  -- Spectrum display
  hdisplay_s <= '1' when hcounter>=videoparam(vidmode_i).h.ddisplaystart and
                         hcounter<videoparam(vidmode_i).h.ddisplayend else '0';

  vdisplay_s <= '1' when vcounter>=videoparam(vidmode_i).v.ddisplaystart and
                         vcounter<videoparam(vidmode_i).v.ddisplayend else '0';

  display_s <= hdisplay_s and vdisplay_s;

  invert_s <= '0'; -- TBD


  -- FIFO and filler

  vf: entity work.videogen_fifo
  port map (
    clk_i         => clk_i,
    rst_i         => rst_i,
    -- Video access
    vaddr_o       => vaddr_o,
    ven_o         => ven_o,
    vbusy_i       => vbusy_i,
    vdata_i       => vdata_i,
    --vborder_i     => vborder_i,

    -- Fifo
    sync_i        => vsync_s,
    rd_i          => fifo_rd_s,
    rclk_i        => pixclk_i,
    pixel_o       => pixel_s,
    attr_o        => attr_s
  );





  -- HCounter
  process(pixclk_i, pixrst_i)
  begin
    if pixrst_i='1' then
      hcounter <= 0;
    elsif rising_edge(pixclk_i) then
      if hoverflow='0' then
        hcounter <= hcounter + 1;
      else
        hcounter <= 0;
      end if;
    end if;
  end process;

  -- VCounter
  process(pixclk_i, pixrst_i)
  begin
    if pixrst_i='1' then
      vcounter <= 0;
    elsif rising_edge(pixclk_i) then
      if hoverflow='1' then
        if voverflow='0' then
          vcounter <= vcounter + 1;
        else
          vcounter <= 0;
        end if;
      end if;
    end if;
  end process;

  -- Hsync
  process(pixclk_i)
    variable vm_v: videoconf_type;
  begin
    if rising_edge(pixclk_i) then
      vm_v := videoparam(vidmode_i);
      if hcounter>=vm_v.h.dfront and hcounter<vm_v.h.dsync then
        hsync_o <= vm_v.hsync;
      else
        hsync_o <= not vm_v.hsync;
      end if;
    end if;
  end process;

  -- Vsync
  process(pixclk_i, pixrst_i)
    variable vm_v: videoconf_type;
  begin
    if pixrst_i='1' then
      vsync_s <= '1';
    elsif rising_edge(pixclk_i) then
      vm_v := videoparam(vidmode_i);
      if vcounter>=vm_v.v.dfront and hcounter<vm_v.v.dsync then
        vsync_o <= vm_v.vsync;
        vsync_s <= '1';
      else
        vsync_o <= not vm_v.vsync;
        vsync_s <= '0';
        end if;
    end if;
  end process;

  -- Display process.

  fifo_rd_s <= '1' when display_s='1' and poffset_r="111" and prepeat_r="00" else '0';

  process(pixclk_i)
    variable pix_v: std_logic;
  begin
    if rising_edge(pixclk_i) then
      --fifo_rd_s <= '0';
      if blank_s='1' then
        poffset_r <= "000";
        if videoparam(vidmode_i).dbltriple='0' then
          prepeat_r <= "01";
        else
          prepeat_r <= "10";
        end if;
        grb_o     <= (others =>'0');
        bright_o  <= '0';
      else
        if display_s='1' then -- Either display image or border
          pix_v     := pixel_s( to_integer(poffset_r) );

          if prepeat_r="00" then
            poffset_r <= poffset_r + 1;
            if videoparam(vidmode_i).dbltriple='0' then
              prepeat_r <= "01";
            else
              prepeat_r <= "10";
            end if;
          else
            prepeat_r <= prepeat_r - 1;
          end if;
          pix_v     := pix_v xor (invert_s and attr_s(7)); -- Invert if flashing
          bright_o  <= attr_s(6); -- Bright attribute
          if pix_v='0' then
            grb_o   <= attr_s(2 downto 0);
          else
            grb_o   <= attr_s(5 downto 3);
          end if;
        else
          poffset_r <= "000";
          grb_o     <= vborder_i;
          bright_o  <= '0';
        end if;
      end if;
    end if;
  end process;

end beh;
