# Clipboard

## Copy

"Copy" for now will do the following:

* Pre-allocate enough memory for conceivable Apple II bitmaps (640x216 should do it)

Call the Display Engine Copy routine

This routine (depending on display engine) will:

* Copy the screen buffer to the temporary memory after it.
* return the bitmap dimensions

"Copy" will then populate the bmp header for use when / if the clipboard callback routine is called.

