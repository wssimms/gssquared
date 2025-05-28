#include <cstring>
#include <fstream>

#include "gs2.hpp"
#include "debugger/trace.hpp"
#include "debugger/trace_opcodes.hpp"
#include "opcodes.hpp"

    system_trace_buffer::system_trace_buffer(size_t capacity) {
        entries = new system_trace_entry_t[capacity];
        size = capacity;
        head = 0;
        tail = 0;
        count = 0;
    }

    void system_trace_buffer::add_entry(const system_trace_entry_t &entry) {
        memcpy(&entries[head], &entry, sizeof(system_trace_entry_t));
        head++;
        if (head >= size) {
            head = 0;
        }
        if (head == tail) {
            tail++;
            count--;
            if (tail >= size) {
                tail = 0;
            }
        }
        count++;
    }   

    void system_trace_buffer::save_to_file(const std::string &filename) {
        printf("Saving trace to file: %s\n", filename.c_str());
        printf("Head: %zu, Tail: %zu, Size: %zu\n", head, tail, size);
        std::ofstream file(filename);
        size_t index = tail;
        while (index != head) {
            // Write the binary record to the file
            file.write(reinterpret_cast<const char*>(&entries[index]), sizeof(system_trace_entry_t));
            index++;
            if (index >= size) {
                index = 0;
            }
        }
        file.close();
    }

    void system_trace_buffer::read_from_file(const std::string &filename) {
        std::ifstream file(filename);
        file.read(reinterpret_cast<char*>(entries), sizeof(system_trace_entry_t) * size);
        file.close();
    }

    system_trace_entry_t *system_trace_buffer::get_entry(size_t index) {
        return &entries[index];
    }

    char hex_table[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    void decode_hex_byte(char *buffer, uint8_t byte) {
        buffer[0] = hex_table[(byte >> 4) & 0x0F];
        buffer[1] = hex_table[byte & 0x0F];
    }

    void decode_hex_word(char *buffer, uint16_t word) {
        buffer[0] = hex_table[(word >> 12) & 0x0F];
        buffer[1] = hex_table[(word >> 8) & 0x0F];
        buffer[2] = hex_table[(word >> 4) & 0x0F];
        buffer[3] = hex_table[word & 0x0F];
    }

#define TB_A 20
#define TB_X 23
#define TB_Y 26
#define TB_SP 29
#define TB_P 34
#define TB_BAR1 37
#define TB_PC 40
#define TB_OPBYTES 46
#define TB_OPCODE 58
#define TB_OPERAND 62
#define TB_OPERAND2 64
#define TB_OPERAND3 66
#define TB_EADDR 78
#define TB_DATA 83

    char * system_trace_buffer::decode_trace_entry(system_trace_entry_t *entry) {
        static char buffer[256];
        char snpbuf[256];

        size_t buffer_size = sizeof(buffer);
        size_t snpbuf_size = sizeof(snpbuf);
        // if in 8-bit mode
        memset(buffer, ' ', buffer_size);
        snprintf(buffer, buffer_size, "%-16llu ", entry->cycle);
        buffer[17] = ' ' ;
        decode_hex_byte(buffer + TB_A, entry->a);
        decode_hex_byte(buffer + TB_X, entry->x);
        decode_hex_byte(buffer + TB_Y, entry->y);
        memcpy(buffer + TB_SP, "01", 2);
        decode_hex_byte(buffer + TB_SP + 2, entry->sp);
        decode_hex_byte(buffer + TB_P, entry->p);
        buffer[TB_BAR1] = '|';

        const disasm_entry *da = &disasm_table[entry->opcode];
        const char *opcode_name = da->opcode;
        const address_mode_entry *am = &address_mode_formats[da->mode];

        decode_hex_word(buffer + TB_PC, entry->pc);
        buffer[TB_PC + 4] = ':';

        decode_hex_byte(buffer + TB_OPBYTES, entry->opcode);
        if (am->size > 1) {
            decode_hex_byte(buffer + TB_OPBYTES + 3, entry->operand & 0xFF);
        }
        if (am->size > 2) {
            decode_hex_byte(buffer + TB_OPBYTES + 6, (entry->operand >> 8) & 0xFF);
        }

        if (opcode_name) memcpy(buffer + TB_OPCODE, opcode_name, 3);
        else memcpy(buffer + TB_OPCODE, "???", 3);

        switch (da->mode) {
            case NONE:
                memcpy(buffer + TB_OPERAND, "???", 3);
                break;
            case ACC:
            case IMP:
                memcpy(buffer + TB_OPERAND, am->format, strlen(am->format));
                break;

            case ABS:
            case ABS_X:
            case ABS_Y:
            case IMM:
            case INDIR:
            case INDEX_INDIR:
            case INDIR_INDEX:
            case ZP:
            case ZP_X:
            case ZP_Y:
                snprintf(snpbuf, snpbuf_size, am->format, entry->operand);
                memcpy(buffer + TB_OPERAND, snpbuf, strlen(snpbuf));
                break;
            case REL:
                uint16_t btarget = (entry->pc+2) + (int8_t)entry->operand;
                snprintf(snpbuf, snpbuf_size, "$%04X", btarget);
                memcpy(buffer + TB_OPERAND, snpbuf, strlen(snpbuf));
                break;
        }

        // print effective memory address
        switch (da->mode) {
            case ACC:
            case IMP:
            case IMM:
            case REL:
                //printf("     ");
                break;
            default:
                decode_hex_word(buffer + TB_EADDR, entry->eaddr);
                break;
        }

        // print memory data
        switch (da->mode) {
            case REL:
            case IMP:
                //printf("   ");
                break;
            
            default:
                if (entry->opcode == OP_JSR_ABS) {
                    decode_hex_word(buffer + TB_DATA, entry->data);
                } else {
                    decode_hex_byte(buffer + TB_DATA, entry->data & 0xFF);
                }
                break;
        }
/*         printf("%02X ", entry->db);
        printf("%02X ", entry->pb);
        printf("%04X ", entry->d); */
        buffer[120] = '\0';
//        printf("\n");
        // else if in 16-bit mode
        return buffer;
//        printf("%s\n", buffer);
#if 0
        printf("%-20llu ", entry->cycle);
        printf("%06X ", entry->pc);
        printf("%04X ", entry->a);
        printf("%04X ", entry->x);
        printf("%04X ", entry->y);
        printf("%04X ", entry->sp);
        printf("%02X ", entry->p);
        printf("%02X ", entry->opcode);
        printf("%06X ", entry->operand);
        printf("%06X ", entry->eaddr);
        printf("%04X ", entry->data);
/*         printf("%02X ", entry->db);
        printf("%02X ", entry->pb);
        printf("%04X ", entry->d); */
        printf("\n");
        // else if in 16-bit mode
#endif
    }
