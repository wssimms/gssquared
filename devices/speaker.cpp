#include <cstdio>
#include <SDL2/SDL.h>

#include <mach/mach_time.h>

#include "../debug.hpp"

#include "speaker.hpp"
#include "../bus.hpp"


// SDL audio device ID after opening
/* SDL_AudioDeviceID device = 0;
int device_started = 0; */

// At 44.1kHz, each sample is ~0.023ms (1/44100 seconds)
// let's say a blip is 1ms. 1ms = 1000us = 43 samples.

#define BLIP_LENGTH_US 1000
#define BLIP_SAMPLES int(BLIP_LENGTH_US / 22.6)

int16_t blip_data[BLIP_SAMPLES] = {
    0,
    0x2000,
    0x4000,
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
    0x6000,
    0x6000,
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
    int device_started = 0;
} speaker_state;

speaker_state speaker_states[2];

#define US_PER_SAMPLE 22.6
#define US_PER_BUFFER (US_PER_SAMPLE * 735)
/*#define CYCLES_PER_BUFFER (int)((float)US_PER_BUFFER * 1.0205)*/
#define CYCLES_PER_BUFFER 17008

#define MAX_SDL_BUFFER_SAMPLES 2048  // Should be larger than what SDL typically requests
#define WORKING_BUFFER_SAMPLES (MAX_SDL_BUFFER_SAMPLES * 2)

int16_t working_buffer[WORKING_BUFFER_SAMPLES] = {0};

void audio_callback(void *userdata, Uint8 *stream, int len) {
    static uint64_t buf_start_cycle = 0;
    static uint64_t buf_end_cycle = CYCLES_PER_BUFFER-1;
    static int inv = 1;
    static uint64_t last_cpu_cycles = 0;
    static uint64_t last_buffer_time = 0;

    int16_t *output_buffer = (int16_t *)stream;
    int output_buffer_size = len / sizeof(int16_t);
    
    uint64_t cpu_delta = speaker_states[0].cpu->cycles - last_cpu_cycles;
    uint64_t time_delta = mach_absolute_time() - last_buffer_time;
    if (DEBUG(DEBUG_SPEAKER)) std::cout << "audio_callback cpu: " << speaker_states[0].cpu->cycles << "/" << cpu_delta << " time: " << time_delta << " len: " << len << " buf start: " << buf_start_cycle << " buf end: " << buf_end_cycle << " event count: " << event_buffer.count << "\n";
    
    last_cpu_cycles = speaker_states[0].cpu->cycles;
    last_buffer_time = mach_absolute_time();
    // Clear the second half of working buffer (first half contains previous overflow)
    memset(working_buffer + output_buffer_size, 0, output_buffer_size * sizeof(int16_t));

    uint64_t event_time;
    while (event_buffer.peek_oldest(event_time)) {
        if (event_time < buf_start_cycle) {
            event_buffer.pop_oldest(event_time);
        } else if (event_time > buf_end_cycle) {
            break;
        } else {
            event_buffer.pop_oldest(event_time);
            int sample_index = int(((float)(event_time-buf_start_cycle) / 1.0205) / US_PER_SAMPLE);
            if (DEBUG(DEBUG_SPEAKER)) std::cout << "event_time " << event_time << " sample_index " << sample_index << "\n";
            
            // Paint into working buffer - we have room for the full blip
            for (int j = 0; j < BLIP_SAMPLES; j++) {
                working_buffer[sample_index + j] += (blip_data[j] * inv);
            }
        }
        inv = -inv;
    }

    // Copy first half of working buffer to output
    memcpy(output_buffer, working_buffer, output_buffer_size * sizeof(int16_t));
    
    // Slide second half to first half
    memmove(working_buffer, working_buffer + output_buffer_size, output_buffer_size * sizeof(int16_t));

    if (buf_end_cycle > speaker_states[0].cpu->cycles) {
        if (DEBUG(DEBUG_SPEAKER)) std::cout << "audio cycle overrun resync from " << buf_end_cycle << " to " << speaker_states[0].cpu->cycles - CYCLES_PER_BUFFER << "\n";
        buf_start_cycle = speaker_states[0].cpu->cycles - CYCLES_PER_BUFFER;
        buf_end_cycle = buf_start_cycle + CYCLES_PER_BUFFER - 1;
    } else if (buf_end_cycle < speaker_states[0].cpu->cycles - CYCLES_PER_BUFFER*2) {
        if (DEBUG(DEBUG_SPEAKER)) std::cout << "audio cycle underrunresync from " << buf_end_cycle << " to " << speaker_states[0].cpu->cycles - CYCLES_PER_BUFFER << "\n";
        buf_start_cycle = speaker_states[0].cpu->cycles - CYCLES_PER_BUFFER;
        buf_end_cycle = buf_start_cycle + CYCLES_PER_BUFFER - 1;
    } else {
        // advance the cycle period
        buf_start_cycle += CYCLES_PER_BUFFER;
        buf_end_cycle += CYCLES_PER_BUFFER;
    }
}

