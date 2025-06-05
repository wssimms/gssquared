#pragma once 

#include "debugger/monitor.hpp"
#include "mmus/mmu.hpp"
#include <sstream>
#include <vector>


class ExecuteCommand {
  private:
    MonitorCommand *cmd;
    MMU *mmu;
    std::vector<std::string> output_buffer;
    
    void addOutput(const std::string& line) {
        output_buffer.push_back(line);
    }
    
    template<typename... Args>
    void addFormattedOutput(const char* format, Args... args) {
        std::ostringstream oss;
        // For printf-style formatting, we'll use a temporary buffer
        constexpr size_t buffer_size = 1024;
        char buffer[buffer_size];
        snprintf(buffer, buffer_size, format, args...);
        output_buffer.push_back(std::string(buffer));
    }

    public:
        ExecuteCommand(MMU *mmu, MonitorCommand *cmd);
        const std::vector<std::string>& getOutput() const;
        void clearOutput();
        void execute();
};