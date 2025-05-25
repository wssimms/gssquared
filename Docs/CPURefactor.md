


### consider refactoring cpu execute_next.

Now, I've got this function pointer for execute_next. let's contemplate..

cpu_state
  cpu_6502_state
  cpu_65c02_state
  cpu_65816_state

Thinking about 6502 vs 65c02. There are a large number of identical opcodes. Then, c02 has a number of additional ones. Currently I do a big switch statement. 