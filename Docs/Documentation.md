# GSSquared - Ultimate Apple II Series Emulator

## Project Status (as of 2025-01-18)

* Supported Platforms
  * MacOS - the primary development platform. This will always be up to date.
  * Linux - Can now build Linux binary. All features working.
  * Windows - Not started.
* CPUs
  * NMOS 6502 - Complete.
  * CMOS 6502 - not started.
  * 65c816 - not started.
* Keyboard - Complete.
* Memory Expansion
  * Language Card - Complete.
  * Apple II Memory Expansion Card ("Slinky")- Complete.
* Display Modes
  * Text
    * 40-column text mode - Complete. Supports normal, inverse, and flash; Screen 1 and 2.
    * Apple II+ 80-column text mode - not started.
    * Apple IIe 80-column text mode - not started.
  * Low-resolution graphics - (Complete - works with Screen 1, 2, split-screen and full-screen).
  * High-resolution graphics - monochrome ('green screen version') done; Color "rgb mode" done; Color "composite mode" done.
  * Green and Amber monochrome modes - done for text, hi-res. Lo-res not started. 
* Storage Devices
  * Cassette - not started.  
  * Disk II Controller Card - Read-only working with DOS and ProDOS interleave disks.
  * ProDOS Block-Device Interface - complete.
  * SmartPort / ProDOS Block-Device Interface - Not started.
  * RAMfast SCSI Interface - Not started.
  * Pascal Storage Device - Not started. (Same as generic ProDOS-compatible?)
* Disk Image Formats
  * .do, .dsk - read only, 143K.
  * .po - read only, 143K.
  * .hdv - any size block device, read/write, complete.
  * .2mg - read/write, for block devices, complete.
  * .nib - read only, 143K.
  * .woz - not started.
* Sound - Complete for 1MHz operation. Does work at higher speeds, but, results are comical.
* I/O Devices
  * Printer / parallel port - not started.
  * Printer / serial port - not started.
  * ImageWriter printer emulation - not started.
  * Joystick / paddles - initial implementation, with mouse. Work in progress. GamePad support started, needs refactored.
  * Shift-key mod and Lowercase Character Generator. Not started.
* Clocks
  * Thunderclock - read of time implemented. Interrupts, writing clock - not implemented. Needs testing.
  * Generic ProDOS-compatible Clock - complete, read-only.

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


### RAMfast SCSI Interface

I'm not sure if there is any benefit to emulating the RAMfast SCSI interface. An Apple II forum
user suggested it, but, I don't think there was any software that required it specifically?
(There was also a suggestion to emulate a SecondSight, and, that would be way easier and
make more sense).

### Pascal Storage Device

There is a standardized device interface that ProDOS supports, called Pascal. This may be what
became known as the SmartPort / ProDOS block-device interface. See above.


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