inline void log_speaker_blip(cpu_state *cpu) {
    if (DEBUG(DEBUG_SPEAKER)) {
        std::cout << "log_speaker_blip " << cpu->cycles << "\n";
    }
    // trigger a speaker start once we have enough cycles to fill a buffer
    if ((speaker_states[0].device_started == 0) && (cpu->cycles > CYCLES_PER_BUFFER * 3)) {
        speaker_start();
    }
    event_buffer.add_event(cpu->cycles);
}

#define SPEAKER_EVENT_LOG_SIZE 16384
uint64_t speaker_event_log[SPEAKER_EVENT_LOG_SIZE];

#ifdef OLD_SPEAKER_EVENT_LOG
int speaker_event_pop = 0; // tracks where reader (audio generator) is
int speaker_event_push = 0; // tracks where writer (emulated software) is.

// just define odd indexes to be +, even indexes to be -

void dump_full_speaker_event_log() {
    FILE *f = fopen("speaker_event_log.txt", "w");
    for (int i = 0; i < speaker_event_push; i++) {
        fprintf(f, "%llu\n", speaker_event_log[i]);
    }
    fclose(f);
}

void dump_partial_speaker_event_log(uint64_t cycles_now) {
    while (speaker_event_pop != speaker_event_push) {
        uint64_t event = speaker_event_log[speaker_event_pop];
        printf("%llu\n", event);
        speaker_event_pop++;
        if (speaker_event_pop >= SPEAKER_EVENT_LOG_SIZE) {
            speaker_event_pop = 0;
        }
    }
}
#endif

uint8_t speaker_memory_read(cpu_state *cpu, uint16_t address) {
    log_speaker_blip(cpu);
    return 0xA0; // what does speaker read return?
}

void speaker_memory_write(cpu_state *cpu, uint16_t address, uint8_t value) {
    log_speaker_blip(cpu);
}

void init_speaker(cpu_state *cpu) {

    speaker_states[0].cpu = cpu;

	// Initialize SDL with video
	SDL_Init(SDL_INIT_AUDIO);
	
    SDL_AudioSpec desired = {};
    desired.freq = 44100;
    desired.format = AUDIO_S16;
    desired.channels = 1;
    desired.samples = 735;
    desired.callback = audio_callback;
    
    SDL_AudioSpec obtained;
    speaker_states[0].device_id = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);

    std::cout << "SDL_OpenAudioDevice returned: " << speaker_states[0].device_id << "\n";

    if ( speaker_states[0].device_id == 0 )
    {
        std::cerr << "Error opening audio device: " << SDL_GetError() << std::endl;
        return;
    }

	std::cout << "Obtained audio specs:\n";
    std::cout << "  Frequency: " << obtained.freq << " Hz\n";
	std::cout << "  Format: " << obtained.format << "\n";
	std::cout << "  Channels: " << (int)obtained.channels << "\n";
	std::cout << "  Silence: " << (int)obtained.silence << "\n";
	std::cout << "  Samples: " << obtained.callback << "\n";
    std::cout << "  size: " << obtained.size << "\n";
    std::cout << "  callback: " << obtained.callback << "\n";
	std::cout << "  userdata: " << obtained.userdata << "\n";

    if (DEBUG(DEBUG_SPEAKER)) fprintf(stdout, "init_speaker\n");
    register_C0xx_memory_read_handler(0xC030, speaker_memory_read);
    register_C0xx_memory_write_handler(0xC030, speaker_memory_write);
}

void speaker_start() {
    // Start audio playback
    SDL_PauseAudioDevice(speaker_states[0].device_id, 0);  // 0 means unpause
    speaker_states[0].device_started = 1;

}

void speaker_stop() {
    // Stop audio playback
    SDL_PauseAudioDevice(speaker_states[0].device_id, 1);  // 1 means pause
    speaker_states[0].device_started = 0;
}

static bool speaker_recording = false;

void toggle_speaker_recording()
{
    speaker_recording = !speaker_recording;
};

