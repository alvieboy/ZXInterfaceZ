# External memory organization

The external memory is 64Mbit in size (8 Megabytes). External memory address space is 23-bit, and spawns from
0x000000 to 0x7f0000. Some parts are mapped to Spectrum memory address space.

Paging control on Spectrum side is controlled by RS (ROMSEL) and MS (MEMSEL)
```
+---------------------+                                  +---+---+ +---------------------+
| External Addr Space |                                  | R | M | | Spectrum Addr Space |
|   Start  |   End    | Size |                           | S | S | | Start    | End      |
+----------+----------+------+---------------------------+---+---+-+----------+----------+
| 0x000000 | 0x001FFF |  8kB | Main InterfaceZ ROM       | 0 | X | |   0x0000 |   0x1FFF |
| 0x002000 | 0x003FFF |  8kB | Alternate small ROM       | 1 | X | |   0x0000 |   0x1FFF |
| 0x004000 | 0x007FFF | 16kB | Alternate big 16 ROM      | 2 | X | |   0x0000 |   0x3FFF |
| 0x008000 | 0x00FFFF | 32kB | Alternate big 32 ROM      | 3 | X | |   0x0000 |   0x7FFF | * only for +2A/+3 models, TBD
| 0x010000 | 0x011FFF |  8kB | RAM bank 0                |0,1| 0 | |   0x2000 |   0x3FFF |
| 0x012000 | 0x013FFF |  8kB | RAM bank 1                |0,1| 1 | |   0x2000 |   0x3FFF |
| 0x014000 | 0x015FFF |  8kB | RAM bank 2                |0,1| 2 | |   0x2000 |   0x3FFF |
| 0x016000 | 0x017FFF |  8kB | RAM bank 3                |0,1| 3 | |   0x2000 |   0x3FFF |
| 0x018000 | 0x019FFF |  8kB | RAM bank 4                |0,1| 4 | |   0x2000 |   0x3FFF |
| 0x01A000 | 0x01BFFF |  8kB | RAM bank 5                |0,1| 5 | |   0x2000 |   0x3FFF |
| 0x01C000 | 0x01DFFF |  8kB | RAM bank 6                |0,1| 6 | |   0x2000 |   0x3FFF |
| 0x01E000 | 0x01FFFF |  8kB | RAM bank 7                |0,1| 7 | |   0x2000 |   0x3FFF |
| 0x020000 | 0x7FFFFF |      | Unused                    | - | - | |   -      |   -      |
+----------+----------+------+---------------------------+---+---+-+----------+----------+
```
