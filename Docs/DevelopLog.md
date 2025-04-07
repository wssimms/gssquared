## Display Handling

It's not easy to see the pattern in the character ROM, but it's there.

```
0x08 xx001000
0x14 xx010100
0x22 xx100010
0x22 xx100010
0x3E xx111110
0x22 xx100010
0x22 xx100010
0x00 xxxxxxxx
```

Apple II text mode : still uses 280x192 matrix.
Each character is 7 x 8 pixels.
280 / 7 = 40 columns.
192 / 8 = 24 rows.

Pixel size: Each character is made up of a 5x7 pixel bitmap, but displayed as a 7x8 character cell on the screen. there will be 1 pixel of padding on each side of the character.

Testing the TIGR library.

It looks like you do the following:
    create the display window.
    draw various things to the buffer.
    update the display.
So we can accumulate changes to the display memory/display buffer, and only update the display when we are ready to show it.
Say once each 1/60th of a second.
I guess we'll see how fast it is.
To start, update buffer every time we write to the screen.

well that sort of works! don't know if it's fast enough, but ok for now.

trying to boot actual apple 2 plus roms. Getting an infinite loop at f181 - 
this is testing how much memory by writing into every page. unfortunately everything
in the system right now is "ram" so it's never finding the end of ram ha ha.

OK that's fixed and it now boots to the keyboard loop! Though the prompt is missing, and there is no cursor because there
is no inverse or flash support right now.
Tigr doesn't look to be very flexible for keyboard input. It is very simple but may not do the trick.
Next one to try: SDL2: https://www.libsdl.org/

---

## Looking at SDL2

Downloaded source from:
https://github.com/libsdl-org/SDL/releases/tag/release-2.30.10

If you are building "the Unix way," we encourage you to use build-scripts/clang-fat.sh in the SDL source tree for your compiler:

mkdir build
cd build 
CC=/where/i/cloned/SDL/build-scripts/clang-fat.sh ../configure
make

the main GIT repo is version 3.0.
Lots of deprecations warnings, but it built.

Well, this is the easier way:

```
bazyar@Jawaids-MacBook-Pro gssquared % brew install sdl2
==> Auto-updating Homebrew...
Adjust how often this is run with HOMEBREW_AUTO_UPDATE_SECS or disable with
```

I'm going to go ahead and scrap the Tigr code because SDL2 looks like it will do the trick.

## Keyboard Logic

What better way to spend Christmas than writing an Apple II emulator?

SDL2 is better. I got the actual character ROM working. Looks pretty good.

Now working on the keyboard.

Currently keyboard code is inside the display code. This is not good.
SDL's use scope is broader than just the display. So we should move this out
into a more generalized module, that calls into display and keyboard 
and other 'devices' as needed.

OK, the SDL2 keyboard code is segregated out properly, and the keyboard code is working pretty well.

If a key has been pressed, its value is in the keyboard latch, 0xC000. The Bit 7 will be set if the strobe hasn't been cleared.
Clear the strobe by writing to 0xC010.

Looking at where I hook this in..

This is the SDL2 documentation for the keycodes:

https://github.com/libsdl-org/SDL/blob/SDL2/include/SDL_keycode.h



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

### High-resolution graphics

Implemented in fake green-screen mode.

Bugs:
[ ] text line 21 does not update in mixed mode.


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
    +     ! : | + 7922 render_text(int, int, unsigned char, bool)  (in gs2) + 488  [0x1006addf8]
    +     ! : | + ! 7078 SDL_UnlockTexture_REAL  (in libSDL2-2.0.0.dylib) + 412  [0x100878ee8]
    +     ! : | + ! : 7075 SDL_UnlockTexture_REAL  (in libSDL2-2.0.0.dylib) + 324  [0x100878e90]

we're doing tons of SDL related calls. They're probably fairly expensive ones.
I'll note that if we're doing a scroll, then we're reading every character on the screen (pure
memory copy is fast), but then writing every character on the screen - which generates a call to 
render_text for every character on the screen.
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

## Dec 28, 2024

The clock cycle emulation is a bit off. I am sleeping the calculated amount. But, at start of each cycle
we need to do:

When CPU is 'booted', we do this:
next_time = current_time + cycle_duration_ticks

do work

when we call "emulate_clock_cycle", we do this:
if (next_time < current_time) {
   clock_slip++;
   next_time = current_time + cycle_duration_ticks;
} else {
   sleep (next_time - current_time);
   next_time = next_time + cycle_duration_ticks;
}

The very first time through we'll likely have a big clock slip, which is fine. But we should
generally not have them otherwise.
I am currently sleeping for cycle_duration_ticks without taking into account the time it takes
to do the work.

Well, it's a little better. I'm hovering pretty close to 1.024 / 2.8mhz.

next: speaker, or lo-res graphics?

## Speaker

Let's look at the audio. This will not be fun.

https://discourse.libsdl.org/t/sdl-mixer-accuracy/25160
https://bumbershootsoft.wordpress.com/2016/09/05/making-music-on-the-apple-ii/

Each tweak of $C030 causes the speaker to pop in, or the speaker to pop out.
If we do the in and out 880 times per second, we'll get an A tone (440 hz)- though not a pleasing round one.
How do we convert this to a WAV file type stream?
a WAV file with a 440hzsquare wave in it, has 96 samples of $2000, followed by 96 samples of $E000.
These are UNSIGNED 16-bit samples. $8000 is 0 (neutral). $e000 is plus $6000, $2000 is minus $6000, i.e. 75% volume.

Presumably it will take a certain amount of time for the speaker cone to travel.
And once we stop pinging the speaker, it stops moving.
Maybe we can simulate that.
We keep track of a direction.
then generate pre-defined samples for predetermined time period.
Use signed samples.
Each hit on the C030 generates either positive samples, or negative samples.
these are added to the current buffer. E.g. if there was a positive, then they hit the
speaker again really fast, it will cancel out the first hit.
hitting it twice in a row one right after the other will cancel out the first completely.
Test this. YES. That is corect. I get a quiet sound, just like the article was saying.

So, we can test different waveforms to add into here. 
How long those waveforms are will determine the size of the buffer.
This will only work at lower virtual cpu speeds. Or, we can slow down the clock for N cycles after a
speaker hit, hoping that stays inside the sound loop. (or does apple iigs software change the timing for this?)
For now, assume 1MHz clock speed.

So the Apple II speaker is: a 1-bit DAC, where every other access is the inverse sign of the previous one.

We assume: a ramp-up time; a hold time; a ramp-down time. The ramp-up and ramp-down are probably very similar.
I don't know the hold time, but, we can try different values. And to go from negative to positive there
will be overall double the ramp time.

4 + 4 + 2 + 216 * (2 + 2 + 2 + 2 + 2 + 2 + 2 + 3) + 3 + 2 
   216 * (17) = 3672

my test code is xd8 (216) iterations at about 20 cycles each iteration, or, 3672 cycles.
An up and down is therefore 7360 cycles. at 1MHz, that is 138 per second. That doesn't sound right.

In any given cycle, if the speaker is hit, add the sample to the buffer at the time index.
The time index for the audio buffer is calculated in 1023000 cycles per second.

The waveform timing won't change with the clock speed, because it's a physical property of the speaker.

https://www.youtube.com/watch?v=-Bjitqh7B0Y

## Dec 29, 2024

ok. SDL_OpenAudioDevice is the function we need for audio streams. When you call it, you set a callback
function. SDL calls it whenever it needs more audio data. So we just need to buffer data to always have some for it.
If we ever don't have enough we'll have to send back 0's (or fill with whatever the last sample was), and this will be
perceived as a skip/glitch in the audio.

Start with a short standalone program. I will return a short square wave every X ms.

Hmm, there is another method. SDL can also do a push method. We call "QueueAudio periodically to feed it
more data. That is the model I was looking for originally. I like the callback model. It is asynchronous -
is it even multithreaded?

ok sample program audio.cpp is generating some blips.
Each time I am being asked for more data, I am sending 4096 samples. 4096 is 10.766 per second. That is awkward.
100 per second would be 441 samples. Try that. 
Yes, that's 100 per second - frame is 10ms. 

What is a reasonable time period I can use for buffering?
We don't want it to be too long, or there will be latency.

the AI says:
#### Summary of the Process

1. Intercept $C030 reads in the emulator:
   1. Toggle an internal speakerState.
   1. Record the time of the toggle as an event.
1. Accumulate toggles over emulated cycles.
1. Resample to the host’s audio rate (e.g., 44.1 kHz) by stepping through time or through the toggle events.
1. Write each sample as a 16-bit signed integer (e.g., +32767 for “high,” –32768 for “low,” or some scaled variant).
1. Feed that buffer to the host OS audio API in blocks (e.g., every 1/60 second or 1/100 second).

This is an interesting approach. however, if we just slammed on $C030 nonstop it would be a lot of memory?
no, only 1/60th of a second's worth. That is maybe on the order of 4262 samples. So a reasonable amount of memory.
Keep the events in a circular buffer.
Each event is just recording the number of cycles.
Each buffer records the current cycle index too.
So then we play through the buffer until we reach the current cycle index.
Each event is laid into our output audio buffer at the appropriate time index.
at 50000 samples per second, each sample is 20us. To simulate a 'tick', we would paint 10 samples
into the output buffer.

For testing, I can dump a time-series log of the events into a file, from inside
the emulator. Then analyze that data in a separate program. "Play it back". Easier to test.
have F8 start and stop the audio recording.

That's working. I can dump the events to a file.
Recording what happens just on a boot - I get exactly 192 events. That is one of those "ah ha"
numbers. 
11871
12402
12933
...
113292

etc.
Each of these is exactly 531 cycles apart.
101421 cycles total duration.
So about 1/10th second.

531 cycles is half the frequency. So, 1062 cycles is a full oscillation.
I read that as 963Hz.

I might be off on my cycle counts a bit. Something to look into.

OK, now let's write the playback code.
Let's generate a 1/60th second buffer each chunk - that will make us have about 6 chunks.
that is 735 samples per chunk.

I think in my math I was slightly off. This might be why:

https://mirrors.apple2.org.za/ground.icaen.uiowa.edu/MiscInfo/Empson/videocycles

```
> Or is the difference too small to be noticeable (hearable? What about
> processor loops that access  $c030? Could you hear a "background-humming"?

If you were trying to generate a precise tone, you would need to allow
for this when determining the number of CPU cycles between accesses to
$C030, i.e. work on the assumption that the CPU is executing uniform
cycles at 1.0205 MHz instead of 1.0227 MHz.
```

Some great resources Apple IIers posted in response to this:

## Dec 30, 2024

OK some of my cycle counts are off. There are no single-cycle instructions, but my
current implementation has all the flag instructions at 1 cycle for instance. So
that's wrong. Also PLA and RTS are 2 cycles too fast.
The following instructions in the beep loop are the wrong cycle counts:
SEC, PLA, RTS

After fixing these, I have 546 cycles between hits on $C030 instead of 531.
That is 2.82% slower through the loop, which should bring the tone down a hair.
However, now my beep is at 926Hz according to the spectrum analyzer.

The SA says the real apple II beep peak is at maybe 937Hz. 915-958 for the whole hump.
Reproduced is 872-969 with peak at 926Hz. Ah, so I need to bring that bottom end up a bit.
That difference is 1.17%, 1.2%.

In the recreation code I have one sample as 2.3us. That's wrong. They're 22.6us.
But I think there is an assumption that 1us = 1 cycle which is not quite right. It's 1% slow. And that's about how far
off we are, 1% low.

Grabbed the "Understanding the Apple II" books. I have some assumptions about clocking that are
not quite right. And would be incredibly hard to replicate. And are probably not the reason for
the differences.

### Audio update


I need to add the concept of an audio buffer.
Each 'event' I need to paint a BLIP worth of samples into the output audio stream. Currently this is 434 samples but it could be an arbitrary number. And, I need to ensure that ALL the samples that are part of a blip, get put into the output, even if they get painted beyond the current audio frame being requested via the sdl2 audio callback.

So, I need an intermediate buffer.

Weird, my speaker_event_log file had the wrong data in it?? Re-ran GS2 with a test of the 
beep routine, and now it's 546 cycles between events as I noted earlier. Weird.

now my "audio working" is too low. Way too low. I went the wrong way.

## Dec 31, 2024

So the audio callback cycle counter is running quite a bit faster than the cpu.
```
4568199 delta 24826466 cycles clock-mode: 2 CPS: 0.913640 MHz [ slips: 8739, busy: 24817727, sleep: 0]
audio_callback 35325884 35342835 0
```

I'm getting 300 audio frames per 5 seconds, that's correct, because I set it up at 60 frames per second.
in that same interval 5,166,383 cycles. 1033276.6 per second average.
But the audio clock ticked up from  25629912 to 30715212 or delta of 5085300.
That's a slight difference but does not account for the discrepancy of 10M cycles.

We want the audio cycle counter to run just behind the cpu cycle counter, such that the 
current cpu cycle count is always greater than the audio cycle count.
So if the audio cycle window count is > cpu_cycle_count, set

buf_start_cycle = cpu_cycle_count - cycles_per_buffer
buf_end_cycle = buf_start_cycle + cycles_per_buffer

see what that does.
We could store time markers as events in the event buffer, and have audio_callback continuously sync to that..

```
    if (buf_end_cycle > speaker_states[0].cpu->cycles) {
        std::cout << "audio cycle overrun resync from " << buf_end_cycle << " to " << speaker_states[0].cpu->cycles - CYCLES_PER_BUFFER << "\n";
        buf_start_cycle = speaker_states[0].cpu->cycles - CYCLES_PER_BUFFER;
        buf_end_cycle = buf_start_cycle + CYCLES_PER_BUFFER;
    } else if (buf_end_cycle < speaker_states[0].cpu->cycles - CYCLES_PER_BUFFER) {
        std::cout << "audio cycle underrunresync from " << buf_end_cycle << " to " << speaker_states[0].cpu->cycles - CYCLES_PER_BUFFER << "\n";
        buf_start_cycle = speaker_states[0].cpu->cycles - CYCLES_PER_BUFFER;
        buf_end_cycle = buf_start_cycle + CYCLES_PER_BUFFER;
    } else {
```

OK, that is keeping it synced, however, I'm getting a lot of overruns and underruns and audio artifacts.
But, the window is staying aligned to the CPU cycle count and I am not getting into situations where
the audio is lagging.

My alternative here is to push audio stream, instead of having SDL2 pull it.

I made a loader function - passing -a filename will load a file into RAM at 0x0801, assuming applesoft basic.
That let me load Lemonade Stand. he he. Lots of sound in there, really shows the issue with audio sync.

I have added some extra 'guardrails' around the buffering, and I improved results and reduced underrun and overrun. But I'm still not entirely happy with the results. I'm going to have to just grind out some hard thinking on it.

