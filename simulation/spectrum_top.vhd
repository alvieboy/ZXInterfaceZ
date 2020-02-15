library IEEE;
use IEEE.STD_LOGIC_1164.all;


entity spectrum_top is
  port (
		RESET_n         : in  std_logic;
		CLK_n           : in  std_logic;
		INT_n           : in  std_logic;
		NMI_n           : in  std_logic;
		M1_n            : out std_logic;
		MREQ_n          : out std_logic;
		IORQ_n          : out std_logic;
		RD_n            : out std_logic;
		WR_n            : out std_logic;
		RFSH_n          : out std_logic;
		A               : out std_logic_vector(15 downto 0);
		D               : inout  std_logic_vector(7 downto 0)
  );
end entity spectrum_top;

architecture sim of spectrum_top is

  signal DI_s, DO_s: std_logic_vector(7 downto 0);
  signal WR_s: std_logic;
begin

  DI_s <= D;
  D <= DO_s when WR_s='0' else (others =>'Z');
  WR_n <= WR_s;

	cpu: entity work.T80se
    port map (
      RESET_n         => RESET_n,
      CLK_n           => CLK_n,
      CLKEN           => '1',
      WAIT_n          => '1',
      INT_n           => INT_n,
      NMI_n           => NMI_n,
      BUSRQ_n         => '1',
      M1_n            => M1_n,
      MREQ_n          => MREQ_n,
      IORQ_n          => IORQ_n,
      RD_n            => RD_n,
      WR_n            => WR_s,
      RFSH_n          => RFSH_n,
      HALT_n          => open,
      BUSAK_n         => open,
      A               => A,
      DI              => DI_s,
      DO              => DO_s
    );

	--ula: ula_port port map (
	--	clock, reset_n,
	--	cpu_do, ula_do,
	--	ula_enable, cpu_wr_n,
	--	ula_border,
	--	ula_ear_out, ula_mic_out,
	--	keyb,
	--	ula_ear_in
	--	);

	-- Address decoding.  Z80 has separate IO and memory address space
	-- IO ports (nominal addresses - incompletely decoded):
	-- 0xXXFE R/W = ULA
	-- 0x7FFD W   = 128K paging register
	-- 0xFFFD W   = 128K AY-3-8912 register select
	-- 0xFFFD R   = 128K AY-3-8912 register read
	-- 0xBFFD W   = 128K AY-3-8912 register write
	-- 0x1FFD W   = +3 paging and control register
	-- 0x2FFD R   = +3 FDC status register
	-- 0x3FFD R/W = +3 FDC data register
	-- 0xXXXF R/W = ZXMMC interface
	-- FIXME: Revisit this - could be neater
	--ula_enable <= (not cpu_ioreq_n) and not cpu_a(0); -- all even IO addresses

	-- ROM is enabled between 0x0000 and 0x3fff except in +3 special mode
	--rom_enable <= (not cpu_mreq_n) and not (plus3_special or cpu_a(15) or cpu_a(14));
	-- RAM is enabled for any memory request when ROM isn't enabled
	--ram_enable <= not (cpu_mreq_n or rom_enable);


end sim;
