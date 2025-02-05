### Thunderclock

From "ThunderClock_Plus.pdf"

For direct hardware access to a TCP (ThunderClock Plus):

* Raise strobe for at least 40uS.
  * CREG is 0xC0S0 (where S is slot number plus 8).
  * Set bit STB (bit 2, value 0x04) high, and store the command plus the STB bit to card.
  * Then set same value back with the STB low.
* Then the data to be shifted out is 10 nibbles worth, 4 bits each.
  * Seems like the high bit (bit 7) of CREG. Code does:

CLK = $2
STB = $4

uPD1990AC hookup:
	bit 0 = data in (to clock)
	bit 1 = CLK
	bit 2 = STB
	bit 3 = C0
	bit 4 = C1
	bit 5 = C2
	bit 7 = data out (to cpu)

  LDA CREG,Y
  ASL  ; data out bit to carry
  ROR TEMP2 ; carry into bit 7 of TEMP2

They call pushing data the the clock "IN". Reading data from the clock is "OUT".

Reads the in bit. Sets the out bit.
Then raises and lowers the clock.
TIMRD = $18 - this puts it into read mode.
They just use the same routine for both IN and OUT, depending on the mode.
How weird.

SO.

When we get the strobe plus command $18, (bits 0b0001 1100):
* calculate current time values, store into a 40-bit buffer.
* each cycle where they then read and tick the clock, shift that
* the 'read' just reads either the lowest or the highest bit, whichever it is.

nibble 0: month (1 - 12 binary)
nibble 1: day of week (0 - 6 BCD)
nibble 2: date (tens place, 0-3 BCD)
nibble 3: date units (0-9 BCD)
nibble 4: hour (tens place, 0-2 BCD)
nibble 5: hour units (0-9 BCD)
nibble 6: minute (tens place, 0-5 BCD)
nibble 7: minute units (0-9 BCD)
nibble 8: second (tens place, 0-5 BCD)
nibble 9: second units (0-9 BCD)

So we need state tracking of the strobe; and state tracking of the 'clock' bit.
