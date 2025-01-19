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

static FILE *speaker_recording = nullptr;

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



#define SAMPLE_BUFFER_SIZE (4096)

int16_t working_buffer[SAMPLE_BUFFER_SIZE] = {0};

void audio_generate_frame(cpu_state *cpu, uint64_t cycle_window_start, uint64_t cycle_window_end) {

    static int inv = 1; // speaker polarity, -1 or +1
    static uint64_t ns_per_sample = 1000000000 / 44100;  // 22675.736
    static uint64_t ns_per_cycle = cpu->cycle_duration_ns; // must calculate from actual results in ludicrous speed

    if (cycle_window_start == 0 && cycle_window_end == 0) {
        printf("audio_generate_frame: first time send empty frame and a bit more\n");
        memset(working_buffer, 0, 1000 * sizeof(int16_t));
        SDL_PutAudioStreamData(speaker_states[0].stream, working_buffer, 1000*sizeof(int16_t));
        return;
    }

    uint64_t queued_samples = SDL_GetAudioStreamQueued(speaker_states[0].stream);
    if (queued_samples < 100) { printf("queue underrun %llu\n", queued_samples); 
        // attempt to calculate how much time slipped and generate that many samples
        memset(working_buffer, 0, 1000 * sizeof(int16_t));
        SDL_PutAudioStreamData(speaker_states[0].stream, working_buffer, 1000*sizeof(int16_t));
    }

    // is it more accurate to convert cycles to time then to samples??
    // convert cycles to time

    uint64_t samples_count = 735 ; // (time_delta / ns_per_sample)+1; // 734.7 - round up to 735

    // if samples_count is too large (say, 1500) then we have somehow fallen behind,
    // and we need to catch up. But not too fast or we blow the sample buffer array.

    uint64_t cpu_delta = cycle_window_end - cycle_window_start;
    uint64_t cycles_per_sample = cpu_delta / samples_count;
    
    if (DEBUG(DEBUG_SPEAKER)) std::cout << " cpu_delta: " << cpu_delta   
        << " samp_c: " << samples_count << " cyc/samp: " << cycles_per_sample
        <<  " cyc range: [" << cycle_window_start << " - " << cycle_window_end << "] evtq: " 
        << event_buffer.count << " qd_samp: " << queued_samples << "\n";

    /**
     * this is the more savvy Chris Torrance algorithm.
     */
    uint64_t event_tick;
    int16_t contribution = 0;
    uint64_t cyc = cycle_window_start;
    for (uint64_t samp = 0; samp < samples_count; samp++) {
        for (uint64_t cyc_i = 0; cyc_i < cycles_per_sample; cyc_i ++) {
            event_buffer.peek_oldest(event_tick);
            if (event_tick <= cyc) {
                event_buffer.pop_oldest(event_tick);
                inv = -inv;
            }
            contribution += inv;
            cyc++;
        }
        working_buffer[samp] = ((float)contribution / (float)cycles_per_sample) * 0x6000;
        contribution = 0;
    }
    // copy samples out to audio stream
    SDL_PutAudioStreamData(speaker_states[0].stream, working_buffer, samples_count*sizeof(int16_t));
}


inline void log_speaker_blip(cpu_state *cpu) {
    event_buffer.add_event(cpu->cycles);
    if (speaker_states[0].device_started == 0) {
        speaker_start();
    }
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
    SDL_PauseAudioDevice(speaker_states[0].device_id);

    // prime the pump with a few frames of silence.
    memset(working_buffer, 0, 735 * sizeof(int16_t));
    SDL_PutAudioStreamData(speaker_states[0].stream, working_buffer, 735*sizeof(int16_t));

    if (DEBUG(DEBUG_SPEAKER)) fprintf(stdout, "init_speaker\n");
    register_C0xx_memory_read_handler(0xC030, speaker_memory_read);
    register_C0xx_memory_write_handler(0xC030, speaker_memory_write);
}

void speaker_start() {
    // Start audio playback
    // put a frame of blank audio into the buffer to prime the pump.

    if (!SDL_ResumeAudioDevice(speaker_states[0].device_id)) {
        std::cerr << "Error resuming audio device: " << SDL_GetError() << std::endl;
    }
    memset(working_buffer, 0, SAMPLE_BUFFER_SIZE * sizeof(int16_t));
    SDL_PutAudioStreamData(speaker_states[0].stream, working_buffer, 735*sizeof(int16_t));

    speaker_states[0].device_started = 1;
}

void speaker_stop() {
    // Stop audio playback
    memset(working_buffer, 0, SAMPLE_BUFFER_SIZE * sizeof(int16_t));
    SDL_PutAudioStreamData(speaker_states[0].stream, working_buffer, SAMPLE_BUFFER_SIZE*sizeof(int16_t));

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

