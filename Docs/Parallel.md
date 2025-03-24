# Parallel Interface (for Printers)

The Apple Parallel Interface card had a ridiculously simple register interface, and, firmware is available.

## Dot-Matrix Printers

libHaru is a C library (that interfaces well with C++) that can create PDF files.

So, explore the approach of implementing printer emulation threads, using libHaru to create PDF files.

The PDF file is the primary output. However, when printing, open a window and show recent portions of the image as it is generated. And it will also have a button to "close print job" which will close the PDF file and set up to create a new one.

PDF files are created somewhere - Desktop? Documents? Somewhere handy like that. With "GS2-Print-YYYYMMDD-HHMMSS.pdf" as the name.

This implies we will treat all output as simply a pixel map. Which is appropriate, as that is what dot matrix printers do. And, this will be easy to both display and stuff into a PDF.

No need to have cancel or anything like that since it's not real paper.

The Printer emulator should run as a separate thread, so it doesn't take up any time in the main loop. So the emu will chuck data to the printer thread.

This will also simplify the state machine management of the printer.

For testing, and initial deployment, we will want to have a printer capture mode that captures the raw data stream to the printer port and saves it to a file. Then have an initial standalone utility to convert those to PDF. That's the best way to get started.

## Laser Printers

There was no software that handled PostScript on the IIe/IIc. People did use LaserWriters but those could apparently emulate an ImageWriter.

# Initial Test Implementation

The implementation is very simple. I have created assemblable source from the Apple II Parallel Card manual, circa 1978 version. For the initial implementation, 

I need to implement something that will flush and close the file after some period of inactivity. Let's say after 5 seconds, it will close the file. Whenever file is not open and a new write comes in, reopen the file in append mode. Also, close the file on a ctrl-reset.
