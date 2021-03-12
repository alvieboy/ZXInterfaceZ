LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.std_logic_misc.all;
USE ieee.numeric_std.all;
USE ieee.numeric_bit.all;

entity usbxcvr_sim is
  port (
    DP        : inout std_logic;
    DM        : inout std_logic;
    VP_o      : out std_logic;
    VM_o      : out std_logic;
    RCV_o     : out std_logic;
    OE_i      : in std_logic;
    SOFTCON_i : in std_logic;
    SPEED_i   : in std_logic;
    VMO_i     : in std_logic;
    VPO_i     : in std_logic
);

end entity;

architecture sim of usbxcvr_sim is

  signal dp_b_s, dm_b_s : bit;
  signal rcv_b_s        : bit;
  signal idle_p, idle_m : std_logic;

begin

  dp_b_s <= to_bit(DP);
  dm_b_s <= to_bit(DM);
  rcv_b_s <= to_bit(DP and not DM);

  process(SOFTCON_i, SPEED_i)
  begin
    if SOFTCON_i='0' then
      idle_p <= 'Z';
      idle_m <= 'Z';
    else
      if SPEED_i='1' then
        idle_p <= 'H';
        idle_m <= 'L';--'Z';
        report "Using FS pullups";
      elsif SPEED_i='0' then
        idle_p <= 'L';--'Z';
        idle_m <= 'H';
        report "Using LS pullups";
      end if;
    end if;
  end process;

  DP <= VPO_i when OE_i='0' else idle_p;
  DM <= VMO_i when OE_i='0' else idle_m;

  VP_o  <= to_X01(dp_b_s);
  VM_o  <= to_X01(dm_b_s);
  RCV_o <= to_X01(rcv_b_s);

end sim;