And, it definitely only works in 1MHz. Actually, let's see what insanity ensues at free-run..
(I'm wondering if I can try syncing not to hard-set 1.0205 but to whatever the specified clock speed is)
Ah, at 2.8 it's sort of legible but super fast. And at free run the entire segment plays a short tone and then
skips to the next screen in under a second, lol.

https://github.com/kurtjd/rust-apple2/blob/main/src/sound.rs

This is an apple 2 emulator written in Rust, and this is the sound code.

** ok, well now. I took out the display optimization. effective cycles per second is 1.01xxxmhz. I think the audio is working better.
[ ] So it's due to the cpu clock running slightly faster than the audio timing.


## Lo-Res Graphics

Let's do a sprint to get LORES:
https://www.mrob.com/pub/xapple2/colors.html
https://sites.google.com/site/drjohnbmatthews/apple2/lores

```
                 --chroma--
 Color name      phase ampl luma   -R- -G- -B-
 black    COLOR=0    0   0    0      0   0   0  0x00000000
 red      COLOR=1   90  60   25    227  30  96 
 dk blue  COLOR=2    0  60   25     96  78 189
 purple   COLOR=3   45 100   50    255  68 253
 dk green COLOR=4  270  60   25      0 163  96
 gray     COLOR=5    0   0   50    156 156 156
 med blue COLOR=6  315 100   50     20 207 253
 lt blue  COLOR=7    0  60   75    208 195 255
 brown    COLOR=8  180  60   25     96 114   3
 orange   COLOR=9  135 100   50    255 106  60
 grey     COLOR=10   0   0   50    156 156 156
 pink     COLOR=11  90  60   75    255 160 208
 lt green COLOR=12 225 100   50     20 245  60
 yellow   COLOR=13 180  60   75    208 221 141
 aqua     COLOR=14 270  60   75    114 255 208
 white    COLOR=15   0   0  100    255 255 255
 
 black    HCOLOR=0   0   0    0      0   0   0
 green    HCOLOR=1 225 100   50     20 245  60
 purple   HCOLOR=2  45 100   50    255  68 253
 white    HCOLOR=3   0   0  100    255 255 255
 black    HCOLOR=4   0   0    0      0   0   0
 orange   HCOLOR=5 135 100   50    255 106  60
 blue     HCOLOR=6 315 100   50     20 207 253
 white    HCOLOR=7   0   0  100    255 255 255
```
 Black [0], Magenta [1], Dark Blue [2], Purple [3], Dark Green [4], Dark Gray [5], Medium Blue [6], Light Blue [7], Brown [8], Orange [9], Gray [10], Pink [11], Green [12], Yellow [13], Aqua [14] and White [15]

```
 0 - 0x00000000
 1 - 0x8A2140FF
 2 - 0x3C22A5FF
 3 - 0xC847E4FF
 4 - 0x07653EFF
 5 - 0x7B7E80FF
 6 - 0x308EF3FF
 7 - 0xB9A9FDFF
 8 - 0x3B5107FF
 9 - 0xC77028FF
 10 - 0x7B7E80FF
 11 - 0xF39AC2FF
 12 - 0x2FB81FFF
 13 - 0xB9D060FF
 14 - 0x6EE1C0FF
 15 - 0xFFFFFFFF
```
OK that was largely working. 

[x] Now, however, I have a bug where when I type 'GR' it only partially clears the screen. There must be a race condition somewhere. (Fixed. in text_40x24.cpp:txt_memory_write I was not setting dirty flag on line when in lores mode.) (This has a hack in in regarding hi-res mode).



## Cassette Interface

On the one hand this seems silly. On the other hand, it is vintage cool, and there are actually web sites that distribute audio streams of old games stored in audio files, that you can play into an Apple II.

https://retrocomputing.stackexchange.com/questions/143/what-format-is-used-for-apple-ii-cassette-tapes

The Apple II recorded data as a frequency-modulated sine wave. A standard consumer cassette deck could be connected to the dedicated cassette port on the Apple ][, ][+, and //e. The //c, ///, and IIgs did not have this port.

A tape could hold one or more chunks of data, each of which had the following structure:

Entry tone: 10.6 seconds of 770Hz (8192 cycles at 1300 usec/cycle). This let the human operator know that the start of data had been found.
Tape-in edge: 1/2 cycle at 400 usec/cycle, followed by 1/2 cycle at 500 usec/cycle. This "short zero" indicated the transition between header and data.
Data: one cycle per bit, using 500 usec/cycle for 0 and 1000 usec/cycle for 1.
There is no "end of data" indication, so it's up to the reader to specify the length of data. The last byte of data is followed by an XOR checksum, initialized to $FF, that can be used to check for success.

For machine-language programs, the length is specified on the monitor command line, e.g. 800.1FFFR would read $1800 (6144) bytes. For BASIC programs and data, the length is included in an initial header section:

 * Integer BASIC programs have a two-byte (little-endian) length.
 * Applesoft BASIC has a two-byte length, followed by a "run" flag byte.
 * Applesoft shape tables (loaded with SHLOAD) have a two-byte length.
 * Applesoft arrays (loaded with RECALL) have a three-byte header.
Note the header section is a full data area, complete with 10.6-second lead-in.

The storage density varies from 2000bps for a file full of 0 bits to 1000bps for a file full of 1 bits. Assuming an equal distribution of bits, you can expect to transfer about 187 bytes/second (ignoring the header).

An annotated 6502 assembly listing, as well as C++ code for deciphering cassette data in WAV files, can be found here. The code in the system monitor that reads and writes data is less then 200 bytes long.


## Pascal Firmware Protocol

The Pascal protocol seems to be geared towards I/O, like serial devices, and displays.
However, there are "device class" assignments for :
Printer, Joystick, Serial/parallel, modem, sound/speech device, clock, mass-storage, 80 column card, network or bus interface, special purpose, and reserved.

Unclear how widely-supported this is.

## Generic Disk Interface - proposed

### Registers

Propose:

```
C0S0 - lo byte of sector number
C0S1 - hi byte of sector number
C0S2 - target address lo byte
C0S3 - target address hi byte
C0S4 - sector count
C0S5 - command. 0x1A = read, 2C = write.
```

Someone could easily accidentally write garbage to the disk by strafing these registers. Give some thought
on how to prevent that... Have the commands be values unlikely to be written by accident.
Require a specific double-strobe. Something like that.
Ah. Write values in. Strobe. Write command. Strobe.
if not done in this sequence, command is ignored.
Then the data is DMA'd into memory.

### Firmware
$CS00 - boot.
$CS??


## Disk II Interface

First, ROMs:

https://mirrors.apple2.org.za/ftp.apple.asimov.net/emulators/rom_images/

   * $C0S0 - Phase 0 Off
   * $C0S1 - Phase 0 On
   * $C0S2 - Phase 1 Off
   * $C0S3 - Phase 1 On
   * $C0S4 - Phase 2 Off
   * $C0S5 - Phase 2 On
   * $C0S6 - Phase 3 Off
   * $C0S7 - Phase 3 On
   * $C0S8 - Turn Motor Off
   * $C0S9 - Turn Motor On
   * $C0SA - Select Drive 1
   * $C0SB - Select Drive 2
   * $C0SC - Q6L
   * $C0SD - Q6H
   * $C0SE - Q7L
   * $C0SF - Q7H

So, the idea I have for this:
each Disk II track is 4kbyte. I can build a complex state machine to run when
$C0xx is referenced. Or, when loading a disk image, I can convert the disk image
into a pre-encoded format that is stored in RAM. Then we just play this back very
simply, in a circle. The $C0xx handler is a simple state machine, keeping track
of these values:
   * Current track position. track number, and phase.
   * Current read/write pointer into the track
   

Each pre-encoded track will be a fixed number of bytes.

If we write to a track, we need to know which sector, so we can update the image
file on the real disk.

Done this way, the disk ought to be emulatable at any emulated MHz speed. Our 
pretend disk spins faster along with the cpu clock! Ha ha.

### Track Encoding

At least 5 all-1's bytes in a row.

Followed by :

```
Mark Bytes for Address Field: D5 AA 96
Mark Bytes for Data Field: D5 AA AD
```

### Head Movement

to step in a track from an even numbered track (e.g. track 0):
```
LDA C0S3        # turn on phase 1
(wait 11.5ms)
LDA C0S5        # turn on phase 2
(wait 0.1ms)
LDA C0S4        # turn off phase 1
(wait 36.6 msec)
LDA C0S6        # turn off phase 2
```

Moving phases 0,1,2,3,0,1,2,3 etc moves the head inward towards center.
Going 3,2,1,0,3,2,1,0 etc moves the head inward.
Even tracks are positioned under phase 0,
Odd tracks are positioned under phase 2.

If track is 0, and we get:
Ph 1 on, Ph 2 on, Ph 1 off
Then we move in one track to track. 1.

So we'll want to debug with printing the track number and phase.

When software is syncing, it's just going to look for 5 FF bytes in a row
from the read register, followed by the marks. That's basically it.
For handling writing, we might want to have each sector in its own block and up
to a certain 

We'll be able to handle both 6-and-2 and 5-and-3 scheme in case anyone wants this -
it would be registering a different disk ii handler that would set a flag.
In theory this scheme could let you nibble-copy disk images too.

The media is traveling faster under the head when the head is at the edge;
slower when under the center. The time for a track to pass under the head is
the same regardless of which track the head is over. In the physical world,
this means the magnetic pulses are physically longer when the head is at the edge.
In our emulated world, it should be roughly the same number of bits per ms
regardless of position. So each track should be the same number of pre-encoded bytes.

If the motor is off, we stop rotating bits out through the registers.

OK, we have a 6:2 encoding table.

# Jan 1, 2025

## Apple IIe Dinking

I implemented some infrastructure to automatically fetch and combine ROMs from the Internet
for use by the emulator.

The Apple IIe ROMs aren't working, because we don't have correct memory mapping in place
for the ROMs.

Apple IIe Technical Reference Manual, Pages 142-143, cover this, but it's dense and the diagram
doesn't exactly match the text.

Booting up, we see this (we didn't get very far, lol):
```
 | PC: $FE84, A: $00, X: $00, Y: $00, P: $00, S: $A5 || FE84: LDY #$FF
 | PC: $FE86, A: $00, X: $00, Y: $FF, P: $80, S: $A5 || FE86: STY $32   [#FF] -> $32
 | PC: $FE88, A: $00, X: $00, Y: $FF, P: $80, S: $A5 || FE88: RTS [#FA65] <- S[0x01 A6]$FA66
 | PC: $FA66, A: $00, X: $00, Y: $FF, P: $80, S: $A7 || FA66: JSR $FB2F [#FA68] -> S[0x01 A6]$FB2F
 | PC: $FB2F, A: $00, X: $00, Y: $FF, P: $80, S: $A5 || FB2F: LDA #$00
 | PC: $FB31, A: $00, X: $00, Y: $FF, P: $02, S: $A5 || FB31: STA $48   [#00] -> $48
 | PC: $FB33, A: $00, X: $00, Y: $FF, P: $02, S: $A5 || FB33: LDA $C056   [#00] <- $C056
 | PC: $FB36, A: $00, X: $00, Y: $FF, P: $02, S: $A5 || FB36: LDA $C054   [#00] <- $C054
 | PC: $FB39, A: $00, X: $00, Y: $FF, P: $02, S: $A5 || FB39: LDA $C051   [#00] <- $C051
 | PC: $FB3C, A: $00, X: $00, Y: $FF, P: $02, S: $A5 || FB3C: LDA #$00
 | PC: $FB3E, A: $00, X: $00, Y: $FF, P: $02, S: $A5 || FB3E: BEQ #$0B => $FB4B (taken)
 | PC: $FB4B, A: $00, X: $00, Y: $FF, P: $02, S: $A5 || FB4B: STA $22   [#00] -> $22
 | PC: $FB4D, A: $00, X: $00, Y: $FF, P: $02, S: $A5 || FB4D: LDA #$00
 | PC: $FB4F, A: $00, X: $00, Y: $FF, P: $02, S: $A5 || FB4F: STA $20   [#00] -> $20
 | PC: $FB51, A: $00, X: $00, Y: $FF, P: $02, S: $A5 || FB51: LDY #$08
 | PC: $FB53, A: $00, X: $00, Y: $08, P: $00, S: $A5 || FB53: BNE #$5F => $FBB4 (taken)
 | PC: $FBB4, A: $00, X: $00, Y: $08, P: $00, S: $A5 || FBB4: PHP [#30] -> S[0x01 A5]
 | PC: $FBB5, A: $00, X: $00, Y: $08, P: $00, S: $A4 || FBB5: SEI
 | PC: $FBB6, A: $00, X: $00, Y: $08, P: $04, S: $A4 || FBB6: BIT $C015   [#EE] <- $C015
 | PC: $FBB9, A: $00, X: $00, Y: $08, P: $C6, S: $A4 || FBB9: PHP [#F6] -> S[0x01 A4]
 | PC: $FBBA, A: $00, X: $00, Y: $08, P: $C6, S: $A3 || FBBA: STA $C007   [#00] <- $C007
 | PC: $FBBD, A: $00, X: $00, Y: $08, P: $C6, S: $A3 || FBBD: JMP $C100
 | PC: $C100, A: $00, X: $00, Y: $08, P: $C6, S: $A3 || C100: INC $EEEE $EEEE   [#E4]
```
 So, it's trying to enable "Internal ROM at $CX00" when it hits $C007:

 "When SLOTCXROM is active (high), the IO memory space from $C100 to $C7FF is allocated
 to the expansion slots, as described previously. Setting SLOTCXROM inactive (low) disables
 the peripheral-card ROM and selects built-in ROM in all of the I/O memory space
 except the part from $C000 to $C0FF (used for soft switches and data I/O).
 In addition to the 80 col firmware at $C300 and $C800, the built-in ROM
 includes firmware that performs the self-test of the Apple IIe's hardware.

So it's trying to enable the internal ROM. Which I have loaded, but I am not exposing that
part of the memory page.

This brings up the question of whether page sizes should be 4K as I was originally thinking,
or 256 bytes. There's a bunch of stuff here that gets paged in on 256 byte boundaries.
I guess I could add some logic to bus.c to try to handle this.

Should review all the 80-column stuff too. Might as well jump in and document all the "extra
memory bank" related stuff, and go straight from where we're at with 64K, to 128K.


## Apple II (non-plus) dinking

Going the other direction, loading Apple II (original) f/w on the emulator works.
Dumps you into monitor ROM, and, ctrl-B into Integer BASIC!

F666G puts you into the mini-assembler, which is especially fun.

This is an Integer basic game :

https://en.wikipedia.org/wiki/Integer_BASIC

that would be a pain in the ass to type in, so, was thinking about a copy/paste feature
to paste data into the emulator.

Since I've started this Platform work, there are some things to think about:

1. Even to support full Apple IIe functionality, we're going to need the ability to handle more than
64K of memory. This is done with switches, and the 'bus' is still only 16 bit.
1. The Platform concept should include selection of the CPU chip, and perhaps certain hardware. Though
we probably want to have some separation so we can have "Any card in any platform in any slot".



# January 2, 2025

I have an interesting concept, of extensive use of function pointers in structs / arrays.
For example, when defining the Apple II Plus platform, we would have a struct that
is a list of all the devices that need to be initialized. They would be executed in order.
A different platform could have a different set.
All the devices would interface specifically through the Bus concept - just like they do in the
real systems, and, must not have any cross-dependencies. And then all the devices
code could be 100% shared between different platform definitions.
A device class would be composed of:
  initialization method
  de-initialization method

They tie into the system through:
  bus_register
  bus_unregister
  timer_register
  timer_unregister

timer_register is something that's hardcoded in the gs2 main loop right now.
But we could maintain a ordered event queue of function pointers to call.
Let's say we've got video routines 60 times a second; and another thing that needs to
happen 10 times a second, etc. Each time a handler ran, it would reschedule itself
for a next run, as appropriate.

To integrate into SDL events, build a data structure that would have callbacks based on certain event types.

This is a goal, before we get too awful many devices into this thing.

At a little higher level, we'll have a slot device class. These will be called with a slot number.
And then register themselves into the system as appropriate.

## January 3, 2025

Thinking about display a little bit.
We're going to need to have the native display be 560x192 pixels. This is for a few reasons.
1) handle 80 column text. that's 7 pixels x 80 = 560.
2) handle double hi-res. This is 560x192.
3) even with standard hi-res, if we want to simulate the slight horizontal displacement of different color pixels, then
we need to have 560 pixels horizontally. This actually sort of simplifies HGR display.

The implication is that either I change the texture scale horizontally by a factor of 2
(e.g., in 40-column modes it would be 8x 4y scaling), or, I can just write twice
as many pixels horizontally to the texture.

Yes. We would still define the texture as 560x192.
In 40-col text and regular lo-res mode, we would only write pixels into 280x192, and SDL_RenderCopy 
double (see below) to stretch it.
In 80-col mode, double lo-res mode, and hi-res mode, and double hi-res mode, we would write pixels into 560x192
and copy w/o stretching.

This implies we need to keep the screen mode per line. i.e. each of the 24 lines would be:
   lores40
   lores80
   text40
   text80
   hires
   dhires

The display overall would also have a color vs monochrome mode.
And we need to keep track of the "color-killer" mode: in mixed text and graphics mode, the text is color-fringed. In pure text mode, the display is set to only display white pixels. Actually this implies that even on lines in text mode, we might need to draw the text with the slight pixel-shift and color-fringed effect. In which case, all buffers would always be 560 wide. And, we would likely post-process the color-fringe effect. I.e., draw a scanline (or, 8 scanlines), then apply filtering to color the bits. Then render.

In monochrome mode, a pixel is just on or off. no color. But, probably still pixel-shifted.

Depending on how intricate this all gets, we may even need to use a large pixel resolution than 560x192. Would have to read up on the Apple II NTSC stuff in more detail.

```
// Source rectangle (original dimensions)
    SDL_Rect srcRect = {
        0,      // X position
        y * 8,  // Y position
        280,    // Original width
        8       // Height
    };

    // Destination rectangle (stretched dimensions)
    SDL_Rect dstRect = {
        0,      // X position
        y * 8,  // Y position
        560,    // Doubled width (or whatever scale you want)
        8       // Height
    };

    SDL_RenderCopy(renderer, texture, &srcRect, &dstRect);
```

Example hires bytes:

  01010101 - 55. Purple dots. even byte (0,2,etc)
  00101010 - 2A. Purple dots. odd byte. (1,3,etc)

  11010101 - D5. Blue dots. even byte (0,2,etc)
  10101010 - AA. Blue dots. odd byte. (1,3,etc)

Apple2TS does not do color fringing on text. 

A very thorough hires renderer:

https://github.com/markadev/AppleII-VGA/blob/main/pico/render_hires.c?fbclid=IwY2xjawHlXzFleHRuA2FlbQIxMAABHY4HQtJ1e_ODqLnjQ3SEOp4Z_js9qJa7JArXYQJj-KmLULgEYBUDO24zUQ_aem_3K8UGHDmTyjZzMBticL4uA#L6

This is a decoder for how a bunch of things are supposed to look:
https://zellyn.com/apple2shader/?fbclid=IwY2xjawHlYI1leHRuA2FlbQIxMAABHWRHc9TVWKrSuP0JBIeQHS4OVXc7_nCmcu2hdOl6c-vtCk2Aiit1uXNFEA_aem_5LQvZKXfjFgJFIU0zM5WnQ

## Jan 4, 2025

The DiskII code is now correctly generating a nibblized disk image in memory.
I have tested this two ways, first, by comparing encoded sectors to the example
in decodeSector.py. Second, by taking an entire disk and inspecting it with 
CiderPress2. This has the ability to read nibblized disk images and pull data
out of them, catalog them, etc. Slick.

Getting ready to write the read register support, then, we ought to be able to
boot a bloody DOS 3.3 image!

In preparation for that, I took the code I was working on which was a separate
set of source files and executable, and moved it into a new folder structure,
src/devices/diskii. There is also apps/nibblizer. There's a hierarchy of Makes
and Cmake apparently makes it very easy to turn some of these into libraries
and link them into the main executable. This means the diskii_fmt files are now
shared between the main executable and the nibblizer executable. Slick.

I also have had some thoughts about how to do runtime configuration and linking of
a user interface with the engine. I was thinking, some kind of IPC mechanism.
If we use web tech like JSON to an API, that could open up some very interesting
possibilies. It would allow remote control over the internet; it would allow 
programmatic / scripted runtime configuration of the system, which could really
help automate testing. This could work kind of like the bittorrent client,
where the configurator is just a web app.

[ ] DiskII should get volume byte from DOS33 VTOC and 'format' the disk image with it.

[ ] Create struct configuration tables for memory map for each platform.

