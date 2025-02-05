# Debugger

The purpose of the debugger is to allow the user who might be diagnosing a bug in the emulator, or, a bug in software they are writing, to have a way to see what is happening in the virtual Apple II.

Debugger will operate in a separate window from the main screen. It will be an "always on top" window.

It should have the following features:

* setting breakpoints in the CPU / memory
* Display of last 10 - 20 disassembled instructions.
* speculative disassembly of next 10-20 instructions, IF execution is paused. Have to assume straght-line execution (no conditional branches). Could follow JSR / JMP or BRA.
* Display of CPU registers & flags
* As we have now, display the data value that went out to, or came in from, memory.

Might be nice to see states of common hardware registers, like keyboard, annunciators, 

Apple2ts has a nice thing where it shows the state of the various bank-switched memory regions. It also has the ability to dump memory ranges.

It would be ideal, to be able to define memory watch ranges, which will display the data in the range and be updated every time an instruction is executed.

now, if we're free running, we can't update display every instruction. So we would 'snapshot' every frame (1/60th second) and just draw what's there.

For controls, there should be normal debugger controls:
* step over - run JSR'd subroutine at full speed, stop when we get back
* step into - step and follow JSR
* step out - run until a RTS or RTI
* continue

I think the above can be handled easily by automatically setting appropriate breakpoints. e.g., step into is "set breakpoint at next instruction, then run".

## Architecture

So, the historical disassembly would be in a circular buffer.