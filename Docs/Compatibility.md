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

## Lode Runner

works in OE.
in GS2, we throw a bunch of "unknown opcode" errors on the value 0x80.
What if we don't halt but just skip the instruction? ok, well now it's starting up. looks good actually. 
there's no audio...
OE has audio. according to Claude you can toggle the speaker by hitting any address in C030 to C03F.
I added that to GS2, and still no audio in Lode Runner.
there is lode runner disassembly at github.
Weird. So Control-S enabled audio. Seems like maybe there is some reason sound is disabled by default?
