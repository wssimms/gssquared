# Tracing / Debugging

We will ease into debugging. Otherwise it will be a giant rabbit hole.

But, tracing.

Have a large memory block.
Debug statements, generate text into the block. Keep track of head and tail. And, wraparound.
For now, have a way to dump that to disk, starting from wherever the head is.
Eventually, be able to scroll through it in-app.
Have a UI widget on that screen that lets you enable the various debug bits one at a time.

So instead of constructing the text output, the trace creates first a binary log. Each binary log line is a compact array of structs.
This is constructed without any conditionals.

E.g.,

struct system_log_entry_t {
    uint64_t cycle;
    uint8_t opcode[4]; // up to 4 bytes per opcode for 65816.

    uint16_t a;
    uint16_t x;
    uint16_t y;
    uint16_t sp;
    uint16_t d;
    uint8_t p;
    uint8_t db;
    uint8_t pb;

    uint8_t address_mode; // decoded address mode for instruction so we don't have to do it again.
    uint32_t effective_addr; // the effective memory address used.
    uint8_t memory_read_write;
}

cpu {
    alignas(64) log_entry_t log;
}

12 + 11 + 1 + 2 + 1 = 31 <- sweet

This records the opcode and operand data; the values of registers before execution of the instruction; and the memory value read or written.

So this is under 32 bytes probably. We can record the last 1 million instructions in 32 megabytes.
Decoding the info of course uses lots of conditionals, but that will be out of band, and only for a relative handful of instructions, and we won't care. Each frame of debugger display might have 60 to 100 lines on it. Big whoop!

Also then easy to dump the whole thing in binary format and decode outside the emu also.
log Trace on-off hot key; when enabled, we stream binary log to disk. This will generate reams of data.

Step over-execute single instruction. if single instruction is JSR, then execute until we get to a RTS.
Step into-just execute single instruction.
breakpoint - break on instruction execution; break on memory access. These can be compact bit arrays of 4KBytes. one bit per byte.

We want to make the log struct be divisible into a cache line, so, 64 bytes, 32 maybe. And be 32-byte aligned. For that matter, we want the whole cpu struct to be aligned that way.

Examining the current code, and how it is compiled, we definitely cause a lot of cruft having the IF(DEBUG()) stuff in there. Since we are bit testing against a variable, the compiler cannot optimize away the IFs even if no flags are set.
I think the log record needs to go into cpu().
Have a single bool in cpu for tracing enabled / disabled.

# Monitor / Debugger interface

Tracing is working pretty well so far. Can trace on/off, can single-step and resume execution. Got a lot of great ideas on FB:
```
* Memory window(s). In particular I recently found myself debugging in an emulator that only let me watch one contiguous chunk of memory, but I wanted to watch two as the date being manipulated were in very different regions.
* a disassembly window centered on the current line of code
* text screen
* lo res screen
* hi res screen
It would also be nice if the tabs you proposed were tear away tabs, so that you could look at multiple at the same time by moving each window around on your monitors as needed.
-Memory view. It’s nice to show different “formats”. Hex, Text (maybe ASCII and Apple II encoding).
- Setting memory breakpoints. When someone reads (or writes) from $A0, stop.
- View the current Text screens, GR, HGR, HGR2, DHGR, even if they are not active. Extra credit to render and arbitrary bit if memory (looking for sprite data, etc) as hgr/gr/etc.
- Disassembly window that shows upcoming instructions. Lets you set breakpoints.
- A panel that shows all soft-switches values.
- Load symbols for code complied in CC65.
- I like the idea of a little monitor too.
- Load a blob of compiled code into memory and execute it. (bload from local machine). You could make a very smooth development workflow. Compile on the PC, load into mem and debug.
- Save / Restore state of the whole system. No need to wait to boot something. Save your games, etc.
```

I had a UI idea: when the mouse is moving over the window, show some controls. One of them will be a triangle that opens the OSD. That same tab also closes the OSD. It will be on the upper right corner sticking out, so it will always be clear what's up with it.

[ ] implement the "temporary message at bottom of screen" idea I had; use it for mouse capture.   
[ ] use general mono_color_table for rendering videx too. Probably bring those variables out and centralize them somewhere.  

Mike sugggested the "save machine state". We would need to save:
* contents of memory
* cycle counter
* PC, registers, etc. of course
* values of softswitches

To restore machine state, seems like the copymaster etc cards had to have done this. Instead of trying to save all of the emulator's state, perhaps we can "play back" the soft switch settings in some cases, i.e. call the routines via the memory bus interface. The audio modules (speaker, mb) are going to be tricky.

So multiple requests for "show upcoming instructions". That's probably straightforward to do. Draw those in a different color to indicate they're not executed yet. Show a highlight or arrow or other indicator. Kicks in when you enter step-wise mode. The trace is everything that -has- executed. The disassembly would be everything that -will- be executed, including whatever is at the current program counter.
Definitely implement a check that doesn't require re-rendering the debug window if nothing has changed. (e.g., sitting and staring at stepwise).

I'm liking the idea of maybe extending the width of the debug window to show:
* breakpoints
* monitored memory
* softswitch values per Mike above
* mike says maybe refer to compiler symbol table?
* show labels for well-known addresses - based on the effective address. would be hard for forward-disassembler.

I was thinking I like the idea of combining monitor and debugger interface. so you'd have the regular monitor interface commands: 
XXXX.YYYY to display memory. 
XXXX YYYY.ZZZZ M to move memory. XXXX:YY YY to set memory. etc.
But also things like "1234 BP", "1234.123 BP" <- set a breakpoint. 
how closely to make this mirror monitor? there aren't that many commands in the monitor. Well, a mini-assembler could be cool.
so, items are:
```
address
address.range
"string"
:val [..val, val, val]
commands are: move, set, clear, (breakpoint set / clear), L (list/disassemble), V (verify), R (from file specified in string), W ()
R would be:
load  "filename" 2000
save  "filename" 3000.4000
break C000
-break 3FE.3FF   - remove breakpoint 3FE.3FF
break    - by itself, list breakpoints
list / L 2000    (disassemble; L by itself, continue disassembly from last dis PC)
mon 2000  (memory monitor set)
-mon 2000.2010  (memory monitor cancel)
mon     - list memory monitors
```

you would enter commands in a SDL "textedit" thing I just read about. and we can keep a small history with arrow key to get to them.

Commands are treated like a FORTH stack (except normal order). You push arguments onto the stack. addresses, address ranges, values, command-words. Command-words will pull values off the stack and execute them.

Step out of: run until next RTS;
Step into: single step
Step over: single step, but if a JSR, run until RTS;

OK, so that's one approach. The other is more of a full GUI approach. That would be a lot of work. maybe there is a widget library I can lean on.
RmlUI is pretty sophisticated. Might do most of the things we want? Looks pretty snazzy too. ImGui looks more like stuff from 20 years ago. Square and blocky.

