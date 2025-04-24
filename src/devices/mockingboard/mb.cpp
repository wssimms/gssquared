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

#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <fstream>
#include <cstdint>
#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_audio.h>

#include "gs2.hpp"
#include "cpu.hpp"
#include "mb.hpp"
#include "bus.hpp"
#include "devices/speaker/speaker.hpp"

#define DEBUG 0


enum AY_Registers {
    A_Tone_Low = 0,
    A_Tone_High = 1,
    B_Tone_Low = 2,
    B_Tone_High = 3,
    C_Tone_Low = 4,
    C_Tone_High = 5,
    Noise_Period = 6,
    Mixer_Control = 7,
    Ampl_A = 8, // 10,
    Ampl_B = 9, // 11,
    Ampl_C = 10, // 12,
    Envelope_Period_Low = 11, // 13,
    Envelope_Period_High = 12, // 14,
    Envelope_Shape = 13, // 15,
};


// Global constants
constexpr double OUTPUT_SAMPLE_RATE = 44100.0;

// Event to represent register changes with timestamps
struct RegisterEvent {
    double timestamp;      // Time in seconds when this event occurs
    uint8_t chip_index;    // Which AY chip (0 or 1)
    uint8_t register_num;  // Which register (0-13)
    uint8_t value;         // New value for the register
    
    // For sorting events by timestamp
    bool operator<(const RegisterEvent& other) const {
        return timestamp < other.timestamp;
    }
};

// Add a static array for the normalized volume levels
static const float normalized_levels[16] = {
    0x0000 / 65535.0f, 0x0385 / 65535.0f, 0x053D / 65535.0f, 0x0770 / 65535.0f,
    0x0AD7 / 65535.0f, 0x0FD5 / 65535.0f, 0x15B0 / 65535.0f, 0x230C / 65535.0f,
    0x2B4C / 65535.0f, 0x43C1 / 65535.0f, 0x5A4B / 65535.0f, 0x732F / 65535.0f,
    0x9204 / 65535.0f, 0xAFF1 / 65535.0f, 0xD921 / 65535.0f, 0xFFFF / 65535.0f
};

class MockingboardEmulator {
private:
    // Constants
    static constexpr double MASTER_CLOCK = 1020500.0; // 1MHz
    static constexpr int CLOCK_DIVIDER = 16;
    static constexpr double CHIP_FREQUENCY = MASTER_CLOCK / CLOCK_DIVIDER; // 62.5kHz
    static constexpr int ENVELOPE_CLOCK_DIVIDER = 256;  // First stage divider for envelope
    static constexpr float FILTER_CUTOFF = 0.26f; // Filter coefficient (0-1)
    
public:
    MockingboardEmulator(std::vector<float>* buffer = nullptr) 
        : current_time(0.0), time_accumulator(0.0), envelope_time_accumulator(0.0), audio_buffer(buffer) {
        // Initialize chips
        for (int c = 0; c < 2; c++) {
            // Initialize registers to 0
            for (int r = 0; r < 16; r++) {
                chips[c].registers[r] = 0;
            }
            
            // Initialize tone channels
            for (int i = 0; i < 3; i++) {
                chips[c].tone_channels[i].period = 1;
                chips[c].tone_channels[i].counter = 1;
                chips[c].tone_channels[i].output = false;
                chips[c].tone_channels[i].volume = 0;
                chips[c].tone_channels[i].use_envelope = false;
            }
            
            // Initialize noise generator
            chips[c].noise_period = 1;
            chips[c].noise_counter = 1;
            chips[c].noise_rng = 1;
            chips[c].noise_output = false;
            chips[c].mixer_control = 0;
            chips[c].envelope_period = 0;
            chips[c].envelope_shape = 0;
            chips[c].envelope_counter = 0;
            chips[c].envelope_output = 0;
            chips[c].envelope_hold = false;
            chips[c].envelope_attack = false;
            chips[c].current_envelope_level = 0.0f;
            chips[c].target_envelope_level = 0.0f;
        }
    }
    
    // Set the audio buffer
    void setAudioBuffer(std::vector<float>* buffer) {
        audio_buffer = buffer;
    }
    
    // Add a register change event
    void queueRegisterChange(double timestamp, uint8_t chip_index, uint8_t reg, uint8_t value) {
        RegisterEvent event;
        event.timestamp = timestamp;
        event.chip_index = chip_index;
        event.register_num = reg;
        event.value = value;
        
        pending_events.push_back(event);
        // Keep events sorted by timestamp
        std::sort(pending_events.begin(), pending_events.end());
    }
    
