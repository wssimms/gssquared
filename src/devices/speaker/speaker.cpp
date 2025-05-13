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
#include <cmath>

/* #include <mach/mach_time.h> */

#include "gs2.hpp"
#include "debug.hpp"
#include "bus.hpp"
#include "devices/speaker/speaker.hpp"
#include "devices/speaker/LowPass.hpp"

/**
 * Each audio frame is for 17000 samples per frame (1020500 samples/second, 1/60th second)
 * These are first downsampled (close to integer) by passing through a low pass filter,
 * and adding each cycle's contribution to the sample.
 * then, output samples are passed through a second low pass filter to remove high frequency noise.
 * We set an audio stream with SDL to be 1700 samples per frame (102000 samples/second),
 * so that our downsampling is close to integer. Otherwise we get significant artifacts because
 * we'd have fractional cycles we're not counting.
 * We keep track of the cycle counter. So an audio frame has a
 * start cycle counter and an end cycle counter.
 * 
 * Keep a circular buffer of 32000 events.
 * 
 */




uint64_t audio_generate_frame(cpu_state *cpu, uint64_t cycle_window_start, uint64_t cycle_window_end) {
    speaker_state_t *speaker_state = (speaker_state_t *)get_module_state(cpu,MODULE_SPEAKER);
    int16_t *working_buffer = speaker_state->working_buffer;
    EventBuffer *event_buffer = &speaker_state->event_buffer;

    //static uint64_t ns_per_sample = 1000000000 / SAMPLE_RATE;  // 22675.736
    //static uint64_t ns_per_cycle = cpu->cycle_duration_ns; // must calculate from actual results in ludicrous speed

    if (cycle_window_start == 0 && cycle_window_end == 0) {
        printf("audio_generate_frame: first time send empty frame and a bit more\n");
        memset(working_buffer, 0, SAMPLES_PER_FRAME * sizeof(int16_t));
        SDL_PutAudioStreamData(speaker_state->stream, working_buffer, SAMPLES_PER_FRAME*sizeof(int16_t));
        SDL_PutAudioStreamData(speaker_state->stream, working_buffer, SAMPLES_PER_FRAME*sizeof(int16_t));
        return SAMPLES_PER_FRAME*2;
    }

    uint64_t queued_samples = SDL_GetAudioStreamQueued(speaker_state->stream);
    if (queued_samples < SAMPLES_PER_FRAME) { printf("queue underrun %llu %f %f\n", queued_samples, speaker_state->amplitude, speaker_state->polarity); 
        // attempt to calculate how much time slipped and generate that many samples
        for (int x = 0; x < SAMPLES_PER_FRAME; x++) {
            working_buffer[x] = speaker_state->amplitude * speaker_state->polarity;
            speaker_state->amplitude = speaker_state->amplitude - AMPLITUDE_DECAY_RATE;
            if (speaker_state->amplitude < 0) speaker_state->amplitude = 0; // TEST
        }
        SDL_PutAudioStreamData(speaker_state->stream, working_buffer, SAMPLES_PER_FRAME*sizeof(int16_t));
    }

    // is it more accurate to convert cycles to time then to samples??
    // convert cycles to time

    uint64_t samples_count = SAMPLES_PER_FRAME; // (time_delta / ns_per_sample)+1; // 734.7 - round up to 735

    // if samples_count is too large (say, 1500) then we have somehow fallen behind,
    // and we need to catch up. But not too fast or we blow the sample buffer array.

    uint64_t cpu_delta = cycle_window_end - cycle_window_start;
    uint64_t cycles_per_sample = cpu_delta / samples_count;
    
    if (DEBUG(DEBUG_SPEAKER)) std::cout << " cpu_delta: " << cpu_delta   
        << " samp_c: " << samples_count << " cyc/samp: " << cycles_per_sample
        <<  " cyc range: [" << cycle_window_start << " - " << cycle_window_end << "] evtq: " 
        << event_buffer->count << " qd_samp: " << queued_samples << " amp: " << speaker_state->amplitude << "\n";

    /**
     * this is the more savvy Chris Torrance algorithm.
     */
    uint64_t event_tick;
    uint64_t cyc = cycle_window_start;

    for (uint64_t samp = 0; samp < samples_count; samp++) {
        double contribution = 0;
        for (uint64_t cyc_i = 0; cyc_i < cycles_per_sample; cyc_i ++) {
            if (event_buffer->peek_oldest(event_tick)) { // only do this if there is an event in the buffer.
                if (event_tick <= cyc) {
                    event_buffer->pop_oldest(event_tick);
                    speaker_state->polarity = -speaker_state->polarity;
                    speaker_state->amplitude = AMPLITUDE_PEAK;                  // we had a speaker blip, reset amplitude.
                }
            }
            contribution += speaker_state->preFilter->process(speaker_state->polarity);
            cyc++;
        }

        double raw_sample_value = 0.0f;
        if (cycles_per_sample > 0) {
            // Calculate raw sample value
            raw_sample_value = (contribution / (double)cycles_per_sample) * speaker_state->amplitude;
        } else {
            // there was no change in speaker state during this cycle.
            // if cycles_per_sample is 0, then something is very wrong. called during startup?
            raw_sample_value = speaker_state->amplitude * speaker_state->polarity;
        }
        
        //float final_value = applyLowPassFilter(speaker_state->current_value);
        double final_value = speaker_state->postFilter->process(raw_sample_value);
        if (final_value > 32767.0f) final_value = 32767.0f;
        if (final_value < -32768.0f) final_value = -32768.0f;
        
        working_buffer[samp] = (int16_t)final_value;

        // Modified amplitude decay - slightly more natural exponential decay
        speaker_state->amplitude *= 0.997f; // Exponential decay factor
        if (speaker_state->amplitude < 0) speaker_state->amplitude = 0;

    }
    // copy samples out to audio stream
    SDL_PutAudioStreamData(speaker_state->stream, working_buffer, samples_count*sizeof(int16_t));
    return samples_count;
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
    desired.freq = SAMPLE_RATE;
    desired.format = SDL_AUDIO_S16LE;
    desired.channels = 1;

    speaker_state->device_id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (speaker_state->device_id == 0) {
        SDL_Log("Couldn't open audio device: %s", SDL_GetError());
        return;
    }

    speaker_state->stream = SDL_CreateAudioStream(&desired, NULL);
    if (!speaker_state->stream) {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return;
    } else if (!SDL_BindAudioStream(speaker_state->device_id, speaker_state->stream)) {  /* once bound, it'll start playing when there is data available! */
        SDL_Log("Failed to bind speaker stream to device: %s", SDL_GetError());
        return;
    }

    SDL_PauseAudioDevice(speaker_state->device_id);

    // prime the pump with a few frames of silence.
    memset(speaker_state->working_buffer, 0, SAMPLES_PER_FRAME * sizeof(int16_t));
    SDL_PutAudioStreamData(speaker_state->stream, speaker_state->working_buffer, SAMPLES_PER_FRAME*sizeof(int16_t));

    if (DEBUG(DEBUG_SPEAKER)) fprintf(stdout, "init_speaker\n");
    for (uint16_t addr = 0xC030; addr <= 0xC03F; addr++) {
        register_C0xx_memory_read_handler(addr, speaker_memory_read);
        register_C0xx_memory_write_handler(addr, speaker_memory_write);
    }
    speaker_state->preFilter = new LowPassFilter();
    speaker_state->preFilter->setCoefficients(8000.0f, (double)1020500); // 1020500 is actual possible sample rate of input toggles.
    speaker_state->postFilter = new LowPassFilter();
    speaker_state->postFilter->setCoefficients(8000.0f, (double)SAMPLE_RATE);
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
    SDL_PutAudioStreamData(speaker_state->stream, working_buffer, SAMPLES_PER_FRAME*sizeof(int16_t));

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

