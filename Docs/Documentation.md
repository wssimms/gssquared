# GSSquared - Ultimate Apple II Series Emulator

## Project Status (as of 2025-01-18)

* CPUs
  * NMOS 6502 - Complete.
  * CMOS 6502 - not started.
  * 65c816 - not started.
* Keyboard - Complete.
* Memory Expansion
  * Language Card - Complete.
* Display Modes
  * Text
    * 40-column text mode - Complete. Supports normal, inverse, and flash; Screen 1 and 2.
    * Apple II+ 80-column text mode. not started.
    * Apple IIe 80-column text mode - not started.
  * Low-resolution graphics - (Complete - works with Screen 1, 2, split-screen and full-screen).
  * High-resolution graphics - monochrome ('green screen version') done. Color hi-res not started.
* Storage Devices
  * Cassette - not started.  
  * Disk II Controller Card - Read-only working with DOS and ProDOS interleave disks.
  * ProDOS Block-Device Interface - complete.
  * SmartPort / ProDOS Block-Device Interface - Not started.
  * RAMfast SCSI Interface - Not started.
  * Pascal Storage Device - Not started.
* Disk Image Formats
  * .do, .dsk - read only, 143K.
  * .po - read only, 143K.
  * .hdv - any size block device, read/write, complete.
  * .2mg - read/write, for block devices, complete.
  * .nib - not started.
  * .woz - not started.
* Sound - Complete and cycle accurate for 1MHz operation. Will need tweaking to work at higher CPU speeds.
* I/O Devices
  * Printer / parallel port - not started.
  * Printer / serial port - not started.
  * Joystick / paddles - initial implementation, with mouse. Work in progress. GamePad support coming.
  * Shift-key mod and Lowercase Character Generator. Not started.
* Clocks
  * Thunderclock - read of time implemented. Interrupts, writing clock - not implemented. Needs testing.
  * Generic ProDOS-compatible Clock - not started.

## ROMs

You will of course need ROM images to run the emulator like an Apple.

1. System ROM Images: https://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Computers/Apple%20II/Apple%20II/ROM%20Images/
2. Character ROM Image: https://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Computers/Apple%20II/Apple%20II/ROM%20Images/Apple%20II%20Character%20ROM%20-%20341-0036.bin

### More Character ROM stuff:

https://github.com/dschmenk/Apple2CharGen

Info on the character set:

https://en.wikipedia.org/wiki/Apple_II_character_set


## Display

The display is managed by the cross-platform SDL3 library, which supports graphics
but also sound, user input, lots of other stuff. It is a library intended for
video games, and in thinking about all the things that make up a good emulator, 
emulators fit the video game category quite closely.


### Text Mode

Text mode is complete. It supports 40-column normal, inverse, and flash.

### Low-Resolution Graphics

Low-resolution graphics are complete. They work with screen 1, 2, split-screen, and full-screen.

### High-Resolution Graphics

Monochrome (green screen) high-resolution graphics are complete.


## Keyboard

Status of keyboard:

1. control keys: YES
1. shift keys: YES
1. arrow keys: YES. II+ only has left and right arrow keys.
1. Escape key: YES
1. RESET key: YES. Map Control+F10 to this.
1. EXIT key: YES. Map F12 to this.
1. REPT key: no. But we don't need this, autorepeat is working. Ignore this.
1. Backspace - YES. This apparently maps to same as back arrow. Got lucky.


## Disk Drive Interfaces

### Disk II Controller Card Design and Operation

https://www.bigmessowires.com/2021/11/12/the-amazing-disk-ii-controller-card/

This is highly timing-dependent, and, the way to go I think is to simulate time passage by
counting cycles, not by counting realtime. This way floppy disks can be simulated at any speed.
If we're at free-run, then the floppy disk would operate at that speed, and we don't have to
slow down the clock.

This is required for DOS 3.3 compatibility.

### SmartPort / ProDOS Block-Device Interface

SmartPort became a standard for: 5.25" drives, 3.5" drives, and some hard disk drives,
for the ProDOS operating system.

It is documented in the Apple IIgs Technical Reference Manual, chapter 7.

$Cn01 - $20
$Cn03 - $00
$Cn05 - $03
$Cn07 - $00 - "SmartPort Signature Byte"
$CnFB - ?? - SmartPort ID Type Byte

bit 7: extended
Bits 6-2: Reserved
Bit 1: SCSI
Bit 0: RAM Card

The ProDOS Dispatch address is at $CnFF. That byte indicates the address of the
dispatch routine. if VV = value at $CnFF, then the ProDOS dispatch routine is at $CnVV.
The SmartPort dispatch address is 3 bytes past that. 

Both the Apple 3.5 Disk and the UniDisk 3.5 have the ability to override various
internal routines, such as scanning for sector headers, etc. This was done to enable
copy protection. We don't need to implement this. We will implement a generic UniDisk 3.5
interface to simulate an 800K floppy. Attempts to call SetHook will just return an error.