    // Process a register change
    void processRegisterChange(const RegisterEvent& event) {
        if (event.chip_index > 1 || event.register_num > 15) {
            return; // Invalid event
        }
        
        AY3_8910& chip = chips[event.chip_index];
        chip.registers[event.register_num] = event.value;
        
        // Debug output for important register changes
        if (event.chip_index == 0) {
            switch (event.register_num) {
                case Envelope_Period_Low: case Envelope_Period_High:
                    if (DEBUG) std::cout << "[" << current_time << "] Envelope period set to: " 
                              << ((chip.registers[Envelope_Period_High] << 8) | chip.registers[Envelope_Period_Low]) 
                              << std::endl;
                    break;
                case Envelope_Shape:
                    if (DEBUG) std::cout << "[" << current_time << "] Envelope shape set to: " << static_cast<int>(event.value & 0x0F) 
                              << " (attack=" << ((event.value & 0x04) != 0)
                              << ", hold=" << ((event.value & 0x01) == 0)
                              << ")" << std::endl;
                    break;
                case Ampl_A: case Ampl_B: case Ampl_C:
                    if (DEBUG) std::cout << "[" << current_time << "] Channel " << (event.register_num - Ampl_A) 
                              << " volume set to: " << static_cast<int>(event.value & 0x0F)
                              << " (envelope=" << ((event.value & 0x10) != 0) << ")" 
                              << std::endl;
                    break;
            }
        }
        
        // Update internal state based on register change
        switch (event.register_num) {
            case A_Tone_Low: // Tone period low bits for channel A
            case B_Tone_Low: // Tone period low bits for channel B
            case C_Tone_Low: // Tone period low bits for channel C
                {
                    int channel = event.register_num / 2;
                    int high_bits = chip.registers[event.register_num + 1] & 0x0F;
                    // The AY-3-8910 frequency is calculated as: f = chip_frequency / (2 * period)
                    // So for a given frequency f, period = chip_frequency / (2 * f)
                    // For example, for 250Hz: period = 62500 / (2 * 250) = 125
                    chip.tone_channels[channel].period = 
                        (high_bits << 8) | event.value;
                    if (chip.tone_channels[channel].period == 0) {
                        chip.tone_channels[channel].period = 1; // Avoid division by zero
                    }
                }
                break;
                
            case A_Tone_High: // Tone period high bits for channel A
            case B_Tone_High: // Tone period high bits for channel B
            case C_Tone_High: // Tone period high bits for channel C
                {
                    int channel = (event.register_num - 1) / 2;
                    int high_bits = event.value & 0x0F;
                    chip.tone_channels[channel].period = 
                        (high_bits << 8) | chip.registers[event.register_num - 1];
                    if (chip.tone_channels[channel].period == 0) {
                        chip.tone_channels[channel].period = 1; // Avoid division by zero
                    }
                }
                break;
                
            case Noise_Period: // Noise period
                chip.noise_period = event.value & 0x1F;
                if (chip.noise_period == 0) {
                    chip.noise_period = 1; // Avoid division by zero
                }
                break;
                
            case Mixer_Control: // Mixer control
                chip.mixer_control = event.value;
                break;
                
            case Envelope_Period_Low: // Envelope period low bits
                chip.envelope_period = (chip.envelope_period & 0xFF00) | event.value;
                break;
                
            case Envelope_Period_High: // Envelope period high bits
                chip.envelope_period = (chip.envelope_period & 0x00FF) | (event.value << 8);
                break;
                
            case Envelope_Shape: // Envelope shape
                chip.envelope_shape = event.value & 0x0F;
                // Reset envelope state
                chip.envelope_counter = 0;
                chip.envelope_hold = false;
                // Start in the correct phase based on Attack bit
                chip.envelope_attack = (chip.envelope_shape & 0x04) != 0;
                // Set initial output value based on attack/decay
                chip.envelope_output = chip.envelope_attack ? 0 : 15;
                chip.target_envelope_level = chip.envelope_output;
                break;
                
            case Ampl_A: // Channel A volume
            case Ampl_B: // Channel B volume
            case Ampl_C: // Channel C volume
                {
                    int channel = event.register_num - Ampl_A;
                    // Check if envelope is enabled (bit 4)
                    if (event.value & 0x10) {
                        chip.tone_channels[channel].use_envelope = true;
                        // Set initial volume from current envelope output, normalized to 0-1
                        chip.tone_channels[channel].volume = chip.envelope_output / 15.0f;
                    } else {
                        chip.tone_channels[channel].use_envelope = false;
                        // Convert direct volume to 0-1 range
                        chip.tone_channels[channel].volume = (event.value & 0x0F) / 15.0f;
                    }
                }
                break;
        }
    }
    
