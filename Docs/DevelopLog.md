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

Pixel size: Each character is made up of a 5x7 pixel bitmap, but displayed as a 7x8 character cell on the screen.
there will be 1 pixel of padding on each side of the character.

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

Integer BASIC programs have a two-byte (little-endian) length.
Applesoft BASIC has a two-byte length, followed by a "run" flag byte.
Applesoft shape tables (loaded with SHLOAD) have a two-byte length.
Applesoft arrays (loaded with RECALL) have a three-byte header.
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
$CS


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

Mark Bytes for Address Field: D5 AA 96
Mark Bytes for Data Field: D5 AA AD

### Head Movement

to step in a track from an even numbered track (e.g. track 0):
LDA C0S3        # turn on phase 1
(wait 11.5ms)
LDA C0S5        # turn on phase 2
(wait 0.1ms)
LDA C0S4        # turn off phase 1
(wait 36.6 msec)
LDA C0S6        # turn off phase 2

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
And we need to keep track of the "color-killer" mode: in mixed text and graphics mode,
the text is color-fringed. In pure text mode, the display is set to only display white pixels.
Actually this implies that even on lines in text mode, we might need to draw the text with
the slight pixel-shift and color-fringed effect. In which case, all buffers would always be
560 wide. And, we would likely post-process the color-fringe effect. I.e., draw a scanline
(or, 8 scanlines), then apply filtering to color the bits. Then render.

In monochrome mode, a pixel is just on or off. no color. But, probably still pixel-shifted.

Depending on how intricate this all gets, we may even need to use a large pixel resolution than 560x192.
Would have to read up on the Apple II NTSC stuff in more detail.

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

Oh. I should disable my "accelerator".

So, 300rpm ?