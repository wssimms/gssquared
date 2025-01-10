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

#pragma once

#define OP_BRK_IMP 0x00

#define OP_JMP 0x4C
#define OP_JMP_IND 0x6C
#define OP_JMP_IND_X 0x7C

#define OP_JSR 0x20
#define OP_JSR_ABS 0x20
#define OP_JSR_IND 0x6C

#define OP_ADC_IMM 0x69
#define OP_ADC_ZP 0x65
#define OP_ADC_ZP_X 0x75
#define OP_ADC_ABS 0x6D
#define OP_ADC_ABS_X 0x7D
#define OP_ADC_ABS_Y 0x79
#define OP_ADC_IND_X 0x61
#define OP_ADC_IND_Y 0x71
#define OP_ADC_IND 0x72

#define OP_AND_IMM 0x29
#define OP_AND_ZP 0x25
#define OP_AND_ZP_X 0x35
#define OP_AND_ABS 0x2D
#define OP_AND_ABS_X 0x3D
#define OP_AND_ABS_Y 0x39
#define OP_AND_IND_X 0x21
#define OP_AND_IND_Y 0x31
#define OP_AND_IND 0x32

#define OP_ASL_ACC 0x0A
#define OP_ASL_ZP 0x06
#define OP_ASL_ZP_X 0x16
#define OP_ASL_ABS 0x0E
#define OP_ASL_ABS_X 0x1E

#define OP_BBR0_REL 0x0F
#define OP_BBR1_REL 0x1F
#define OP_BBR2_REL 0x2F
#define OP_BBR3_REL 0x3F
#define OP_BBR4_REL 0x4F
#define OP_BBR5_REL 0x5F
#define OP_BBR6_REL 0x6F
#define OP_BBR7_REL 0x7F

#define OP_BBS0_REL 0x8F
#define OP_BBS1_REL 0x9F
#define OP_BBS2_REL 0xAF
#define OP_BBS3_REL 0xBF
#define OP_BBS4_REL 0xCF
#define OP_BBS5_REL 0xDF
#define OP_BBS6_REL 0xEF
#define OP_BBS7_REL 0xFF

#define OP_BCC_REL 0x90
#define OP_BCS_REL 0xB0
#define OP_BEQ_REL 0xF0

#define OP_BIT_IMM 0x89
#define OP_BIT_ZP 0x24
#define OP_BIT_ZP_X 0x34
#define OP_BIT_ABS 0x2C
#define OP_BIT_ABS_X 0x3C

#define OP_BMI_REL 0x30
#define OP_BNE_REL 0xD0
#define OP_BPL_REL 0x10
#define OP_BRA_REL 0x80

#define OP_BVC_REL 0x50
#define OP_BVS_REL 0x70

#define OP_CLC_IMP 0x18
#define OP_CLD_IMP 0xD8
#define OP_CLI_IMP 0x58
#define OP_CLV_IMP 0xB8

#define OP_CMP_IMM 0xC9
#define OP_CMP_ZP 0xC5
#define OP_CMP_ZP_X 0xD5
#define OP_CMP_ABS 0xCD
#define OP_CMP_ABS_X 0xDD
#define OP_CMP_ABS_Y 0xD9
#define OP_CMP_IND_X 0xC1
#define OP_CMP_IND_Y 0xD1
#define OP_CMP_IND 0xD2

#define OP_CPX_IMM 0xE0
#define OP_CPX_ZP 0xE4
#define OP_CPX_ABS 0xEC

#define OP_CPY_IMM 0xC0
#define OP_CPY_ZP 0xC4
#define OP_CPY_ABS 0xCC

#define OP_DEA_ACC 0x3A
#define OP_DEC_ZP 0xC6
#define OP_DEC_ZP_X 0xD6
#define OP_DEC_ABS 0xCE
#define OP_DEC_ABS_X 0xDE

#define OP_DEX_IMP 0xCA
#define OP_DEY_IMP 0x88

#define OP_EOR_IMM 0x49
#define OP_EOR_ZP 0x45
#define OP_EOR_ZP_X 0x55
#define OP_EOR_ABS 0x4D
#define OP_EOR_ABS_X 0x5D
#define OP_EOR_ABS_Y 0x59
#define OP_EOR_IND_X 0x41
#define OP_EOR_IND_Y 0x51
#define OP_EOR_IND 0x52

#define OP_INA_ACC 0x1A
#define OP_INC_ZP 0xE6
#define OP_INC_ZP_X 0xF6
#define OP_INC_ABS 0xEE
#define OP_INC_ABS_X 0xFE

#define OP_INX_IMP 0xE8
#define OP_INY_IMP 0xC8

