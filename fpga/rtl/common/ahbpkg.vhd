--
-- Copyright (c) 2020 Alvaro Lopes <alvieboy@alvie.com>
--
-- All rights reserved.
-- 
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions are met:
--     * Redistributions of source code must retain the above copyright
--       notice, this list of conditions and the following disclaimer.
--     * Redistributions in binary form must reproduce the above copyright
--       notice, this list of conditions and the following disclaimer in the
--       documentation and/or other materials provided with the distribution.
--     * Neither the name of the Interface Z project nor the
--       names of its contributors may be used to endorse or promote products
--       derived from this software without specific prior written permission.
-- 
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
-- ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
-- WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
-- DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
-- DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
-- (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
-- LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
-- ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
-- (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
-- SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.NUMERIC_STD.ALL;

PACKAGE ahbpkg IS

   CONSTANT C_AHB_TRANS_IDLE    : STD_LOGIC_VECTOR(1 DOWNTO 0) := "00";
   CONSTANT C_AHB_TRANS_BUSY    : STD_LOGIC_VECTOR(1 DOWNTO 0) := "01";
   CONSTANT C_AHB_TRANS_NONSEQ  : STD_LOGIC_VECTOR(1 DOWNTO 0) := "10";
   CONSTANT C_AHB_TRANS_SEQ     : STD_LOGIC_VECTOR(1 DOWNTO 0) := "11";
   
   CONSTANT C_AHB_SIZE_BYTE     : STD_LOGIC_VECTOR(2 DOWNTO 0) := "000";
   CONSTANT C_AHB_SIZE_HALFWORD : STD_LOGIC_VECTOR(2 DOWNTO 0) := "001";
   CONSTANT C_AHB_SIZE_WORD     : STD_LOGIC_VECTOR(2 DOWNTO 0) := "010";

   CONSTANT C_AHB_BURST_SINGLE  : STD_LOGIC_VECTOR(2 DOWNTO 0) := "000";
   CONSTANT C_AHB_BURST_INCR    : STD_LOGIC_VECTOR(2 DOWNTO 0) := "001";
   CONSTANT C_AHB_BURST_WRAP4   : STD_LOGIC_VECTOR(2 DOWNTO 0) := "010";
   CONSTANT C_AHB_BURST_INCR4   : STD_LOGIC_VECTOR(2 DOWNTO 0) := "011";
   CONSTANT C_AHB_BURST_WRAP8   : STD_LOGIC_VECTOR(2 DOWNTO 0) := "100";
   CONSTANT C_AHB_BURST_INCR8   : STD_LOGIC_VECTOR(2 DOWNTO 0) := "101";
   CONSTANT C_AHB_BURST_WRAP16  : STD_LOGIC_VECTOR(2 DOWNTO 0) := "110";
   CONSTANT C_AHB_BURST_INCR16  : STD_LOGIC_VECTOR(2 DOWNTO 0) := "111";

   TYPE AHB_M2S IS RECORD
      HADDR    : STD_LOGIC_VECTOR(31 DOWNTO 0);
      HBURST   : STD_LOGIC_VECTOR(2 DOWNTO 0);
      HMASTLOCK: STD_LOGIC;
      HPROT    : STD_LOGIC_VECTOR(3 DOWNTO 0);
      HSIZE    : STD_LOGIC_VECTOR(2 DOWNTO 0);
      HTRANS   : STD_LOGIC_VECTOR(1 DOWNTO 0);
      HWDATA   : STD_LOGIC_VECTOR(31 DOWNTO 0);
      HWRITE   : STD_LOGIC;
   END RECORD;

   TYPE AHB_S2M IS RECORD
      HRDATA   : STD_LOGIC_VECTOR(31 DOWNTO 0);
      HREADY   : STD_LOGIC;
      HRESP    : STD_LOGIC;
   END RECORD;

  CONSTANT C_AHB_NULL_M2S: AHB_M2S := (
    (others => 'X'),
    "000",
    '0',
    "0000",
    C_AHB_SIZE_BYTE,
    C_AHB_TRANS_IDLE,
    (others => 'X'),
    '0'
  );

END PACKAGE ahbpkg;
