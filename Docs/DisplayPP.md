# Display ++

This is the next evoluion of the display code.

We break down the Apple II motherboard display system into several stages.

1. Generate 560x192 bitstream.
2. Render bitstream to a pixelmap (and then texture)
3. Render texture to window

There are a few motivations for this:

1. have all display routines work on data in and out in the same manner and with the same interfaces
2. allow great stuff like WmS cycle-count based display logic
3. allow API for "give me the current lo-res/text/hgr/dhgr frame" for the debugger.

Currently we have:

update_display_apple2

This loops the 24 "lines" and if changed, calls the line type specified. However, is is calling functions named stuff like "render_line_ntsc". So, that's inside-out from what we want. This part of the code should not care what the final rendering routine is.

So, update_frame_apple2 should be modified to call function that generates an intermediate bit stream:

* generate_line_text
* generate_line_lgr
* generate_line_hgr
* generate_line_text80
* generate_line_dlgr
* generate_line_dhgr

Something then sends the frame to one of these display render routines:

* render_frame_ntsc
* render_frame_rgb
* render_frame_mono

#define BITSTREAM_WIDTH 567 - it's still 560, it's just displayed earlier. We compose into pixelmap in different spot.
#define BITSTREAM_WIDTH_ULL ((uint64_t)BISTREAM_WIDTH+64)/64

This is because the IIe starts lines 7 pixels early (and phase shifted) when in 80-column modes. We may want to tweak this to include the borders. And the GS is even more differenter.

#define BITSTREAM_HEIGHT 192

Same questions here.

class Frame_Bitstream
    Bitstream(uint16_t width, uint16_t height)  // pixels
    uint64_t display_bitstream[BITSTREAM_HEIGHT][BITSTREAM_WIDTH_ULL]
    uint64_t working = 0;
    int scanline = 0;
    int hpos = 0;
    int hloc = 0;
    push(bit) { working <<1 | bit; hpos++; if hpos>=64 { display_bitstream[scanline][hloc++] = working; } }
    bool pull() { return working & (1 << 63); hpos++; if hpos>=64 { working = display_bitstream[scanline][hloc++]} };
    setline(line) { scanline = line; hpos = 0; hloc = 0; }

use cases:
    setline(line); then push a line's worth of bits.
    setline(line); then pull a line's worth of bits;
    mixing will have undefined results.

display() owns one of these. But the debugger can allocate its own and call the generators and renderers to get its own display images of not-the-current-display-mode.
(For extra credit: implement a VNC server inside GS2 that allows remote access to the VM - how hard could that be? tee hee).
We can also do a "wayback" thing that lets you see video frames from prior points in time. Offering a kind of scrollback functionality. These compact bitstreams would be very memory efficient.

directories

displaypp/render/ntsc.cpp
displaypp/render/rgb.cpp
displaypp/render/mono.cpp
displaypp/frame/frame.cpp, .hpp
displaypp/generate/text.cpp
displaypp/generate/text80.cpp
displaypp/generate/cyclestream.cpp
etc.

## cycle-based video stream

stream.cpp is what handles cycle-based video streaming for emulating cycle-accurate video routines on the II/II+/IIe/IIc. When activated, this feature causes incr_cycle to lookup and read current video byte, inside incr_cycle. Then that is streamed to a buffer that holds three byte values:
[mode] [maindata] [altdata]

Then generate/cyclestream.cpp reads that and renders a whole frame to a Frame_Bitstream. Then rendering stage as usual.

# Pixel Encoding

Pixels with the new Frame concept are 1 or 0. However, I think the NTSC code expects 0xFF or 0.
Monochrome seems like the easist place to start, and, from the bottom up.
I wonder if I should have a little bit more formal pixelmap class. Yes.
ok I created a template version of the class that will take any old value, 0, 1, 0xFF. But I should decide if there is a reason I want to keep using 0xFF.

PIXELFORMAT_ABGR_8888 is twice as fast as PIXELFORMAT_RGBA! Well now that's special. I still seem to be going way slower than gs2 itself..

differences are that I'm using rectangles. I could try that..

## Character ROMs

So, the IIe character ROMs are 4K for US, and 8K for foreign.
The first 2K seems to be characters as usual.
The next 2K seems to be related to the character index. Feels graphics-related in some way.. maybe they feed bits into hires / double hires with this?

```
Character 0x1FD (offset 0x0FE8):
*.....*.  
*.....*.  
..*...*.  
*...*...
*.....*.
*.....*.
........
........

Character 0x17D (offset 0x0BE8):
*.....*.
*.....*.
..*...*.
*...*...
*.....*.
*.....*.
*...*...
..*...*.
```

The bit order is reversed compared to II+. Also, the meaning of bits may be reversed here too (i.e., 1 means "pixel off"). So we need to decide whether to reverse the ROM file. I could have different code modules for each, but, it seems like a smart ROM loader that would mangle the bits the way we need so the same routine could draw ii+, iie, what have you would be the way to go. I think we want to reverse the II+ one, and shift out the right. 

## Quick entry into Double Hi-Res

If you know how to write to the iie 80 column text screen directly, it should be easy enough.
The cleanest sample I have seen to turn it on is
10 D$=CHR$(4)
20 PRINT D$;"PR#3" : REM 80-column mode
30 POKE 49246,0 : REM $C05E - enable double resolution graphics
40 GR : REM Turn on double lo-res mode
Once in this mode, data stored to the 80 column screen will show up as double low res. On the iigs you can just use the plot command in AppleSoft. I think on the iie you have to do it with pokes.
