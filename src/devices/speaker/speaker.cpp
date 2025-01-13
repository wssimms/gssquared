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

#include <cstdio>
#include <SDL3/SDL.h>

/* #include <mach/mach_time.h> */

#include "debug.hpp"
#include "bus.hpp"
#include "devices/speaker/speaker.hpp"


// SDL audio device ID after opening
/* SDL_AudioDeviceID device = 0;
int device_started = 0; */

// At 44.1kHz, each sample is ~0.023ms (1/44100 seconds)
// let's say a blip is 1ms. 1ms = 1000us = 43 samples.

#define BLIP_LENGTH_US 400
#define BLIP_SAMPLES int(BLIP_LENGTH_US / 22.6)

static FILE *speaker_recording = nullptr;

int16_t blip_data[BLIP_SAMPLES] = {
    0,
    0x3000,
    0x6000,
    0x6000,
    0x6000,
    0x6000,
    0x6000,
    0x6000,
    0x6000,
    0x6000,
    0x6000,
    0x6000,
    0x6000,
    0x6000,    
    0x6000,
    0x3000,
    0   
};

/*
    0x6000,
    0x6000,
    0x6000,
    0x6000,
    0x6000,
    0x6000,
    0x6000,
    0x4000,
    0x2000,
    0
};
*/

#define EVENT_BUFFER_SIZE 32000

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

EventBuffer event_buffer;


/**
 * Each audio frame is for 735 samples.
 * 
 * We keep track of the cycle counter. So an audio frame has a
 * start cycle counter and an end cycle counter.
 * 
 * Keep a circular buffer of 32000 events.
 * 735 samples is 16905 uS.
 * This is how many cycles at 1.0205MHz?
 * 16905 * 1.0205 = 17250 cycles.
 * 
 */

typedef struct speaker_state {
    cpu_state *cpu = NULL;
    SDL_AudioDeviceID device_id = 0;
    SDL_AudioStream *stream = NULL;
    int device_started = 0;
} speaker_state;

speaker_state speaker_states[2];

#define US_PER_SAMPLE 22.6
#define US_PER_BUFFER (US_PER_SAMPLE * 735)
/*#define CYCLES_PER_BUFFER (int)((float)US_PER_BUFFER * 1.0205)*/
#define CYCLES_PER_BUFFER 17008

#define WORKING_BUFFER_SAMPLES (2048)

int16_t working_buffer[WORKING_BUFFER_SAMPLES] = {0};


uint64_t cumulative_time = 0;
uint64_t startup_time = 0;
uint64_t cumulative_samples = 0;

// this is averaging about 10 too many samples per interval.
// of course the emulator is actually running faster than 1.0205MHz., like 1.0325. ;
// 1.1%; 
// instead of using assumption of how many cpu cycles; keep track of last time and last
// cpu cycles and use that to calculate the number of samples.
// that's cpu_delta and time_delta_last. 
// (cpu_delta / time_delta_last) * 44100 = samples_expected

void audio_generate_frame(cpu_state *cpu) {

    static uint64_t buf_start_cycle = 0; // first time, we start at 0.
    uint64_t buf_end_cycle = cpu->cycles -1; // TODO: adjust this down for the length of a blip
    uint64_t cpu_delta = buf_end_cycle - buf_start_cycle;
    static int inv = 1;

    static uint64_t last_time = 0;
    uint64_t current_time = SDL_GetTicksNS();
    if (last_time == 0) {
        last_time = current_time - 33333333; /* 33.3 milliseconds */
        startup_time = last_time;
    }
    uint64_t time_delta_last = current_time - last_time;
    uint64_t time_delta_startup = (current_time - startup_time);

    uint64_t desired_ns_per_sample = (1000000000 / 44100);
    uint64_t desired_time = 1000000000;
    uint64_t desired_samples = 44100;

    uint64_t cumulative_samples_expected = (time_delta_startup * desired_samples) / desired_time;
    uint64_t samples_count = (time_delta_last * desired_samples) / desired_time;

    int64_t samples_diverged = cumulative_samples + samples_count - cumulative_samples_expected;

    static int iterations = 60;

    // gentle adjustment to number of samples to try to stay in sync. (maybe do something logarithmic)
    if (samples_diverged < -10) {
        samples_count+=10;
    }
    if (samples_diverged > 10) {
        samples_count-=10;
    }

    int cycles_per_sample = cpu_delta / samples_count ;

    if (iterations-- == 0) {
        if (DEBUG(DEBUG_SPEAKER)) std::cout << "time_delta: " 
            << time_delta_last << " cpu_delta: " << cpu_delta << " samples_count: " << samples_count << " cycles_per_sample: " << cycles_per_sample
            <<  " buf-cycle range: [" << buf_start_cycle << " - " << buf_end_cycle << "] event count: " << event_buffer.count << "\n";

        if (DEBUG(DEBUG_SPEAKER)) std::cout << "Time since start: " << time_delta_startup/1000000 << "(ms) Samples: cumulative / expected / diverged: " 
            << cumulative_samples + samples_count << " / " << cumulative_samples_expected << " / " << samples_diverged <<  "\n";
        iterations = 60;
    }

    if (samples_count <= 0) {
        return;
    }

    // Clear the second half of working buffer (first half contains previous overflow)
    if (samples_count > WORKING_BUFFER_SAMPLES) {
        //printf("samples_requested exceeeded working buffer samples\n");
        // I can reduce samples_count to WORKING_BUFFER_SAMPLES, but then need to change the buf_end_cycle.
        // increase the buffer size for now.
        samples_count = WORKING_BUFFER_SAMPLES;
        buf_end_cycle = (buf_start_cycle + (int)((float)WORKING_BUFFER_SAMPLES * 23.14) );
         if (DEBUG(DEBUG_SPEAKER)) printf(" -> constrained buf_end_cycle %llu\n", buf_end_cycle);
    }

    // clear the buffer
    memset(working_buffer + BLIP_SAMPLES, 0, (WORKING_BUFFER_SAMPLES-BLIP_SAMPLES) * sizeof(int16_t));

    // this will be more accurate with FP math.
    uint64_t event_time;
    while (event_buffer.peek_oldest(event_time)) {
        if (event_time < buf_start_cycle) { // discard too old events.
            event_buffer.pop_oldest(event_time);
        } else if (event_time > buf_end_cycle) {
            break;
        } else {
            event_buffer.pop_oldest(event_time);
            /* int sample_index = int(((float)(event_time-buf_start_cycle) / 1.0205) / US_PER_SAMPLE); */
            int sample_index = (event_time-buf_start_cycle) / cycles_per_sample;

            //if (DEBUG(DEBUG_SPEAKER)) std::cout << "event_time " << event_time << " sample_index " << sample_index << "\n";
            
            // Paint into working buffer - we have room for the full blip
            for (int j = 0; j < BLIP_SAMPLES; j++) {
                working_buffer[sample_index + j] += (blip_data[j] * inv);
            }
        }
        inv = -inv;
    }

    // Copy as much data as we had cycles to generate
    // So, 1024050 cycles = 44100 samples.
    // What is cycles delta? cpu_delta.
    SDL_PutAudioStreamData(speaker_states[0].stream, working_buffer, samples_count*sizeof(int16_t));
    //debug_dump_pointer((uint8_t *) working_buffer, 0x200);

    // move the remaining data to the start of the buffer
    //memcpy(working_buffer, working_buffer + samples_count*sizeof(int16_t), (WORKING_BUFFER_SAMPLES-samples_count) * sizeof(int16_t));
    memcpy(working_buffer, working_buffer + samples_count*sizeof(int16_t), BLIP_SAMPLES * sizeof(int16_t));
    buf_start_cycle = buf_end_cycle + 1;
    last_time = current_time;
    cumulative_samples += samples_count;

    //if (DEBUG(DEBUG_SPEAKER)) printf("new buf_start_cycle %llu\n", buf_start_cycle);

}

