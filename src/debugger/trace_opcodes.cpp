/*
 *   Copyright (c) 2025 Jawaid Bazyar

 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "debugger/trace_opcodes.hpp"

const address_mode_entry address_mode_formats[] = {
   { "???", 1 },
    { "A", 1 },
    { "$%04X", 3 },
    { "$%04X,X", 3 },
    { "$%04X,Y", 3 },
    { "#$%02X", 2 },
    { "", 1 },
    { "($%04X)", 3 },
    { "($%02X,X)", 2 },
    { "($%02X),Y", 2 },
    { "$%02X", 2 },
    { "$%02X", 2 },
    { "$%02X,X", 2 },
    { "$%02X,Y", 2 },
};

const disasm_entry disasm_table[256] = { 
    { "BRK", IMP },  /* 00 */
    { "ORA", INDEX_INDIR }, /* 01 */
    { NULL, NONE }, /* 02 */
    { NULL, NONE }, /* 03 */
    { NULL, NONE }, /* 04 */
    { "ORA", ZP }, /* 05 */
    { "ASL", ZP }, /* 06 */
    { NULL, NONE }, /* 07 */
    { "PHP", IMP }, /* 08 */
    { "ORA", IMM }, /* 09 */
    { "ASL", ACC }, /* 0a */
    { NULL, NONE }, /* 0b */
    { NULL, NONE }, /* 0c */
    { "ORA", ABS }, /* 0d */
    { "ASL", ABS }, /* 0e */
    { NULL, NONE }, /* 0f */
    { "BPL", REL }, /* 10 */
    { "ORA", INDEX_INDIR }, /* 11 */
    { NULL, NONE }, /* 12 */
    { NULL, NONE }, /* 13 */
    { NULL, NONE }, /* 14 */
    { "ORA", ZP_X }, /* 15 */
    { "ASL", ZP_X }, /* 16 */
    { NULL, NONE }, /* 17 */
    { "CLC", IMP }, /* 18 */
    { "ORA", ABS_Y }, /* 19 */
    { NULL, NONE }, /* 1a */
    { NULL, NONE }, /* 1b */
    { NULL, NONE }, /* 1c */
    { "ORA", ABS_X }, /* 1d */
    { "ASL", ABS_X }, /* 1e */
    { NULL, NONE }, /* 1f */
    { "JSR", ABS }, /* 20 */
    { "AND", INDEX_INDIR }, /* 21 */
    { NULL, NONE }, /* 22 */
    { NULL, NONE }, /* 23 */
    { "BIT", ZP }, /* 24 */
    { "AND", ZP }, /* 25 */
    { "ROL", ZP }, /* 26 */
    { NULL, NONE }, /* 27 */
    { "PLP", IMP }, /* 28 */
    { "AND", IMM }, /* 29 */
    { "ROL", ACC }, /* 2a */
    { NULL, NONE }, /* 2b */
    { "BIT", ABS }, /* 2c */
    { "AND", ABS }, /* 2d */
    { "ROL", ABS }, /* 2e */
    { NULL, NONE }, /* 2f */
    { "BMI", REL }, /* 30 */
    { "AND", INDIR_INDEX }, /* 31 */
    { NULL, NONE }, /* 32 */
    { NULL, NONE }, /* 33 */
    { NULL, NONE }, /* 34 */
    { "AND", ZP_X }, /* 35 */
    { "ROL", ZP_X }, /* 36 */
    { NULL, NONE }, /* 37 */
    { "SEC", IMP }, /* 38 */
    { "AND", ABS_Y }, /* 39 */
    { NULL, NONE }, /* 3a */
    { NULL, NONE }, /* 3b */
    { NULL, NONE }, /* 3c */
    { "AND", ABS_X }, /* 3d */
    { "ROL", ABS_X }, /* 3e */
    { NULL, NONE }, /* 3f */
    { "RTI", IMP }, /* 40 */
    { "EOR", INDEX_INDIR }, /* 41 */
    { NULL, NONE }, /* 42 */
    { NULL, NONE }, /* 43 */
    { NULL, NONE }, /* 44 */
    { "EOR", ZP }, /* 45 */
    { "LSR", ZP }, /* 46 */
    { NULL, NONE }, /* 47 */
    { "PHA", IMP }, /* 48 */
    { "EOR", IMM }, /* 49 */
    { "LSR", ACC }, /* 4a */
    { NULL, NONE }, /* 4b */
    { "JMP", ABS }, /* 4c */
    { "EOR", ABS }, /* 4d */
    { "LSR", ABS }, /* 4e */
    { NULL, NONE }, /* 4f */
    { "BVC", REL }, /* 50 */
    { "EOR", INDIR_INDEX }, /* 51 */
    { NULL, NONE }, /* 52 */
    { NULL, NONE }, /* 53 */
    { NULL, NONE }, /* 54 */
    { "EOR", ZP_X }, /* 55 */
    { "LSR", ZP_X }, /* 56 */
    { NULL, NONE }, /* 57 */
    { "CLI", IMP }, /* 58 */
    { "EOR", ABS_Y }, /* 59 */
    { NULL, NONE }, /* 5a */
    { NULL, NONE }, /* 5b */
    { NULL, NONE }, /* 5c */
    { "EOR", ABS_X }, /* 5d */
    { "LSR", ABS_X }, /* 5e */
    { NULL, NONE }, /* 5f */
    { "RTS", IMP }, /* 60 */
    { "ADC", INDEX_INDIR }, /* 61 */
    { NULL, NONE }, /* 62 */
    { NULL, NONE }, /* 63 */
    { NULL, NONE }, /* 64 */
    { "ADC", ZP }, /* 65 */
    { "ROR", ZP }, /* 66 */
    { NULL, NONE }, /* 67 */
    { "PLA", IMP }, /* 68 */
    { "ADC", IMM }, /* 69 */
    { "ROR", ACC }, /* 6a */
    { NULL, NONE }, /* 6b */
    { "JMP", INDIR }, /* 6c */
    { "ADC", ABS }, /* 6d */
    { "ROR", ABS }, /* 6e */
    { NULL, NONE }, /* 6f */
    { "BVS", REL }, /* 70 */
    { "ADC", INDIR_INDEX }, /* 71 */
    { NULL, NONE }, /* 72 */
    { NULL, NONE }, /* 73 */
    { NULL, NONE }, /* 74 */
    { "ADC", ZP_X }, /* 75 */
    { "ROR", ZP_X }, /* 76 */
    { NULL, NONE }, /* 77 */
    { "SEI", IMP }, /* 78 */
    { "ADC", ABS_Y }, /* 79 */
    { NULL, NONE }, /* 7a */
    { NULL, NONE }, /* 7b */
    { NULL, NONE }, /* 7c */
    { "ADC", ABS_X }, /* 7d */
    { "ROR", ABS_X }, /* 7e */
    { NULL, NONE }, /* 7f */
    { NULL, NONE }, /* 80 */
    { "STA", INDEX_INDIR }, /* 81 */
    { NULL, NONE }, /* 82 */
    { NULL, NONE }, /* 83 */
    { "STY", ZP }, /* 84 */
    { "STA", ZP }, /* 85 */
    { "STX", ZP }, /* 86 */
    { NULL, NONE }, /* 87 */
    { "DEY", IMP }, /* 88 */
    { NULL, NONE }, /* 89 */
    { "TXA", IMP }, /* 8a */
    { NULL, NONE }, /* 8b */
    { "STY", ABS }, /* 8c */
    { "STA", ABS }, /* 8d */
    { "STX", ABS }, /* 8e */
    { NULL, NONE }, /* 8f */
    { "BCC", REL }, /* 90 */
    { "STA", INDIR_INDEX}, /* 91 */
    { NULL, NONE }, /* 92 */
    { NULL, NONE }, /* 93 */
    { "STY", ZP_X }, /* 94 */
    { "STA", ZP_X }, /* 95 */
    { "STX", ZP_Y }, /* 96 */
    { NULL, NONE }, /* 97 */
    { "TYA", IMP }, /* 98 */
    { "STA", ABS_Y }, /* 99 */
    { "TXS", IMP }, /* 9a */
    { NULL, NONE }, /* 9b */
    { NULL, NONE }, /* 9c */
    { "STA", ABS_X }, /* 9d */
    { NULL, NONE }, /* 9e */
    { NULL, NONE }, /* 9f */
    { "LDY", IMM }, /* a0 */
    { "LDA", INDEX_INDIR }, /* a1 */
    { "LDX", IMM }, /* a2 */
    { NULL, NONE }, /* a3 */
    { "LDY", ZP }, /* a4 */
    { "LDA", ZP }, /* a5 */
    { "LDX", ZP }, /* a6 */
    { NULL, NONE }, /* a7 */
    { "TAY", IMP }, /* a8 */
    { "LDA", IMM }, /* a9 */
    { "TAX", IMP }, /* aa */
    { NULL, NONE }, /* ab */
    { "LDY", ABS }, /* ac */
    { "LDA", ABS }, /* ad */
    { "LDX", ABS }, /* ae */
    { NULL, NONE }, /* af */
    { "BCS", REL }, /* b0 */
    { "LDA", INDIR_INDEX }, /* b1 */
    { NULL, NONE }, /* b2 */
    { NULL, NONE }, /* b3 */
    { "LDY", ZP_X }, /* b4 */
    { "LDA", ZP_X }, /* b5 */
    { "LDX", ZP_Y }, /* b6 */
    { NULL, NONE }, /* b7 */
    { "CLV", IMP }, /* b8 */
    { "LDA", ABS_Y }, /* b9 */
    { "TSX", IMP }, /* ba */
    { NULL, NONE }, /* bb */
    { "LDY", ABS_X }, /* bc */
    { "LDA", ABS_X }, /* bd */
    { "LDX", ABS_Y }, /* be */
    { NULL, NONE }, /* bf */
    { "CPY", IMM }, /* c0 */
    { "CMP", INDEX_INDIR }, /* c1 */
    { NULL, NONE }, /* c2 */
    { NULL, NONE }, /* c3 */
    { "CPY", ZP }, /* c4 */
    { "CMP", ZP }, /* c5 */
    { "DEC", ZP }, /* c6 */
    { NULL, NONE }, /* c7 */
    { "INY", IMP }, /* c8 */
    { "CMP", IMM }, /* c9 */
    { "DEX", IMP }, /* ca */
    { NULL, NONE }, /* cb */
    { "CPY", ABS }, /* cc */
    { "CMP", ABS }, /* cd */
    { "DEC", ABS }, /* ce */
    { NULL, NONE }, /* cf */
    { "BNE", REL }, /* d0 */
    { "CMP", INDIR_INDEX }, /* d1 */
    { NULL, NONE }, /* d2 */
    { NULL, NONE }, /* d3 */
    { NULL, NONE }, /* d4 */
    { "CMP", ZP_X }, /* d5 */
    { "DEC", ZP_X }, /* d6 */
    { NULL, NONE }, /* d7 */
    { "CLD", IMP }, /* d8 */
    { "CMP", ABS_Y }, /* d9 */
    { NULL, NONE }, /* da */
    { NULL, NONE }, /* db */
    { NULL, NONE }, /* dc */
    { "CMP", ABS_X }, /* dd */
    { "DEC", ABS_X }, /* de */
    { NULL, NONE }, /* df */
    { "CPX", IMM }, /* e0 */
    { "SBC", INDEX_INDIR }, /* e1 */
    { NULL, NONE }, /* e2 */
    { NULL, NONE }, /* e3 */
    { "CPX", ZP }, /* e4 */
    { "SBC", ZP }, /* e5 */
    { "INC", ZP }, /* e6 */
    { NULL, NONE }, /* e7 */
    { "INX", IMP }, /* e8 */
    { "SBC", IMM }, /* e9 */
    { "NOP", IMP }, /* ea */
    { NULL, NONE }, /* eb */
    { "CPX", ABS }, /* ec */
    { "SBC", ABS }, /* ed */
    { "INC", ABS }, /* ee */
    { NULL, NONE }, /* ef */
    { "BEQ", REL }, /* f0 */
    { "SBC", INDIR_INDEX }, /* f1 */
    { NULL, NONE }, /* f2 */
    { NULL, NONE }, /* f3 */
    { NULL, NONE }, /* f4 */
    { "SBC", ZP_X }, /* f5 */
    { "INC", ZP_X }, /* f6 */
    { NULL, NONE }, /* f7 */
    { "SED", IMP }, /* f8 */
    { "SBC", ABS_Y }, /* f9 */
    { NULL, NONE }, /* fa */
    { NULL, NONE }, /* fb */
    { NULL, NONE }, /* fc */
    { "SBC", ABS_X }, /* fd */
    { "INC", ABS_X }, /* fe */
    { NULL, NONE }, /* ff */
} ;

