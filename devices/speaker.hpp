#pragma once

void init_speaker(cpu_state *cpu);
void toggle_speaker_recording();
void dump_full_speaker_event_log();
void dump_partial_speaker_event_log(uint64_t cycles_now);
void speaker_start();
void speaker_stop();