#define OP_JMP_ABS 0x4C
#define OP_JMP_IND 0x6C
#define OP_JMP_IND_X 0x7C

#define OP_JSR_ABS 0x20

#define OP_LDA_IMM 0xA9
#define OP_LDA_ZP 0xA5
#define OP_LDA_ZP_X 0xB5
#define OP_LDA_ABS 0xAD
#define OP_LDA_ABS_X 0xBD
#define OP_LDA_ABS_Y 0xB9
#define OP_LDA_IND_X 0xA1
#define OP_LDA_IND_Y 0xB1
#define OP_LDA_IND 0xB2

#define OP_LDX_IMM 0xA2
#define OP_LDX_ZP 0xA6
#define OP_LDX_ZP_Y 0xB6
#define OP_LDX_ABS 0xAE
#define OP_LDX_ABS_Y 0xBE

#define OP_LDY_IMM 0xA0
#define OP_LDY_ZP 0xA4
#define OP_LDY_ZP_X 0xB4
#define OP_LDY_ABS 0xAC
#define OP_LDY_ABS_X 0xBC

#define OP_LSR_ACC 0x4A
#define OP_LSR_ZP 0x46
#define OP_LSR_ZP_X 0x56
#define OP_LSR_ABS 0x4E
#define OP_LSR_ABS_X 0x5E

#define OP_NOP_IMP 0xEA

#define OP_ORA_IMM 0x09
#define OP_ORA_ZP 0x05
#define OP_ORA_ZP_X 0x15
#define OP_ORA_ABS 0x0D
#define OP_ORA_ABS_X 0x1D
#define OP_ORA_ABS_Y 0x19
#define OP_ORA_IND_X 0x01
#define OP_ORA_IND_Y 0x11
#define OP_ORA_IND 0x12

#define OP_PHA_IMP 0x48
#define OP_PHX_IMP 0xDA
#define OP_PHY_IMP 0x5A
#define OP_PHP_IMP 0x08

#define OP_PLA_IMP 0x68
#define OP_PLX_IMP 0xFA
#define OP_PLY_IMP 0x7A
#define OP_PLP_IMP 0x28

#define OP_ROL_ACC 0x2A
#define OP_ROL_ZP 0x26
#define OP_ROL_ZP_X 0x36
#define OP_ROL_ABS 0x2E
#define OP_ROL_ABS_X 0x3E

#define OP_ROR_ACC 0x6A
#define OP_ROR_ZP 0x66
#define OP_ROR_ZP_X 0x76
#define OP_ROR_ABS 0x6E
#define OP_ROR_ABS_X 0x7E

#define OP_RTI_IMP 0x40
#define OP_RTS_IMP 0x60

#define OP_SBC_IMM 0xE9
#define OP_SBC_ZP 0xE5
#define OP_SBC_ZP_X 0xF5
#define OP_SBC_ABS 0xED
#define OP_SBC_ABS_X 0xFD
#define OP_SBC_ABS_Y 0xF9
#define OP_SBC_IND_X 0xE1
#define OP_SBC_IND_Y 0xF1
#define OP_SBC_IND 0xF2

#define OP_SEC_IMP 0x38
#define OP_SED_IMP 0xF8
#define OP_SEI_IMP 0x78

#define OP_STA_ZP 0x85
#define OP_STA_ZP_X 0x95
#define OP_STA_ABS 0x8D
#define OP_STA_ABS_X 0x9D
#define OP_STA_ABS_Y 0x99
#define OP_STA_IND_X 0x81
#define OP_STA_IND_Y 0x91
#define OP_STA_IND 0x92

#define OP_STX_ZP 0x86
#define OP_STX_ZP_Y 0x96
#define OP_STX_ABS 0x8E

#define OP_STY_ZP 0x84
#define OP_STY_ZP_X 0x94
#define OP_STY_ABS 0x8C

#define OP_STZ_ZP 0x64
#define OP_STZ_ZP_X 0x74
#define OP_STZ_ABS 0x9C
#define OP_STZ_ABS_X 0x9E

#define OP_TAX_IMP 0xAA
#define OP_TAY_IMP 0xA8

#define OP_TRB_ZP 0x14
#define OP_TRB_ABS 0x1C

#define OP_TSB_ZP 0x04
#define OP_TSB_ABS 0x0C

#define OP_TSX_IMP 0xBA
#define OP_TXA_IMP 0x8A
#define OP_TXS_IMP 0x9A
#define OP_TYA_IMP 0x98

#define OP_HLT_IMP 0xFF

const char *get_opcode_name(uint8_t);
