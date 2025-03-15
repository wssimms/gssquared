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

#include "gs2.hpp"
#include "debug.hpp"
#include "bus.hpp"
#include "devices/speaker/speaker.hpp"

/**
 * Each audio frame is for 735 samples (44100 samples/second, 1/60th second)
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


void audio_generate_frame(cpu_state *cpu, uint64_t cycle_window_start, uint64_t cycle_window_end) {
    speaker_state_t *speaker_state = (speaker_state_t *)get_module_state(cpu,MODULE_SPEAKER);
    int16_t *working_buffer = speaker_state->working_buffer;
    EventBuffer *event_buffer = &speaker_state->event_buffer;

    // int polarity = 1; // speaker polarity, -1 or +1
    static uint64_t ns_per_sample = 1000000000 / 44100;  // 22675.736
    static uint64_t ns_per_cycle = cpu->cycle_duration_ns; // must calculate from actual results in ludicrous speed

    if (cycle_window_start == 0 && cycle_window_end == 0) {
        printf("audio_generate_frame: first time send empty frame and a bit more\n");
        memset(working_buffer, 0, 735 * sizeof(int16_t));
        SDL_PutAudioStreamData(speaker_state->stream, working_buffer, 735*sizeof(int16_t));
        SDL_PutAudioStreamData(speaker_state->stream, working_buffer, 735*sizeof(int16_t));
        return;
    }

    uint64_t queued_samples = SDL_GetAudioStreamQueued(speaker_state->stream);
    if (queued_samples < 735) { printf("queue underrun %llu\n", queued_samples); 
        // attempt to calculate how much time slipped and generate that many samples
        for (int x = 0; x < 735; x++) {
            working_buffer[x] = speaker_state->last_sample;
        }
        SDL_PutAudioStreamData(speaker_state->stream, working_buffer, 735*sizeof(int16_t));
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
        << event_buffer->count << " qd_samp: " << queued_samples << "\n";

    /**
     * this is the more savvy Chris Torrance algorithm.
     */
    uint64_t event_tick;
    int16_t contribution = 0;
    uint64_t cyc = cycle_window_start;
    for (uint64_t samp = 0; samp < samples_count; samp++) {
        for (uint64_t cyc_i = 0; cyc_i < cycles_per_sample; cyc_i ++) {
            event_buffer->peek_oldest(event_tick);
            if (event_tick <= cyc) {
                event_buffer->pop_oldest(event_tick);
                speaker_state->polarity = -speaker_state->polarity;
            }
            contribution += speaker_state->polarity;
            cyc++;
        }
 //       working_buffer[samp] = ((float)contribution / (float)cycles_per_sample) * 0x6000;
        if (cycles_per_sample > 0) {  // Prevent division by zero
            float sample_value = ((float)contribution / (float)cycles_per_sample) * 0x6000;
            // Clamp the value to valid int16_t range
            if (sample_value > 32767.0f) sample_value = 32767.0f;
            if (sample_value < -32768.0f) sample_value = -32768.0f;
            working_buffer[samp] = (int16_t)sample_value;
            speaker_state->last_sample = (int16_t)sample_value;
        } else {
            working_buffer[samp] = speaker_state->last_sample;  // Safe default when we can't calculate
        }
        contribution = 0;
    }
    // copy samples out to audio stream
    SDL_PutAudioStreamData(speaker_state->stream, working_buffer, samples_count*sizeof(int16_t));
}


inline void log_speaker_blip(cpu_state *cpu) {
    speaker_state_t *speaker_state = (speaker_state_t *)get_module_state(cpu, MODULE_SPEAKER);
    EventBuffer *event_buffer = &speaker_state->event_buffer;

    event_buffer->add_event(cpu->cycles);
    if (speaker_state->device_started == 0) {
        speaker_start(cpu);
    }
    if (speaker_state->speaker_recording) {
        fprintf(speaker_state->speaker_recording, "%llu\n", cpu->cycles);
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

void init_mb_speaker(cpu_state *cpu, SlotType_t slot) {

    speaker_state_t *speaker_state = new speaker_state_t;

    //speaker_state->cpu = cpu;
    speaker_state->speaker_recording = nullptr;

    set_module_state(cpu, MODULE_SPEAKER, speaker_state);

	// Initialize SDL audio - is this right, to do this again here?
	SDL_Init(SDL_INIT_AUDIO);
	
    SDL_AudioSpec desired = {};
    desired.freq = 44100;
    desired.format = SDL_AUDIO_S16LE;
    desired.channels = 1;
    
    speaker_state->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, NULL, NULL);

    std::cout << "SDL_OpenAudioDevice returned: " << speaker_state->device_id << "\n";

    if ( speaker_state->stream == nullptr )
    {
        std::cerr << "Error opening audio device: " << SDL_GetError() << std::endl;
        return;
    }

    speaker_state->device_id = SDL_GetAudioStreamDevice(speaker_state->stream);
    SDL_PauseAudioDevice(speaker_state->device_id);

    // prime the pump with a few frames of silence.
    memset(speaker_state->working_buffer, 0, 735 * sizeof(int16_t));
    SDL_PutAudioStreamData(speaker_state->stream, speaker_state->working_buffer, 735*sizeof(int16_t));

    if (DEBUG(DEBUG_SPEAKER)) fprintf(stdout, "init_speaker\n");
    for (uint16_t addr = 0xC030; addr <= 0xC03F; addr++) {
        register_C0xx_memory_read_handler(addr, speaker_memory_read);
        register_C0xx_memory_write_handler(addr, speaker_memory_write);
    }
}

void speaker_start(cpu_state *cpu) {
    speaker_state_t *speaker_state = (speaker_state_t *)get_module_state(cpu, MODULE_SPEAKER);
    int16_t *working_buffer = speaker_state->working_buffer;

    // Start audio playback
    // put a frame of blank audio into the buffer to prime the pump.

    if (!SDL_ResumeAudioDevice(speaker_state->device_id)) {
        std::cerr << "Error resuming audio device: " << SDL_GetError() << std::endl;
    }
    memset(working_buffer, 0, SAMPLE_BUFFER_SIZE * sizeof(int16_t));
    SDL_PutAudioStreamData(speaker_state->stream, working_buffer, 735*sizeof(int16_t));

    speaker_state->device_started = 1;
}

void speaker_stop(cpu_state *cpu) {
    speaker_state_t *speaker_state = (speaker_state_t *)get_module_state(cpu, MODULE_SPEAKER);
    int16_t *working_buffer = speaker_state->working_buffer;

    // Stop audio playback
    memset(working_buffer, 0, SAMPLE_BUFFER_SIZE * sizeof(int16_t));
    SDL_PutAudioStreamData(speaker_state->stream, working_buffer, SAMPLE_BUFFER_SIZE*sizeof(int16_t));

    SDL_PauseAudioDevice(speaker_state->device_id);  // 1 means pause
    speaker_state->device_started = 0;
}

/**
 * this is for debugging. This will write a log file with the following data:
 * the time in cycles of every 'speaker blip'.
 */

void toggle_speaker_recording(cpu_state *cpu)
{
    speaker_state_t *speaker_state = (speaker_state_t *)get_module_state(cpu, MODULE_SPEAKER);

    if (speaker_state->speaker_recording == nullptr) {
        speaker_state->speaker_recording = fopen("speaker_event_log.txt", "w");
    } else {
        fclose(speaker_state->speaker_recording);
        speaker_state->speaker_recording = nullptr;
    }
};

