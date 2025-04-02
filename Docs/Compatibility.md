# Compatibility

## Apple Pascal - Disk 0.po

Does not boot. Loads some stuff from the disk then says "NO FILE SYSTEM.APPLE" or something like that. Same on both emulators.

## fireworks.dsk

Apple II+.

FIREWORKS.BAS works (it's just applesoft).

hello + fireworks (B) does not work. It is apparently doing some crazy cycle-timed stuff, switching display modes using very tight timing.
So this is "mixing video modes". This won't work with our emulation approach at all.

## crazy cycles 2.dsk

Apple II+.

when booting, displays "MB#" But disk is still running. it's loading a LOT? trying in OE it sounds like the head is scanning back and forth.
Nothing happens.
Same on OE with II+ and IIe. I guess this is just not working.

## four_voice_music.dsk

A random collection, but the most interesting thing here is a demo program that plays 4 voice music pieces.
Comparing OE to GS2, GS2 is accurately replicating some extremely high frequency harmonics. The overall tone is a little buzzier, but we know that already.

## keywin.2mg

OE does not support .2mg files.
Does not boot on GS2, disk spins forever and no visible boot process proceeds.
This is an 800k .2mg disk. Still won't boot on GS2.

## Lode Runner

works in OE.
in GS2, we throw a bunch of "unknown opcode" errors on the value 0x80.
What if we don't halt but just skip the instruction? ok, well now it's starting up. looks good actually. 
there's no audio...
OE has audio. according to Claude you can toggle the speaker by hitting any address in C030 to C03F.
I added that to GS2, and still no audio in Lode Runner.
there is lode runner disassembly at github.
Weird. So Control-S enabled audio. Seems like maybe there is some reason sound is disabled by default?

## timelord.dsk

gs2: prodos 8 v1.9 boots partially, cursor flashes over a . in the upper left, but does not complete booting.
oe: boots and it's a pretty good synthesizer using the apple ii speaker.

Timelord now boots after the Disk II head movement fix.

## applevision.dsk

works correctly.

## prodos 1.1.1.dsk

Works in GS2, but:
"Display slot assignments" shows "USED" for slots that don't have stuff in them. It should read "EMPTY".
It thinks there is a 80 column card in slot 3. That ain't right.
It thinks the slinky card is a "PROFILE". (This is correct for Prodos 1.1.1).

The issue with this could be that we are supposed to be reading floating bus values when c100-c1xx is read and there is no card in it. We should have a page table setting for "floating". And then pick a bit of display memory at random and return those values. ("At random" in other emulators would be based on where the display update virtual beam scan is. We don't do it that way. )

## ProDOS1.9.po

This crashes on boot at address 0x0914 in both GS2 and OE, on both II+ and IIe modes. I think this disk image is bad.

## ProDOS_2_4_3.po 

Works in II+ on OE. 
Hangs during boot on GS2 after showing the prodos splash screen. Drive light is still on.
Will need to trace this and see what's going on. Is supposed to work on Apple II plus and standard 6502. So who knows what is going on..
Though, OE II plus seems to have an 80 column card that activates on PR#3.

## Locksmith

### 6.0 
"Scan Disk" is not working.
locksmith ability to quarter track isn't going to work the way the code is written now.
16 sector fast disk claims "address" field is missing on every sector and track. I wonder if this is all due to lack of quarter-track support.
This did not work in the previous iteration of the code either.

### 5.0
nothing working here either. It's never turning the disk drive motors on. it must be doing it some different way.

## Glider

This boots far enough to say "LOADING GLIDER..." but then hangs. Docs say on a II+ it requires a mouse.
hard looping at $0E86. This is a tight loop BIT $C019 BMI $0E86. I.e., it's looking for NOT VBL. Then it looks for VBL, and exits when it has seen VBL not and then VBL plus.

The Apple II plus did not have the VBL register. So this software not compatible with Apple II plus!


