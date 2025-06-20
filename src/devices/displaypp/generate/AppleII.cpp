#include <cstdint>
#include <cstring>
#include "devices/displaypp/frame/frame.hpp"
#include "devices/displaypp/CharRom.hpp"

alignas(64) uint16_t textMap[24] =
  {   // text page 1 line addresses
            0x0000,
            0x0080,
            0x0100,
            0x0180,
            0x0200,
            0x0280,
            0x0300,
            0x0380,

            0x0028,
            0x00A8,
            0x0128,
            0x01A8,
            0x0228,
            0x02A8,
            0x0328,
            0x03A8,

            0x0050,
            0x00D0,
            0x0150,
            0x01D0,
            0x0250,
            0x02D0,
            0x0350,
            0x03D0,
        };

using Frame560 = Frame<uint8_t, 192, 560>;

class AppleII_Display {

private: 
    //uint8_t char_rom[4096];
    CharRom &char_rom;
    bool flash_state = false;
    bool alt_char_set = false;
    uint16_t altbase = 0x0000;

public:
    AppleII_Display(CharRom &char_rom) : char_rom(char_rom) { }

    void set_char_set(bool alt_char_set) {
        this->alt_char_set = alt_char_set;
        this->altbase = alt_char_set ? 2048 : 0;
    }

/** Call with: pointer to text memory; pointer to frame; linegroup number */
    void generate_text40(uint8_t *textpage, Frame560 *f, uint16_t linegroup) {
        uint16_t scanline = linegroup * 8;
        uint16_t x = 0;
        bool pixel_on = 1;
        bool pixel_off = 0;

        for (int y = 0; y < 8; y++) {
            uint16_t char_addr = textMap[linegroup];
            f->set_line(scanline);
            
            for (x = 0; x <= 39; x++) {
                uint8_t tchar = textpage[char_addr];

                if ((tchar & 0xC0) == 0) {
                    pixel_on = 0;
                    pixel_off = 1;
                } else if (((tchar & 0xC0) == 0x40)) {
                     pixel_on = flash_state;
                     pixel_off = !flash_state;
                }

                uint8_t cdata = char_rom.get_char_scanline(tchar, y + altbase);

                f->push(cdata & 1 ? pixel_on : pixel_off); 
                f->push(cdata & 1 ? pixel_on : pixel_off); 
                cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); 
                f->push(cdata & 1 ? pixel_on : pixel_off); 
                cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); 
                f->push(cdata & 1 ? pixel_on : pixel_off); 
                cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); 
                f->push(cdata & 1 ? pixel_on : pixel_off); 
                cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); 
                f->push(cdata & 1 ? pixel_on : pixel_off); 
                cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); 
                f->push(cdata & 1 ? pixel_on : pixel_off); 
                cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); 
                f->push(cdata & 1 ? pixel_on : pixel_off); 

                char_addr++;
            }
            scanline++;
        }
    }

void generate_text80(uint8_t *textpage, uint8_t *alttextpage, Frame560 *f, uint16_t linegroup) {
        uint16_t scanline = linegroup * 8;
        uint16_t x = 0;
        bool pixel_on = 1;
        bool pixel_off = 0;

        for (int y = 0; y < 8; y++) {
            uint16_t char_addr = textMap[linegroup];
            f->set_line(scanline);
            
            for (x = 0; x <= 39; x++) {
                uint8_t tchar = alttextpage[char_addr];

                uint8_t cdata = char_rom.get_char_scanline(tchar, y + altbase);

                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;

                tchar = textpage[char_addr];
                cdata = char_rom.get_char_scanline(tchar, y + altbase);

                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;
                f->push(cdata & 1 ? pixel_on : pixel_off); cdata>>=1;

                char_addr++;
            }
            scanline++;
        }
    }

    void generate_lores40(uint8_t *textpage, Frame560 *f, uint16_t linegroup) {
        uint16_t scanline = linegroup * 8;
        uint16_t x = 0;
        bool pixel_on = 1;
        bool pixel_off = 0;

        for (int y = 0; y < 8; y++) {
            uint16_t char_addr = textMap[linegroup];
            f->set_line(scanline);
            
            for (x = 0; x <= 39; x++) {
                uint8_t tchar = textpage[char_addr];
                
                if (y & 4) { // if we're in the second half of the scanline, shift the byte right 4 bits to get the other nibble
                    tchar = tchar >> 4;
                }
                uint16_t pixeloff = (x * 14) % 4;

                for (int bits = 0; bits < 14; bits++) {
                    uint8_t bit = ((tchar >> pixeloff) & 0x01);
                    f->push(bit);
                    pixeloff = (pixeloff + 1) % 4;
                }
                char_addr++;
            }
            scanline++;
        }
    }
    
    // TODO: this is just a guess right now.
    // Will need to start the phase 180 degrees off since we start 'early' in alt?
    void generate_lores80(uint8_t *textpage, uint8_t *alttextpage, Frame560 *f, uint16_t linegroup) {
        uint16_t scanline = linegroup * 8;
        //uint16_t x = 0;
        bool pixel_on = 1;
        bool pixel_off = 0;

        for (int y = 0; y < 8; y++) {
            uint16_t char_addr = textMap[linegroup];
            f->set_line(scanline);
            
            for (uint16_t x = 0; x <= 39; x++) {
                uint8_t tchar = alttextpage[char_addr];
                
                if (y & 4) { // if we're in the second half of the scanline, shift the byte right 4 bits to get the other nibble
                    tchar = tchar >> 4;
                }
                uint16_t pixeloff = (x * 14) % 4;

                for (uint16_t bits = 0; bits < 7; bits++) {
                    uint8_t bit = ((tchar >> pixeloff) & 0x01);
                    f->push(bit);
                    pixeloff = (pixeloff + 1) % 4;
                }
                
                tchar = textpage[char_addr];
                
                if (y & 4) { // if we're in the second half of the scanline, shift the byte right 4 bits to get the other nibble
                    tchar = tchar >> 4;
                }
                // TODO: we might need to continue pixeloff from the previous loop rather than reset it here?
                //pixeloff = (x * 14) % 4;

                for (uint16_t bits = 0; bits < 7; bits++) {
                    uint8_t bit = ((tchar >> pixeloff) & 0x01);
                    f->push(bit);
                    pixeloff = (pixeloff + 1) % 4;
                }

                char_addr++;
            }
            scanline++;
        }
    }
};
