
## Dec 25, 2024

I'm going to go ahead and scrap the Tigr code because SDL2 looks like it will do the trick.

For reference, the current design target is Apple II+ level.

I have some options for the next piece to tackle. Generally speaking, here is the project
status:

1. NMOS 6502 - (Complete.)
1. Keyboard - Complete. There may be tweaks needed.
1. 40-column text mode - Complete. Supports normal, inverse, and flash.

1. 40-column text mode - Screen 2. (Complete)
1. Low-resolution graphics - not started.
1. High-resolution graphics - not started.
1. Disk II Controller Card - Not started.
1. Sound - not started.
1. Printer / parallel port - not started.
1. Printer / serial port - not started.
1. Cassette - not started.
1. Joystick / paddles - not started.

Pretty much all of this is carried forward into the Apple IIe, IIc, and IIgs,
with the exception that the IIe non-enhanced uses 65n02, IIe enhanced uses 65c02, and IIc 
does also. Of course, the IIgs uses the 65816.

What is the best piece to tackle next? I think I'll work on the graphics modes. Not much I
can boot off a disk will do much without them.

## 40-column text mode - Screen 2

Poking 0xC055 will enable screen 2. It's still a 40-column text mode but it uses
memory at 0x0800-0x0BFF instead of 0x0400-0x07FF.

0xC054 will return to screen 1. These two flags affect text, lo-res, and hi-res graphics.

This should be a pretty easy piece to tackle.

We'll need:
   handlers to catch the accesses to 0xC054, 0xC055. When these are accessed we change
   the display buffer base address, and redraw the whole screen (only if the mode has changed).
   And we need to monitor for writes to the new display buffer base address.

This is where I want to implement the jump table for handling the 0xC000 - 0xC07F space.

### Low-resolution graphics

The low-resolution graphics are 40 x 48 blocks, 16 colors. Each nibble in a display page
byte represents a block. These will be 8 x 4 pixels in terms of the current display
buffer design. The color is one of a specific palette of 16 colors:

https://en.wikipedia.org/wiki/Apple_II_graphics


## Dec 26, 2024

Oddly, when I switch to screen 2, the display comes up. 
And when I write to 0x0800 with a character, it displays.
but it's not supposed to. txt_memory_write is not being called.
so it must be the flash handler. Try disabling that and see if the screen
doesn't update as expected.
if so, then the flash handler is updating too much.

Testing emulation speed.
Every 5 seconds, print the number of cycles since the last 5 seconds.
it's hovering around 1000000 cycles per second:

```
3747795 cycles
5194629 cycles
5233284 cycles
5287931 cycles
5082248 cycles
```

the low number was when generating tons of video display debug output.
I am running in the free-run mode, so slowness is not due to the clock sync emulation
let me turn off the display.
ok the display update is off, and it's in the same area. And it started at about 6M and worked its way down to 5.1.
it's pretty consistent..

```
5796507 cycles
6093662 cycles
5983934 cycles
5550091 cycles
5107896 cycles
5123350 cycles
5147278 cycles
5142563 cycles
5118742 cycles
5123895 cycles
```
am I sure I'm in free run ? I am.

SO. I was calling event_poll() every time through the instruction loop. So. Many. Times.
I put it inside the check for the display update, once every 1/60th second. This is per
docs that suggest "once per frame".

Removing that, I get a effective MHz speed of 150MHz, with optimizations -O3 enabled.
With -O0, I get about 61MHz effective, at free-run mode.
And of course I can turn off free mode.

I should implement some keyboard commands to toggle free-run mode. F9. That's convenient.
Maybe a key to toggle certain debug flags?

F9 works well. I should have it toggle between free-run, 1MHz, and 2.8MHz.

ok, that is working. I changed free-run from a flag, to a enum mode. So we can add other modes and even specify the exact mode.

This is a fun little program. Appleosft, and all text mode.
https://www.leucht.com/blog/2016/09/my-apple-iie-a-simple-text-based-arcade-game-in-applesoft-basic/
works just fine. All the scrolling causes the emulator to slow down to 3MHz effective rate.

It may end up being better to just draw the screen 60 times a second, and not worry about updating for every character. That feels weird, but, this
may be the modern way. 

