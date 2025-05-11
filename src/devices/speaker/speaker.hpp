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

#include "cpu.hpp"
#include "slots.hpp"

#define AMPLITUDE_DECAY_RATE (0x20)
#define AMPLITUDE_PEAK (0x2000)

#define SAMPLE_BUFFER_SIZE (4096)

#define SAMPLE_RATE (51000)
#define SAMPLES_PER_FRAME (850)

#define EVENT_BUFFER_SIZE 128000

struct EventBuffer {
    uint64_t events[EVENT_BUFFER_SIZE];
    int write_pos;
    int read_pos;
    int count;

    EventBuffer() : write_pos(0), read_pos(0), count(0) {}

    bool add_event(uint64_t cycle) {
        if (count >= EVENT_BUFFER_SIZE) {
            return false; // Buffer full
        }
        
        events[write_pos] = cycle;
        write_pos = (write_pos + 1) % EVENT_BUFFER_SIZE;
        count++;
        return true;
    }

    bool peek_oldest(uint64_t& cycle) {
        if (count == 0) {
            return false; // Buffer empty
        }
        cycle = events[read_pos];
        return true;
    }

    bool pop_oldest(uint64_t& cycle) {
        if (count == 0) {
            return false; // Buffer empty
        }
        
        cycle = events[read_pos];
        read_pos = (read_pos + 1) % EVENT_BUFFER_SIZE;
        count--;
        return true;
    }
};


class LowPassFilter {
private:
    double a0, a1, a2, b0, b1, b2;
    double x1, x2, y1, y2;

public:
    // Constructor - initialize the filter with default values
    LowPassFilter() : x1(0), x2(0), y1(0), y2(0) {
        setCoefficients(1000, 44100); // Default: 1kHz cutoff at 44.1kHz sample rate
    }

    // Calculate filter coefficients based on cutoff frequency and sample rate
    void setCoefficients(double cutoffFreq, double sampleRate) {
        // Prevent issues with extreme values
        cutoffFreq = std::fmax(10.0, std::fmin(cutoffFreq, sampleRate * 0.49));
        
        // Precompute values for efficiency
        double omega = 2.0 * M_PI * cutoffFreq / sampleRate;
        double cosOmega = std::cos(omega);
        double alpha = std::sin(omega) / (2.0 * 0.7071); // Q factor = 0.7071 (Butterworth)
        
        // Calculate filter coefficients
        b0 = (1.0 - cosOmega) / 2.0;
        b1 = 1.0 - cosOmega;
        b2 = (1.0 - cosOmega) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cosOmega;
        a2 = 1.0 - alpha;
        
        // Normalize coefficients
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
        b0 /= a0;
        a0 = 1.0;
    }

    // Process a single sample
    double process(double input) {
        // Calculate output
        double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        
        // Update state variables
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;
        
        return output;
    }
    
    // Process a block of samples
    std::vector<double> processBlock(const std::vector<double>& inputBlock) {
        std::vector<double> outputBlock(inputBlock.size());
        for (size_t i = 0; i < inputBlock.size(); i++) {
            outputBlock[i] = process(inputBlock[i]);
        }
        return outputBlock;
    }
    
    // Reset filter state
    void reset() {
        x1 = x2 = y1 = y2 = 0.0;
    }
};

typedef struct speaker_state_t {
    FILE *speaker_recording = NULL;
    SDL_AudioDeviceID device_id = 0;
    SDL_AudioStream *stream = NULL;
    int device_started = 0;
    float polarity = 1.0f;
    float target_polarity = 1.0f;
    float amplitude = AMPLITUDE_PEAK; // suggested 50%

    LowPassFilter *preFilter;
    LowPassFilter *postFilter;

    int16_t working_buffer[SAMPLE_BUFFER_SIZE];
    EventBuffer event_buffer;
} speaker_state_t;

void init_mb_speaker(cpu_state *cpu, SlotType_t slot);
void toggle_speaker_recording(cpu_state *cpu);
void dump_full_speaker_event_log();
void dump_partial_speaker_event_log(uint64_t cycles_now);
void speaker_start(cpu_state *cpu);
void speaker_stop();
//void audio_generate_frame(cpu_state *cpu);
uint64_t audio_generate_frame(cpu_state *cpu, uint64_t last_cycle_window_start, uint64_t cycle_window_start);