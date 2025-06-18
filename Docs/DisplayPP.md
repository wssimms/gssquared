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

