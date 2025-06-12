# AppleMouse

The AppleMouse card is fairly complex considering what it does. It's got an onboard microprocessor (6805), a 6520 chip (according to one thing I read). 6520 is like 6522 without timers.

The programming references for it specify use of a bunch of standardized firmware vectors. It's got a 2K ROM for C800-CFFF.

So there are two options: ParaVirtual, or emulate the hardware.

https://github.com/freitz85/AppleIIMouse/blob/master/AppleMouse.txt

```
ClampMouseX ->  W 4 Bytes   LLX, LHX, HLX, HHX
ClampMouseY ->  W 4 Bytes   LLY, LHY, HLY, HHY
SetMouse    ->  W 1 Byte    MODE
ServeMouse  ->  R 1 Byte    STATUS
ReadMouse   ->  R 6 Bytes   LX, LY, HX, HY, STATUS, MODE
ClearMouse  ->  W 4 Bytes   LX, LY, HX, HY
PosMouse    ->  W 4 Bytes   LX, LY, HX, HY
HomeMouse   ->  R 4 Bytes   LLX, HLX, LLY, HLY
                W 4 Bytes   LX, LY, HX, HY

A0-A3:

Write:
0x00    Position:   LX, HX
0x01                LY, HY
0x02    BoundaryX:  LLX, LHX
0x03                HLX, HHX
0x04    BoundaryY:  LLY, LHY
0x05                HLY, HHY
0x06    State:      MODE
0x07    Set to Input

Read:
0x08    Position:   LX, HX
0x09                LY, HY
0x0a    BoundaryX:  LLX, LHX
0x0b                HLX, HHX
0x0c    BoundaryY:  LLY, LHY
0x0d                HLY, HHY
0x0e    State:      STATUS
0x0f    Reset to default
```

These are 6805 microcontroller register indexes. PA on the 6520 is connected to the microcontroller data lines. PB4-7 are connected to the microcontroller PC0-3 lines - i.e. the register selects.
PB1-3 are tied to ground. PB0 is connected to "sync latch" from some clock related chip.

A0-1 are connected to 6520 RS0-1. 


Here's a disassembly of the ROM:

https://github.com/freitz85/AppleIIMouse/blob/master/mouse%20rom.s

ok I'm going to guess the following:

BoundaryX provides a range from L to H - low value to high value. And a similar range for Y low to high.
This clamps the mouse Position to be inside that range. Simple enough.
What we need is the State: MODE and STATUS values.

The position X and Y can range from -32768 to 32767 (signed 16 bit integer). "Are normally clamped to range $0 to $3FF (0 to 1023 decimal).

ah ok these are in the manual. The Mode byte:

| Bit | Function |
|-----|----------|
| 0 | Turn mouse on |
| 1 | Enable interrupts with mouse movement |
| 2 | Enable interrupts when button pushed |
| 3 | Enable interrupts every screen refresh |
| 4-7 | Reserved |

if you write value of $01 to mode, it's "passive mode" - i.e., no interrupts.

Status

| Bit | Function |
|-----|----------|
| 7 | Button is down |
| 6 | Button was down at last reading |
| 5 | x or Y changed since last reading |
| 4 | Reserved |
| 3 | interrupt caused by screen refresh |
| 2 | interrupt caused by button press |
| 1 | interrupt caused by mouse movement |
| 0 | Reserved |

Oh, somewhere we need a register for a mouse click?
What is SERVEMOUSE?

if you use an interrupt mode, the interrupt handler must call SERVEMOUSE as part of that - it determines whether interrupt was caused by mouse so we can call READMOUSE if it was.

So what clears the interrupts? Just reading the STATUS byte perhaps?

## 6520

6520 has these registers:
| Register | Function |
|-----|----------|
| CRA | Control Register A |
| DIR | Data Input Register |
| DDRA | Data Direction Register A |
| DDRB | Data Direction Register B |
| PIBA | Peripheral Interface Buffer A |
| PIBB | Peripheral Interface Buffer B |
| DDRB | Data Direction Register B |
| ISCB | Interrupt Status Control B |
| CRB | Control Register B |
| ORA | Peripheral Output Register A |
| ORB | Peripheral Output Register B |

