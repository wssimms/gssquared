#include "debugger/ExecuteCommand.hpp"
#include "debugger/MonitorCommand.hpp"
#include "mmus/mmu.hpp"
#include <sstream>
#include <vector>
#include <iomanip>
#include "debugger/MemoryWatch.hpp"

ExecuteCommand::ExecuteCommand(MMU *mmu, MonitorCommand *cmd, MemoryWatch *watches, MemoryWatch *breaks) {
    this->mmu = mmu;
    this->cmd = cmd;
    this->memory_watches = watches;
    this->breaks = breaks;
}

const std::vector<std::string>& ExecuteCommand::getOutput() const {
    return output_buffer;
}

void ExecuteCommand::clearOutput() {
    output_buffer.clear();
}

void ExecuteCommand::execute() {
    output_buffer.clear(); // Clear previous output
    
    if (cmd->nodes.size() == 0) {
        addOutput("Error: no command provided"); // TODO: maybe continue memory dump another 16 bytes like wozmon
        return;
    }
    auto &node0 = cmd->nodes[0];
    if (node0.type == MON_NODE_TYPE_NUMBER) {
        // start with number, it's an address to show a single value.
        uint16_t address = node0.val_number;
        addFormattedOutput("%04X: %02x", address, mmu->read(address));
    }
    if (node0.type == MON_NODE_TYPE_ADDRESS_SET) {
        // start with address, it's an address to write a value starting at address - short hand for "set address val val val"
        uint16_t address = node0.val_address;
        for (int i = 1; i < cmd->nodes.size(); i++) {
            auto &node = cmd->nodes[i];
            if (node.type == MON_NODE_TYPE_NUMBER) {
                mmu->write(address, node.val_number);
                address++;
            } else {
                addFormattedOutput("Error: unexpected value type: %d", node.type);
                return;
            }
        }
    }
    if (node0.type == MON_NODE_TYPE_RANGE) {
        // dump memory from range
        int bytecounter = 0;

        // print address 
        // then print hex bytes for this line, up to 16 bytes
        // then print ASCII representation of the bytes up to 16 bytes.
        uint32_t address = node0.val_range.lo;
        while (address <= node0.val_range.hi) {
            std::ostringstream line;
            line << std::hex << std::uppercase;
            line << std::setfill('0') << std::setw(4) << address << ": ";
            
            // Hex bytes
            for (int i = 0 ; i < 16; i++) {
                if (address + i <= node0.val_range.hi) {
                    line << std::setfill('0') << std::setw(2) << (int)mmu->read(address + i) << " ";
                } else {
                    line << "   ";
                }
            }
            line << "  ";
            
            // ASCII representation
            for (int i = 0 ; i < 16; i++) {
                if (address + i <= node0.val_range.hi) {
                    uint8_t val = mmu->read(address + i);
                    val &= 0x7F;
                    if (val < 32 || val > 126) {
                        line << ".";
                    } else {
                        line << (char)val;
                    }
                } else {
                    line << " ";
                }
            }
            
            addOutput(line.str());
            address += 16;
        }
        addOutput(""); // Empty line at the end
    }
    if (memory_watches && (node0.type == MON_NODE_TYPE_COMMAND) && (node0.val_cmd == MON_CMD_WATCH)) {
        // set memory monitor
        if (cmd->nodes.size() < 2) {
            addOutput("Current memory watches:");
            for (MemoryWatch::iterator watch = memory_watches->begin(); watch != memory_watches->end(); ++watch) {
                std::ostringstream line;
                line << " >> " << std::hex << std::uppercase << std::setfill('0') 
                     << std::setw(4) << watch->start << " - " 
                     << std::setw(4) << watch->end;
                addOutput(line.str());
            }
            return;
        }
        auto &node1 = cmd->nodes[1];
        if (node1.type == MON_NODE_TYPE_RANGE) {
            memory_watches->add(node1.val_range.lo, node1.val_range.hi);
        } else {
            addOutput("Error: expected range as first argument");
        }
    }
    if (memory_watches && (node0.type == MON_NODE_TYPE_COMMAND) && (node0.val_cmd == MON_CMD_NOWATCH)) {
        // clear memory monitor
        auto &node1 = cmd->nodes[1];
        if (node1.type == MON_NODE_TYPE_NUMBER) {
            memory_watches->remove(node1.val_number);
        } else {
            addOutput("Error: expected address as first argument");
        }
    }
    if (breaks && (node0.type == MON_NODE_TYPE_COMMAND) && (node0.val_cmd == MON_CMD_BP)) {
        // set memory monitor
        if (cmd->nodes.size() < 2) {
            addOutput("Current breakpoints:");
            for (MemoryWatch::iterator watch = breaks->begin(); watch != breaks->end(); ++watch) {
                std::ostringstream line;
                line << " >> " << std::hex << std::uppercase << std::setfill('0') 
                     << std::setw(4) << watch->start << " - " 
                     << std::setw(4) << watch->end;
                addOutput(line.str());
            }
            return;
        }
        auto &node1 = cmd->nodes[1];
        if (node1.type == MON_NODE_TYPE_RANGE) {
            breaks->add(node1.val_range.lo, node1.val_range.hi);
        } else {
            addOutput("Error: expected range as first argument");
        }
    }
    if (breaks && (node0.type == MON_NODE_TYPE_COMMAND) && (node0.val_cmd == MON_CMD_NOBP)) {
        // clear memory monitor
        auto &node1 = cmd->nodes[1];
        if (node1.type == MON_NODE_TYPE_NUMBER) {
            breaks->remove(node1.val_number);
        } else {
            addOutput("Error: expected address as first argument");
        }
    }

    if ((node0.type == MON_NODE_TYPE_COMMAND) && (node0.val_cmd == MON_CMD_HELP)) {
        addOutput("watch range_lo:range_hi      - watch memory range");
        addOutput("watch                        - list watches");
        addOutput("nowatch address              - remove watch");
        addOutput("bp range_lo:range_hi         - set breakpoint");
        addOutput("bp                           - list breakpoints");
        addOutput("nobp address                 - remove breakpoint");
        addOutput("set address value [value...] - set memory values");
        addOutput("address:value [value...]     - set memory values");
        addOutput("load \"filename\" address      - load memory from file");
        addOutput("save \"filename\" lo.hi        - save memory range to file");
        addOutput("move lo.hi address           - move memory from lo to hi to address");
        addOutput("help                         - this help");
        return;
    }
    if ((node0.type == MON_NODE_TYPE_COMMAND) && (node0.val_cmd == MON_CMD_SET)) {
        // set memory from range
        auto &node1 = cmd->nodes[1];
        uint16_t address = node1.val_number;
        for (int i = 2; i < cmd->nodes.size(); i++) {
            auto &node = cmd->nodes[i];
            if (node.type == MON_NODE_TYPE_NUMBER) {
                mmu->write(address, node.val_number);
                address++;
            } else {
                addFormattedOutput("Error: unexpected value type: %d", node.type);
                return;
            }
        }
    }
    if ((node0.type == MON_NODE_TYPE_COMMAND) && (node0.val_cmd == MON_CMD_SAVE)) {
        // save memory to file
        auto &node1 = cmd->nodes[1];
        std::string filename;
        if (node1.type == MON_NODE_TYPE_STRING) {
            // save memory from range to file
            filename = node1.val_string;
        } else {
            addOutput("Error: expected string 'filename' as first argument");
            return;
        }
        auto &node2 = cmd->nodes[2];
        if (node2.type == MON_NODE_TYPE_RANGE) {
            FILE *file = fopen(filename.c_str(), "wb");
            if (file) {
                for (int i = node2.val_range.lo; i <= node2.val_range.hi; i++) {
                    uint8_t val = mmu->read(i);
                    fwrite(&val, 1, 1, file);
                }
                fclose(file);
                addFormattedOutput("Saved %d bytes to %s", node2.val_range.hi - node2.val_range.lo + 1, filename.c_str());
            } else {
                addFormattedOutput("Error: could not open file: %s", filename.c_str());
            }
        } else {
            addOutput("Error: expected range as second argument");
            return;
        }
    }
    if ((node0.type == MON_NODE_TYPE_COMMAND) && (node0.val_cmd == MON_CMD_LOAD)) {
        // load memory from file
        auto &node1 = cmd->nodes[1];

        std::string filename;
        if (node1.type == MON_NODE_TYPE_STRING) {
            // load memory from file to address
            filename = node1.val_string;
        } else {
            addOutput("Error: expected string 'filename' as first argument");
            return;
        }
        auto &node2 = cmd->nodes[2];
        if (node2.type == MON_NODE_TYPE_NUMBER) {
            uint16_t address = node2.val_number;
            FILE *file = fopen(filename.c_str(), "rb");
            int i = 0;
            if (file) {
                while (!feof(file)) {
                    uint8_t val = 0;
                    fread(&val, 1, 1, file);
                    mmu->write(address + i, val);
                    i++;
                }
                fclose(file);
                addFormattedOutput("Loaded %d bytes from %s to %04X - %04X", i, filename.c_str(), address, address + i - 1);
            } else {
                addFormattedOutput("Error: could not open file: %s", filename.c_str());
            }
        }
    }
}
