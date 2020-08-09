LIBRARY ieee;
USE ieee.std_logic_1164.all;

ENTITY swfifo IS
  GENERIC (
    WIDTH: natural := 8
  );
	PORT
	(
		aclr		: IN STD_LOGIC  := '0';
		data		: IN STD_LOGIC_VECTOR (WIDTH-1 DOWNTO 0);
		rdclk		: IN STD_LOGIC ;
		rdreq		: IN STD_LOGIC ;
		wrclk		: IN STD_LOGIC ;
		wrreq		: IN STD_LOGIC ;
		q		    : OUT STD_LOGIC_VECTOR (WIDTH-1 DOWNTO 0);
		rdempty		: OUT STD_LOGIC ;
		wrfull		: OUT STD_LOGIC 
	);
END swfifo;

architecture beh of swfifo is

  signal wrptr_r        : std_logic;
  signal wrptr_in_rd_s  : std_logic;
  signal rdptr_r        : std_logic;
  signal rdptr_in_wr_s  : std_logic;
  signal wdata_r        : std_logic_vector(WIDTH-1 downto 0);

begin

  process(wrclk, aclr)
  begin
    if aclr='1' then
      wrptr_r <= '0';
    elsif rising_edge(wrclk) then
      if wrreq='1' then
        wrptr_r <= not wrptr_r;
        wdata_r <= data;
      end if;
    end if;
  end process;

  wr_to_rd_sync: entity work.sync
    generic map (
      RESET => '0',
      STAGES => 3
    )
    port map (
      clk_i   => rdclk,
      arst_i  => aclr,
      din_i   => wrptr_r,
      dout_o  => wrptr_in_rd_s
    );

  rd_to_wr_sync: entity work.sync
    generic map (
      RESET => '0',
      STAGES => 3
    )
    port map (
      clk_i   => wrclk,
      arst_i  => aclr,
      din_i   => rdptr_r,
      dout_o  => rdptr_in_wr_s
    );

  data_sync: entity work.syncv
    generic map (
      RESET => '0',
      WIDTH => WIDTH
    )
    port map (
      clk_i   => rdclk,
      arst_i  => aclr,
      din_i   => wdata_r,
      dout_o  => q
    );

  process(rdclk, aclr)
  begin
    if aclr='1' then
      rdptr_r <= '0';
    elsif rising_edge(rdclk) then
      if rdreq='1' then
        rdptr_r <= not rdptr_r;
      end if;
    end if;
  end process;

  rdempty <= '1' when rdptr_r = wrptr_in_rd_s else '0';
  wrfull  <= '1' when wrptr_r /= rdptr_in_wr_s else '0';

end beh;

