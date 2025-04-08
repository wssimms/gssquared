# GSSquared - Ultimate Apple II Series Emulator

The ambition for this project is large - and it will take a while to get it done. So at any given point in time, consider it a work-in-progress complete with bugs, missing features, and experiments. Of course there will be plenty that "just works".

# Project Status (as of 2025-04-07)

## Supported Host Platforms

| Platform | Status |
|----------|--------|
| MacOS    | The primary development platform. This will always be up to date. |
| Linux    | Can now build Linux binary. All features working. |
| Windows  | Not started. |

## Supported Video Modes

We currently have the following implemented:

| Apple II Mode | Composite | RGB | Monochrome |
|---------------|-----------|-----|------------|
| Text (40-col) | ❌        | ✅  | ✅         |
| Text (80-col) | ❌        | ❌  | ❌         |
| LGR           | ❌        | ✅  | ❌         |
| HGR           | ✅        | ✅  | ✅         |
| DHGR          | ❌        | ❌  | ❌         |

By "Composite" we mean DisplayNG, to accurately render composite artifacts.

## CPUs

| CPU Type | Status |
|----------|--------|
| NMOS 6502 | ✅ |
| CMOS 6502 | ❌ |
| 65c816 | ❌ |

## Keyboard

| Model | Status |
|-------|--------|
| Apple II+ | ✅ |
| Apple IIe | ❌ |
| Apple IIc | ❌ |
| Apple IIgs | ❌ |


## Memory Expansion

| Type | Status |
|------|--------|
| Language Card | ✅ |
| Apple II Memory Expansion Card ("Slinky") | ✅ | 

## Storage Devices

| Type | Status |
|------|--------|
| Cassette | Not started |
| Disk II Controller Card | ✅ |
| ProDOS Block-Device Interface | ✅ |
| SmartPort / ProDOS Block-Device Interface | Not started |
| RAMfast SCSI Interface | Not started |
| Pascal Storage Device | Not started (Same as generic ProDOS-compatible?) |

Additional notes: Disk II does not support quarter or half tracks.

## Disk Image Formats

| Format | Status | Size | Notes |
|--------|--------|------|-------|
| .do, .dsk | Read only | 143K | |
| .po | Read only | 143K | |
| .hdv | Read/write | Any size | Block device, complete |
| .2mg | Read/write | Various | For block devices, complete |
| .nib | Read only | 143K | |
| .woz | Not started | | |


| Component | Status | Notes |
|-----------|--------|-------|
| Basic Speaker | ✅ Complete | Needs a little more work to make less square wavy |
| Ensoniq | Not started |

## I/O Devices

| Device | Status | Notes |
|--------|--------|-------|
| Printer / parallel port | Not started | |
| Printer / serial port | Not started | |
| ImageWriter printer emulation | Not started | |
| Joystick / paddles | Work in progress | Initial implementation with mouse. GamePad support started, needs refactored. |
| Shift-key mod and Lowercase Character Generator | Not started | |

## Clocks

| Type | Status | Notes |
|------|--------|-------|
| Thunderclock | Partial | Read of time implemented. Interrupts, writing clock - not implemented. Needs testing. |
| Generic ProDOS-compatible Clock | Complete | Read-only. |

# More Detailed Notes on Status

## Keyboard

### Apple II+

1. control keys: YES
1. shift keys: YES
1. arrow keys: YES. II+ only has left and right arrow keys.
1. Escape key: YES
1. RESET key: YES. Map Control+F10 to this.
1. EXIT key: YES. Map F12 to this.
1. REPT key: no. But we don't need this, autorepeat is working. Ignore this.
1. Backspace - YES. This apparently maps to same as back arrow. Got lucky.

## ROMs

You will of course need ROM images to run the emulator like an Apple.

1. System ROM Images: https://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Computers/Apple%20II/Apple%20II/ROM%20Images/
2. Character ROM Image: https://mirrors.apple2.org.za/Apple%20II%20Documentation%20Project/Computers/Apple%20II/Apple%20II/ROM%20Images/Apple%20II%20Character%20ROM%20-%20341-0036.bin

In the directory /roms/, there is a Makefile that will download the ROM images and combine them into single files, for the various platforms (Apple II+, //e, etc.) After the main gs2 build:

```
cd roms
make
```

This is driven by a python script, and a JSON file in each folder.

Separately, you need roms for the Thunderclock Plus, and Apple II memory expansion card. Holler if you need them.

We will support doing a 'make' on those ROMs too soon.


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