Our primary goal here is to support a simple block device interface, the fastest way to get
disk storage on here so we can boot ProDOS < 2.0. on my apple II+.

Some info is at:
https://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Companies/Apple/Documentation/Apple%20II%20Technical%20Notes%201989-09.pdf

(Apple II Technical Notes)

We would want to support this using ParaVirtualization. We will 'notice' and capture any JSR DISPATCH
instructions. Then instead of performing the normal instruction emulation, we will instead call
our paravirtual routines.


### RAMfast SCSI Interface

I'm not sure if there is any benefit to emulating the RAMfast SCSI interface. An Apple II forum
user suggested it, but, I don't think there was any software that required it specifically?
(There was also a suggestion to emulate a SecondSight, and, that would be way easier and
make more sense).

### Pascal Storage Device

There is a standardized device interface that ProDOS supports, called Pascal. This may be what
became known as the SmartPort / ProDOS block-device interface. See above.


## Other Devices

### Thunderclock

From "ThunderClock_Plus.pdf"

For direct hardware access to a TCP (ThunderClock Plus):

* Raise strobe for at least 40uS.
  * CREG is 0xC0S0 (where S is slot number plus 8).
  * Set bit STB (bit 2, value 0x04) high, and store the command plus the STB bit to card.
  * Then set same value back with the STB low.
* Then the data to be shifted out is 10 nibbles worth, 4 bits each.
  * Seems like the high bit (bit 7) of CREG. Code does:

CLK = $2
STB = $4

uPD1990AC hookup:
	bit 0 = data in (to clock)
	bit 1 = CLK
	bit 2 = STB
	bit 3 = C0
	bit 4 = C1
	bit 5 = C2
	bit 7 = data out (to cpu)

  LDA CREG,Y
  ASL  ; data out bit to carry
  ROR TEMP2 ; carry into bit 7 of TEMP2

They call pushing data the the clock "IN". Reading data from the clock is "OUT".

Reads the in bit. Sets the out bit.
Then raises and lowers the clock.
TIMRD = $18 - this puts it into read mode.
They just use the same routine for both IN and OUT, depending on the mode.
How weird.

SO.

When we get the strobe plus command $18, (bits 0b0001 1100):
* calculate current time values, store into a 40-bit buffer.
* each cycle where they then read and tick the clock, shift that
* the 'read' just reads either the lowest or the highest bit, whichever it is.

nibble 0: month (1 - 12 binary)
nibble 1: day of week (0 - 6 BCD)
nibble 2: date (tens place, 0-3 BCD)
nibble 3: date units (0-9 BCD)
nibble 4: hour (tens place, 0-2 BCD)
nibble 5: hour units (0-9 BCD)
nibble 6: minute (tens place, 0-5 BCD)
nibble 7: minute units (0-9 BCD)
nibble 8: second (tens place, 0-5 BCD)
nibble 9: second units (0-9 BCD)

So we need state tracking of the strobe; and state tracking of the 'clock' bit.


# Building

gs2 is currently built on a Mac using:

* clang
* SDL3 downloaded and built from the SDL web site.
* git

I use vscode as my IDE, but, this isn't required.

1. git clone xxxxxxx
1. cd xxxxxxx
1. mkdir build
1. cd build
1. cmake .

If you want to help create build tooling for Windows or Linux, let me know! All of this should
work on all 3 platforms courtesy of SDL3.


## Code Organization

### Platforms

When a virtual Apple II is created and 'powered on', there are several stages.

1. Platform is initialized.
1. CPU structure is created.
1. Memory allocated and initial memory map assigned.
1. Devices are initialized and 'registered'.
1. The CPU is 'powered on'.

A "Platform" is a specific selection and combination of: CPU type, devices, and memory map.
Because so much of the Apple II series share significant systems and subsystems,
we define and implement those with granularity and "compose" them together when a 
particular virtual machine is set up.

As one example, the Apple II, II+, IIe, IIc, and IIgs all share the same $C030 primitive
speaker toggle. So, we do that only once. When we 'boot' a IIgs OR a IIe, we call
"init_mb_speaker" against the CPU structure, and that hooks in this functionality.

Similarly with CPUs. You want a IIe with nmos 6502? Great. Select those components.
You want a IIe with original firmware but cmos 6502? You can define a platform
to do exactly that.

Want to create your own platform that never existed in reality and experiment?
Go for it! Design the next generation of Apple II! Then maybe someone can take your
idea and implement in hardware.

Contribute new devices back to the project for fun and profit! Well, for fun anyway.

### CPU

### Bus

The Bus is the concept that ties the CPU together with memory and devices.
Apple II devices of course are memory-mapped I/O, i.e., their control registers
are accessed at specific memory addresses.