[ ] There's an obvious optimization in the DiskII area - we don't have to shift the bits out of the register. We can just provide the full byte with the hi-bit set. This would 
probably work and speed up disk emulation mightily.

## Jan 5, 2025

Well as of this morning I'm booting disk images!

However, they are exceptionally slow operating. I think it must have to do with the interleaving.
I am laying them out in memory in logical disk order, but with the sector numbers mapped
to interleave. I think that's insufficient.

Check out my CiderPress2 generated .nib image and see what order the sectors are in.

yeah, physical sectors numbers in the CP2 image are: 0, 1, etc. So not only do they change the sector numbers
in the address field, but, we need to reorder the data so the physical sectors in the stream are in the physical sector order:
i.e., 0, 1, 2, 3, etc.

original way

gs2 -d testdev/disk2/disk_sample.do  50.24s user 0.40s system 98% cpu 51.397 total

new way (physical order)

gs2 -d testdev/disk2/disk_sample.do  47.40s user 0.35s system 99% cpu 48.173 total

That's not much difference - 3 seconds.

Here's some more potential stuff:

From Understanding the Apple II. page 9-37:
"Is is possible to check whether a drive at a slot is on by configuring for reading
data and monitoring the data register. If a drive is turned on, the data register will
be changing and vice-versa."

So when the disk is off, it is turning it on and waiting a second for it to spin up.

Understanding the Apple II. page 9-13: "The effect of this timer is to
delay drive turn-off until one second after a reference to $C088,X".

So I need to emulate the timer. Each iteration through the DiskII code, check
cpu cycles from "mark_cycles_turnoff". if it's non-zero, then see if it's
been one second. Only then mark the motor disabled. On a reset the delay timer
is cleared and drive turned off almost immediately."

So, simulate this timer. Because I think the II is waiting one second every time it
turns the disk motor on.

We will need a "list of routines to call on a reset" queue. Disk:
"RESET forces all disk switches to off, and clears the delay timer."

There is also this: "Access to even addresses causes the data register contents to be
transferred to thedata bus."

my attempt to delay motor off is not working, its causing the disk to chugga chugga.
Don't forget I'm starting on track 13 every time to be silly. Would help a little
to have it start at track 0.

None of this has really helped. here's what I'm thinking. say I'm in an apple ii.
I read a sector. I have to go off and do stuff with it.
by the time I come back to read the next sector, the disk has spun around a circle
for a while. But my emulator hasn't moved the virtual head. So all this interleave 
stuff is just wrong. 

I need to simulate the disk continuing to spin while we're away. So, between reads
from the disk, I need to spin the disk that many nybbles.

Never mind. I disabled my "accelerator" and now it boots fast. That code was causing a big
mismatch between the simulated disk and cycles going by - it was messing with the interleave
timing.

Now we're at 11 seconds!

This may yet be an issue when writing. Have to take a look at the write code in DOS.
And, of course, we'll need a denibblizer routine. I bet I can steal that from Woz too!

A ton of software requires 64K. So I guess that's the next thing to do.

I am thinking, a more generalized event queue is needed. For instance, the disk needs to turn
off one second after the command to turn it off is actually sent. Right now I am cheating
and checking cpu cycles but only when the disk registers are read. Software is going to want
to turn it off and then NOT READ REGISTERS for a while.

Of course I also have timing routines related to video and audio. So that's where the event queue
comes in. It needs to be -ordered-, when we read out of it.

It's going to be the following:
cycle count to trigger at
function pointer to call
argument 1 uint64_t
argument 2 uint64_t

This will be part of the CPU struct, so the actual function can be called with the CPU pointer.

[ ] Add 'Language card' / 64K support.
[ ] Add event queue system.
[ ] Add Disk II write support.
[ ] Add .nib and .po format support.
[ ] Note infinite loop on the console, but do not terminate.


### Language Card:

4K RAM Bank 1: D000 - DFFF
4K RAM Bank 2: D000 - DFFF

10K RAM: E000 - F7FF
2K RAM: F800 - FFFF
Language ROM: F800 - FFFF

So thinking about 256 byte memory map. In the IIe like the stack page can be swapped
between main and aux memory.

So, 64K memory map is 256 pages of 256 bytes. Create memory table with 256 byte pages.

For GS, we could have a multi-level memory map that lets us have two different page sizes. An extension to do
later.

Separately allocate the actual memory areas and assign them to variables that exist independently of the memory map.
Because we will need to swap these in and out.

So for instance, for the II+ we'll do:
  * Main_RAM = alloc(48K)
  * Main_IO = alloc(4k)
  * Main_ROM = alloc(12K)

The pages do not have to be allocated separately. Contigious is fine.

Then assign subsets of these to the memory map in 256 byte increments. 

For language card, we'll have:
  * Bank_RAM_4K_1 = alloc(4K)
  * Bank_RAM_4K_2 = alloc(4K)
  * Bank_RAM_6K = alloc(6K)
  * Bank_RAM_2K = alloc(2K)
  * Bank_ROM = alloc(2K)

And then memory map altered when softswitches are toggled appropriately.
We will want some external functions to mediate this, in bus.cpp.
It seems straightforward how a card would swap its pages in. How would we swap back? Does it save the old entries? hm, yes, I think it does. On init_card, it will save the memory map entries that were present at init time. 
  * OR, 3rd: store the main system memory blocks into well-known variable names.

ok I'm gonna cut for the night - I have the new memory map implemented, and have gs2 booting again. I think the language card hooks are in, and now need to start testing that.

I ended up allocating all in one big 64K chunk. That's more like the real thing and is less to keep track of.

## Jan 6, 2025

Some of the memory management might be a lot easier if I think about it this way:
instead of allocating all these different chunks, do:
  * 1 64KByte chunk for RAM.
  * 1 12KB chunk for ROM.

Then I map the effective address space into the appropriate bits of these chunks.
E.g.:

1st 48K is mapped straight.
```
Then D000 bank 1 => RAM[0xC000]
D000 bank 2 => RAM[0xD000]
```

Also this is more likely closer to how the apple IIe does it - there aren't all
these separate RAM chips. they just had a 64K bank of RAM. This is because the same
patterns apply to aux memory (the additional 64K chunk there.) Then we can just have
aux memory be treated the same way.

OK, running the Apple II Diagnostic disk. I have C083 * 2 'good', C083 'bad'. I think this
is because the manual says we are supposed to read C083 TWICE to switch into RAM/RAM read/write
mode. What constitutes reading twice in a row? This is almost certainly some protection against
accidental reads of C083.

The first time C083 is hit, it's the same as hitting C080 - ram read, no writing. Hitting C083 a second time enables writing to RAM.

ok, it passes the diagnostic test now!!!

Trying to boot ProDOS - I got the ProDOS boot screen and blip!
but infinite loop doing this:
```
Thunderclock Plus read register C090 => 00
Thunderclock Plus read register C090 => 00
Thunderclock Plus read register C090 => 00
Thunderclock Plus read register C090 => 00
```
turned that off..
now fails here:
```
schedule motor off at 15126338 (is now 14376338)
languagecard_read_C08B - LANG_RAM_BANK1 | RAM_RAM
bank_number: 1, reads: 1, writes: 0
page: D0 read: 0x148014000 write: 0x14700ae00 canwrite: 0
...
languagecard_read_C08B - LANG_RAM_BANK1 | RAM_RAM
bank_number: 1, reads: 1, writes: 1
page: D0 read: 0x148014000 write: 0x148014000 canwrite: 1
...
Unknown opcode: FD6F: 0x82CPU halted: 1
```

it's running from ram.

So it's loaded code into page D6 and then hit invalid opcode 0x82.
When we hit the system halt, I should be able to dump memory and various things.



Hm, this could be a disk order thing.. maybe I can check bytes in the image to detect if it's DOS 3.3 or ProDOS and implement the correct interleave? doesn't seem likely..

In the debug prints, display the buffers in more informative way. Instead of raw pointer, say "main_ram(DE)" to specify main_ram page DE. Make a little subroutine for it.

Carmen Sandiego got a little bit further. Wants me to put in disk 2. now what?! hehe.

I don't think hires page 2 is working.. Hm, no, it is.

Something is still not quite working with the memory map stuff.

OK, Thunderclock utils disk downloaded. These things want "thunderclock plus firmware" to be present to work. 

I should also be able to freeze execution any other time and do the same.
And disassemble code. So, I need a system monitor app.
I could do a running disassembly, i.e. have the last 10 or 20 instructions in a circular buffer,
dumped also.


So, various things are trying to load Integer basic, but then failing to do so. There is some sort of test that's being done that fails, for if there is a language card present. The first thing I note, is that this hits:

C081, then C081 again 4 cycles later. and Apple IIe manual has "RR" next to C081. I think all the modes that enable RAM writes, require the double-read in order to turn on RAM writes. Let's go ahead and try that.

Apparently Locksmith 6.0 has a "really good language card tester".

Apple2ts , when it hits C08B once, switches immediately to R/W RAM Bank 1 for D0 - FF. That is not what the manual says. Maybe the manual is wrong.
also, when it hits C081, I can wait a very long time before hitting again and then it switches to read rom write ram.
and then C08B switches immediately to RW RAM Bank 1. wut.
c081 c08b goes to rw ram bank 1.
c089 by itself does nothing. again does read rom write ram.
So the 2nd read doesn't have to be the same switch. It can be any of the other double switches.
and C08b and c083 go immediately to read ram.
(They also trigger c100-c7ff / and c800-cfff 'peripheral' instead of 'internal ROM', which must be an Apple IIe thing.)


Notes from apple2ts

```
  // All addresses from $C000-C00F will read the keyboard and keystrobe
  // R/W to $C010 or any write to $C011-$C01F will clear the keyboard strobe

  if (addr >= 0xC080 && addr <= 0xC08F) {
    // $C084...87 --> $C080...83, $C08C...8F --> $C088...8B
    handleBankedRAM(addr & ~4, calledFromMemSet)
    return
```

There appears to be an extensive discussion of this stuff in Understanding the Apple II.
Chapter 5, page 5-26, The 16K RAM CARD

ok, going by this, my implementation is wrong, and misses the effect writes can have. This
was not documented by Apple. Bad Apple.

Sather discusses a number of flip-flops (state bits):
Bank_1 flip flop. 0 = bank 2, 1 = bank 1.
Read_Enable flip flop. 1 = enable read from expansion RAM. 0 = enable read from ROM.
Write_Enable' flip flop. 0 = enable write to expansion RAM. 1 = no write.
Pre_Write flip flop.

the write flip flops can be thought of as a write counter which counts odd read accesses
in the C08X range. The counter is set to zero by even or write access in the C08X range.
If the write counter reaches the count of 2, writing to expansion RAM is enabled.
Writing will stay enabled until an even access is made in the C08X range.

```
at power on:
Bank_1 = 0
Pre_Write = 0
Read_Enable = 0
Write_Enable' = 0 ()
 - says Reset has no effect. I bet they changed this.

A3
= 0, C080-C087 - any access resets Bank_1. 
= 1, C088-C08F - any access sets Bank_1.

A0/A1
==00, 11 : C080, C083, C084, C087, C088, C08B, C08C, C08F - set READ_ENABLE.
==01, 10 : C081, C082, C085, C086, C089, C08A, C08D, C08E - reset READ_ENABLE.

PRE_Write = 1
then can reset WRITE_ENABLE'
```

Pre_Write is set by Odd Read Access in C08X range.
Pre_Write is reset of Even access in range, OR a write access anywhere in C08X range.

WRITE_ENABLE' is reset by an odd read access in C08X when PRE_Write is 1.
WRITE_ENABLE' is set by an even access in C08X range.

So, Bank and Read enable are set on both reads and writes. That was not at all clear.

**I can now successfully boot ProDOS 1.1.1. I can load integer basic on the DOS3.3 master disk.**

## Jan 7, 2025

The JACE (java) emulator has a working Thunderclock implementation, including the thunderclock ROM file.
That's probably what I need to check to fix my implementation.

Looking at generic ProDOS clock implementation.

ProDOS recognizes a clock card if:
```
Cn00 = $08
Cn02 = $28
Cn04 = $58
Cn06 = $70
Cn08 - READ entry point
Cn0B - WRITE entry point
```

The ProDOS routine stores date and time in:
```
$BF91 - $BF90
day: bits 4-0
month: bits 8-5
year: bits 15-9
$BF93 - hour
$BF92 - minute
```

The ProDOS clock driver expects the clock card to send an ASCII
string to the GETLN input buffer ($200). This string must have the
following format (including the commas):
  * mo,da,dt,hr,mn
  * mo is the month (01 = January...12 = December)
  * da is the day of the week (00 = Sunday...06 = Saturday)
  * dt is the date (00 through 31)
  * hr is the hour (00 through 23)
  * mn is the minute (00 through 59)

It doesn't say but presumably the $200 getln ends with a carriage return.

Well this seems very simple. The only question is, how do we trigger a call
into a native routine?
CPU is going to do this:
JSR $Cn08
Cn08: XX YY ZZ
What values can we put there we can capture?
Options.
1) write our own ASM firmware that runs in Cn00. This will read registers.
2) some other option I forgot.

Ah ha! 6.3 of ProDOS-8-Tech-Ref is Disk Driver Routines. Same conversation regarding
"3rd party disk drives".

```
$Cn01 = $20
$Cn03 = $00
$Cn05 = $03
```

if $CnFF = $00, ProDOS assumes a 16-sector Disk II card.
If $CnFF = $FF, ProDOS assumes a 13-sector Disk II card.
If $CnFF <> $00 or $FF, assumes has found an intelligent disk controller.
If Status byte at $CnFE indicates it supports READ and STATUS requests,
ProDOS marks the global page with a device driver whose high-byte is
$Cn and low-byte is $CnFF.
E.g., in Slot 5, that would be $C5<value of $CnFF>. Let's say $CnFF was
23. Then ProDOS would record a device driver address at $C523.

This is the boot code.

So the start of the device driver is:
```
C700    LDX #$20
C702    LDA #$00
C704    LDX #$03
C706    LDA #$00
C708    BIT $CFFF - turn off other device ROM in c8-cf
C70B    LDA #$01
C70D    STA $42
C70F    LDA #$4C
C711    STA $07FD
C714    LDA #$C0
C716    STA $07FE
C719    LDA #$60
C71B    STA $07FF
C71E    JSR $07FF     ; call an RTS to get our page number
C721    TSX
C722    LDA $0100,X
C725    STA $07FF     ; finish writing instruction JMP $Cn60 to $7FD
C728    ASL
C729    ASL
C72A    ASL
C72B    ASL
C72C    STA $43
C72E    LDA #$08
C730    STA $45
C732    LDA #$00
C734    STA $44
C736    STA $46
C738    STA $47
C73A    JSR $07FD
C73D    BCS C75D  - if carry set, error
C73F    LDA #$0A
C741    STA $45
C743    LDA #$01
C745    STA $46
C747    JSR $07FD
C74A    BCS $c75D - if carry set, error
C74C    LDA $0801
C74F    BEQ $C75D - if zero, error
C751    LDA #$01
C753    CMP $0800
C756    BNE $C75D - if not equal, error.
C758    LDX $43
C75A    JMP $0801 - proceed with boot.
C75D    JMP $E000 - jump into basic (or something)
```

So when a JSR to $Cn60 is made, the Apple2TS emulator does its pretend
call. Here it's full of BRK, so, it's not actually executing anything.

Disk driver calls are: STATUS, READ, WRITE, FORMAT.
STATUS
   checks if device is ready for a read or write. If not, set carry,
   and return error in A.

   If ready, clear carry, set A = $00, and return the number of blocks
   on the device in X (low byte) and Y (high byte).

```
DEVICE numbers (physical slot 5)
  S5,D1 = $50
  S5,D2 = $D0
  S1, D1 = $10
  S1, D2 = $90

DEVICE numbers (physical slot 5)
  S6,D1 = $60
  S6,D2 = $E0
  S2, D1 = $20
  S2, D2 = $A0
```

  What are return vlaues for read and write?

Device number is ( (Slot + (D-1)) * 0x10  )

```
Special locations in ROM:
$CnFC - $CnFD - total number of blocks on device
$CnFE - Status byte
  bit 7 - medium is removable
  bit 6 - device is interruptable
  bit 5-4 - number of volumes on device (0-3)
  bit 3 - device supports formatting
  bit 2 - device can be written to
  bit 1 - device can be read from
  bit 0 - device's status can be read (must be 1)
$CnFF - low byte of entry to driver routines
```

Ah, so this lets us do CDROMs and stuff (read-only, removable).

```
Call Parameters: - Zero Page

$42 - Command: 0 = STATUS, 1 = READ, 2 = WRITE, 3 = FORMAT
$43 - Unit number. 
  7  6  5  4  3  2  1  0
+--+--+--+--+--+--+--+--+
|DR|  SLOT  | NOT USED  |
+--+--+--+--+--+--+--+--+
DR = Disk Read
SLOT = Slot number

$44-$45 - buffer pointer. (Start of 512 memory buffer for data transfer)
$46-$47 - Block number - block on disk for data transfer.
Error codes:
$00 - no error
$27 - I/O error
$28 - No device connected
$2B - write protected
```
So, one method to handle this would be to do this:
when we're about to enter the execution loop, if the PC
is $Cn60, or, one of a list of other registered addresses)
then we can call our special handler.
Which needs to do the following:
  * execute
  * pretend to do an RTS

OK this is exactly what Apple2ts does, and it seems reasonable. And it will
be lightning fast.

So, we can implement:
  * ProDOS Disk II Drive Controller (280 max blocks)
  * ProDOS 800k Drive Controller (1600 max blocks)
  * ProDOS Hard Disk Controller ($FFFF max blocks)
I recommend we flush write on every write.
The extended commands for SmartPort would let us do multiple blocks at once,
improving performance there.

