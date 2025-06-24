
#pragma once

#include <cstdint>
#include "mmus/mmu.hpp"
#include "computer.hpp"

typedef enum {
    VM_TEXT40 = 0,
    VM_TEXT80,
    VM_LORES,
    VM_DLORES,
    VM_HIRES,
    VM_DHIRES,
    VM_SHR320,
    VM_SHR640,
    VM_LORES_MIXED,
    VM_DLORES_MIXED,
    VM_HIRES_MIXED,
    VM_DHIRES_MIXED,
    VM_PALETTE_DATA,
    VM_BORDER_COLOR
} video_mode_t;

class VideoScannerII
{
protected:
    // LUTs for video addresses
    uint16_t lores_p1_addresses[65*262];
    uint16_t lores_p2_addresses[65*262];
    uint16_t hires_p1_addresses[65*262];
    uint16_t hires_p2_addresses[65*262];
    uint16_t mixed_p1_addresses[65*262];
    uint16_t mixed_p2_addresses[65*262];

    uint16_t (*video_addresses)[65*262];
    
    // video mode/memory data
    uint8_t   video_data[2*40*192];
    int       video_data_size;

    // floating bus video data
    uint8_t   video_byte;

    uint32_t  hcount;       // use separate hcount and vcount in order
    uint32_t  vcount;       // to simplify IIgs scanline interrupts

    bool      graf;
    bool      hires;
    bool      mixed;
    bool      page2;

    video_mode_t video_mode;

    MMU * mmu;

    virtual void set_video_mode();

public:
    VideoScannerII(MMU * mmu);

    virtual void video_cycle();
    virtual void init_video_addresses();

    inline int       get_video_data_size() { return video_data_size; }
    inline void      end_video_cycle()     { video_data_size = 0; }
    inline uint8_t   get_video_byte()      { return video_byte; }
    inline uint8_t * get_video_data()      { return video_data; }

    inline bool is_hbl()     { return hcount < 25; }
    inline bool is_vbl()     { return vcount >= 192; }

    inline void set_page_1() { page2 = false; set_video_mode(); }
    inline void set_page_2() { page2 = true;  set_video_mode(); }
    inline void set_full()   { mixed = false; set_video_mode(); }
    inline void set_mixed()  { mixed = true;  set_video_mode(); }
    inline void set_lores()  { hires = false; set_video_mode(); }
    inline void set_hires()  { hires = true;  set_video_mode(); }
    inline void set_text()   { graf  = false; set_video_mode(); }
    inline void set_graf()   { graf  = true;  set_video_mode(); }

    inline bool is_page_1() { return !page2; }
    inline bool is_page_2() { return page2; }
    inline bool is_full()   { return !mixed; }
    inline bool is_mixed()  { return mixed; }
    inline bool is_lores()  { return !hires; }
    inline bool is_hires()  { return hires; }
    inline bool is_text()   { return !graf; }
    inline bool is_graf()   { return graf; }
};

void init_mb_video_scanner(computer_t *computer, SlotType_t slot);