    // Process a single chip clock cycle
    void processChipCycle(double cycle_time) {
        // Process any pending register changes that should happen by this time
        while (!pending_events.empty() && pending_events.front().timestamp <= cycle_time) {
            processRegisterChange(pending_events.front());
            pending_events.pop_front();
        }
        
        // Process both chips
        for (int c = 0; c < 2; c++) {
            AY3_8910& chip = chips[c];
            
            // Process tone generators
            for (int i = 0; i < 3; i++) {
                ToneChannel& channel = chip.tone_channels[i];
                
                if (channel.counter > 0) {
                    channel.counter--;
                    // Check for half period
                    if (channel.counter == channel.period / 2) {
                        channel.output = !channel.output;
                    }
                } else {
                    // Reset counter and toggle output
                    channel.counter = channel.period;
                    channel.output = !channel.output;
                }
            }
            
            // Process noise generator (simplified)
            chip.noise_counter--;
            if (chip.noise_counter <= 0) {
                chip.noise_counter = chip.noise_period;
                
                // Update noise RNG (simplified LFSR)
                uint32_t bit0 = chip.noise_rng & 1;
                uint32_t bit3 = (chip.noise_rng >> 3) & 1;
                uint32_t new_bit = bit0 ^ bit3;
                chip.noise_rng = (chip.noise_rng >> 1) | (new_bit << 16);
                chip.noise_output = (chip.noise_rng & 1) != 0;  // Use LSB of RNG as noise output
            }
        }
    }
    
