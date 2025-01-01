# GSSquared - Ultimate Apple II Series Emulator

## Project Status (as of 2025-01-01)

* CPUs
  * NMOS 6502 - Complete.
  * CMOS 6502 - not started.
  * 65c816 - not started.
* Keyboard - Complete.
* Display Modes
  * Text
    * 40-column text mode - Complete. Supports normal, inverse, and flash; Screen 1 and 2.
    * 80-column text mode - not started.
  * Low-resolution graphics - (Complete - works with Screen 1, 2, split-screen and full-screen).
  * High-resolution graphics - not started.
* Storage Devices
  * Cassette - not started.  
  * Disk II Controller Card - Not started.
  * SmartPort / ProDOS Block-Device Interface - Not started.
  * RAMfast SCSI Interface - Not started.
  * Pascal Storage Device - Not started.
* Sound - Partially complete. Working, but has some audio artifact issues due to timing sync. Work in progress.
* I/O Devices
  * Printer / parallel port - not started.
  * Printer / serial port - not started.
  * Joystick / paddles - not started.


## ROMs

You will of course need ROM images to run the emulator like an Apple.

1. System ROM Images: https://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Computers/Apple%20II/Apple%20II/ROM%20Images/
2. Character ROM Image: https://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Computers/Apple%20II/Apple%20II/ROM%20Images/Apple%20II%20Character%20ROM%20-%20341-0036.bin

### More Character ROM stuff:

https://github.com/dschmenk/Apple2CharGen

Info on the character set:

https://en.wikipedia.org/wiki/Apple_II_character_set


## Display

The display is managed by the cross-platform SDL2 library, which supports graphics
but also sound, user input, lots of other stuff. It is a library intended for
video games, and in thinking about all the things that make up a good emulator, 
it fits the video game category quite closely.



### Text Mode

Text mode is working pretty well. It supports normal, inverse, and flash.

### Low-Resolution Graphics

Low-resolution graphics are working pretty well. They work with screen 1, 2, split-screen, and full-screen.

### High-Resolution Graphics

High-resolution graphics are not yet implemented.


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

So that is one device we have to emulate, for DOS 3.3 compatibility.

### SmartPort / ProDOS Block-Device Interface

SmartPort became a standard for: 5.25" drives, 3.5" drives, and some hard disk drives.
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
