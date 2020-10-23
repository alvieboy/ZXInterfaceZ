library IEEE;
use IEEE.STD_LOGIC_1164.all;


entity z80_top is
  port (
		RESET_n         : in  std_logic;
		CLK_n           : in  std_logic;
		INT_n           : in  std_logic;
		NMI_n           : in  std_logic;
    WAIT_n          : in  std_logic;
		M1_n            : out std_logic;
		MREQ_n          : out std_logic;
		IORQ_n          : out std_logic;
		RD_n            : out std_logic;
		WR_n            : out std_logic;
		RFSH_n          : out std_logic;
		A               : out std_logic_vector(15 downto 0);
		D               : inout  std_logic_vector(7 downto 0)
  );
end entity z80_top;

architecture sim of z80_top is

  signal DI_s, DO_s: std_logic_vector(7 downto 0);
  signal WR_s: std_logic;
begin

--  DI_s <= D;
--  D <= DO_s when WR_s='0' else (others =>'Z');
--  WR_n <= WR_s;

	cpu: entity work.T80a
    port map (
      RESET_n         => RESET_n,
      CLK_n           => CLK_n,
      --CLKEN           => '1',
      WAIT_n          => WAIT_n,
      INT_n           => INT_n,
      NMI_n           => NMI_n,
      BUSRQ_n         => '1',
      M1_n            => M1_n,
      MREQ_n          => MREQ_n,
      IORQ_n          => IORQ_n,
      RD_n            => RD_n,
      WR_n            => WR_n,
      RFSH_n          => RFSH_n,
      HALT_n          => open,
      BUSAK_n         => open,
      A               => A,
      --DI              => DI_s,
      --DO              => DO_s
      D               => D
    );

end sim;