inline void log_speaker_blip(cpu_state *cpu) {
/*     if (DEBUG(DEBUG_SPEAKER)) {
        std::cout << "log_speaker_blip " << cpu->cycles << "\n";
    } */
    // trigger a speaker start once we have enough cycles to fill a buffer
    /* if ((speaker_states[0].device_started == 0) && (cpu->cycles > CYCLES_PER_BUFFER * 3)) {
        speaker_start();
    } */
    event_buffer.add_event(cpu->cycles);
    if (speaker_recording) {
        fprintf(speaker_recording, "%llu\n", cpu->cycles);
    }
}


#define SPEAKER_EVENT_LOG_SIZE 16384
uint64_t speaker_event_log[SPEAKER_EVENT_LOG_SIZE];

uint8_t speaker_memory_read(cpu_state *cpu, uint16_t address) {
    log_speaker_blip(cpu);
    return 0xA0; // what does speaker read return?
}

void speaker_memory_write(cpu_state *cpu, uint16_t address, uint8_t value) {
    log_speaker_blip(cpu);
}

void init_mb_speaker(cpu_state *cpu) {

    speaker_states[0].cpu = cpu;

	// Initialize SDL audio - is this right, to do this again here?
	SDL_Init(SDL_INIT_AUDIO);
	
    SDL_AudioSpec desired = {};
    desired.freq = 44100;
    desired.format = SDL_AUDIO_S16LE;
    desired.channels = 1;
    
    speaker_states[0].stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, NULL, NULL);

    std::cout << "SDL_OpenAudioDevice returned: " << speaker_states[0].device_id << "\n";

    if ( speaker_states[0].stream == nullptr )
    {
        std::cerr << "Error opening audio device: " << SDL_GetError() << std::endl;
        return;
    }

    speaker_states[0].device_id = SDL_GetAudioStreamDevice(speaker_states[0].stream);

    if (DEBUG(DEBUG_SPEAKER)) fprintf(stdout, "init_speaker\n");
    register_C0xx_memory_read_handler(0xC030, speaker_memory_read);
    register_C0xx_memory_write_handler(0xC030, speaker_memory_write);
}

void speaker_start() {
    // Start audio playback
    // put a frame of blank audio into the buffer to prime the pump.

    memset(working_buffer, 0, WORKING_BUFFER_SAMPLES * sizeof(int16_t));
    SDL_PutAudioStreamData(speaker_states[0].stream, working_buffer, WORKING_BUFFER_SAMPLES*sizeof(int16_t));
    
    if (!SDL_ResumeAudioDevice(speaker_states[0].device_id)) {
        std::cerr << "Error resuming audio device: " << SDL_GetError() << std::endl;
    }
    speaker_states[0].device_started = 1;
}

void speaker_stop() {
    // Stop audio playback
    SDL_PauseAudioDevice(speaker_states[0].device_id);  // 1 means pause
    speaker_states[0].device_started = 0;
}

/**
 * this is for debugging. This will write a log file with the following data:
 * the time in cycles of every 'speaker blip'.
 */

void toggle_speaker_recording()
{
    if (speaker_recording == nullptr) {
        speaker_recording = fopen("speaker_event_log.txt", "w");
    } else {
        fclose(speaker_recording);
        speaker_recording = nullptr;
    }
};