    // Process envelope generator at correct rate
    void processEnvelope(double time_step) {
        for (int c = 0; c < 2; c++) {
            AY3_8910& chip = chips[c];
            
            if (chip.envelope_period > 0) {
                // Update envelope counter at the correct rate
                chip.envelope_counter++;
                if (chip.envelope_counter >= (chip.envelope_period / 16)) {
                    chip.envelope_counter = 0;
                    
                    // Extract control bits
                    bool hold = (chip.envelope_shape & 0x01) != 0;      // Bit 0 (inverted in hardware)
                    bool alternate = (chip.envelope_shape & 0x02) != 0; // Bit 1
                    bool attack = (chip.envelope_shape & 0x04) != 0;    // Bit 2
                    bool cont = (chip.envelope_shape & 0x08) != 0;      // Bit 3
                    
                    // State machine logic
                    if (chip.envelope_hold) {
                        // Do nothing when in hold state
                        return;
                    }
                    
                    if (chip.envelope_attack) {
                        // In attack (rising) phase
                        if (chip.envelope_output < 15) {
                            // Still rising
                            chip.envelope_output++;
                            chip.target_envelope_level = chip.envelope_output;
                        } else {
                            // Reached peak, determine next state
                            // If continue and hold are both set, determine held value by (attack XOR alternate)
                            if (cont && hold) {
                                bool held_at_15 = attack != alternate; // XOR operation
                                chip.envelope_output = held_at_15 ? 15 : 0;
                                chip.target_envelope_level = chip.envelope_output;
                                chip.envelope_hold = true;
                            }
                            // Regular processing for other cases
                            else if (hold) {
                                chip.envelope_hold = true;
                            } else if (!cont) {
                                chip.envelope_output = 0;
                                chip.target_envelope_level = 0;
                                chip.envelope_hold = true;
                            } else if (alternate) {
                                chip.envelope_attack = false; // Switch to decay
                            } else {
                                // Reset to start of phase
                                chip.envelope_output = attack ? 0 : 15;
                                chip.target_envelope_level = chip.envelope_output;
                            }
                        }
                    } else {
                        // In decay (falling) phase
                        if (chip.envelope_output > 0) {
                            // Still falling
                            chip.envelope_output--;
                            chip.target_envelope_level = chip.envelope_output;
                        } else {
                            // Reached zero, determine next state
                            // If continue and hold are both set, determine held value by (attack XOR alternate)
                            if (cont && hold) {
                                bool held_at_15 = attack != alternate; // XOR operation
                                chip.envelope_output = held_at_15 ? 15 : 0;
                                chip.target_envelope_level = chip.envelope_output;
                                chip.envelope_hold = true;
                            }
                            // Regular processing for other cases
                            else if (hold) {
                                chip.envelope_hold = true;
                            } else if (!cont) {
                                chip.envelope_hold = true;
                            } else if (alternate) {
                                chip.envelope_attack = true; // Switch to attack
                            } else {
                                // Reset to start of phase
                                chip.envelope_output = attack ? 0 : 15;
                                chip.target_envelope_level = chip.envelope_output;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Generate audio samples at 44.1kHz
    void generateSamples(int num_samples) {
        if (!audio_buffer) {
            return;  // No buffer to write to
        }
        
        // Debug output for envelope level at start of each call
        if (DEBUG) std::cout << "[" << current_time << "] Envelope status - Level: " << static_cast<int>(chips[0].envelope_output) 
                  << " (counter: " << static_cast<int>(chips[0].envelope_counter) 
                  << "/" << static_cast<int>(chips[0].envelope_period) 
                  << ", shape: " << static_cast<int>(chips[0].envelope_shape)
                  << ", attack: " << (chips[0].envelope_attack ? "true" : "false")
                  << ", hold: " << (chips[0].envelope_hold ? "true" : "false")
                  << ")" << std::endl;
        
        const double output_time_step = 1.0 / OUTPUT_SAMPLE_RATE;
        const double chip_time_step = 1.0 / CHIP_FREQUENCY;
        const double envelope_base_frequency = MASTER_CLOCK / ENVELOPE_CLOCK_DIVIDER;
        const double envelope_base_time_step = 1.0 / envelope_base_frequency;
        
        for (int i = 0; i < num_samples; i++) {
            // Track current time based on the sample index and output time step
            double sample_time = current_time + (i * output_time_step);
            time_accumulator += output_time_step;
            envelope_time_accumulator += output_time_step;
            
            // Process all chip cycles that should occur during this output sample
            while (time_accumulator >= chip_time_step) {
                double cycle_time = sample_time - time_accumulator;
                processChipCycle(cycle_time);
                time_accumulator -= chip_time_step;
            }
            
            // Process envelope at the correct rate (master_clock / 256)
            while (envelope_time_accumulator >= envelope_base_time_step) {
                processEnvelope(envelope_base_time_step);
                envelope_time_accumulator -= envelope_base_time_step;
            }
            
            // Do envelope interpolation at audio rate. This makes changes to envelope levels transition smoothly on a per-sample basis.
            for (int c = 0; c < 2; c++) {
                AY3_8910& chip = chips[c];
                
                // Base the interpolation speed on the envelope period
                // Shorter periods need faster interpolation
                float base_speed = 0.0003f;  // Our known good value for the test case
                float period_factor = 1.0f;
                if (chip.envelope_period > 0) {
                    // Scale interpolation speed inversely with period
                    // The larger the period, the slower the interpolation should be
                    period_factor = static_cast<float>(0x3000) / chip.envelope_period;  // 0x3000 is a reference period
                }
                float interpolation_speed = base_speed * period_factor;
                
                chip.current_envelope_level += (chip.target_envelope_level - chip.current_envelope_level) * interpolation_speed;
                
                // Update channel volumes with interpolated value
                for (int i = 0; i < 3; i++) {
                    if (chip.tone_channels[i].use_envelope) {
                        // Keep as float, divide by 15 here to normalize to 0-1 range
                        chip.tone_channels[i].volume = chip.current_envelope_level / 15.0f;
                    }
                }
            }
            
            // Mix output from all channels of both chips
            float mixed_output = 0.0f;
            int active_channels = 0;
            
            for (int c = 0; c < 2; c++) {
                const AY3_8910& chip = chips[c];
                
                for (int channel = 0; channel < 3; channel++) {
                    const ToneChannel& tone = chip.tone_channels[channel];
                    
                    // Only process if the channel has volume
                    if (tone.volume > 0) {
                        bool tone_enabled = !(chip.mixer_control & (1 << channel));
                        bool noise_enabled = !(chip.mixer_control & (1 << (channel + 3)));
                        
                        // If either tone or noise is enabled for this channel
                        if (tone_enabled || noise_enabled) {
                            // For tone: true = +volume, false = -volume
                            float tone_contribution = tone.output ? tone.volume : -tone.volume;
                            
                            // For noise: true = +volume, false = -volume
                            float noise_contribution = chip.noise_output ? tone.volume : -tone.volume;
                            
                            // If both are enabled, average them
                            if (tone_enabled && noise_enabled) {
                                mixed_output += (tone_contribution + noise_contribution) * 0.5f;
                            } else if (tone_enabled) {
                                mixed_output += tone_contribution;
                            } else if (noise_enabled) {
                                mixed_output += noise_contribution;
                            }
                            
                            active_channels++;
                        }
                    }
                }
                
                // Apply filter per chip (this is still valid as it's about signal processing)
                mixed_output = applyLowPassFilter(mixed_output, c);
            }
            
            // Single scaling stage
            if (active_channels > 0) {
                //printf("active_channels: %d mixed_output: %f\n", active_channels, mixed_output);
                mixed_output /= active_channels;  // Now correctly scaling by number of active channels
            }
            
            // Append the mixed sample to the buffer
            audio_buffer->push_back(mixed_output);
        }
        
        // Update current_time for the next call
        current_time += num_samples * output_time_step;
    }
    
    // Write audio samples to a WAV file
    bool writeToWav(const std::string& filename) {
        if (!audio_buffer) {
            return false;
        }
        
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            return false;
        }
        
        // WAV header
        file.write("RIFF", 4);                    // Chunk ID
        write32(file, 36 + audio_buffer->size() * 2);   // Chunk size (header + data)
        file.write("WAVE", 4);                    // Format
        
        // Format subchunk
        file.write("fmt ", 4);                    // Subchunk ID
        write32(file, 16);                        // Subchunk size
        write16(file, 1);                         // Audio format (1 = PCM)
        write16(file, 1);                         // Number of channels
        write32(file, static_cast<uint32_t>(OUTPUT_SAMPLE_RATE));  // Sample rate
        write32(file, static_cast<uint32_t>(OUTPUT_SAMPLE_RATE * 2));  // Byte rate
        write16(file, 2);                         // Block align
        write16(file, 16);                        // Bits per sample
        
        // Data subchunk
        file.write("data", 4);                    // Subchunk ID
        write32(file, audio_buffer->size() * 2);        // Subchunk size
        
        // Write audio data (convert float to 16-bit PCM)
        for (float sample : *audio_buffer) {
            // Clamp sample to [-1.0, 1.0] and convert to 16-bit PCM
            int16_t pcm_sample = static_cast<int16_t>(
                std::max(-1.0f, std::min(1.0f, sample)) * 32767.0f
            );
            file.write(reinterpret_cast<const char*>(&pcm_sample), sizeof(pcm_sample));
        }
        
        return true;
    }
    
private:
    // Filter state
    float last_sample[2] = {0.0f, 0.0f}; // One state per chip
    
    // State for each tone channel (3 per chip, 2 chips)
    struct ToneChannel {
        uint16_t period;     // Tone period (12 bits, 0-4095)
        uint16_t counter;    // Current counter value
        bool output;         // Current output state
        float volume;        // Volume (0-1)
        bool use_envelope;   // Whether to use envelope generator
    };
    
    // State for each chip
    struct AY3_8910 {
        ToneChannel tone_channels[3];
        uint16_t noise_period;
        uint16_t noise_counter;
        uint32_t noise_rng;
        bool noise_output;    // Add noise output state
        uint8_t mixer_control;
        uint16_t envelope_period;  // Changed to 16-bit
        uint8_t envelope_shape;
        uint16_t envelope_counter;  // Changed to 16-bit to handle larger counts at master clock rate
        uint8_t envelope_output;   // Current envelope value (0-15)
        bool envelope_hold;        // Whether envelope is holding
        bool envelope_attack;      // Whether envelope is in attack phase
        float current_envelope_level;  // Interpolated envelope level
        float target_envelope_level;   // Target level we're interpolating towards
        uint8_t registers[16];  // Raw register values
    };
    
    // Emulator state
    AY3_8910 chips[2];
    double current_time;
    double time_accumulator;
    double envelope_time_accumulator;  // New accumulator for envelope timing
    std::deque<RegisterEvent> pending_events;
    std::vector<float>* audio_buffer;  // Pointer to external audio buffer
    
    // Helper function to write a 16-bit value to a file
    void write16(std::ofstream& file, uint16_t value) {
        file.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
    
    // Helper function to write a 32-bit value to a file
    void write32(std::ofstream& file, uint32_t value) {
        file.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
    
    // Apply simple one-pole low-pass filter
    float applyLowPassFilter(float input, int chip_index) {
        float filtered_sample = last_sample[chip_index] + FILTER_CUTOFF * (input - last_sample[chip_index]);
        last_sample[chip_index] = filtered_sample;
        return filtered_sample;
    }
};

#ifdef STANDALONE
int main() {
    // Create audio buffer
    std::vector<float> audio_buffer;
    SDL_Event event;

    SDL_Init(SDL_INIT_AUDIO);
    int device_id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (device_id == 0) {
        printf("Couldn't open audio device: %s", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec spec = {
        .freq = 44100,
        .format = SDL_AUDIO_F32LE,
        .channels = 1
    };

    SDL_AudioStream *stream = SDL_CreateAudioStream(&spec, NULL);
    if (!stream) {
        printf("Couldn't create audio stream: %s", SDL_GetError());
    } else if (!SDL_BindAudioStream(device_id, stream)) {  /* once bound, it'll start playing when there is data available! */
        printf("Failed to bind stream to device: %s", SDL_GetError());
    }

    // Create mockingboard with injected buffer
    MockingboardEmulator mockingboard(&audio_buffer);
    
    // Example: Queue some register changes to play a tone on chip 0, channel A
    // For 250Hz: period = 62500 / (2 * 250) = 125
    mockingboard.queueRegisterChange(0.0, 0, A_Tone_Low, 34);  // Low byte
    mockingboard.queueRegisterChange(0.0, 0, A_Tone_High, 1);   // High byte
    
    // Set up envelope generator
    // Shape 6: Continue=1, Attack=1, Alternate=0, Hold=1 (0b0110)
    mockingboard.queueRegisterChange(0.0, 0, Envelope_Period_Low, 0x00);  // Envelope period low
    mockingboard.queueRegisterChange(0.0, 0, Envelope_Period_High, 0x10);  // Envelope period high (0x1000 = 4096)
    mockingboard.queueRegisterChange(0.0, 0, Envelope_Shape, 0x06);  // Envelope shape 6
  
    // Enable envelope on channel A (bit 4 = 1)
    mockingboard.queueRegisterChange(0.0, 0, Ampl_A, 0x10);   // Channel A volume with envelope

    mockingboard.queueRegisterChange(0.0, 0, Mixer_Control, 0xF8); // Mixer control (bit 0 = 0 enables tone A)
    mockingboard.queueRegisterChange(0.0, 0, Ampl_B, 0x10);   // Channel B volume with envelope
    mockingboard.queueRegisterChange(0.0, 0, Ampl_C, 0x10);   // Channel B volume with envelope

#if 0
    // Enable tone on channel A
    mockingboard.queueRegisterChange(0.0, 0, Mixer_Control, 0xFE); // Mixer control (bit 0 = 0 enables tone A)
       //0b1111_1010

    mockingboard.queueRegisterChange(2.0, 0, C_Tone_Low, 244);  // Low byte
    mockingboard.queueRegisterChange(2.0, 0, C_Tone_High, 0);   // High byte
    mockingboard.queueRegisterChange(2.0, 0, Ampl_C, 0x10);    // Set channel C volume w envelope

    mockingboard.queueRegisterChange(4.0, 0, Mixer_Control, 0b11111010); // Mixer control (bit 0 = 0 enables tone A)

    mockingboard.queueRegisterChange(6.0, 0, Mixer_Control, 0b11111011); // Mixer control (bit 0 = 0 enables tone A)

    // Change the frequency after 4 seconds to 312.5Hz (period = 100)
    mockingboard.queueRegisterChange(4.0, 0, A_Tone_Low, 130);  // Lower period = higher frequency
    
    // Change the frequency after 5 seconds to 156.25Hz (period = 200)
    mockingboard.queueRegisterChange(5.0, 0, A_Tone_Low, 255);  // Higher period = lower frequency
    
    // Change the frequency after 6 seconds to 78.125Hz (period = 400)
    mockingboard.queueRegisterChange(6.0, 0, A_Tone_Low, 65);  // Low byte of 400
    mockingboard.queueRegisterChange(6.0, 0, A_Tone_High, 0);    // High byte of 400 (0x190)
    
    // Add noise on channel B for 1 second
    mockingboard.queueRegisterChange(7.0, 0, Noise_Period, 0x1F);  // Set noise period to maximum (31)
    mockingboard.queueRegisterChange(7.0, 0, Ampl_B, 15);    // Set channel B volume to max
    mockingboard.queueRegisterChange(7.0, 0, Mixer_Control, 0xE6);  // Enable noise on channel B (bits 3 and 1 = 0)
    mockingboard.queueRegisterChange(8.0, 0, Mixer_Control, 0xFA);  // Disable noise after 1 second, keep A going
    mockingboard.queueRegisterChange(8.0, 0, Ampl_B, 0);     // Set channel B volume to 0
#endif

    float iter = 1.0;
    for (int i = 65; i <= 1800; i *= 1.1) {

        uint8_t low = i & 0xFF;
        uint8_t hi = (i >> 8) & 0xFF;

        mockingboard.queueRegisterChange(iter, 0, A_Tone_Low, low);
        mockingboard.queueRegisterChange(iter, 0, A_Tone_High, hi);

        int fifth = i * 1.5;
        uint8_t low2 = fifth & 0xFF;
        uint8_t hi2 = (fifth >> 8) & 0xFF;

        mockingboard.queueRegisterChange(iter + 0.25, 0, B_Tone_Low, low2);
        mockingboard.queueRegisterChange(iter + 0.25, 0, B_Tone_High, hi2);

        int seventh = i * 1.5 * 1.2;
        uint8_t low3 = seventh & 0xFF;
        uint8_t hi3 = (seventh >> 8) & 0xFF;

        mockingboard.queueRegisterChange(iter + 0.5, 0, C_Tone_Low, low3);
        mockingboard.queueRegisterChange(iter + 0.5, 0, C_Tone_High, hi3);


        iter += 0.5;
    }

#define FRAME_TIME 16666666
#define PLAY_TIME 20000

    // Generate 6 seconds of audio (at 44.1kHz)
    const int samples_per_frame = static_cast<int>(735);  // 16.67ms worth of samples
    const int total_frames = PLAY_TIME / 16.67;  // 6 seconds / 16.67ms
    uint64_t start_time = SDL_GetTicksNS();
    uint64_t processing_time = 0;
    uint64_t avg_loop_time = 0;
    for (int i = 0; i < total_frames; i++) {
        // Process SDL events
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                // Exit the loop if the user closes the window
                i = total_frames;
                break;
            }
        }
        uint64_t frame_start = SDL_GetTicksNS();
        mockingboard.generateSamples(samples_per_frame);
        // Clear the audio buffer after each frame to prevent memory buildup
        // Send the generated audio data to the SDL audio stream
        if (audio_buffer.size() > 0) {
            SDL_PutAudioStreamData(stream, audio_buffer.data(), audio_buffer.size() * sizeof(float));
        }
        audio_buffer.clear();
        uint64_t frame_end = SDL_GetTicksNS();
        processing_time += frame_end - frame_start;

        avg_loop_time += frame_end - start_time;
        printf("frame %6d (st-end: %12lld)\r", i, avg_loop_time/i);
        fflush(stdout);
        if (frame_end - start_time < FRAME_TIME) {
            SDL_DelayNS(FRAME_TIME-(frame_end - start_time));
        }
        start_time += FRAME_TIME;
        
    }
    auto end_time = SDL_GetTicksNS();
    std::cout << "Average Frame Processing Time: " << processing_time / total_frames << " ns" << std::endl;
#if 0  
    // Write audio to WAV file
    if (mockingboard.writeToWav("mockingboard_output.wav")) {
        std::cout << "Successfully wrote audio to mockingboard_output.wav" << std::endl;
    } else {
        std::cout << "Failed to write audio file" << std::endl;
    }
#endif
while (1) {
     SDL_PollEvent(&event);
        if (event.type == SDL_EVENT_QUIT) {
            // Exit the loop if the user closes the window
            break;
        }
}
    return 0;
}
#endif

void mb_write_C0x0(cpu_state *cpu, uint16_t addr, uint8_t data) {
    mb_cpu_data *mb_d = (mb_cpu_data *)get_module_state(cpu, MODULE_MB);
    uint8_t alow = addr & 0x7F;
    uint8_t chip = (addr & 0x80) ? 0 : 1;

    printf("mb_write_C0x0: %02x %02x\n", alow, data);

    if (alow == MB_6522_DDRA) {
        mb_d->d_6522[0].ddra = data;
    } else if (alow == MB_6522_DDRB) {
        mb_d->d_6522[0].ddrb = data;
    } else if (alow == MB_6522_ORA) {
        mb_d->d_6522[0].ora = data;
    } else if (alow == MB_6522_ORB) {
        mb_d->d_6522[0].orb = data;
        if ((data & 0b100) == 0) { // /RESET is low, hence assert reset, reset the chip.
            // reset the chip. Set all 16 registers to 0.
            for (int i = 0; i < 16; i++) {
                double time = cpu->cycles / 1020500.0;
                mb_d->mockingboard->queueRegisterChange(time, chip, i, 0);
            }
        }

        if (data == 7) { // this is the register number.
           mb_d->d_6522[0].reg_num = mb_d->d_6522[0].ora;
           printf("reg_num: %02x\n", mb_d->d_6522[0].reg_num);
        } else if (data == 6) { // write to the specified register
           double time = cpu->cycles / 1020500.0;
           mb_d->mockingboard->queueRegisterChange(time, chip, mb_d->d_6522[0].reg_num, mb_d->d_6522[0].ora);
           printf("queueRegisterChange: [%lf] chip: %d reg: %02x val: %02x\n", time, chip, mb_d->d_6522[0].reg_num, mb_d->d_6522[0].ora);
        }
    }
}

void generate_mockingboard_frame(cpu_state *cpu) {
    mb_cpu_data *mb_d = (mb_cpu_data *)get_module_state(cpu, MODULE_MB);
    const int samples_per_frame = static_cast<int>(736);  // 16.67ms worth of samples -
    // TODO: 736 is an ugly hack. We need to calculate number of samples based on cycles.
    
    mb_d->mockingboard->generateSamples(samples_per_frame);
    // Clear the audio buffer after each frame to prevent memory buildup
    // Send the generated audio data to the SDL audio stream
    if (mb_d->audio_buffer.size() > 0) {
        //printf("generate_mockingboard_frame: %zu\n", mb_d->audio_buffer.size());
        SDL_PutAudioStreamData(mb_d->stream, mb_d->audio_buffer.data(), mb_d->audio_buffer.size() * sizeof(float));
    }
    mb_d->audio_buffer.clear();
}


void init_slot_mockingboard(cpu_state *cpu, SlotType_t slot) {
    uint16_t slot_base = 0xC080 + (slot * 0x10);
    printf("init_slot_mockingboard: %d\n", slot);

    mb_cpu_data *mb_d = new mb_cpu_data;
    mb_d->d_6522[0].ddra = 0xFF;
    mb_d->d_6522[0].ddrb = 0xFF;
    mb_d->d_6522[0].ora = 0x00;
    mb_d->d_6522[0].orb = 0x00;
    mb_d->d_6522[0].reg_num = 0x00;
    mb_d->mockingboard = new MockingboardEmulator(&mb_d->audio_buffer);

    speaker_state_t *speaker_d = (speaker_state_t *)get_module_state(cpu, MODULE_SPEAKER);
    int dev_id = speaker_d->device_id;

/** Init audio stream for the mockingboard device */
    SDL_AudioSpec spec = {
        .freq = 44100,
        .format = SDL_AUDIO_F32LE,
        .channels = 1
    };

    SDL_AudioStream *stream = SDL_CreateAudioStream(&spec, NULL);
    if (!stream) {
        printf("Couldn't create audio stream: %s", SDL_GetError());
    } else if (!SDL_BindAudioStream(dev_id, stream)) {  /* once bound, it'll start playing when there is data available! */
        printf("Failed to bind stream to device: %s", SDL_GetError());
    }
    mb_d->stream = stream;

    set_module_state(cpu, MODULE_MB, mb_d);

    // These won't work because we need to set up registers in CS00 and CS80. So weird.
    // for now, just tie this in inside bus.cpp which is ugly. But we need a more generalized
    // mechanism for all these different ways we can read/write to I/O memory. Maybe a function 
    // pointer in the memory map alongside the memory type flag?

    /* register_C0xx_memory_write_handler(slot_base + MB_6522_DDRA, mb_write_C0x0);
    register_C0xx_memory_write_handler(slot_base + MB_6522_DDRB, mb_write_C0x0);
    register_C0xx_memory_write_handler(slot_base + MB_6522_ORA, mb_write_C0x0);
    register_C0xx_memory_write_handler(slot_base + MB_6522_ORB, mb_write_C0x0); */

}