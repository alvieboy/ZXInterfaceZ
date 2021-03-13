#include <inttypes.h>
const char *disassemble_ops[] = {
"NOP"
,"LD"
,"INC"
,"DEC"
,"RLCA"
,"EX"
,"ADD"
,"RRCA"
,"DJNZ"
,"RLA"
,"JR"
,"RRA"
,"DAA"
,"CPL"
,"SCF"
,"CCF"
,"HALT"
,"ADC"
,"SUB"
,"SBC"
,"AND"
,"XOR"
,"OR"
,"CP"
,"RET"
,"POP"
,"JP"
,"CALL"
,"PUSH"
,"RST"
,"OUT"
,"EXX"
,"IN"
,"DI"
,"EI"
,"RLC"
,"RRC"
,"RL"
,"RR"
,"SLA"
,"SRA"
,"SLL"
,"SRL"
,"BIT"
,"RES"
,"SET"
,"NEG"
,"RETN"
,"IM"
,"RETI"
,"RRD"
,"RLD"
,"LDI"
,"CPI"
,"INI"
,"OUTI"
,"LDD"
,"CPD"
,"IND"
,"OUTD"
,"LDIR"
,"CPIR"
,"INIR"
,"OTIR"
,"LDDR"
,"CPDR"
,"INDR"
,"OTDR"
};
const char *disassemble_args[] = {
""
,"BC,w"
,"(BC),A"
,"BC"
,"B"
,"B,b"
,"AF,AF'"
,"z,BC"
,"A,(BC)"
,"C"
,"C,b"
,"d"
,"DE,w"
,"(DE),A"
,"DE"
,"D"
,"D,b"
,"z,DE"
,"A,(DE)"
,"E"
,"E,b"
,"NZ,d"
,"z,w"
,"(w),z"
,"z"
,"h"
,"h,b"
,"Z,d"
,"z,hl"
,"z,(w)"
,"l"
,"l,b"
,"NC,d"
,"SP,w"
,"(w),A"
,"SP"
,"(x)"
,"(x),b"
,"C,d"
,"z,SP"
,"A,(w)"
,"A"
,"A,b"
,"B,B"
,"B,C"
,"B,D"
,"B,E"
,"B,h"
,"B,l"
,"B,(x)"
,"B,A"
,"C,B"
,"C,C"
,"C,D"
,"C,E"
,"C,h"
,"C,l"
,"C,(x)"
,"C,A"
,"D,B"
,"D,C"
,"D,D"
,"D,E"
,"D,h"
,"D,l"
,"D,(x)"
,"D,A"
,"E,B"
,"E,C"
,"E,D"
,"E,E"
,"E,h"
,"E,l"
,"E,(x)"
,"E,A"
,"h,B"
,"h,C"
,"h,D"
,"h,E"
,"h,h"
,"h,l"
,"H,(x)"
,"h,A"
,"l,B"
,"l,C"
,"l,D"
,"l,E"
,"l,h"
,"l,l"
,"L,(x)"
,"l,A"
,"(x),B"
,"(x),C"
,"(x),D"
,"(x),E"
,"(x),H"
,"(x),L"
,"(x),A"
,"A,B"
,"A,C"
,"A,D"
,"A,E"
,"A,h"
,"A,l"
,"A,(x)"
,"A,A"
,"NZ"
,"NZ,w"
,"w"
,"00"
,"Z"
,"Z,w"
,"08"
,"NC"
,"NC,w"
,"(b),A"
,"b"
,"10"
,"C,w"
,"A,(b)"
,"18"
,"PO"
,"PO,w"
,"(SP),z"
,"20"
,"PE"
,"(z)"
,"PE,w"
,"DE,z"
,"28"
,"P"
,"AF"
,"P,w"
,"30"
,"M"
,"SP,z"
,"M,w"
,"38"
,"0,B"
,"0,C"
,"0,D"
,"0,E"
,"0,h"
,"0,l"
,"0,(x)"
,"0,A"
,"1,B"
,"1,C"
,"1,D"
,"1,E"
,"1,h"
,"1,l"
,"1,(x)"
,"1,A"
,"2,B"
,"2,C"
,"2,D"
,"2,E"
,"2,h"
,"2,l"
,"2,(x)"
,"2,A"
,"3,B"
,"3,C"
,"3,D"
,"3,E"
,"3,h"
,"3,l"
,"3,(x)"
,"3,A"
,"4,B"
,"4,C"
,"4,D"
,"4,E"
,"4,h"
,"4,l"
,"4,(x)"
,"4,A"
,"5,B"
,"5,C"
,"5,D"
,"5,E"
,"5,h"
,"5,l"
,"5,(x)"
,"5,A"
,"6,B"
,"6,C"
,"6,D"
,"6,E"
,"6,h"
,"6,l"
,"6,(x)"
,"6,A"
,"7,B"
,"7,C"
,"7,D"
,"7,E"
,"7,h"
,"7,l"
,"7,(x)"
,"7,A"
,"B,(C)"
,"(C),B"
,"(w),BC"
,"0"
,"I,A"
,"C,(C)"
,"(C),C"
,"BC,(w)"
,"R,A"
,"D,(C)"
,"(C),D"
,"(w),DE"
,"1"
,"A,I"
,"E,(C)"
,"(C),E"
,"DE,(w)"
,"2"
,"A,R"
,"h,(C)"
,"(C),h"
,"l,(C)"
,"(C),l"
,"(w),SP"
,"A,(C)"
,"(C),A"
,"SP,(w)"
};
const uint16_t disassemble_tables[3][256]={
{
0x0000, /* 0 00	NOP */
0x0101, /* 1 01	LD      BC,w */
0x0102, /* 2 02	LD      (BC),A */
0x0203, /* 3 03	INC     BC */
0x0204, /* 4 04	INC     B */
0x0304, /* 5 05	DEC     B */
0x0105, /* 6 06	LD      B,b */
0x0400, /* 7 07	RLCA */
0x0506, /* 8 08	EX      AF,AF' */
0x0607, /* 9 09	ADD     z,BC */
0x0108, /* 10 0A	LD      A,(BC) */
0x0303, /* 11 0B	DEC     BC */
0x0209, /* 12 0C	INC     C */
0x0309, /* 13 0D	DEC     C */
0x010a, /* 14 0E	LD      C,b */
0x0700, /* 15 0F	RRCA */
0x080b, /* 16 10	DJNZ    d */
0x010c, /* 17 11	LD      DE,w */
0x010d, /* 18 12	LD      (DE),A */
0x020e, /* 19 13	INC     DE */
0x020f, /* 20 14	INC     D */
0x030f, /* 21 15	DEC     D */
0x0110, /* 22 16	LD      D,b */
0x0900, /* 23 17	RLA */
0x0a0b, /* 24 18	JR      d */
0x0611, /* 25 19	ADD     z,DE */
0x0112, /* 26 1A	LD      A,(DE) */
0x030e, /* 27 1B	DEC     DE */
0x0213, /* 28 1C	INC     E */
0x0313, /* 29 1D	DEC     E */
0x0114, /* 30 1E	LD      E,b */
0x0b00, /* 31 1F	RRA */
0x0a15, /* 32 20	JR      NZ,d */
0x0116, /* 33 21	LD      z,w */
0x0117, /* 34 22	LD      (w),z */
0x0218, /* 35 23	INC     z */
0x0219, /* 36 24	INC     h */
0x0319, /* 37 25	DEC     h */
0x011a, /* 38 26	LD      h,b */
0x0c00, /* 39 27	DAA */
0x0a1b, /* 40 28	JR      Z,d */
0x061c, /* 41 29	ADD     z,hl */
0x011d, /* 42 2A	LD      z,(w) */
0x0318, /* 43 2B	DEC     z */
0x021e, /* 44 2C	INC     l */
0x031e, /* 45 2D	DEC     l */
0x011f, /* 46 2E	LD      l,b */
0x0d00, /* 47 2F	CPL */
0x0a20, /* 48 30	JR      NC,d */
0x0121, /* 49 31	LD      SP,w */
0x0122, /* 50 32	LD      (w),A */
0x0223, /* 51 33	INC     SP */
0x0224, /* 52 34	INC     (x) */
0x0324, /* 53 35	DEC     (x) */
0x0125, /* 54 36	LD      (x),b */
0x0e00, /* 55 37	SCF */
0x0a26, /* 56 38	JR      C,d */
0x0627, /* 57 39	ADD     z,SP */
0x0128, /* 58 3A	LD      A,(w) */
0x0323, /* 59 3B	DEC     SP */
0x0229, /* 60 3C	INC     A */
0x0329, /* 61 3D	DEC     A */
0x012a, /* 62 3E	LD      A,b */
0x0f00, /* 63 3F	CCF */
0x012b, /* 64 40	LD      B,B */
0x012c, /* 65 41	LD      B,C */
0x012d, /* 66 42	LD      B,D */
0x012e, /* 67 43	LD      B,E */
0x012f, /* 68 44	LD      B,h */
0x0130, /* 69 45	LD      B,l */
0x0131, /* 70 46	LD      B,(x) */
0x0132, /* 71 47	LD      B,A */
0x0133, /* 72 48	LD      C,B */
0x0134, /* 73 49	LD      C,C */
0x0135, /* 74 4A	LD      C,D */
0x0136, /* 75 4B	LD      C,E */
0x0137, /* 76 4C	LD      C,h */
0x0138, /* 77 4D	LD      C,l */
0x0139, /* 78 4E	LD      C,(x) */
0x013a, /* 79 4F	LD      C,A */
0x013b, /* 80 50	LD      D,B */
0x013c, /* 81 51	LD      D,C */
0x013d, /* 82 52	LD      D,D */
0x013e, /* 83 53	LD      D,E */
0x013f, /* 84 54	LD      D,h */
0x0140, /* 85 55	LD      D,l */
0x0141, /* 86 56	LD      D,(x) */
0x0142, /* 87 57	LD      D,A */
0x0143, /* 88 58	LD      E,B */
0x0144, /* 89 59	LD      E,C */
0x0145, /* 90 5A	LD      E,D */
0x0146, /* 91 5B	LD      E,E */
0x0147, /* 92 5C	LD      E,h */
0x0148, /* 93 5D	LD      E,l */
0x0149, /* 94 5E	LD      E,(x) */
0x014a, /* 95 5F	LD      E,A */
0x014b, /* 96 60	LD      h,B */
0x014c, /* 97 61	LD      h,C */
0x014d, /* 98 62	LD      h,D */
0x014e, /* 99 63	LD      h,E */
0x014f, /* 100 64	LD      h,h */
0x0150, /* 101 65	LD      h,l */
0x0151, /* 102 66	LD      H,(x) */
0x0152, /* 103 67	LD      h,A */
0x0153, /* 104 68	LD      l,B */
0x0154, /* 105 69	LD      l,C */
0x0155, /* 106 6A	LD      l,D */
0x0156, /* 107 6B	LD      l,E */
0x0157, /* 108 6C	LD      l,h */
0x0158, /* 109 6D	LD      l,l */
0x0159, /* 110 6E	LD      L,(x) */
0x015a, /* 111 6F	LD      l,A */
0x015b, /* 112 70	LD      (x),B */
0x015c, /* 113 71	LD      (x),C */
0x015d, /* 114 72	LD      (x),D */
0x015e, /* 115 73	LD      (x),E */
0x015f, /* 116 74	LD      (x),H */
0x0160, /* 117 75	LD      (x),L */
0x1000, /* 118 76	HALT */
0x0161, /* 119 77	LD      (x),A */
0x0162, /* 120 78	LD      A,B */
0x0163, /* 121 79	LD      A,C */
0x0164, /* 122 7A	LD      A,D */
0x0165, /* 123 7B	LD      A,E */
0x0166, /* 124 7C	LD      A,h */
0x0167, /* 125 7D	LD      A,l */
0x0168, /* 126 7E	LD      A,(x) */
0x0169, /* 127 7F	LD      A,A */
0x0662, /* 128 80	ADD     A,B */
0x0663, /* 129 81	ADD     A,C */
0x0664, /* 130 82	ADD     A,D */
0x0665, /* 131 83	ADD     A,E */
0x0666, /* 132 84	ADD     A,h */
0x0667, /* 133 85	ADD     A,l */
0x0668, /* 134 86	ADD     A,(x) */
0x0669, /* 135 87	ADD     A,A */
0x1162, /* 136 88	ADC     A,B */
0x1163, /* 137 89	ADC     A,C */
0x1164, /* 138 8A	ADC     A,D */
0x1165, /* 139 8B	ADC     A,E */
0x1166, /* 140 8C	ADC     A,h */
0x1167, /* 141 8D	ADC     A,l */
0x1168, /* 142 8E	ADC     A,(x) */
0x1169, /* 143 8F	ADC     A,A */
0x1204, /* 144 90	SUB     B */
0x1209, /* 145 91	SUB     C */
0x120f, /* 146 92	SUB     D */
0x1213, /* 147 93	SUB     E */
0x1219, /* 148 94	SUB     h */
0x121e, /* 149 95	SUB     l */
0x1224, /* 150 96	SUB     (x) */
0x1229, /* 151 97	SUB     A */
0x1362, /* 152 98	SBC     A,B */
0x1363, /* 153 99	SBC     A,C */
0x1364, /* 154 9A	SBC     A,D */
0x1365, /* 155 9B	SBC     A,E */
0x1366, /* 156 9C	SBC     A,h */
0x1367, /* 157 9D	SBC     A,l */
0x1368, /* 158 9E	SBC     A,(x) */
0x1369, /* 159 9F	SBC     A,A */
0x1404, /* 160 A0	AND     B */
0x1409, /* 161 A1	AND     C */
0x140f, /* 162 A2	AND     D */
0x1413, /* 163 A3	AND     E */
0x1419, /* 164 A4	AND     h */
0x141e, /* 165 A5	AND     l */
0x1424, /* 166 A6	AND     (x) */
0x1429, /* 167 A7	AND     A */
0x1504, /* 168 A8	XOR     B */
0x1509, /* 169 A9	XOR     C */
0x150f, /* 170 AA	XOR     D */
0x1513, /* 171 AB	XOR     E */
0x1519, /* 172 AC	XOR     h */
0x151e, /* 173 AD	XOR     l */
0x1524, /* 174 AE	XOR     (x) */
0x1529, /* 175 AF	XOR     A */
0x1604, /* 176 B0	OR      B */
0x1609, /* 177 B1	OR      C */
0x160f, /* 178 B2	OR      D */
0x1613, /* 179 B3	OR      E */
0x1619, /* 180 B4	OR      h */
0x161e, /* 181 B5	OR      l */
0x1624, /* 182 B6	OR      (x) */
0x1629, /* 183 B7	OR      A */
0x1704, /* 184 B8	CP      B */
0x1709, /* 185 B9	CP      C */
0x170f, /* 186 BA	CP      D */
0x1713, /* 187 BB	CP      E */
0x1719, /* 188 BC	CP      h */
0x171e, /* 189 BD	CP      l */
0x1724, /* 190 BE	CP      (x) */
0x1729, /* 191 BF	CP      A */
0x186a, /* 192 C0	RET     NZ */
0x1903, /* 193 C1	POP     BC */
0x1a6b, /* 194 C2	JP      NZ,w */
0x1a6c, /* 195 C3	JP      w */
0x1b6b, /* 196 C4	CALL    NZ,w */
0x1c03, /* 197 C5	PUSH    BC */
0x062a, /* 198 C6	ADD     A,b */
0x1d6d, /* 199 C7	RST     00 */
0x186e, /* 200 C8	RET     Z */
0x1800, /* 201 C9	RET */
0x1a6f, /* 202 CA	JP      Z,w */
0xFF01, /* 203 - Extended table 1 */
0x1b6f, /* 204 CC	CALL    Z,w */
0x1b6c, /* 205 CD	CALL    w */
0x112a, /* 206 CE	ADC     A,b */
0x1d70, /* 207 CF	RST     08 */
0x1871, /* 208 D0	RET     NC */
0x190e, /* 209 D1	POP     DE */
0x1a72, /* 210 D2	JP      NC,w */
0x1e73, /* 211 D3	OUT     (b),A */
0x1b72, /* 212 D4	CALL    NC,w */
0x1c0e, /* 213 D5	PUSH    DE */
0x1274, /* 214 D6	SUB     b */
0x1d75, /* 215 D7	RST     10 */
0x1809, /* 216 D8	RET     C */
0x1f00, /* 217 D9	EXX */
0x1a76, /* 218 DA	JP      C,w */
0x2077, /* 219 DB	IN A,(b) */
0x1b76, /* 220 DC	CALL    C,w */
0xFE01, /* 221 - IY */
0x132a, /* 222 DE	SBC     A,b */
0x1d78, /* 223 DF	RST     18 */
0x1879, /* 224 E0	RET     PO */
0x1918, /* 225 E1	POP     z */
0x1a7a, /* 226 E2	JP      PO,w */
0x057b, /* 227 E3	EX      (SP),z */
0x1b7a, /* 228 E4	CALL    PO,w */
0x1c18, /* 229 E5	PUSH    z */
0x1474, /* 230 E6	AND     b */
0x1d7c, /* 231 E7	RST     20 */
0x187d, /* 232 E8	RET     PE */
0x1a7e, /* 233 E9	JP      (z) */
0x1a7f, /* 234 EA	JP      PE,w */
0x0580, /* 235 EB	EX      DE,z */
0x1b7f, /* 236 EC	CALL    PE,w */
0xFF02, /* 237 - Extended table 2 */
0x1574, /* 238 EE	XOR     b */
0x1d81, /* 239 EF	RST     28 */
0x1882, /* 240 F0	RET     P */
0x1983, /* 241 F1	POP     AF */
0x1a84, /* 242 F2	JP      P,w */
0x2100, /* 243 F3	DI */
0x1b84, /* 244 F4	CALL    P,w */
0x1c83, /* 245 F5	PUSH    AF */
0x1674, /* 246 F6	OR      b */
0x1d85, /* 247 F7	RST     30 */
0x1886, /* 248 F8	RET     M */
0x0187, /* 249 F9	LD      SP,z */
0x1a88, /* 250 FA	JP      M,w */
0x2200, /* 251 FB	EI */
0x1b88, /* 252 FC	CALL    M,w */
0xFE02, /* 253 - IX */
0x1774, /* 254 FE	CP      b */
0x1d89, /* 255 FF	RST     38 */
},
{
0x2304, /* 0 */
0x2309, /* 1 */
0x230f, /* 2 */
0x2313, /* 3 */
0x2319, /* 4 */
0x231e, /* 5 */
0x2324, /* 6 */
0x2329, /* 7 */
0x2404, /* 8 */
0x2409, /* 9 */
0x240f, /* 10 */
0x2413, /* 11 */
0x2419, /* 12 */
0x241e, /* 13 */
0x2424, /* 14 */
0x2429, /* 15 */
0x2504, /* 16 */
0x2509, /* 17 */
0x250f, /* 18 */
0x2513, /* 19 */
0x2519, /* 20 */
0x251e, /* 21 */
0x2524, /* 22 */
0x2529, /* 23 */
0x2604, /* 24 */
0x2609, /* 25 */
0x260f, /* 26 */
0x2613, /* 27 */
0x2619, /* 28 */
0x261e, /* 29 */
0x2624, /* 30 */
0x2629, /* 31 */
0x2704, /* 32 */
0x2709, /* 33 */
0x270f, /* 34 */
0x2713, /* 35 */
0x2719, /* 36 */
0x271e, /* 37 */
0x2724, /* 38 */
0x2729, /* 39 */
0x2804, /* 40 */
0x2809, /* 41 */
0x280f, /* 42 */
0x2813, /* 43 */
0x2819, /* 44 */
0x281e, /* 45 */
0x2824, /* 46 */
0x2829, /* 47 */
0x2904, /* 48 */
0x2909, /* 49 */
0x290f, /* 50 */
0x2913, /* 51 */
0x2919, /* 52 */
0x291e, /* 53 */
0x2924, /* 54 */
0x2929, /* 55 */
0x2a04, /* 56 */
0x2a09, /* 57 */
0x2a0f, /* 58 */
0x2a13, /* 59 */
0x2a19, /* 60 */
0x2a1e, /* 61 */
0x2a24, /* 62 */
0x2a29, /* 63 */
0x2b8a, /* 64 */
0x2b8b, /* 65 */
0x2b8c, /* 66 */
0x2b8d, /* 67 */
0x2b8e, /* 68 */
0x2b8f, /* 69 */
0x2b90, /* 70 */
0x2b91, /* 71 */
0x2b92, /* 72 */
0x2b93, /* 73 */
0x2b94, /* 74 */
0x2b95, /* 75 */
0x2b96, /* 76 */
0x2b97, /* 77 */
0x2b98, /* 78 */
0x2b99, /* 79 */
0x2b9a, /* 80 */
0x2b9b, /* 81 */
0x2b9c, /* 82 */
0x2b9d, /* 83 */
0x2b9e, /* 84 */
0x2b9f, /* 85 */
0x2ba0, /* 86 */
0x2ba1, /* 87 */
0x2ba2, /* 88 */
0x2ba3, /* 89 */
0x2ba4, /* 90 */
0x2ba5, /* 91 */
0x2ba6, /* 92 */
0x2ba7, /* 93 */
0x2ba8, /* 94 */
0x2ba9, /* 95 */
0x2baa, /* 96 */
0x2bab, /* 97 */
0x2bac, /* 98 */
0x2bad, /* 99 */
0x2bae, /* 100 */
0x2baf, /* 101 */
0x2bb0, /* 102 */
0x2bb1, /* 103 */
0x2bb2, /* 104 */
0x2bb3, /* 105 */
0x2bb4, /* 106 */
0x2bb5, /* 107 */
0x2bb6, /* 108 */
0x2bb7, /* 109 */
0x2bb8, /* 110 */
0x2bb9, /* 111 */
0x2bba, /* 112 */
0x2bbb, /* 113 */
0x2bbc, /* 114 */
0x2bbd, /* 115 */
0x2bbe, /* 116 */
0x2bbf, /* 117 */
0x2bc0, /* 118 */
0x2bc1, /* 119 */
0x2bc2, /* 120 */
0x2bc3, /* 121 */
0x2bc4, /* 122 */
0x2bc5, /* 123 */
0x2bc6, /* 124 */
0x2bc7, /* 125 */
0x2bc8, /* 126 */
0x2bc9, /* 127 */
0x2c8a, /* 128 */
0x2c8b, /* 129 */
0x2c8c, /* 130 */
0x2c8d, /* 131 */
0x2c8e, /* 132 */
0x2c8f, /* 133 */
0x2c90, /* 134 */
0x2c91, /* 135 */
0x2c92, /* 136 */
0x2c93, /* 137 */
0x2c94, /* 138 */
0x2c95, /* 139 */
0x2c96, /* 140 */
0x2c97, /* 141 */
0x2c98, /* 142 */
0x2c99, /* 143 */
0x2c9a, /* 144 */
0x2c9b, /* 145 */
0x2c9c, /* 146 */
0x2c9d, /* 147 */
0x2c9e, /* 148 */
0x2c9f, /* 149 */
0x2ca0, /* 150 */
0x2ca1, /* 151 */
0x2ca2, /* 152 */
0x2ca3, /* 153 */
0x2ca4, /* 154 */
0x2ca5, /* 155 */
0x2ca6, /* 156 */
0x2ca7, /* 157 */
0x2ca8, /* 158 */
0x2ca9, /* 159 */
0x2caa, /* 160 */
0x2cab, /* 161 */
0x2cac, /* 162 */
0x2cad, /* 163 */
0x2cae, /* 164 */
0x2caf, /* 165 */
0x2cb0, /* 166 */
0x2cb1, /* 167 */
0x2cb2, /* 168 */
0x2cb3, /* 169 */
0x2cb4, /* 170 */
0x2cb5, /* 171 */
0x2cb6, /* 172 */
0x2cb7, /* 173 */
0x2cb8, /* 174 */
0x2cb9, /* 175 */
0x2cba, /* 176 */
0x2cbb, /* 177 */
0x2cbc, /* 178 */
0x2cbd, /* 179 */
0x2cbe, /* 180 */
0x2cbf, /* 181 */
0x2cc0, /* 182 */
0x2cc1, /* 183 */
0x2cc2, /* 184 */
0x2cc3, /* 185 */
0x2cc4, /* 186 */
0x2cc5, /* 187 */
0x2cc6, /* 188 */
0x2cc7, /* 189 */
0x2cc8, /* 190 */
0x2cc9, /* 191 */
0x2d8a, /* 192 */
0x2d8b, /* 193 */
0x2d8c, /* 194 */
0x2d8d, /* 195 */
0x2d8e, /* 196 */
0x2d8f, /* 197 */
0x2d90, /* 198 */
0x2d91, /* 199 */
0x2d92, /* 200 */
0x2d93, /* 201 */
0x2d94, /* 202 */
0x2d95, /* 203 */
0x2d96, /* 204 */
0x2d97, /* 205 */
0x2d98, /* 206 */
0x2d99, /* 207 */
0x2d9a, /* 208 */
0x2d9b, /* 209 */
0x2d9c, /* 210 */
0x2d9d, /* 211 */
0x2d9e, /* 212 */
0x2d9f, /* 213 */
0x2da0, /* 214 */
0x2da1, /* 215 */
0x2da2, /* 216 */
0x2da3, /* 217 */
0x2da4, /* 218 */
0x2da5, /* 219 */
0x2da6, /* 220 */
0x2da7, /* 221 */
0x2da8, /* 222 */
0x2da9, /* 223 */
0x2daa, /* 224 */
0x2dab, /* 225 */
0x2dac, /* 226 */
0x2dad, /* 227 */
0x2dae, /* 228 */
0x2daf, /* 229 */
0x2db0, /* 230 */
0x2db1, /* 231 */
0x2db2, /* 232 */
0x2db3, /* 233 */
0x2db4, /* 234 */
0x2db5, /* 235 */
0x2db6, /* 236 */
0x2db7, /* 237 */
0x2db8, /* 238 */
0x2db9, /* 239 */
0x2dba, /* 240 */
0x2dbb, /* 241 */
0x2dbc, /* 242 */
0x2dbd, /* 243 */
0x2dbe, /* 244 */
0x2dbf, /* 245 */
0x2dc0, /* 246 */
0x2dc1, /* 247 */
0x2dc2, /* 248 */
0x2dc3, /* 249 */
0x2dc4, /* 250 */
0x2dc5, /* 251 */
0x2dc6, /* 252 */
0x2dc7, /* 253 */
0x2dc8, /* 254 */
0x2dc9, /* 255 */
},
{
0xFFFF, /* 0 - undefined*/
0xFFFF, /* 1 - undefined*/
0xFFFF, /* 2 - undefined*/
0xFFFF, /* 3 - undefined*/
0xFFFF, /* 4 - undefined*/
0xFFFF, /* 5 - undefined*/
0xFFFF, /* 6 - undefined*/
0xFFFF, /* 7 - undefined*/
0xFFFF, /* 8 - undefined*/
0xFFFF, /* 9 - undefined*/
0xFFFF, /* 10 - undefined*/
0xFFFF, /* 11 - undefined*/
0xFFFF, /* 12 - undefined*/
0xFFFF, /* 13 - undefined*/
0xFFFF, /* 14 - undefined*/
0xFFFF, /* 15 - undefined*/
0xFFFF, /* 16 - undefined*/
0xFFFF, /* 17 - undefined*/
0xFFFF, /* 18 - undefined*/
0xFFFF, /* 19 - undefined*/
0xFFFF, /* 20 - undefined*/
0xFFFF, /* 21 - undefined*/
0xFFFF, /* 22 - undefined*/
0xFFFF, /* 23 - undefined*/
0xFFFF, /* 24 - undefined*/
0xFFFF, /* 25 - undefined*/
0xFFFF, /* 26 - undefined*/
0xFFFF, /* 27 - undefined*/
0xFFFF, /* 28 - undefined*/
0xFFFF, /* 29 - undefined*/
0xFFFF, /* 30 - undefined*/
0xFFFF, /* 31 - undefined*/
0xFFFF, /* 32 - undefined*/
0xFFFF, /* 33 - undefined*/
0xFFFF, /* 34 - undefined*/
0xFFFF, /* 35 - undefined*/
0xFFFF, /* 36 - undefined*/
0xFFFF, /* 37 - undefined*/
0xFFFF, /* 38 - undefined*/
0xFFFF, /* 39 - undefined*/
0xFFFF, /* 40 - undefined*/
0xFFFF, /* 41 - undefined*/
0xFFFF, /* 42 - undefined*/
0xFFFF, /* 43 - undefined*/
0xFFFF, /* 44 - undefined*/
0xFFFF, /* 45 - undefined*/
0xFFFF, /* 46 - undefined*/
0xFFFF, /* 47 - undefined*/
0xFFFF, /* 48 - undefined*/
0xFFFF, /* 49 - undefined*/
0xFFFF, /* 50 - undefined*/
0xFFFF, /* 51 - undefined*/
0xFFFF, /* 52 - undefined*/
0xFFFF, /* 53 - undefined*/
0xFFFF, /* 54 - undefined*/
0xFFFF, /* 55 - undefined*/
0xFFFF, /* 56 - undefined*/
0xFFFF, /* 57 - undefined*/
0xFFFF, /* 58 - undefined*/
0xFFFF, /* 59 - undefined*/
0xFFFF, /* 60 - undefined*/
0xFFFF, /* 61 - undefined*/
0xFFFF, /* 62 - undefined*/
0xFFFF, /* 63 - undefined*/
0x20ca, /* 64 */
0x1ecb, /* 65 */
0x1307, /* 66 */
0x01cc, /* 67 */
0x2e00, /* 68 */
0x2f00, /* 69 */
0x30cd, /* 70 */
0x01ce, /* 71 */
0x20cf, /* 72 */
0x1ed0, /* 73 */
0x1107, /* 74 */
0x01d1, /* 75 */
0xFFFF, /* 76 - undefined*/
0x3100, /* 77 */
0xFFFF, /* 78 - undefined*/
0x01d2, /* 79 */
0x20d3, /* 80 */
0x1ed4, /* 81 */
0x1311, /* 82 */
0x01d5, /* 83 */
0xFFFF, /* 84 - undefined*/
0xFFFF, /* 85 - undefined*/
0x30d6, /* 86 */
0x01d7, /* 87 */
0x20d8, /* 88 */
0x1ed9, /* 89 */
0x1111, /* 90 */
0x01da, /* 91 */
0xFFFF, /* 92 - undefined*/
0xFFFF, /* 93 - undefined*/
0x30db, /* 94 */
0x01dc, /* 95 */
0x20dd, /* 96 */
0x1ede, /* 97 */
0x131c, /* 98 */
0x0117, /* 99 */
0xFFFF, /* 100 - undefined*/
0xFFFF, /* 101 - undefined*/
0xFFFF, /* 102 - undefined*/
0x3200, /* 103 */
0x20df, /* 104 */
0x1ee0, /* 105 */
0x111c, /* 106 */
0x011d, /* 107 */
0xFFFF, /* 108 - undefined*/
0xFFFF, /* 109 - undefined*/
0xFFFF, /* 110 - undefined*/
0x3300, /* 111 */
0xFFFF, /* 112 - undefined*/
0xFFFF, /* 113 - undefined*/
0x1327, /* 114 */
0x01e1, /* 115 */
0xFFFF, /* 116 - undefined*/
0xFFFF, /* 117 - undefined*/
0xFFFF, /* 118 - undefined*/
0xFFFF, /* 119 - undefined*/
0x20e2, /* 120 */
0x1ee3, /* 121 */
0x1127, /* 122 */
0x01e4, /* 123 */
0xFFFF, /* 124 - undefined*/
0xFFFF, /* 125 - undefined*/
0xFFFF, /* 126 - undefined*/
0xFFFF, /* 127 - undefined*/
0xFFFF, /* 128 - undefined*/
0xFFFF, /* 129 - undefined*/
0xFFFF, /* 130 - undefined*/
0xFFFF, /* 131 - undefined*/
0xFFFF, /* 132 - undefined*/
0xFFFF, /* 133 - undefined*/
0xFFFF, /* 134 - undefined*/
0xFFFF, /* 135 - undefined*/
0xFFFF, /* 136 - undefined*/
0xFFFF, /* 137 - undefined*/
0xFFFF, /* 138 - undefined*/
0xFFFF, /* 139 - undefined*/
0xFFFF, /* 140 - undefined*/
0xFFFF, /* 141 - undefined*/
0xFFFF, /* 142 - undefined*/
0xFFFF, /* 143 - undefined*/
0xFFFF, /* 144 - undefined*/
0xFFFF, /* 145 - undefined*/
0xFFFF, /* 146 - undefined*/
0xFFFF, /* 147 - undefined*/
0xFFFF, /* 148 - undefined*/
0xFFFF, /* 149 - undefined*/
0xFFFF, /* 150 - undefined*/
0xFFFF, /* 151 - undefined*/
0xFFFF, /* 152 - undefined*/
0xFFFF, /* 153 - undefined*/
0xFFFF, /* 154 - undefined*/
0xFFFF, /* 155 - undefined*/
0xFFFF, /* 156 - undefined*/
0xFFFF, /* 157 - undefined*/
0xFFFF, /* 158 - undefined*/
0xFFFF, /* 159 - undefined*/
0x3400, /* 160 */
0x3500, /* 161 */
0x3600, /* 162 */
0x3700, /* 163 */
0xFFFF, /* 164 - undefined*/
0xFFFF, /* 165 - undefined*/
0xFFFF, /* 166 - undefined*/
0xFFFF, /* 167 - undefined*/
0x3800, /* 168 */
0x3900, /* 169 */
0x3a00, /* 170 */
0x3b00, /* 171 */
0xFFFF, /* 172 - undefined*/
0xFFFF, /* 173 - undefined*/
0xFFFF, /* 174 - undefined*/
0xFFFF, /* 175 - undefined*/
0x3c00, /* 176 */
0x3d00, /* 177 */
0x3e00, /* 178 */
0x3f00, /* 179 */
0xFFFF, /* 180 - undefined*/
0xFFFF, /* 181 - undefined*/
0xFFFF, /* 182 - undefined*/
0xFFFF, /* 183 - undefined*/
0x4000, /* 184 */
0x4100, /* 185 */
0x4200, /* 186 */
0x4300, /* 187 */
0xFFFF, /* 188 - undefined*/
0xFFFF, /* 189 - undefined*/
0xFFFF, /* 190 - undefined*/
0xFFFF, /* 191 - undefined*/
0xFFFF, /* 192 - undefined*/
0xFFFF, /* 193 - undefined*/
0xFFFF, /* 194 - undefined*/
0xFFFF, /* 195 - undefined*/
0xFFFF, /* 196 - undefined*/
0xFFFF, /* 197 - undefined*/
0xFFFF, /* 198 - undefined*/
0xFFFF, /* 199 - undefined*/
0xFFFF, /* 200 - undefined*/
0xFFFF, /* 201 - undefined*/
0xFFFF, /* 202 - undefined*/
0xFFFF, /* 203 - undefined*/
0xFFFF, /* 204 - undefined*/
0xFFFF, /* 205 - undefined*/
0xFFFF, /* 206 - undefined*/
0xFFFF, /* 207 - undefined*/
0xFFFF, /* 208 - undefined*/
0xFFFF, /* 209 - undefined*/
0xFFFF, /* 210 - undefined*/
0xFFFF, /* 211 - undefined*/
0xFFFF, /* 212 - undefined*/
0xFFFF, /* 213 - undefined*/
0xFFFF, /* 214 - undefined*/
0xFFFF, /* 215 - undefined*/
0xFFFF, /* 216 - undefined*/
0xFFFF, /* 217 - undefined*/
0xFFFF, /* 218 - undefined*/
0xFFFF, /* 219 - undefined*/
0xFFFF, /* 220 - undefined*/
0xFFFF, /* 221 - undefined*/
0xFFFF, /* 222 - undefined*/
0xFFFF, /* 223 - undefined*/
0xFFFF, /* 224 - undefined*/
0xFFFF, /* 225 - undefined*/
0xFFFF, /* 226 - undefined*/
0xFFFF, /* 227 - undefined*/
0xFFFF, /* 228 - undefined*/
0xFFFF, /* 229 - undefined*/
0xFFFF, /* 230 - undefined*/
0xFFFF, /* 231 - undefined*/
0xFFFF, /* 232 - undefined*/
0xFFFF, /* 233 - undefined*/
0xFFFF, /* 234 - undefined*/
0xFFFF, /* 235 - undefined*/
0xFFFF, /* 236 - undefined*/
0xFFFF, /* 237 - undefined*/
0xFFFF, /* 238 - undefined*/
0xFFFF, /* 239 - undefined*/
0xFFFF, /* 240 - undefined*/
0xFFFF, /* 241 - undefined*/
0xFFFF, /* 242 - undefined*/
0xFFFF, /* 243 - undefined*/
0xFFFF, /* 244 - undefined*/
0xFFFF, /* 245 - undefined*/
0xFFFF, /* 246 - undefined*/
0xFFFF, /* 247 - undefined*/
0xFFFF, /* 248 - undefined*/
0xFFFF, /* 249 - undefined*/
0xFFFF, /* 250 - undefined*/
0xFFFF, /* 251 - undefined*/
0xFFFF, /* 252 - undefined*/
0xFFFF, /* 253 - undefined*/
0xFFFF, /* 254 - undefined*/
0xFFFF, /* 255 - undefined*/
},
};