ok, I need byte Cn07 to be set to $3C. $00 means "smartport", which I'm not supporting yet. This disk
I have wants a //e or //c. So let's find another image.

Holy grail! I am booting ProDOS 1.1.1 off 800k "disk" media and the ProDOS block driver is working!

ok, tomorrow, implement writing, it should be a nearly trivial addition.

NO DO IT NOW.

## January 9, 2025

So I have this idea for how to handle the infinite loop detection - the 65c02 (or maybe just 816) implemented
an instruction called WAI, that halted processing until an interrupt occurred. So we can do that when we hit an infinite
loop case - trigger a WAI state - this will then not run CPU instructions, but, will still do the other
event loop stuff, and, will sleep for 1/60th of a second at a time, letting the cpu rest.
infinite loops are not that unusual an occurence on old software. Seems reasonable.

I am working on pushing the media_descriptor stuff down into the hardware layer as far as makes sense.
This way there is only one place where media is updated.
Also, we can have a single place in the main loop and/or exit, where we can "unmount" and safely write
media to real disk.

I need to find a variety of 2mg images to test:
  * dos 3.3 image
  * prodos 143k image
  * prodos 800k image
  * hard disk drive image

## January 10, 2025

I have a cool idea. Set up a web site that mirrors the various image sites.
Provide a search engine that lets you search for images by name, description,
or, by the various attributes, AND BY file contents on the files inside the images!
And quickly browse the image contents (catalog), and, view the files inside. Could
use cp2 behind the scenes since it already knows all the image and file formats.
This would be handy for calaloguing my own stuff.

For later reference: Apple III hardware description:
https://groups.google.com/g/comp.sys.apple2/c/_NYADLx16G8/m/MZKv-Y20uTQJ
https://ftp.apple.asimov.net/documentation/apple3/A3SOSReferenceVol.1.pdf

Everything seems straightforward except:
"Extended/enhanced indirect addressing"
https://www.apple3.org/Documents/Magazines/AppleIIIExtendedAddressing.html
also discussed in detail in 2.4.2.2 in the A3 SOS Reference above.

Huge amount of Apple /// info here: https://ftp.apple.asimov.net/documentation/apple3/

There are great disassemblies of Apple III SOS and probably ROM. These can be used to 
answer questions about the hardware.

Apple III disk image formats:
https://retrocomputing.stackexchange.com/questions/12684/what-are-the-most-common-apple-ii-disk-image-formats-and-what-hardware-disk-driv

A whole bunch of Apple III software:
https://www.apple3.org/iiisoftware.html

I started thinking about other Apple II+ things need to be done; 
  * color hi-res.
  * shift-key mod. (Should be super simple, but need software to test).

Shift-key mod. Basically, if shift was pressed with key, then mark PB2 high. What address is this?

More generally: 

https://gswv.apple2.org.za/a2zine/faqs/Csa2KBPADJS.html

[x] Hm, I need to add in handling mapping of the $C800-$CFFF ROM space based on which peripheral card was accessed.

Let's look at the game controller stuff.

| Function | State | Address |
|----------|--------|----------|
| Annunciator 0 | off | $C058 |
| Annunciator 0 | on | $C059 |
| Annunciator 1 | off | $C05A |
| Annunciator 1 | on | $C05B |
| Annunciator 2 | off | $C05C |
| Annunciator 2 | on | $C05D |
| Annunciator 3 | off | $C05E |
| Annunciator 3 | on | $C05F |
| Strobe Output | | $C040 |
| Switch Input | 0 | $C061 |
| Switch Input | 1 | $C062 |
| Switch Input | 2 | $C063 |
| Analog Input | 0 | $C064 |
| Analog Input | 1 | $C065 |
| Analog Input | 2 | $C066 |
| Analog Input | 3 | $C067 |
| Analog Input | Reset | $C070 |


The Analog inputs work like this: hit the reset switch.
Then, read the analog input over and over until the "bit at the appropriate
memory location changes to 0".
About 3millisecond delay. The time it takes to decay is directly proportional
to the resistance across the input.

This is a super-useful looking general purpose - machine identifier, and joystick tester!

Converting a mouse location to a pseudo joystick location shouldn't be too hard. Let's say
we have a sort of "dead" zone in the middle of the virtual display, where the mouse can rest
and the joystick will read 128/128. then it's probably a pretty straight translation to
x/y coordinates of mouse relative to focus window, to a timer.

Well that wasn't bad at all. A weird thing happened playing sabotage, the mouse buttons
stopped firing. Keyboard still worked, and, mouse still steered. So there may be something
weird or it may be a bug in the game crack or something. Choplifter didn't seem to have any
issues. Overall, having the mouse->joystick be linear makes for somewhat slow movement.
Depends on the size of the screen. And I guess you can speed up your mouse movement on the host.

Also, if you zoom off the window and then click, something comes up and covers the screen
and you're way off base. Ah, so what I need is if I click in that window, the mouse locks to that window
and can't leave it.
The mechanics of the analog input simulation work just fine.

OK, the SDL Relative Mouse mode helps. The mouse can't leave. What I did is, when you click
inside the window, it locks mouse to window. Hit F1 to unlock. Shades of deep VMware purple.

in thinking about the shift key mod, it's super easy. However, it needs to tie into the keyboard.
(Just like I did F1 above).
This will be the same with //e, open apple and closed apple which need to map alt to game controller buttons.
But would like it to be a separate module. And there are some other modules that 
tie into the keyboard too where you don't necessarily want the code to handle the *whatever*
in the event loop or keyboard handler. Like, there is keyboard handling for the emulated
machine, and keyboard handling for other stuff.

I think the number of things that will do this is probably relatively small, so create an array
of key values and handler function pointers and just iterate them on a key event.

Todo for this:

[ ] get window dimensions by calling SDL_GetWindowSize
[x] buy a usb joystick and see what is needed for that to work right.


Hm, how hard to go to a fullscreen mode?

## Jan 11, 2025

Push media write protect handling down into the hardware drivers.

have two interfaces for device init.

```
init_mb_DEVICENAME(cpu_state *cpu)
init_slot_DEVICENAME(cpu_state *cpu, int slot)
```

Ultimately a platform definition will include a list of these functions, in order,
to initialize the VM. Pick and choose the ones you want.

ok, did that, then worked on getting this thing to run as a Mac app bundle. That opened a rabbit hole of "where are my resource files? How are resources packaged?"

Learned a lot. A modern Mac app is just a folder with a .app extension. There is some 
metadata, but, this is way better than what they used to call Resource Forks. It's basically just a convention for the Finder. SDL provides some utility functions for finding your Resource folder. This is good for cross-platform. Linux and Windows will then likely work similarly, though Windows you'll register your icon somewhere. Cross that bridge when we get to it.

Also Chat Gippity helped improve structure of the CMakeLists where all the Mac-specific stuff.
Also, changed to require C++17 in order to access std::filesystem and maybe some other stuff.
This will be the start of a bunch of refactoring to make the code more C++17 compliant.
Since I got past my include file location issues of earlier, I can now be consistent and disciplined about using only modern C++ idioms for I/O and other stuff.

So have more thinking to do - for debug output, can write to a file. And create a debug log abstraction. 

