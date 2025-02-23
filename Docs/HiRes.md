# Hi-Res Version 3

The current hires implementation (v2) is a little messy, with weird handling of filling between pixels, etc.

Doing a lot of reading and AI research / testing on this.

The color is determined by the phase of the data signal in comparison to the color reference signal.

The color reference signal is 3.5M.

Here's a 3.5 signal:

```
Color Ref:
     ____      ____      ____      ____      ____      ____      ____      ____      
____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

Bit Clock:
     ____      ____      ____      ____      ____      ____      ____      ____      
____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

````

if the bit clock is in phase with color reference, and we have a 3.5M signal (i.e it's actually oscillating), then we have green.

```
Color Ref:
     ____      ____      ____      ____      ____      ____      ____      ____      
____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

Bit Clock:
          ____      ____      ____      ____      ____      ____      ____      ____      
     ____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

````

If the phase is off by 1/2 cycle (180 degrees) then we have violet / purple.

Note, in a real apple II these values are offset from the "Text-hires clock" by not quite 90' extra or so, because of delays in the hardware and the way the Color Ref signal is generated. But this is how they get the colors they do.

If the hi bit is set in a data byte, then we are offset by another delay of a 14M (1/4, or 90 degrees).

```
Color Ref:
     ____      ____      ____      ____      ____      ____      ____      ____      
____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

Bit Clock:
       ____      ____      ____      ____      ____      ____      ____      ____      
  ____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    


Color Ref:
     ____      ____      ____      ____      ____      ____      ____      ____      
____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

Bit Clock:
            ____      ____      ____      ____      ____      ____      ____      ____      
       ____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    |____|    

````

These waveforms generate blue and orange.

We can calculate phase by 'measuring' the time between the zero crossings of the color reference and the bit clock. Remember, the "bit clock" is not a clock, but just our data stream of bits from video RAM.

So we need to track the phase in order to determine color. At any given pixel location on the screen (assume 560 pixels), we read the phase at that point in time and select color accordingly.

Lo-res colors and double-hires and various other artifacts are created by different kinds of waveforms: for example, non-square waves (duty cycles other than 50%).

Lo-res colors "dark" have 25% duty cycle. "Light" have 75% duty cycle. The duty cycle affects the luminance. Is that how we get white ("100%") and black (0%) luminance?

