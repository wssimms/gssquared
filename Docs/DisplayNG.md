# Display Next Generation

Currently hgrdecode uses source data of single bytes that are either FF or 00.

The new ntsc.cpp implementation in it converts to bits.

We use a 17-bit sliding window of the bits and feed that into a lookup table. Instead of starting with bytes and converting to bits, just start with bits and feed bits. This routine is then going to be hyper-efficient.

We're going to want routines that adds certain numbers of bits to the bit stream, that all the display modes will use:
   * 4 bits, for lo-res.
   * 1 bit, for text mode.
   * 14 bits for hires mode.
   * 2 bits for double hires.

All the display modes will be reimplemented the same way - as a 560x192 bit grid, that will be fed directly into the composite renderer.

Display data is converted from whatever source format, into bits.

64 bits 

Since 560 isn't divisible by 64, we'll pad to get us to 576 bits per line layout in memory.

Then the rendering routine will have a func that provides the next 17-bit window of bits from the source data. This can all be implemented with keeping a counter, and two 64-bit values from the source line. The current 17-bit value is just "reg0 & 0x1FFFF" into the lookup table. Whenever reg1 is empty we fetch the next 64-bit value from the source line or a 0 if we're at the end of the line.

One goal of this, is to optimize for caching. The M1 L1 cache line size is 128 bytes. This means that every one of our scanlines will be able to be fetched with either 1 or 2 cache lines, only two memory accesses. Most of the heavy lifting will be done shifting stuff in registers.

L1 is 128KB, L2 is 12MB. This means the entire lookup table will easily fit into L2 cache. We may want to lay out the lookup table in memory in a way that further optimizes cache usage. For instance, we know we will iterate across the phase parameter 0, 0.25, 0.5, 0.75 as we process a scanline. If we've got a run of similar color pixels (e.g. 101010101010) that will be effectively cached. Easy enough to test.

We can do testing to see if the 13-bit is sufficient - it would work the same way. It will use less memory for the lookup table. And may affect overall cache performance. And would be faster to regenerate if the user wants to change any of the input parameters.

The renderer iterates 192 scanlines and 560 pixels per line, drawing each pixel.
It will have two modes:
   * color-kill (i.e., white or monochrome)
   * composite

Maybe I will eventually create a RGB mode that will have different filter parameters going into the lookup table - faster response times.

If we are in a pure text mode, we use color-kill. If we are in a graphics mode, either full or mixed mode, we use the composite mode for the whole frame. Or, do we use a LUT for monochrome that captures the luma value from the composite calculations? I guess it depends on whether luma ever is significantly different from 1. Should be able to back-convert a rendered image to YUV to look at the Y values. If there is variation in the Y values then it would make sense to do LUT for monochrome. If not, just do it the way we do it not.

We don't have to re-calculate the LUT whenever params change. We're a desktop app, we can just pre-generate as many different LUTs as we want to support different modes and mode combinations.

Currently it takes 100microseconds to render a frame. I am betting that we can reduce this quite a bit further.

On the one hand, at 100uS we are already doing great. On the other hand, this is with pretty fast modern hardware. If we can optimize more we can improve operation on older hardware.

## Pseudo-code for frame rendering

At startup time:

   * buildHires40Font(MODEL_IIE, 0);
   * setupConfig();
   * init_hgr_LUT();

In main loop:
   * processAppleIIFrame_LUT(graymap, hgrdecode_LUT_outputImage);
      * this needs to be modified to be called for 8-scanline-chunks instead of the entire frame.

## Video modes - generation of bitstream.

### Text mode

Only for updated lines:
Read and double values from the font. Based on scanline % 8, grab that scanline of data from the pre-computed font and emit each byte twice to double it.

### LGR mode

only for updated lines:
For each scanline, read the byte. scanline / 8 to find the line, then convert line to memory location.
If scanline % 8 between 0-3, emit the lo nibble. If scanline % 8 between 4-7, emit the hi nibble. (or whichever it is).

as long as we're on an X location in this dot, emit output based on the pixel X location mod 4 as an index into the bits of the lo-res pattern. Sometimes we start halfway through a pattern; sometimes we end halfway through a pattern. This is what generates artifacts between different colored lores blocks.

### HGR mode

only for updated lines:
Already know how to do that.

### DHGR mode

only for updated lines:
Basically same as HGR mode, except pulling values from two memory locations and 'mixing'.


## Fancy timing-based updates

The video frame can now be generated, if we like, down to the cycle level. We can track where the "beam" is based on cycle counts. This is just some simple math. So if display mode is changed in the middle of a scanline, we could in theory start rendering the bitstream based on the new mode from that point on. This is how some of the other emulators do it (OE, AppleWin).
I still am not sure how important this is to me. But, we're much closer to being able to do it.