[ ] When a VM is off, its window can display the apple logo and the machine name underneath. (e.g. Apple IIe, //c etc.)
[ ] edit the icon so it's square, and, has a transparent background where the white is.

Thinking about UI. Two ways to go here.
1. Do a web browser type, discussed above. Gives access to a broad range of UI tools in-browser, except perhaps the one we really need, which is select local file.
1. Switch to SDL3, which has a variety of UI tools and a bunch of other stuff to boot.
1. Stick with SDL2 and use this: https://github.com/btzy/nativefiledialog-extended and other such things.

I suspect switching to SDL3 now is the way to go, before I get too much further invested in SDL2.
and it's got the file dialog stuff, etc.

It looks like the audio stuff may be a bit of a lift - but, it will be cleaner and thread-safe. Important if I'm going
to do multiple VMs at once.

So UI. I have a vision.

Control Window:

The whole Control Window can be shown/hidden with a key shortcut (e.g. F1)

At the top, we have a menu bar with some basic stuff like help, about, quit.

There is a pane you can open to show the current machine's state: picture of the emulated machine (e.g. an Apple IIe pic, IIc pic, IIgs pic, etc.);
effective MHz; etc

Below that, a pane with some controls: reset button, power off, break into debugger; reverse analog input axes; change display mode (color, green screen amber screen);   

An accordion pane with disk drive images, to provide visual feedback on: disk activity; slot and drive; what disk image is mounted; etc.
Click on an eject button to eject a disk from that drive, which will do a clean unmount. Then, click on the disk image to mount a new image.

An accordion pane with info on any other virtual peripherals where it makes sense. map serial / parallel port to real device with an icon of the virtual device (e.g. a super serial card); choose joystick/gamepad device; choose audio device; we could map a serial port to a TCP IP and port (fun); virtual modem (ATDT1.2.3.4); Each device registers its own icon / UI logic.

An optional debugger window. The debugger provides a part GUI / part CLI interface into the system.

An optional printer output window. Of course you can hook up a real printer. I bet a bunch of Apple II software supports HP PCL, and of course later
and GS stuff will support PostScript. The ImageWriter was ubiquitous. Also Epson MX-80.  

Images and iconography are SVG.


SDL3 - ugh! lot of work.
Looks like there is SDL_SetRenderDrawColor which might let us draw with white pixels but translate to a different color. Maybe good for a "green screen" mode or Amber mode.

I ripped the display code out of the emulator and put it in a separate test program. It's working. That is weird. I must have a memory management bug somewhere.

## Jan 12, 2025

OH there was a comment that "the texture is write-only.":
Warning: Please note that SDL_LockTexture() is intended to be write-only; it will not guarantee the previous contents of the texture will be provided. You must fully initialize any area of a texture that you lock before unlocking it, as the pixels might otherwise be uninitialized memory.

That's the issue, and that's what is different between the main thing and the test thing. The test thing is writing the entire texture.
Let me try writing only part of them.

NO that wasn't the issue. Though it could have been.

The issue was very simple: SDL3 defaults to BLEND. Previously we had set hint to overwrite. So, that was that! What a PITA!
Also, it defaults to fuzzy (linear) scaling. This provides a more old-CRT-like effect, which is common on other emulators. But for certain 
applications you might want to use nearest neighbor scaling, which provides exact-sharp pixel scaling. ultimately provide a toggle for this.

OK, I think the last thing I need to do is to get the audio working.

They changed a few things, including how the callback works. Now, the callback is called, and you use a call PUT to put however many bytes
of audio data you want into the stream. This is likely better. What may be even better yet is to include an audio frame processor
into the main event loop. Now I'm curious, I wonder if I can now open a 1-bit channel.. no. But they do offer floating point samples.
Consider that later.

I can shove as much or as little audio data as I want into the stream. It will take care of synchronizing buffers. So I will
do that. We will return data generated in the cpu cycles between this time and last time.

OK! I got the speaker/audio working again this time as a push-based system. Every 17000 'cpu cycles' I generate an audio frame.
However, over time the audio stream is getting delayed more and more. I.e., Out of sync with realtime. 
This means I'm pushing more data than I should, and the player is getting behind playing it.
So I need to compress the output data stream a little if it gets behind.
How do I know if I'm behind? Calculate how many bytes we sent to the player based on cycles, compared to how many we should have sent
based on realtime.

## Jan 13, 2025

I think I'm mostly done with the SDL3 refactor. I am not happy with the audio stuff. I am going to break
it out into a standalone test program to continue to tune it. There is almost certainly a bug somewhere
when handling blips that cross frame boundaries. But I took a long recording of audio and 
will be able to test iterations much faster this way.

In the meantime, clean up the mess in the code and push into the repo.

Watched a video by Chris Torrance on how to do audio.

Post events: cycles AND time. (or, just time).
Then time to samples.

## Jan 14, 2025

First pass at redoing the audio, as a test program, is done. I take the cycle-event recording,
generate audio data, and output to a WAV file. The WAV file reconstruction is based purely on
cycle timing. However, I think I'm underrunning the buffer, so need to fix that.

Then, I need to add the smarts per the Chris Torrance algorithm.

It will be imporant to fix the overall cycle timing. The last iteration, in the audio I was patching
around that. The timing is running a bit fast.

Let's say I have the event loop:

```
while (true) {
a1:
    execute_next();
       fetch opcode & cycle
       fetch operand & cycle (potentially many times)
a2:
    do_some_event_loop_stuff();
a3:
    video_update();
a4:
    every X cycles, generation audio frame.
}
```

When we 'wait', we want to pause for a time based on the last time. I.e., each cycle, 
calculate the next "wakeup" time. Then, any sleep, is "sleep for wakeup_time - current_time".
The wakeup_time is basically a1 + X. Everything else needs to be ignored.
This will take into account all the time spent doing other stuff in rest of the event loop.
And, keep track of whether we fall behind. If we do, we will need to be sure not to trigger
audio and video updates in the same event loop. (That should be easy to fix, do if else if else
instead of if, if.)

One thing that might be required, is to sleep and sync only every X cycles. Instead of trying to
microsleep every cpu cycle, sleeps are likely to be more accurate if we sleep less often for longer.
This also provides more time to do other stuff in the event loop.

Since audio events are recorded based on the cpu cycles, it shouldn't matter if we run 10 instructions
at full speed then sleep some. And it will all be too fast for humans to notice. Hopefully.

The fastest cycle time required to support is the 2.8MHz of the IIgs. Everything else can be
Ludicrous Speed.

At start of program, last_cycle_count = 0, last_cycle_time = GetTicksNS();
We don't know how many cycles will be executed beforehand, because it depends on the opcode
etc.
But, when we're through with an opcode, however many minimum number of cycles, we keep track of that
with cpu->cycles - last_cycle_count. 

Unless there is a slip event - the next target time should be calculated based on the **original start time**.

next_cycle_time = start_cycle_time + (ns_per_cycle * cycles_elapsed)
WaitNS(next_cycle_time - GetTicksNS())

Let's just do a test program for cycle timing.

Simple loop, I can do busy waits pretty well. I run 30 iterations, and I wake up right on time.
A single printf in the loop causes the timing to be off by a total of 130,000 to 260,000 ns over the 
loop.

## Jan 16, 2025

### Threads vs short bursts of free-run CPU

### Free-run bursts

I think we definitely need to be smarter about how we handle cycle timing.
One issue I was thinking about, is that the time it takes to execute a video frame update
is not 0. It's also not constant. In a real system, it's going on in the background, constantly.
Not just each 1/60th second. I think it makes sense to do this as a separate thread. Same with the audio.
The other general event processing, maybe. Some of that is probably pretty fast.
Certainly, anything that might print to the console is going to be slow as we've seen. 250-500 microseconds.
I've demonstrated that, absent doing things that take a huge amount of time, we can key pretty accurately
on cycle times with the nanosecond precision of SDL_GetTicksNS().

One approach is to separate cpu from other stuff in separate threads. Another, is to bunch up CPU
processing into bursts. I.e. not try to delay and sync every cycle, but instead run a bunch of cycles,
then execute 'stuff', then do a cycle sync.

My concern with this is over things like, say, reading the keyboard. So let's say we read the keyboard in
a tight loop like apples do. The average user might type a key every 1-2 seconds. An Apple II would
wait for keyboard like 500,000 loop iterations during that time. maybe it's going to do other things too. I guess if the
resolution of these is still much higher than human reaction time, it won't be noticeable. But, I still am
unsure about it. I guess the only way is to try it.

Video frames are 60 times per second. Audio maybe 30-60.

So a loop would be:

```
free-run CPU for 17,008 cpu cycles. (60fps @ 1.0205mhz)
video update.
audio update.
whatever else update.
wait/sleep for end of 17,000 cpu cycle.
```

17K cycles is 16,667 microseconds.
or, 16,667,000 nanoseconds. That seems like a fair bit of time to get things done.

We could do this at 100fps or 120fps. (Still only update the video 60fps). That's 8M nanoseconds.
What if we "free run" one instruction, then do other stuff. 1 instruction is anywhere from 2 to 6/7
cycles. That's 2000 to 7000 nanoseconds. A few thousand instructions.

This shouldn't be that hard to implement.

```
last_cycle_count; last_cycle_time;

execute_next();
do_some_other_stuff();
busy wait until (current_time > last_cycle_time + (this_cycle_count - last_cycle_count) * ns_per_cycle)

loop
```

This loop can operate with however many "execute_next" in a row we want. i.e. can have an inner loop
that does X instructions. This will always sync up afterward.

Today we have huge numbers of clock slips because we can't get much of anything done in the one microsecond
we have after the last cycle of an instruction.

Let's try this first.

Weird audio artifacts. Not worried about that right now. Doing a loop every instruction, we are getting a few
thousand slips per minute.

Let's do an inner loop of 10 instructions.

Ha! Now I'm getting almost exactly 300 slips per 5 seconds. That's 60 slips per second, i.e., video/audio frames.

ok, how about 100 instructions. (100-500 microseconds).. 11 to 12 slips per second.
free-run 17000 cycles:
   5119863 delta 75640749 cycles clock-mode: 2 CPS: 1.023973 MHz [ slips: 4, busy: 0, sleep: 0]

virtually no slips. Two weird artifacts: clicks when clicking in and out of the window.
and, on boot, the display didn't properly clear after showing all 0's.

Also note that the frequency check is now reading 1.023973. 

Remember I think clicks are due to underruns. The current speaker code is only ever emitting +1 or -1, but 
I bet the audio system fills in with 0s when it's behind.

Choplifter works well. Taxman did too. I did in fact see some screen flicker with this approach. If we implement
the IIe VBL register, and set it appropriately (hmm, it should basically always be off), then newer stuff
will work well. Notably I didn't get clicks on this run. It probably happens when it starts with timing
slightly wonky. 

ha! Forgot. I'm running the absolutely wrong audio code.

On a clock speed change, we need to bail out of the CPU thread early, to give our algorithm a chance
to resync to the new clock speed.

Overall so far this is working well. Just the video bug on startup.

### Threads

A second option is to do multiple threads. This will add pretty significant complexity, requiring mutex around
shared data structures. Let's examine.

SDL video updates can only occur from the main thread. That's pretty definitive. If video has to be on main
thread, then the CPU is going to have to be an alternate thread.

SDL has some support for inter-thread communication. SDL_RunOnMainThread() requests a callback be run on the main thread. There is also the ability to define custom messages for the event queue, and pass messages between threads this way.

So let's look at the data structures we have, and consider them in a multithreaded context.

Video memory. Video ram is written to by CPU. Then read by video code. As long as the location of the video
RAM doesn't change, then this is one-way data flow and should be thread safe. The CPU thread writing new values
into video memory won't hurt or break the video update process. it could cause flicker. But that's like the
real computer.

Paravirtual disk driver. right now, the cpu is sort of 'halted' and disk I/O performed reading or writing into
CPU RAM, then cpu resumed. This will not likely happen in space between cycles. So CPU would halt, disk I/O
performed in a separate thread, then CPU resume. We also don't want this I/O to tie up the video or audio
output.

For GS, there will be a keyboard buffer. There are several bits of hardware like that. That's all going to be
write at one point and read from another point. Synchronization here not strictly required.

The audio event buffer is like that, too. If over overrun (overfill the buffer) then events just have to get dropped.
But, we should be able to manage this w/o mutexes.

In fact, any mutex here could re-cause the problem we're trying to avoid. So these must be done anywhere
except in the CPU thread.

If we go this second route, then we get our support for multiple VMs much more easily.

Support vertical blanking sync emulation would be a lot easier as a separate thread. And the
IIgs "3200 colors" mode, which is vertical blanking sync on steroids, would also not likely work
without a multi-threaded approach.

Certain devices will have a "cpu context" and an "event loop context". E.g., video is split.
The CPU thread will decode a video address to set the line dirty flag. But it won't do the 
video update itself. Code that runs from memory_write etc also needs to be sure to be short.

## Revisiting the speaker code.

ok now that we have the clocking synced pretty well at 1.0205MHz (1.020571 to be precise),
let's revisit speaker code.

yes, I improved cycle timing in audio3, and the buzzing has gone away while it's running, and it's staying
very well synced.

The trick will be how to make that work at 2.8MHz and in Ludicrous Speed.

## Jan 17, 2025

audio4 (current iteration of the audio test) isn't bad. the timing is good. The algorithm needs to be improved
per Torrance. So. Let's do that, then, change the buffer algorithm to:

1. Call audio_generate_frame() roughly every 17ms. (As best as we can).
1. calculate number of samples to generate based on time elspsed since last call into audio_generate_frame(). Because it's critical that we generate enough samples to keep the buffer from underrunning.
1. map a cpu cycle range to the samples range. This will be based on the effective cycles per second: time_delta / cycles_delta.
1. Iterate the samples.

The naive approach is:
1. Loop number of samples:
    1. count cycles one by one, checking the queue for events.
        1.   If there's an event, switch the sign of contribution.
        1.   add +1 (or -1) for each cycle (i.e. iterate for cycles_per_sample).
    1. emit sample, repeat

Downside of this approach is that we have a loop iteration for every cycle.
It feels like it would be better to iterate only the samples and do some 
math to figure out how many cycles to skip. Let's get this working first,
then see if we can improve it.

When the emulator starts up, and this is a big difference from the audio4 test program,
we immediately encounter clock slips:

```
last_cycle_count: 2 last_cycle_time: 279866291
last_cycle_count: 17011 last_cycle_time: 296535125
 tm_delta: 16669375 cpu_delta: 17009 samp_c: 736 cyc/samp: 23 cyc range: [0 - 17009] evtq: 41 qd_samp: 1470
last_cycle_count: 34019 last_cycle_time: 612695375
queue underrun 0
 tm_delta: 92876800 cpu_delta: 94772 samp_c: 4096 cyc/samp: 23 cyc range: [17010 - 111782] evtq: 62 qd_samp: 0
last_cycle_count: 51029 last_cycle_time: 658447458
 tm_delta: 92876800 cpu_delta: 94772 samp_c: 4096 cyc/samp: 23 cyc range: [111783 - 206555] evtq: 31 qd_samp: 4096
last_cycle_count: 68037 last_cycle_time: 675941458
 tm_delta: 92876800 cpu_delta: 94772 samp_c: 4096 cyc/samp: 23 cyc range: [206556 - 301328] evtq: 31 qd_samp: 10240
last_cycle_count: 85045 last_cycle_time: 693876750
 tm_delta: 92876800 cpu_delta: 94772 samp_c: 4096 cyc/samp: 23 cyc range: [301329 - 396101] evtq: 32 qd_samp: 16384
last_cycle_count: 102056 last_cycle_time: 711035000
 tm_delta: 45802508 cpu_delta: 46737 samp_c: 2020 cyc/samp: 23 cyc range: [396102 - 442839] evtq: 26 qd_samp: 22528
last_cycle_count: 119066 last_cycle_time: 727704875
 tm_delta: 13960042 cpu_delta: 14244 samp_c: 616 cyc/samp: 23 cyc range: [442840 - 457084] evtq: 0 qd_samp: 26568
last_cycle_count: 136075 last_cycle_time: 744373833
 tm_delta: 16596833 cpu_delta: 16935 samp_c: 732 cyc/samp: 23 cyc range: [457085 - 474020] evtq: 0 qd_samp: 25752
last_cycle_count: 153083 last_cycle_time: 761041791
 tm_delta: 16648000 cpu_delta: 16987 samp_c: 735 cyc/samp: 23 cyc range: [474021 - 491008] evtq: 0 qd_samp: 25168
 ```

 The first time through the loop is fine, but then there is a huge delay to get
 back around. 
 There is an initial huge jump. The video is being called. I bet there is a bunch of
 setup required in event_poll(), the video display, etc the first we call them.
 So, call them once before the CPU loop starts.

 ## Jan 18, 2025

```
 last_cycle_count: 2 last_cycle_time: 582707583
last_cycle_count: 17011 last_cycle_time: 640102875
 tm_delta: 1554875 cpu_delta: 1586 samp_c: 69 cyc/samp: 22 cyc range: [0 - 1586] evtq: 41 qd_samp: 1784
last_cycle_count: 34019 last_cycle_time: 656770750
queue underrun 0
```

I've been jiggering around with the audio code. There are lots of assumptions here.
One was, the stream doesn't start automatically, but I think that might have been wrong. The SDL 
docs are inconsistent on this point.

The first time to audio_generate_frame, we do nothing.
The second time through, we're generating only 69 samples because the time delta
is only 1.5 microseconds. so that's why we underrun. Why is this..

the issue is that something near startup is sucking up a lot of time in the first
couple loops. Instrument to see what this is. There is a big jump in cycle time delta.

OK after a herculean (well perhaps I exaggerate) effort, I've got the audio working, at 1MHz
at least. At ludicrous speed, several things aren't working right.

At higher speeds, quickly gets out of sync. Cross that bridge later. The 1-bit speaker
is sort of ludicrous at 250MHz anyway ha ha. (It's playing back events at 1MHz speed no matter what, 
but at faster speeds it's loading up the buffer way faster). When speed shifting, flush the audio
buffer. (for a IIgs running Apple II stuff in emulation mode, slow down to 1MHz when PB=0 or PB=1).

Video - there is no restriction on the video update rate.

If we are in ludicrous speed, limit video updates to 60fps. ok, done. I did for events, etc. So we're back to 

```
1280493489 delta 4415324039 cycles clock-mode: 0 CPS: 256.098694 MHz [ slips: 0, busy: 0, sleep: 0]
```

256MHz apple II. I mean, come on man.

ok what's next?

I'm going to implement green and amber text and lores modes, and implement a color mode switch.

But then I need to do a code cleanup and move globals and statics in a few places into structs attached
to the cpu_state.

For that (which I should implement for this color mode thing as the first implementation of it), I need
a generalized data storage thing. If this was JavaScript I would just pile things into an associative array,
or properties.

But this is C++, and we need to allocate memory. What if I do an enum of 'names' ('properties'), and then
the cpu_state has an array of void pointers. Each module that needs this type of storage gets a name assigned.
When it needs its state, it typecasts the pointer to its correct type. Then the cpu_state doesn't care
what the structs look like. If a module needs multiple chunks it can define its static struct to have them.

## Jan 19, 2025

OK that's done for the display code. Wasn't too bad. I even made it a class instead of a struct to use
a constructor init. Will be pushing the codebase more into this direction.

For color-mode switch, I need to figure out how to display lo-res on a greenscreen. And, I need color
hi-res.

To do hi-res properly I need to double the number of horizontal pixels to simulate the phase shifting.
I think I probably also want to implement a border around the simulated NTSC screen. (GS will eventually require
this because you can set the border color).

these are pretty good discussions of hi-res:
https://forums.atariage.com/topic/241626-double-hi-res-graphics/page/4/
https://nicole.express/2024/phasing-in-and-out-of-existence.html

Hm found a bug. There is a bizarro artifact. If there is text update on lines y=22 or =23, it gets
blasted into the borders both bottom and right.

I don't think I'm doing anything wrong. If I don't specify the dstrect or srcrect, it defaults to scaling the texture to the window size. If I
specify dstrect, or, dstrect and srcrect, I get the artifacts in the odd places.

Yep. If I select the opengl renderer, it works fine. Using the default renderer (specifying with 'null' in CreateRenderer) 
I get the artifacts. WEIRD. And annoying.

So now I'll have to read up on these. And I guess send in a bug report to SDL.

ok Color Hi-res.

Understanding the Apple II has the best discussion of this. That guy was a genius.

Let's see if I can boil the rules down.
1. Two adjacent one bits are white. I.e., even if we set a pixel as a color due to preceding 0 bit, a second 1 bit in a row will turn the previous dot and the current dot white.
1. If even dot is turned on, they are violet.
1. Odd dots are green.

If the high bit of a hi-res byte is set, then the signal is delayed by 1/2 a dot, and the colors
violet and green become blue and orange.

Delayed and undelayed signals interfere with each other. So if we go from a 0 to 1 or 1 to 0, there will be special
stuff happening at the edges of that intersection.

"There are 560 dot positions in a row. Color depends on position. There are 140 violet,
140 green, 140 blue, and 140 orange positions."

"Any time you plot a green dot in the same 7-dot pattern as an orange dot, that orange dot
turns to green because D7 had to be reset in that memory location to plot the green dot.

Two adjacent color dots of the same color appear solid because of analog switch time and blurring. This is simulated
on Apple2TS by drawing 3 purple dots in a row in a case like this.

Pages 8-21 to 8-23:
The first 14M period of a delayed pattern (bit 7 = 1) is controlled by the last dot of the previous pattern.
Switching from 1 to 0 (delayed to undelayed) "will cut off the tail of the current pattern, cuts the last
dot in half".
cutting off or extending the dot has the effect of slightly changing the dot pattern and noticaebly,
changing the coloring of the border dots.
LORES colors 7 and 2 can be produced at even/odd memory addressing order.
Colors D and 8 can be produced at odd/even borders.
Colors B and E can be produced at odd/even or even/odd borders.
Lores 7 can also be produced at the far left of the screen, and color E can produced
at the far right.
color 1 can be produced only at the far left the hires screen. orange hires dots at the right side of the screen
are dark brown.

So Apple IIts has handled some of this (the stuff in-line) , but not all of it (the stuff that crosses scanlines). This all sounds complex but it's a fairly simple set of rules, I think.

In any event my idea of using the 560 dot positions seems correct.
```
                     0.1.2.3.4.5.6.
2000: 01             *_______
2400: 81             _*______
2800: 02             __*_____
2c00: 82             ___*____
```
creates a purple dot, then blue, then green, then orange, in very tight pixel positions.
Numbers on the top are a whole dot position. The periods are a half dot position. My IIe monitor
is super-fringy, you can clearly see a half dot, a full dot, and a half dot of each single color.

If hi bit of a byte is set, we will plot into odd pixel positions. And pick color based on 
bits being last=0, current =1; last=1, current=0; last=0, current=0; last=1, current=1. Keep a
sliding window of 2 bits.

So, a design choice. Do we make the entire Texture 560 pixels and change the scaling factor.
Or, only change for hires? I think it would be simpler to do all of them. The text and lores
code will need to change to plot twice as many horizontal pixels.

Thinking about this - color fringe effect on split screen text and lores/hires, the text pixels will be
colored exactly as if they were hires pixels with hi bit always cleared.
So architecturally, perhaps we paint everything into 560, and have a post-processing step that does
the coloration, which would be the same for all modes. Have to ponder how lores fits into this scheme
too, i.e., how are the pixels shifted there.. don't have to post-process. just draw using the hi-res algo,
but draw the data from the text matrix.

There will be an "RGB Monitor" mode that will have no doubling of color hires pixels and no text fringe; and 
composite mode that will fringe text and blue the color hires.


## Jan 20, 2025

It's the third Monday in January! You know what that means! It's National GSSquared Got Color Hi-Res Day!

So the above wasn't too bad at all. 

Like I did with the hgr function, move the x = 0 to 40 loops for text and lores into those routines.

All dislay rendering is now done with scanlines, i.e., an entire row of pixels horizontally.
This moves loop inside the render function, saving a bunch of function calls. Also makes
the API for a display handler consistent across all display types.

of course, it wasn't a bug - I had a brief exchange with the SDL lead dev and he pointed out
I needed to do SDL_RenderClear() every single frame. This is part of the GPU voodoo I don't have
experience with. But now I do! A little anyway.

## Jan 22, 2025


There are SDL variables that can make the drawing even blurrier. Experiment with them.

For the color-stretch mode, I'd say, if I draw a color dot and two dots to the left is the same color,
fill in the middle dot with same color.
I am also not sure about the drawing the white dot algorithm as it is. Review it and compare to other examples.

Hi-res tweaks. "Composite" mode is drawing too many pixels I think. Instead of 2 pixels (4 dots) maybe do 1.5 pixels (3 dots)?
letters like lowercase 'm' are completely filled in, and that's not quite right..

So I'm trying the Introducing the Apple II disk. It said it was iie only. it's running on my II+-ish.
it understands lowercase, but, they are reversed. shift-letter gives me lowercase. letter gives me uppercase.
I bet that's because my keyboard routines are not converting to uppercase when I am holding shift. That's funny.
up and down arrow don't work because I never mapped them. Can't play the rabbit game then!

## Jan 23, 2025

To get sound working on the Linux build:
install libasound2-dev libpulse-dev
and doing a cmake reconfigure. Works great with the $15 USB speaker I bought. That's just crazy, and fun. I remember how expensive sound cards used to be. And that didn't even include the speakers.

Experimenting with window resizing. I have it constraining size to the correct aspect ratio, and, 
modifying the scale factor in the renderer to match. Now test full screen mode. We'll toggle that
based on the F3 key since I already have F11 used for something else.

I think there is a bug in the lo-res code now where on switching to lo-res mode it's not dirtying the lines
to force a redraw. At least, my lo-res apple is not being drawn on boot of some of these disks. Ah, it was
a change in the code that reacts to writes to text memory. I "optimized" it to only do text updates
in lower portion when in graphics mode. But needed to also check to hi-res mode.

Full screen mode works but it allows use of the wrong aspect ratio. I think in fullscreen mode we need to 
force the scale to match aspect ratio, then center content inside the fullscreen window.

[ ] in game code, window_width and height need to read the actual window size, or, use some other method
to constrain the mouse movement.

## Jan 24, 2025

The new gamepad arrived. This one is bluetooth and wired (supposedly). Compatible with everything supposedly. Works
on the Mac. I have it coded in to GS2 and it's working well. One notable thing, is if you go straight horizontal or vertical, it will scale to +/- 32767. IF you go diagonal, it will scale to +/- 24000 or so. The range of motion on the joystick is actually circular, whereas on the original apple II joystick it was square. I get the rationale here, my recollection is that the Apple II stick would sometimes get bound up in the corners.

This thing has a crazy number of controls. There are like throttle buttons on each side, that are pressure-sensitive (i.e.
scale depending on how far you depress them). Four X / Y / A / B buttons. And of course the four-way nintendo style + control. I am unclear on how button mappings work, and it is possible I will have to switch to the "gamepad" API which supposedly handles all the axis and button mapping stuff for you. Look into that more.

But I have successfully played the ole Choplifter with it. It's a lot easier with a proper joystick, even if it is
tiny compared to my original II stick, which I really miss. (Those things were great).

This had some sample code that helped me get started.

https://blog.rubenwardy.com/2023/01/24/using_sdl_gamecontroller/


## Jan 25, 2025

This doc has a pretty useful description of the hi-res circuitry.

"Apple II Circuit Description"
https://ia902301.us.archive.org/31/items/apple-ii-circuit-description/Image081317140426.merged.pdf

also the game controller stuff. That is extremely simple. I am pondering building paddles.

Need to re-do the gamecontroller logic to use the GameController API instead. This will help ensure compatibility with many different gamepad controller devices.

Interesting thought: map the nintendo style + control to A Z ARROWS or IJKL as an option. If we put a single game onto a floppy, that disk image could have
metadata to indicate what the controller - keyboard mapping should be. That is actually some hot stuff right there. Define some sort of metadata format.
Probably JSON, easy to store and generate and work with.

Other metadata: minimum system requirements. 

## Jan 27, 2025

had an idea. Do a "screen shot" key, that will dump $2000 to $3FFF to a file. Then we can write a hi-res tester that will much more easily
allow us to test hi-res rendering. Like what I did with the audio recording and test stuff. Yah baby.

## Jan 30, 2025

looking at the disk II code. The program Applesauce actually visualizes a disk image showing the sectors, data, etc.
There are some interesting differences in how mine looks versus their nibblization. First off, they show stuff on the quarter tracks.
Like, why. Overall, my image is darker. Probably because of the quarter track thing. Anyway.
Their nibblization has the following differences to mine:
They have more Sync A bytes than I do. 120 vs 80.
They have FEWER Sync B bytes. 6 vs 10.
Fewer Sync C than me. 17 vs 20.
Of course, in their thing, they display the sync bytes as 10 bit. Is that relevant to anything?
That said, their gap duration is a bit LONGER than mine - 657 microseconds vs 626. This is because of the 10-bit thing.
Perhaps I should *extend* my gaps a bit to make them the same number of microseconds. It's possible code isn't getting back
to this part of the read loop fast enough, and skipping. Though that would feel like 'stop working' entirely since it would
be exactly the same each time. Something to consider.

I found a "blank.nib" file. That image does not look like clean conical slices. Every track is skewed relative to the last one.
Inside a single track I'm fine. When we skip to the next track, we might have to wait a whole revolution. Whereas a regular inited
disk is probably more like the skewed version (blank.nib) because there is no sector alignment on the Apple II. And that
might affect performance. This would be implemented by starting the next track at the current location, and wrapping
the write position around based on wherever the virtual head is. NOT resetting to 0. This would really depend on the specific
init process used on a disk.

Looking at "DOS 3.3 System Master.woz" which is another nibblied format - that DOES track the 10-bit vs 8-bit thing.
These sectors are lined up in clean cones. And it has the quarter tracks too. Of course, .woz does not imply nibblized.
It can contain other formats too.

I think another difference between mine and the others, is that the blank space at the end of a track is all 0's in my image.
And the gap is large - it amounts to almost an entire sector width.

In blank.nib, the tracks are exactly 0x1A00 long as in mine. But, their tracks definitely do not always start with a sync gap.
```
0019F0: FF FF FF FF  FF FF FF FF  FF FF FF FF  FF FF FF FF  . . . . . . . . . . . . . . . .
001A00: B2 B7 CE B2  B7 CE A6 E7  D9 AF 9B A6  E6 DA AE 9A  . . . . . . . . . . . . . . . .
001A10: DC D9 9B 9A  CB CE B2 B7  CE FE F9 9B  AC DA DE AA  . . . . . . . . . . . . . . . .
001A20: EB FF EB FF  FE FF FF FF  FF FF FF FF  FF FF FF FF  . . . . . . . . . . . . . . . .
```

We go from sync bytes directly into some data. Then sync pattern later. This is what generates the skewed pattern we see.

This is being triggered by how much faster an image seems to boot up in apple2ts. I should time the same image in both
a2ts and gs2. Do that tomorrow when I'm not yawning.

Walking through the tracks in blank.nib one by one, each track starts with a different sector. F, then E, then D, etc.

[ ] So, those two things to try: (1) pad out the sector gaps to make their uS duration similar to these other nibs; and do the skew
thing. And if there are any bytes left, we should pad out with FF instead of with 00. (2) try just jumping to next track at
same byte index, instead of resetting to 0.

I did a fair bit of work today, on arqyv. Got the vm setup, got asimov mirrored. There are other sites to mirror. I'm going to
need a lot more storage for those, likely (I'd prefer to just host here but Xfinity upload is too slow). If I ever get fiber again
I can self-host it.

So I can test this by d/l some nib format disks that are done this way and see how fast they are.

## Jan 31, 2025

So, add .nib support to the diskii routines.

Kaves of Karkhan seems to boot and play. It's a copy-protected disk that uses AB as the sync byte instead of FF.
There may be other differences too. But, the emulation works well enough to do this!

## Feb 1, 2025

The .Woz format document has some very interesting and useful info about copy protection schemes. And about the hardware.
Take-aways:

https://applesaucefdc.com/woz/reference1/

"Every soft switch on an even address should actually return the value of the data latch."

Also, "1 second delay after accessing the drive motor off" at $C088,X needs to be implemented.

We might need a concept that the virtual disk is spinning underneath the head. E.g., each time we're read, check the CPU
cycle count and move the virtual head position accordingly. 4uS per bit.

Also, "When you do change tracks, you need to start the new bitstream at the same relative bitstream position – you cannot simply start the pointer at the beginning of the stream. You need to maintain the illusion of the head being over the same area of the disk, just shifted to a new track."

This is for copy protection purposes. Finally,

"Now that we are back to having long runs of 0s in the bitstream, we now need to emulate the MC3470 freaking out about them. The recommended method is that once we have passed three 0 bits in a row from the WOZ bitstream to the emulated disk controller card, we need to start passing in random bits until the WOZ bitstream contains a 1 bit. We then send the 1 and continue on with the real data."

That is pretty wild. 

This document is implying that it stores the extra 0 bits in a track stream in the sync nibbles. Will need to find a .woz 
image of some crazy copy protected disk to test this. (And review with Applesauce).

## Feb 2, 2025

So I have some various stuff working to support the Apple II Memory Expansion Card.
The C8xx mapping, for one, and CFFF de-mapping.
Booting ProDOS, is causing a hang. I must have done something wrong. I will manually exercise
the 'hardware' to test.

## Feb 4, 2025

So, when there are things that are about to happen that we know will suck up a bunch of time (for example, opening a file dialog), we can Pause the audio stream until that call has completed. Other things in this realm would be, loading a floppy disk image.

After opening a file dialog, our window is no longer in focus. I have to click to bring it into focus, then click again to open the dialog again.

The dialog thing is only callable from the main thread. So we're back to considering, is this fine, or, do we need to put the CPU and audio into their own threads, to prevent stuff like this from interfering with the (one, really) realtime aspect to the software?

## Feb 9, 2025

When the OSD is open, keyboard and mouse events should go to it. So, perhaps the event routine should do this: return true if the event was handled, false if not. 

The joystick is now reading upside down in Choplifter. Is there a flag for controlling how the axes are read?

## Feb 11, 2025

Looking at Disk II code. When we switch tracks, we DO have the head in the same relative index into the track data. What we are not simulating, is the disk continuing to spin under the head when the CPU is off doing other stuff. 

The math here should be:

3.910us per bit. 

| Gap | Our bits | Applesauce bits | Action | 
|-----|----------|-----------------|------|
| Gap A | 640 | 1198 | add 1198 - 640 / 8 = 69 bytes more FF |
| Gap B | 80 | 60 | remove 1 byte FF |
| Gap C | 160 | 168 | add 1 byte FF |

Gap A is track start.

Now, at end, we also need to write 0xFF to the end of the track buffer. So from whatever the index is, through 0x19FF. That I did. Didn't make a difference.

I don't measure much difference at this point between the A2 emulator I downloaded and GS2.

## Feb 12, 2025

Bring checking of application-specific hot keys back into main event handler area, and out of 'keyboard'. We will have multiple keyboard handlers at some point.

Make speed control buttons.

## Feb 13, 2025

thinking about halt. Two things shouldn't crash the emulator:
* Halt (STP instruction)
* jump to self (infinite loop).

Now, we can treat the second as a case of the first. So what we want to do on a HLT, is, continue the event loop but stop executing instructions on the CPU. OK, this has been taken care of. On reset, halt is cleared. We only exit the event loop if halt is set to HLT_USER. (i.e., F12 or window close).

I disabled the jump to self test. We may want to enable it under special circumstances (i.e., running the test suite).

### Reset button

that was pretty quick.

### Power On / Power Off Button

Isolate the stuff that allocates memory, etc, and starts up the CPU. This is as distinguished from setting up the display window, and other host-level stuff.

Like, I should do something! The big daddy is to mount new disk images from the OSD. So let's work on that.

### Need to draw a black background with 100% opacity before we draw anything else in the OSD. Or, just fill the rest of the OSD. I may not be filling the entire thing.

### Hook up mount disk image code

let's do the naive approach first - change out the disk image from inside the callback handler. Apparently these are called from the event loop. We may assume it will take some significant amount of time to process the mount. That means we will likely need to fire off a thread to do it. But, start first and then measure.

Create a util/mount_unmount.cpp file to handle these.

## Feb 14, 2025

We need some kind of hook between the OSD display stuff, and something we can query to get and manipulate the status of disk drives, from a higher level than the diskii module for instance. From the GUI, we need to:

mount media
unmount media
write protect media (eventually)
query status (running, mounted, write protected, etc.)

The status will tell us how to display the particular widget in the OSD.

## Feb 15, 2025

Fixed a bug in the system - only 48K RAM was being allocated but of course the language card was using memory locations above that. Somehow that was working on the Mac, mostly, but blew chunks quickly on Linux. Enabled bounds checking in CMake Debug and rapidly diagnosed the problem, as well as a few other bounds problems.

The checks also identify memory leaks. There is a small amount of that going on right now. Of course we largely don't deallocate any memory on shutdown. Investigate later.

## Feb 16, 2025

We are now successfully mounting disk images at runtime. Was able to play Oregon Trail which requires flipping back and forth between two disks. And, can pretty reliably switch disks in DOS / ProDOS and CAT/CATALOG etc.

The OSD display of the drives does not track the actual drive status though. Have to somehow tie that in.

Getting close. However, the diskii drive motor never turns off. Here's what is happening:
* drive is running
* sector read
* drive off sent
* timer is set
* but the disk code never hits any diskii softswitches again, so I never get a chance to see the timer expired and to turn the motor off.

So, the diskII module needs to be called periodically to check on timers and update state.

I need a way for a slot to register a callback, called whenever a timer expires. SDL has SDL_AddTimer(), however, I am thinking of a more general purpose mechanism. Some devices will ultimately need to generate interrupts on the basis of timers.

Also, diskII needs a reset handler. From UTA2: "Pressing RESET causes the delay timer to clear and turns off the drive almost immediately."

ok, some progress. Drive status working. Need to be able to unmount, not just mount new media.

I sort of have mounting image on Drive 2 working. However, I get an I/O error or infinite spin the first time I catalog,d2. SEems like I'm overlooking a state change to drive 2 somewhere. If I then RESET, and try again, it works. OK. When I've booted, and then I do a catalog,s6,d2, it turns on the motor on drive 1, not drive 2! I reset, then do it again, then it turns on the motor on drive 2!

It's not seeing my motor on after doing drive_select 1 the first time:
I have a drive 0 motor 0 for some reason..

```
slot 6, drive 0, motor 0
slot 6, drive 0, drive_select 1
```

second time it does it:
```
slot 6, drive 1, motor 0
slot 6, drive 1, drive_select 1
slot 6, drive 1, motor 1
```

doing catalogs, the wrong motor drive light is coming on sometimes.

I guess I will need to read the UTA2 again. 

Whatever drive was last selected, if I try to catalog the other one, it barfs with I/O error. Then doing it again, works. It's probably not reading anything, it does a lot of CHUGGAS to try to reset the head, and then still fails, so gets I/O error.

## Feb 18, 2025

Thunderclock Plus firmware is now loaded, and confirmed working with ProDOS 1.1.1 disk. TCP DOS utils disk also worked.

It is however acting le funky using the TCP ProDOS utils disk. Dos 3.3 one booted and worked.. like it is reading register C090 => 00 in an infinite loop, never returning.
That's not cool, bro.

something caused cpu->cycle count to reset to 0; is it my reset routine or something? That's not right. cycle count should be set to 0 only on power up.

Apparently the CFFF to disable slot card map into C800, works on read or write. So tweaked that.

Trying to troubleshoot why the TCP c800 firmware isn't coming in and out properly. Here's the deal:

* whenever CnXX is read or written to, C8xx is enabled for that slot. IF it's not already enabled?
* read or write to CFFF turns that rom off.
* in memory_read, if the page type is RAM or ROM, we're fine. I just changed it so that if the page type is IO, we call the memory_bus_read handler.
* However, this means once a page is set to I/O, we are not reading memory values from the ROM.

In memory_bus_read after checking all the various I/O addresses, I return by reading the memory mapped value, instead of returning 0xEE. So far that seems to be working okay.

Now, still have the issue where we get infinite C090 reads. ok, the demo program 'clock' is doing "-TUT" which is loading the sys program TUT. That is what is infinite looping. I think that loads a new ProDOS command called TIME. TIME %, TIME #, etc then generate some output the basic program reads in.

Clock is a cute program that displays a seconds-ticking clock with moving hour, minute, and second hand. The DOS33 version works.
I must be missing a hardware command somehow. There is a program TEST on the DOS33 disk. It says my thunderclock is not operating properly.

```
Thunderclock Plus write register C090 value 40
Thunderclock Plus write register C090 value 0
```

in VirtualII, if I do this:
```
C0f0:40
c0f0:00
c0f0
c0f0-60
c0f8 (clear interrupt)
c0f0-40
```

So on a pure read, 0x20 is the interrupt set bit.

So it's turning on interrupts. And I'm not generating one, so it is probably then turning interrupts off and saying you failed, I didn't get an interrupt.


This is what I get when I tell it to set the interrupt rate to 1/64 sec.
```
Thunderclock Plus write register C090 value 0
Thunderclock Plus write register C090 value 20
Thunderclock Plus write register C090 value 24
Thunderclock Plus write register C090 value 20
Thunderclock Plus write register C090 value 40
Thunderclock Plus write register C090 value 40
Thunderclock Plus write register C090 value 40
```

The behavior of Cn00, C800, CFFF seems that once a particular slot is latched in, you HAVE to CFFF to disconnect it, and only then can you enable a different slot. This behavior is unclear.

This is correct. The first card hit will "hold on" until CFFF is accessed. 

## Feb 19, 2025

I have the data sheet for the UPD1990AC which is the clock chip used in the Thunderclock Plus. It is not very clear, it's in Japanglish. 

I did a detour and implemented a "generic prodos clock" device. This supports only month date day of week hour minute. But it's something.

The main bit of the TCP that will be complex, is supporting its timer interrupts. There is also a "test mode" which says it increments every counter in parallel 1024 hz. like, why, what does this do. I see that other emulators also moved to support No Slot Clock, and, generic prodos clock, instead of the TCP. Though one does attempt to emulate the TCP interrupt timers.

Consider this. We run chunks of 6502 code in bursts of 17,000 cycles, 60 times per second. In order to support a timer interrupt of 1/64, 1/256, and 1/2048 second intervals, we would need to create an event queue and have the CPU loop pick those events up each iteration. Because we don't execute the code evenly (i.e., it's fullspeed during each burst) then we can't use real system timers. And that's probably okay.

So, the timer routine would "post" an event to occur based on a calculated number of cycles.

```
cpu->cycles + (cycles_per_second / 64)
cpu->cycles + (cycles_per_second / 256)
cpu->cycles + (cycles_per_second / 2048)
```

if at the top of the cpu instruction handler we have hit this timer, we call the device interrupt handler.

Let's think about this a bit too from the perspective of something like a serial card, where we may want to generate an IRQ on an incoming character. Characters do come in quite a lot faster than 1/60th second. They'll get buffered of course, but, if we have it in interrupt mode, in a separate thread, and it comes in while executing code, we want to trigger the interrupt. Even if not an interrupt, we want to set the "data ready" flag. nah, character ready check would just check a buffer. That's no biggy. It's more specifically interrupt stuff where we want to use this queue.

We will need this same stuff when we get to the IIgs emulation, since it has a variety of built-in IRQ generating devices.

So, this is a proposal for an Interrupt event queue. And it will be a lot like the audio event queue. Do we actually need a queue? What if we just emulate IRQ systems. I.e., each device or slot triggers IRQ. All the IRQs are ORd together. Interrupt handler has to search for the device that triggered it. 

We'll have some callbacks that can be registered for timers / etc. Each iteration through the CPU loop, we call the routine. The routine will set a flag if the timer IRQ should fire.

IRQ status for the devices stored in a 64 bit bitfield. We can check for -any- IRQ by checking if that value is non-zero. The position of each one bit indicates which devices' callback to call. They can be registered in an array of callback handlers (up to 64 of them).

Instead of an ordered set or something, just do this sort of optimized thing:

* bit field - each bit is assigned to a particular device.
* array of handlers, array element corresponding to the bit position.
* array of "next cycle triggers", also corresponding to the bit position.
* once a trigger is called, the bit is cleared.
* if the device wants, it sets the bit again.
* Each time a next cycle trigger is reached, the next cycle is determined and stored. As an optimization so we don't scan the list every time, we scan it only when it needs to be updated.
* Whenever a event is registered, if the cycle desired is less than current next cycle trigger, we update the trigger to that.

## Feb 22, 2025

Taking a look at OpenEmulator, another Mac A2 emu. It is Mac specific, they claim other platforms coming but never got done.

Uses 3D effects to simulate CRT monitor "barrel distortion" and such. I think it's overkill. It's fairly accurate I think. It's flash mode is quite a bit faster than mine. I may have that wrong. I can check on the IIe. I do!

Clock speed: only 1MHz and Free Run are working correctly. 2.8 and 4MHz are not working right. They are both reading at like 1.2-1.4mhz effective rate.

I had an idea about the audio/speaker handler. Whenever C030 is tweaked, have it write directly into the audio buffer. However, the problem there is that could take up a lot of real cpu time.

another idea: whenever we change clock speed, flush current audio buffer and reset parameters so the new buffer is calculated only at new clock speed.

Maybe I need to tweak the number of clock cycles we run each burst. In fact that might be the issue with 2.8MHz and 4MHz. We need to run more emulated cycles at higher clock speeds. Let's see..

In free run mode, we run 17000 cycles per burst. Then we check to execute audio, video, and events only once every 1/60th second realtime.

In 1MHz mode, we run 17000 cycles per burst. Then check. Then sleep until next burst. So yeah, we definitely need to run more cycles at higher clock speeds. OK, easily done.. That's not quite right. When I do a simple lookup table, the flash gets faster. What? Ah my lookup table is in the wrong order. still not right. Duh have to use the lookup table, not just define it. THERE WE GO.

(I consolidated all the clock mode variables into a single place).

Uh, all the sudden audio is working correctly at higher speeds. It seems the audio code was written correctly, but, I was calling it in the wrong context.

WHEE!!!!

Reading some of the OpenEmulator code. It draws pixels out purely as a bitstream, then, it uses OpenGL shaders to render the pixels in different modes (RGB, Composite). The shader is readable-ish. It uses vector processing in the GPU, I see reference to PI in there. So it's definitely implementing something like the approach I've been testing on in hgr5 - simulating a color wheel.

hgr5 and hgr6 differ in how they handle starting the scan line. On the first couple pixels, we do not have enough prior pixel data to do a proper average from. So what should we start it with?  hgr5 averages only the samples we have. hgr6 averages the samples we have with zeroes.

Once you're 3-4 pixels into the scanline, it actually seems to be working really well.

If we adopt this technique, then we could use it for every mode. And I have been digging into this because handling DHR the way I've been doing HR is going to be problematic - it will just be too complex. But DHR with this color wheel thing will be pretty straightforward. Also, text with fringing in mixed mode will work this way too. Just emit the same bit stream. And lo-res would work this way too.

## Feb 23, 2025

I just had an incident where there was a blip, and then the audio got delayed by a good 30-45 seconds or so. So there may be some edge case bug where we get out of sync due to some bad math somewhere. It's definitely much improved.

When I stuff audio data into the buffer because we're running low on samples, I fill it with 0's. That is causing clicks. I should probably decay the signal over time like I saw mentioned in some other projects. Alternatively, fill the buffer with the last value. Let's say we tail it off - multiply by 1 / (time since last event).

Trying the "repeat last sample" method. Now we are still running out of samples in certain cases. One such case is when the emulator window is obscured. like, just click to bring a browser window to completely cover it. Something will have interrupted execution long enough to bring the effective cycles per second to 800khz. It's not bringing it up, it's covering it. The Mac must be doing something weird. Maybe there is a task priority setting; I should learn more about this memory compression business.

Another event that can trigger a buffer underrun is dragging the window to another screen.

## Feb 27, 2025

been programming my brain with NTSC stuff over the last week. I am starting to get a clear picture of it. The section of UTA2 page 8-20 is making sense.

The display emulation in OpenEmulator is the best one I've found. By far the most accurate. It processes in a number of steps. First, it creates a bitmap of 1/0 signal that is delayed or undelayed in a 560x192 grid. Good, that's basically what I came up with, though it generates it a little differently than I did. But that difference doesn't matter.
Two, it then applies a number of shaders to that data. A shader is just a small function/program that runs inside a GPU. OE is using the OpenGL 3D API to run shaders. It has shaders that do all kinds of crazy stuff, like project the display onto a curved surface of a simulated CRT tube, handle brightness and color hue controls, etc. I'm not interested in going that crazy, as I think those effects interfere with the usability of the emulator. (Though, they accurately simulate how frustrating it was to use computers on cheap monitors in the 1980s!)
The basic shader in OE though compares a phase signal to the bit pattern to calculate the color of each pixels, and, also performs multiple samples of each pixel at slightly different places to simulate blur that occurs because an old CRT can't shift its beams around very fast. (It is this that causes 010101 to generate solid green for instance even though every other pixel is off, and, that shows alternating green and black stripes on screen on a high-frequency-response CRT like the AppleColor RGB on the IIgs.)
I've got a utility program that can read a hires file and output a PPM image of the display (PGM, as it's in grayscale right now). Next step is to apply the logic from the shaders and see if I can get a good color output.

## Mar 6, 2025

Finally, success!! There were a few issues with the ./comp version of the openemulator display code. First, -33 needed to be coded as +33 offset. Second, the matrices in the openemulator code were transposed (row/column) from the perspective of my matrix multiply routines, at least. Flipping that fixed a bunch of issues. Finally, the C++ code that applied the filters was not right. Claude identified it was not looking at the neighboring pixels correctly. Once fixed, I am successfully convering hi-res data files into composite-style images!

They look really good.

Next step is to clean up the code, modularize it, make it so it can be a library. Then see if there are obvious optimizations that can be made to speed it up for emulator purposes. I don't know if it will be fast enough. But we can test and give it the old college try!


## Mar 8, 2025

Added some hot keys to dump the hires and text pages to files.

Added support to hot-mount disk images into the 3.5 drives in slot 5, and display their icons.

## Mar 9, 2025

there are some double hires pictures in /asimov/images/productivity/graphics/misc/picpacdoubleres.dsk
They are, however, compressed somehow. 

Got some info on forcing a cold start reset:

$3f2/3f3 hold the warmstart vector. $3f4 is the "checksum"; if $3F3 XOR'd with $a5 is equal to $3f4, then do a warmstart.

So on ctrl-alt-F10 (reset) set $3f2-$3f4 to $00 $00 $00. 

A remaining speaker issue - certain things cause the cpu to halt, but the speaker code is still running and then I'm not sure what happens.
This occurs on invalid opcodes as well as "infinite jump" loops.

So on a halt we stop executing code. But perhaps should continue to increment the cycle counter like we expect..
on a reset, we restart the cpu but the audio code is then out of sync. ok incrementing cycle counter anyway (based on expected cycles) is working there.

Our audio beeps are very buzzy. We're not filtering the audio in any way.

## Mar 12, 2025

On the Mac, "App Nap" changes execution priority, probably triggers memory unloading / compression etc., when an app is not the foreground. There are ways to disable this - you can set a flag via finder, and you can do it programmatically. The latter Claude says requires tying in to Objective C - erk why? 

It does not seem like App Nap is the culprit. When I flagellate a Alacritty window over top, it doesn't cause the same issues as using a Cursor window.
Hinting to use opengl, cause the problem a lot less also, though opengl itself is slower than metal. Keep an eye on it.

## Mar 14, 2025

Created some abstractions for a SystemConfig. This is the selection of a platform, and a list of devices and their slots. Currently pre-define several of these, with only an Apple II+ fully speced out. Eventually, default configs for each type of system will be here, and, users will be able to clone, customize, load and save their own configs.

As part of this, the OSD now displays the name of the card in each slot that has one.

This also cleans up the init code in gs2.cpp, which now iterates through the data structure to initialize devices. Next step is to gracefully de-initialize them on exit. And then, we'll have the framework we need to support powering a system on and off, and changing config at runtime.

## Mar 15, 2025

I've never been thrilled with the prodos block implementation. It relies on "ParaVirtualization" - something performs a JSR to a particular address, then we trap that in the CPU instruction execution loop and call a handler. Currently, this address is hardcoded in that loop, so the device in question is hard-wired to a specific slot.

I also just thought about a weakness in the design of devices, how we store state information in a block identified by an ID assigned to the device. This will prevent us from having multiple instances of the same device type in different slots. Is that something people care about in an emulator? Is there anything out there that requires having two Disk II controllers / 4 drives? Old timey BBS might have had a setup like that, but in emulatorville everyone gets a free hard drive, so.. The obvious solution is to use new slot data structure to store that data. 

I am also going to make the following changes to the build system to simplify the process. We currently have system dependencies on python, and as65 from Brutal Deluxe.

* Populate system ROMs into the assets directory
* Firmware we build will be in separate projects.
  * For instance: ProDOS Block firmware; system roms download and combine.
  * These are copied into resources/roms; and resources/cards. These files then stored in repo.
  * Such firmware will be loaded like any other ROM image asset.
  * Then we can just distribute the ROMs with the main gs2 source code.

Instead of putting roms in assets, we have the roms/ directory in the project root. So this is where they go. They are copied from here into resources/roms and Resources/roms on build-package.

This will greatly simplify other people building.

What is distinction between assets and resources? Assets is predominately image data; perhaps also sound data eventually. Resources are things like ROMs, and other data that is not user-visible.

Do I need this distinction? Let's put ROMs in assets. These are "inputs". They are processed. Then copied into the Resources directory.

Resources is either: *.app/Contents/Resources/ on Mac, or ./resources/ on Linux (or Mac when run from the command line).

The system ROMs; should I bother combining them? Why not just load them individually and directly in the code? We now have the handy routine to make it easy to load ROM and other asset files.

ok the new pdblock2 device is working. This eliminates the poorly thought-out Paravirtualization stuff. Maybe in the future we can do this better - for example, this could let us PV DOS33 RWTS routines so DOS33 could use super-fast disk access.

I did not properly implement the PD_STATUS command. It wants A=0, X/Y = number of blocks on device, and CLC. 

```
LDA ERR,X
PHA
LDA STATUS1,X
PHA
LDA STATUS2,X
TAX
PLA
TAY
PLA
CMP
RTS
```

This will let us return the correct number of blocks on the device (something the old PV wasn't even doing).

Interestingly, if no disk is mounted, PD_STATUS does return an error 0x28 and trying to boot with c500g causes it to jump to BASIC. However, if it's unmounted and you do PR#5, it retries in an infinite loop to boot. Weird. Is that because it's constantly trying to output data and thus calling the boot routine over and over?

Double-check to make sure we can't mount a disk that is not 140K onto a disk ii.

## Mar 16, 2025

```
<> Media Descriptor: /Users/bazyar/src/gssquared/newdisk.nib
  Media Type: PRE-NYBBLE
  Interleave: NONE
  Block Size: 256
  Block Count: 560
  File Size: 232960
  Data Offset: 0
  Write Protected: No
  DOS 3.3 Volume: 254
Disk image is not 140K
```

The size check I added to diskII mount fails on anything except a raw 140K disk image. Media Descriptor should populate Data Size. OK that's fixed, and I fixed a bug where trying to mount a .nib wasn't setting all the right properties.

I still see the occasional issue where booting dos3.3, trying to catalog,s6,d2, drive 1 lights up instead of drive 2. Reset then do it again, and it works.

Doing a little thinking about how we might handle demos that change screen modes in tight cycle loops to create interesting (albeit useless) effects. Instead of immediately changing screen mode variables, we could post an event to a queue. Then when we render a video frame, do it scanline by scanline regardless of mode. And keep track of the events, switching modes as we go. This is a fair bit of work to get a couple of demos working that if you want you can watch in OE or another emulator. I think the question will come down to, do any actual useful programs (games, etc.) do these tricks.

## Mar 18, 2025

thinking about data flow for OSD. Storage devices should Register with the OSD. either the OSD, or the Mount Manager. Mount Manager can be a standalone thing. OSD then uses Mounts to get drive / disk status etc. Currently the disk buttons / status are manually called (by name). so use Mount as an abstraction for both. Ultimately other things in the system might want drive info.
Device init registers. Device de-init de-registers with Mounts.

## Mar 27, 2025

Going to dive into debugging ProDOS 2.4.3 hanging on boot. It's getting stuck in a loop around $D380-$D3FF. There are two loops.
The first is this:

```
 | PC: $D3A5, A: $FF, X: $60, Y: $FF, P: $A5, S: $99 || D3A5: LDA $C08C,X   [#01] <- $C0EC
 | PC: $D3A8, A: $01, X: $60, Y: $FF, P: $25, S: $99 || D3A8: BPL #$FB => $D3A5 (taken)
 | PC: $D3A5, A: $01, X: $60, Y: $FF, P: $25, S: $99 || D3A5: LDA $C08C,X   [#03] <- $C0EC
 | PC: $D3A8, A: $03, X: $60, Y: $FF, P: $25, S: $99 || D3A8: BPL #$FB => $D3A5 (taken)
 | PC: $D3A5, A: $03, X: $60, Y: $FF, P: $25, S: $99 || D3A5: LDA $C08C,X   [#07] <- $C0EC
 | PC: $D3A8, A: $07, X: $60, Y: $FF, P: $25, S: $99 || D3A8: BPL #$FB => $D3A5 (taken)
 | PC: $D3A5, A: $07, X: $60, Y: $FF, P: $25, S: $99 || D3A5: LDA $C08C,X   [#0F] <- $C0EC
 | PC: $D3A8, A: $0F, X: $60, Y: $FF, P: $25, S: $99 || D3A8: BPL #$FB => $D3A5 (taken)
 | PC: $D3A5, A: $0F, X: $60, Y: $FF, P: $25, S: $99 || D3A5: LDA $C08C,X   [#1F] <- $C0EC
 | PC: $D3A8, A: $1F, X: $60, Y: $FF, P: $25, S: $99 || D3A8: BPL #$FB => $D3A5 (taken)
 | PC: $D3A5, A: $1F, X: $60, Y: $FF, P: $25, S: $99 || D3A5: LDA $C08C,X   [#3F] <- $C0EC
 | PC: $D3A8, A: $3F, X: $60, Y: $FF, P: $25, S: $99 || D3A8: BPL #$FB => $D3A5 (taken)
 | PC: $D3A5, A: $3F, X: $60, Y: $FF, P: $25, S: $99 || D3A5: LDA $C08C,X   [#7F] <- $C0EC
 | PC: $D3A8, A: $7F, X: $60, Y: $FF, P: $25, S: $99 || D3A8: BPL #$FB => $D3A5 (taken)
 | PC: $D3A5, A: $7F, X: $60, Y: $FF, P: $25, S: $99 || D3A5: LDA $C08C,X   [#FF] <- $C0EC
 | PC: $D3A8, A: $FF, X: $60, Y: $FF, P: $A5, S: $99 || D3A8: BPL #$FB => $D3A5
 | PC: $D3AA, A: $FF, X: $60, Y: $FF, P: $A5, S: $99 || D3AA: CMP #$D5   M: FF  N: D5  S: 2A  Z:0 C:1 N:0 V:0
```

That is code loading a value from the disk and looking for the marker $D5.

And then later it's spinning around here:
```
 | PC: $D385, A: $01, X: $60, Y: $00, P: $24, S: $95 || D385: LDX #$11
 | PC: $D387, A: $01, X: $11, Y: $00, P: $24, S: $95 || D387: DEX
 | PC: $D388, A: $01, X: $10, Y: $00, P: $24, S: $95 || D388: BNE #$FD => $D387 (taken)
 | PC: $D387, A: $01, X: $10, Y: $00, P: $24, S: $95 || D387: DEX
 | PC: $D388, A: $01, X: $0F, Y: $00, P: $24, S: $95 || D388: BNE #$FD => $D387 (taken)
...
 | PC: $D387, A: $01, X: $01, Y: $00, P: $24, S: $95 || D387: DEX
 | PC: $D388, A: $01, X: $00, Y: $00, P: $26, S: $95 || D388: BNE #$FD => $D387
 | PC: $D38A, A: $01, X: $00, Y: $00, P: $26, S: $95 || D38A: INC $D36F $D36F   [#01]
 | PC: $D38D, A: $01, X: $00, Y: $00, P: $24, S: $95 || D38D: BNE #$03 => $D392 (taken)
 | PC: $D392, A: $01, X: $00, Y: $00, P: $24, S: $95 || D392: SEC
 | PC: $D393, A: $01, X: $00, Y: $00, P: $25, S: $95 || D393: SBC #$01   M: 01  N: 01  S: 00  Z:1 C:1 N:0 V:0
 | PC: $D395, A: $00, X: $00, Y: $00, P: $27, S: $95 || D395: BNE #$EE => $D385
 | PC: $D397, A: $00, X: $00, Y: $00, P: $27, S: $95 || D397: RTS [#D170] <- S[0x01 96]$D171
```

it's updating a variable at $D36F. there are some other vars here too. 

Short version, it's scanning track data and not getting what it is expecting.
ok the track is starting at 0 and it's hammering it CHUGGA style. About 35 times as we'd expect. This is the C600 boot loader here.
we get to real track 5 (done loading prodos) and then it jumps into D1xx. 
Then it's jumping back and forth between track 1 halftracks 0 and 1.

So is it doing that to try to rehome on the track, or, are my disk2 register emulation not working as expected?

It's cycling doing :
* turn off all phases 
* ph1 on
* ph2 off
* ph0 on
* ph1 off
* turn off all phases

repeat.

OpenEmulator won't boot my image.nib. 

looking at my output .nib, first difference is AppleSauce is using Volume 001, whereas I'm using volume 254.
Second thing, my sector order/numbers are wrong. 

It was track 0 sector 1 as 03 00 05 00 ...
I have it as all 0's.
My sector that has 03 00 05 00 is marked as Sector E.

So I am generating the sectors incorrectly. The boot code is working ok for whatever reason, but when it switches to the ProDOS 2.4.3 code it barfs.

First thing to try, is use Volume 001 like a ProDOS disk should.

If Interleave = ProDOS, use volume 1.
Oh, nibblizer is not using the media descriptor thing and is manually generating DOS33 interleave and volume 254. Ergh.
ok, nibblizer is fixed. It converts a .po image to a .nib and this boots in OE. Also boots in Virtual2.
The volume deal did not fix ProDOS 243.

I did confirm with nibblizer that we are generating the right interleave etc. So it must think it's on the wrong track. Or it is on the wrong track. The phase cycling above is the indication.

## Mar 28, 2025

ok, I got some source snippets for these routines. John mentioned a couple possibilities. First, if we don't get an address field in 2000 nybbles then we generate a RDERR - this is a branch to $D3FB. I am not executing that.

The code being hit is the "fast seek routine".

It's the "fast seek" routine that keeps cycling over and over. It is trying to seek back to track 0, starting at halftrack A. However, it gets to halftrack 1 and then sticks there.
CURTRK starts at 0, and TRKN is also 0.

```
 | PC: $D122, A: $00, X: $0C, Y: $FF, P: $26, S: $99 || D122: JSR $D133 [#D124] -> S[0x01 98]$D133
 | PC: $D133, A: $00, X: $0C, Y: $FF, P: $26, S: $97 || D133: STA $D372   [#00] <- $D372
 | PC: $D136, A: $00, X: $0C, Y: $FF, P: $26, S: $97 || D136: CMP $D35A   [#0A] <- $D35A   M: 00  N: 0A  S: F6  Z:0 C:0 N:1 V:0
```
Current track is A. Desired track is 0. We go from this to:
```
 | PC: $D122, A: $00, X: $0C, Y: $FF, P: $26, S: $99 || D122: JSR $D133 [#D124] -> S[0x01 98]$D133
 | PC: $D133, A: $00, X: $0C, Y: $FF, P: $26, S: $97 || D133: STA $D372   [#00] <- $D372
 | PC: $D136, A: $00, X: $0C, Y: $FF, P: $26, S: $97 || D136: CMP $D35A   [#02] <- $D35A   M: 00  N: 02  S: FE  Z:0 C:0 N:1 V:0
```
track 5 to track 1?
How did D35A change to that in one call to JSR SEEK?
ok look at this...

```
 | PC: $0975, A: $65, X: $65, Y: $20, P: $20, S: $A1 || 0975: LDA $C080,XPH: slot 6, drive 0, phase 2, onoff 1
new (internal track): 10, realtrack 5, halftrack 0
 | PC: $D190, A: $63, X: $63, Y: $00, P: $24, S: $95 || D190: LDA $C080,XPH: slot 6, drive 0, phase 1, onoff 1
new (internal track): 11, realtrack 5, halftrack 1
 | PC: $D190, A: $61, X: $61, Y: $01, P: $24, S: $95 || D190: LDA $C080,XPH: slot 6, drive 0, phase 0, onoff 1
new (internal track): 10, realtrack 5, halftrack 0
```

The first step we are going in the wrong direction a half track. Then when we end all these seeks we end up at track 1 (02) instead of 0.
I wonder if it's got code somewhere I'm not seeing that reads the track number from the disk to update CURTRK. Yes, at D171 it takes the read TRACK number and calls CLRPHASE.

Is it possible that from the first to 2nd step we're not decremending the track number when we should? A half track down from track 5/0 is track 4/1, not 5/1. No, we count in half-tracks. 

```
 | PC: $D190, A: $61, X: $61, Y: $FF, P: $24, S: $96 || D190: LDA $C080,XPH: slot 6, drive 0, phase 0, onoff 1
 | PC: $D190, A: $66, X: $66, Y: $03, P: $24, S: $96 || D190: LDA $C080,XPH: slot 6, drive 0, phase 3, onoff 0
 | PC: $D190, A: $64, X: $64, Y: $02, P: $24, S: $96 || D190: LDA $C080,XPH: slot 6, drive 0, phase 2, onoff 0
 | PC: $D190, A: $62, X: $62, Y: $01, P: $24, S: $96 || D190: LDA $C080,XPH: slot 6, drive 0, phase 1, onoff 0
 | PC: $D190, A: $60, X: $60, Y: $00, P: $24, S: $96 || D190: LDA $C080,XPH: slot 6, drive 0, phase 0, onoff 0
 | PC: $D190, A: $66, X: $66, Y: $03, P: $24, S: $95 || D190: LDA $C080,XPH: slot 6, drive 0, phase 3, onoff 0
 | PC: $D190, A: $64, X: $64, Y: $02, P: $24, S: $95 || D190: LDA $C080,XPH: slot 6, drive 0, phase 2, onoff 0
 | PC: $D190, A: $62, X: $62, Y: $01, P: $24, S: $95 || D190: LDA $C080,XPH: slot 6, drive 0, phase 1, onoff 0
 | PC: $D190, A: $60, X: $60, Y: $00, P: $24, S: $95 || D190: LDA $C080,XPH: slot 6, drive 0, phase 0, onoff 0
 | PC: $D190, A: $63, X: $63, Y: $00, P: $24, S: $95 || D190: LDA $C080,XPH: slot 6, drive 0, phase 1, onoff 1
new (internal track): 11, realtrack 5, halftrack 1
 | PC: $D190, A: $64, X: $64, Y: $00, P: $24, S: $95 || D190: LDA $C080,XPH: slot 6, drive 0, phase 2, onoff 0
 | PC: $D190, A: $61, X: $61, Y: $01, P: $24, S: $95 || D190: LDA $C080,XPH: slot 6, drive 0, phase 0, onoff 1
new (internal track): 10, realtrack 5, halftrack 0
```

when we turn phase 1 on on that 3rd from last line, the code says:

```
case DiskII_Ph1_On:
            if (DEBUG(DEBUG_DISKII)) DEBUG_PH(slot, drive, 1, 1);
            if (last_phase_on ==2) {
                seldrive.track--;
            } else if (last_phase_on == 0) {
                seldrive.track++;
            }
            seldrive.phase1 = 1;
            seldrive.last_phase_on = 1;
            break;
```
the last phase on was 0 (first line) so we increment the track - which is probably wrong.

UTA2 - "Even numbered tracks are phase-0 aligned, and odd-numbered tracks are phase-2 aligned".
This means that if we're on track 5, we're aligned with phase 2, which means turning on phase 1 will DECREMENT the track.
ok so instead of using "last phase" . last phase is not indicative necessarily. because we might be *changing direction* of movement.

Dude, that is working.

So, I see one of the "acceleration" things people talk about is probably setting all the prodos timer values to low numbers for track seeking, the phase on-off tables at $D373. We could detect when ProDOS is loaded and running and then set those variables to all be 1 or something, to minimize the amount of waiting around for a virtual disk head to move.

## Mar 29, 2025

I have two major pieces to do at the Apple II Plus stage:
* Writing to floppy images.
* Videx VideoTerm 80-column support.

I might want to take a stab at returning reasonable value for "floating bus reads". Doesn't seem to be preventing anything from working right now though.

See DiskII.md for notes on writing to floppy images.

Working on hgrdecode for dhgr. I have the pixels correct. However, the colors are wrong. Green becomes blue. orange becomes green. purple becomes red/yellow. blue becomes purple.
So the phase is just off by 90 degrees ish. Are we off by one bit position? It's 90 degrees off. that is one bit. If I insert an extra blank bit at the start of each scanline then the colors are correct. But that's not right... In the diagram in UTA2E it shows the Auxiliary byte coming in before the hgr bytes in the other examples for single hires? I don't understand.. One fix is to simply change the phase. But something is off here. It could well be the sample files? I need to do some paid work now!
There is something fonky in the OE where it's blowing off the last bit of pixel data?

this is at the end of drawHires80:
  if (x==39) {
      blank80Segment(p+CELL_WIDTH);
  }
"blank an 8 pixel segment".

ok, this is because the whole display is shifted left by one byte position (one 80 column char, or one dhgr byte) when in 80-column mode. It does start a byte early, so the phase is going to be shifted here because it starts early.

Something's not right here. And maybe it's the core - remember how I shifted negative colorburst to positive colorburst? Maybe that was covering up some other problem, and now I'm wrong by about 90 degrees.

Double hi-res uses four bits per pixel. 16 colors. How does it "get started" - i.e., does it start with 3 blank bits and 1 data bit? Or does it clock in data bits first before displaying a color? Maybe that's the pre-shift I see in the diagram in UTA2E.

Nick Westgate: "IIRC, all double resolution modes start one byte before all normal resolution modes."
So that explains the stuff to the left of the diagram on page 10 of UTA2E.

Also, UTA2E says the video ROM lookup inverts all the bits going out to video in hires modes. WHAT?!

Maybe it's not +1.. but -7?  (or -6).

Each byte in the diagram is a 90 degree shift? 

So tweak phase to start at 0.25 (90 degrees). problem solved.

## Mar 30, 2025

tweak to hgrdecode to make it start with phase difference of 90 degrees for doublehires.

Implemented write-protect detection on image files in Disk II. Still need to implement that in pdblock2. A nice workflow. If a file is erroneously WPd, you can unmount, change the FS bit, then re-mount. Voila!

Need to change the OSD so when I click on a mounted disk, it unmounts it. Then a separate click, to mount anew.

## Mar 31, 2025

Instead of setting a flag to raise the window, let's use an event queue structure. I was thinking we'd use this to manage the disk drive sound sample stuff.

Let's make a little test program to play sound samples. That was easy, sheesh.

To simulate disk sounds: the motor on sound is easy, just keep playing the same audio data in. Head movement: it seems like we want to play the whole track sound, but, if we determine the head has -stopped- moving, flush it at that point. Have to come up with some appropriate hueristic for "stopped moving" (for example, hasn't moved in 1/60th second?) The bump "stop" sound should be a "one and done", we play it, then forget it. 

Looking at the existing speaker code with this example code in consideration, I now see that I don't HAVE TO send it constant data including empty frames. I -can- just feed it data when I have it. It will fill with silence otherwise. And, if I decay my values (going from + or - to 0) in a reasonable time, then there won't be clicks when that occurs. Something to think about..

I should also move the general audio subsystem init into its own file, util/sound.cpp. Then the various things that need to emit sound will reference that. When we get to the GS, we'll need lots of AudioStreams (one for each "voice" of the Ensoniq chip).

## April 1, 2025

We need to somehow detect multi-track head movements. One track at a time sounds right. But when the head swoops across a big area we need to play successive chunks of the sound, not simply repeat the first bit of the sound for each track moved.

I think we need more border. I would like to draw disk drives in the margin when they're running, so more border will help. Draw them with maybe 50% opacity. And draw using a Container. Instantiate new Disk II / Unidisk buttons. And the container needs to be dynamically managed to contain only actively spinning media. Have the opacity fade in and fade out.

Trying to format a 5.25 with ProDOS 1.1.1 filer, I get the error "Disk II drive too slow". That is special.
DOS33 is doing an INIT. I guess we'll see what it looks like in AppleSauce!
Well it looks ok to me. 

The DiskII "wrong drive thing"... this is what we do.

MOT: slot 6, drive 0, motor 1
DS:slot 6, drive 0, drive_select 1
PH: slot 6, drive 1, phase 1, onoff 1

turns on the motor, then does drive select 1. apparently, this is supposed to switch the motor to drive 1.

FIXED!

Well, well. You can write to the drive select and drive on/off registers, and, locksmith does that. Locksmith is now validating disks.

A format isn't working right. And maybe my DOS3.3 format didn't actually work right, but are they verified when you do an INIT? Unsure. This is likely going to be caused by not fake rotating the fake disk under the fake head even when we're not doing anything. I could try reenabling that code...

OK, still have problems initializing disks. I'm only modifying track position on reads. That's an issue. So need to bring that code out to the top of the disk II registers routine. So any time we hit a register we're updating.

Call me crazy, the dos33 system master boot seems faster.. well it's not working right.

I bet on inits, this thing is trying to write 10-bit FFs. and here in a sector header we have..

FF FF FF FF D5 AA 96 FF FE AA AA AF AF FA FB DE AA EB E7 FF FF FF FF FF D5 AA AD 96 96....

DE AA EB E7 FF ?

That E7 is supposed to be an FF.

here's another broken one:
FF96FF
that's after a sector, it ought to be FF.

ok, will have to study the cycle timing on writes. 

## Apr 2, 2025

Writing a self-sync 

Proposal: read or write a bit at a time.
on read: set "bitsleft" to 10 if FF. otherwise to 8. So we actually feed emulated DOS 2 zero bits on a read.
on write: we're not going to store the 0 bits, so just set it to 8.

I have made the following tweaks: increment the disk 'head' position by one BEFORE a write. An init is working and can save the file, however, I am getting extraneous 96 after the newly written data fields. Before, I was getting the last byte of the address field chopped off - that was creating I/O errors.

(I may have to go ahead and implement this sequencer state machine.)

ProDOS still not working, filer reports "disk ii drive too slow".

So, what if that is ProDOS complaining that there are too many bytes in Gap A? This whole 0x1A00 thing is a suggestion, not a command.. let's measure how many bytes we are actually generating from our conversion.
yes. So comparing to an AppleSauce conversion to nibblized, we have 2100 more bits, 360 more nibbles, and duration of 208200us versus 199997us. 300rpm is 5 rotations per second is 200,000us. So, we need to actually mark how much data we write, and then use THAT as, um, track_data_size.
I am generating a track.size. So let's use it..
ProDOS still no likey, however, disk stuff ought to be up to 5% faster since we cut 300 bytes out of 6000. DOS read, write, init is working ok still.

Comment from Nick Westgate:
ProDOS will have something similar to what I mentioned in a previous comment: [For DOS] you need to provide at least one different nibble in every 16 nibbles to defeat the SAMESLOT drive speed check in DOS/RWTS at BD34-BD53 (according to Beneath Apple DOS). ProDOS is different though IIRC.

that's not it.

However, I am starting to think ProDOS is doing something not mentioned in manuals. 

## Apr 4, 2025

Thanks to a tip from Nelson, forcing a disk image track to be fewer bytes (reducing GAP A when generating nibblized version for instance) makes ProDOS Format work.
But now DOS 3.3 format does not. I reduced from 149 to 64 bytes. Let's increase a bit..

Final update: Many thanks to Nelson Waissman who put me on the right ... track ... ha ha pun intended. The number of nibbles in the virtual disk track determine the "speed" the disk is spinning. 
If too short, DOS3.3 won't format. (not enough room for how DOS33 init's a track!)
If too long, ProDOS thinks the disk is too slow.
His number of 0x18D0 is exactly right.

Locksmith now: Verify 16 sector ok; Format 16 sector ok; it cannot bit-copy. It keeps complaining about "FRAME BITS 02, LONGSS #FB02". run locksmith disk speed thing. Still reports the disk is really really slow.
ProDOS now: format volume ok and can copy files to it;
DOS3.3 now: init volume ok and can write files to it.
DOS33 and ProDOS 2.4.3 format, disk copy, etc with CopyIIPlus- works.
Copy II Plus drive speed test - doesn't work.

Proposed task list for next release (0.20)
* finish up Disk II write by denibblizing (if needed) and writing disk II data back to disk file.
* clean up video code so we call old monochrome hires code in that mode. [ complete ]
* Center display in window when in fullscreen mode. [ complete ]
* Redo joystick code so it maps correctly when it comes online after emu start. (sometimes starts with Y reversed!, or not right joystick at all)
* Decay the audio so we don't get so many annoying clicks due to OS interrupting my event loops [ complete ]

That ought to be easy!

## Apr 5, 2025

building on windows - Win doesn't like strndup because it's a POSIX function. Find a standard c++ library alternative.
and really do some cleanup, use C++ variants of std library stuff instead of a mishmash of c and c++.

First pass, not awful, though having some issues when linking against SDL. Some weird Windowsism.

on the speaker decay. When we hit an event, set the amplitude to 0x6000. Each sample after that, decay the amplitude 0x0100. (that's about 90 samples, can adjust this). If we fill empty frame, continue using decaying amplitude. Sample value at any point is amplitude * polarity. I currently have it to decay at 0x0300. That doesn't seem to hurt frequency response at all. I also reduced the max amplitude to 0x5000 from 0x6000. 

There is a bug occurring where the display stops updating right when I move the window between screens, sometimes. oh weird. It's when the left edge of the window is near the left edge of the screen?? only my right screen. And, if the left edge is within the first inch or so. What the. Special.

hm, see what events we might be getting when we're close to the window edge.

Time to make the denibblizer!

To be fair, I have denibblizer examples in both the DOS3.3 source code, and, the Disk II boot firmware. How hard could it be?

The basic process for what was block data file:

1. For each track:
   1. Mark 16 "sector found" flags all false.
   2. Start at position 0 in the track. As we scan, we might have to wrap back to the start of track.
   3. Scan until we get to a D5 AA 96 whatever the sector address field marker is.
   4. Extract the sector info.
   5. Scan until we get to a D5 AA AD?
   6. Read 342 nibbles into denibblizer buffer
   7. Decode buffer and validate checksum
   8. Copy decoded buffer into track block buffer with offset based on track X + sector Y * 256. Will have to take the interleave into account.
   9. Set "sector found" flag for that sector.
   10. If all sectors were found, Write track out to image file 
2. Repeat for all tracks.

### For the user-interface:

Position an overlay container over OSD:
   * Media FILENAME in SLOT X DRIVE Y was modified! Write changes back to file?
   * YES | DISCARD | CANCEL

Perhaps this will be a Dialog, not a Container?

For this we will draw everything, then draw this on top (it will always be after all other containers), and direct input events only to this container. (it is a modal dialog after all). 

[ ] strndup is a posix function. Replace with a C++ convention.

## Apr 6, 2025

OK the denibblizer is done! A lot of my trouble was being tired and mixing up what files were going where so all my testing was bogus. Don't TARRED AND CODE!

Denibblizing in the standalone tool, we need to know what interleave to use on output. We don't have it from raw nibble form. So the user needs to specify it on the command line. either --prodos or --dos33. (-p or -d).

Inside the emulator, we will track the interleave 

Either way it should get passed into the denibblizer routine as a parameter. pass the table, or a flag? 

Audio issue - I still get clicks in the first few minutes after boot when opening/closing the file dialog and causing underruns. So we must still be generating non-zero fills somewhere.

in the OSD code that mounts a disk, it is passing in the char * filename. I bet this is getting deallocated. We better strndup it.

Some additional thoughts:
don't allow unmounting if modified is true and the motor is on? or just punt that and give them UI choice anyway?

OK, the diskII write code is DONE! Works for DOS33, and ProDOS. Writes back to original block format, or, writes back to nibblized file, depending on origin. (i.e., back to original format). There is no UI yet - if you unmount a modified disk, the image WILL BE saved back out if you unmount.

If you don't want to save changes that were written to an image, for now, don't unmount it, just F12 / close the emu which will exit without saving.