ok let's think about this. 16 milliseconds per frame. 0.000087 seconds per scanline. (192 scanlines * 60 frames, 11520 scanlines per second).

I should only be copying the back buffer each frame, 16ms. this slows down to 1MHz:!
10 A=1
20 PRINT A
30 A = A + 1
40 GOTO 20

So something is wrong. This ought to FLY.

So let's try that other approach. Instead of updating the display for every character,
we'll just redraw the screen every 16ms. Will this be faster?

## Dec 27, 2024

Big performance drain on the video display.

Something that causes tons of scrolling:

795867523 cycles clock-mode: 0 cycles per second: 159.173508 MHz
796674469 cycles clock-mode: 0 cycles per second: 159.334900 MHz
218963831 cycles clock-mode: 0 cycles per second: 43.792767 MHz
5476743 cycles clock-mode: 0 cycles per second: 1.095349 MHz
5443788 cycles clock-mode: 0 cycles per second: 1.088758 MHz

Effective performance drops to 1MHz.

Sample shows:
Call graph:
    8077 Thread_49405054   DispatchQueue_1: com.apple.main-thread  (serial)
    + 8077 start  (in dyld) + 2840  [0x183e2c274]
    +   8077 main  (in gs2) + 376  [0x1006abf00]
    +     7981 run_cpus()  (in gs2) + 112  [0x1006abcc8]
    +     ! 7949 execute_next_6502(cpu_state*)  (in gs2) + 2052  [0x1006a9060]
    +     ! : 7943 store_operand_zeropage_indirect_y(cpu_state*, unsigned char)  (in gs2) + 120  [0x1006ab78c]
    +     ! : | 7938 memory_bus_write(cpu_state*, unsigned short, unsigned char)  (in gs2) + 52  [0x1006ac40c]
    +     ! : | + 7922 render_character(int, int, unsigned char, bool)  (in gs2) + 488  [0x1006addf8]
    +     ! : | + ! 7078 SDL_UnlockTexture_REAL  (in libSDL2-2.0.0.dylib) + 412  [0x100878ee8]
    +     ! : | + ! : 7075 SDL_UnlockTexture_REAL  (in libSDL2-2.0.0.dylib) + 324  [0x100878e90]

we're doing tons of SDL related calls. They're probably fairly expensive ones.
I'll note that if we're doing a scroll, then we're reading every character on the screen (pure
memory copy is fast), but then writing every character on the screen - which generates a call to 
render_character for every character on the screen.
Render character is a loop in C that is 56 total iterations; so that's around 60K iterations of generating
the character bitmap.
Let's get rid of that inner loop by pre-generating the character bitmap.
Each Apple II character is 7x8 pixels. One bit per pixel.
The output we need to feed into SDL2 is 32-bit values, one per pixel.
So one character becomes 64 bytes, pre-coded. 

ok well that generated this:
5468271 cycles clock-mode: 0 cycles per second: 1.093654 MHz
5356073 cycles clock-mode: 0 cycles per second: 1.071215 MHz

basically no change.
I still have a loop. And while I unrolled it, I am still copying from main RAM to the texture.
So that's expensive.
Can I create a second texture that just contains the character bitmaps, and then copy chunks of
that to the screen?

ok look at row at a time.
txt_memory_write is called for each byte written.
currently it calculates row and column from the address.
Now, it will just calculate row, and mark the row as dirty.
on a screen update, we recalculate the dirty rows and render them.

Wow, holy cow. So what I implemented:
dirty line checker. (24 element array)
txt_memory_write is called for each byte written. But this now ONLY marks the line as dirty.
The actual rendering is done in update_display which is called every 16ms.
That scans the dirty line array. For each dirty line, it renders the line.
I can do 2000.7fff in the rom monitor - which dumps 28KBytes of memory.
It occurs virtually instantly.
This is because no matter how many characters are written to vram, the actual rendering
is of at most 24 lines, 60 times per second. So that limits the number of locks.
So, this could probably render all 24 lines at a time no matter what. I guess I should
try that because it would simplify flash mode etc. Flash wasn't hard at all - just
identify screen rows that have a flashing character, then mark the row as dirty. And
move the flash state to a global so it's accessible to the other routines.

### Apple III info:

For reference:
https://www.apple3.org/Documents/Magazines/AppleIIIExtendedAddressing.html
