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

#include <stdio.h>
#include <stdint.h>

#include "diskii_fmt.hpp"
#include "debug.hpp"


interleave_t po_phys_to_logical = {   // also Pascal Order.
    0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15
};

interleave_t po_logical_to_phys = {   // also Pascal Order.
    0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15
};

interleave_t do_phys_to_logical = {
    0, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 15
};

interleave_t do_logical_to_phys = {
    0x0, 0xD, 0xB, 0x9, 0x7, 0x5, 0x3, 0x1, 0xE, 0xC, 0xA, 0x8, 0x6, 0x4, 0x2, 0xF
};

interleave_t cpm_order = {  // to be complete.
    0, 10, 6, 1, 12, 7, 2, 13, 8, 3, 14, 9, 4, 15, 10, 5
};

/**
 * This is a translation table to implement the 6-and-2 nibble encoding.
 * 
 */

// AA, D5 - Reserved Bytes, not included in the 6-and-2 encoding.

extern uint8_t translate_62[64];
uint8_t translate_62[64] = {
    0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6, 0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,
    0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xCB, 0xCD, 0xCE, 0xCF, 0xD3,
    0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,
    0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF    
};

/**
 * **********************************************************************************************
 * Utility functions for debugging.
 * 'dump' dumps a set of bytes to the console in hexadecimal and ASCII.
 */

void dump_bytes(uint8_t *in, int len) {
    for (int i = 0; i < len; i+=0x10) {

        int bytes_to_print = (i + 16 <= len) ? 16 : (len - i);
        printf("%04x: ", i);

        for (int j = 0; j < bytes_to_print; j++) {
            printf("%02x ", in[i+j]);
        }
        for (int j = bytes_to_print; j < 16; j++) {
            printf("   ");
        }
        printf("  ");
        // Print ASCII representation
        for (int j = 0; j < bytes_to_print; j++) {
            if (in[i+j] >= 32 && in[i+j] <= 126) {
                printf("%c ", in[i+j]);
            } else {
                printf(". ");
            }
        }
        printf("\n");
    }
    printf("\n");
}

void dump_disk_image(disk_image_t& disk_image) {
    for (int t = 0; t < TRACKS_PER_DISK; t++) {
        for (int s = 0; s < SECTORS_PER_TRACK; s++) {
            printf("track %d sector %d\n", t, s);
            dump_bytes(disk_image.sectors[t][s], SECTOR_SIZE);
        }
    }
}

void dump_sector(sector_t in) {
    dump_bytes(in, SECTOR_SIZE);
}

void dump_sector_62(sector_62_t in) {
    dump_bytes(in, 0x156);
}

void dump_track(track_t& track) {
    dump_bytes(track.data, track.size);
}

void dump_disk(nibblized_disk_t& disk) {
    for (int t = 0; t < TRACKS_PER_DISK; t++) {
        printf("Encoded Track %d, position: %d\n", t, disk.tracks[t].position);
        dump_track(disk.tracks[t]);
    }
}


/** **********************************************************************************************
 * The Core of this is the nibblizer, which I shamelessly stole from Woz.
 * DOS 3.3 source code is in folder dos33source. the file PRENIBL contains this assembly routine:

nbuf1 = 0xBB00
nbuf2 = 0xBC00

This is the PRENIBLIZE routine written by Woz and others @ Apple.

 SBTL '16-SECTOR PRENIBLIZE'
****************************
*                          *
*    PRENIBLIZE SUBR       *
*   (16-SECTOR FORMAT)     *
*                          *
****************************
*                          *
*  CONVERTS 256 BYTES OF   *
*  USER DATA IN (BUF),0    *
*  TO (BUF),255 INTO 342   *
*  6-BIT NIBLS (00ABCDEF)  *
*  IN NBUF1 AND NBUF2.     *
*                          *
*    ---- ON ENTRY ----    *
*                          *
*  BUF IS 2-BYTE POINTER   *
*    TO 256 BYTES OF USER  *
*    DATA.                 *
*                          *
*    ---- ON EXIT -----    *
*                          *
*  A-REG UNCERTAIN.        *
*  X-REG HOLDS $FF.        *
*  Y-REG HOLDS $FF.        *
*  CARRY SET.              *
*                          *
*  NBUF1 AND NBUF2 CONTAIN *
*    6-BIT NIBLS OF FORM   *
*    00ABCDEF.             *
*                          *
****************************

PRENIB16    LDX #$0                    ;START NBUF2 INDEX. CHANGED BY WOZ 
            LDY #2                     ;START USER BUF INDEX. CHANGED BY WOZ. 
PRENIB1     DEY                        ; NEXT USER BYTE.
            LDA (BUF),Y
            LSR A                      ;SHIFT TWO BITS OF
            ROL NBUF2,X                ;CURRENT USER BYTE
            LSR A                      ;INTO CURRENT NBUF2
            ROL NBUF2,X                ;BYTE.
            STA NBUF1,Y                ;(6 BITS LEFT).
            INX                        ;FROM 0 TO $55.
            CPX #$56
            BCC PRENIB1                ;BR IF NO WRAPAROUND.
            LDX #0                     ;RESET NBUF2 INDEX.
            TYA                        ;USER BUF INDEX.
            BNE PRENIB1                ;(DONE IF ZERO)

            LDX #$55                   ;NBUF2 IDX $55 TO 0.
PRENIB2     LDA NBUF2,X
            AND #$3F                   ;STRIP EACH BYTE
            STA NBUF2,X                ;OF NBUF2 TO 6 BITS.
            DEX                        ;
            BPL PRENIB2                 ;LOOP UNTIL X NEG.
            RTS                        ;RETURN.
*/

/** **********************************************************************************************
 * Emits a single byte to the track's data stream.
 * Keeps track of the position in the track we are in, and,
 * the size, increasing as necessary as we add bytes.
 * If the position or size exceed the track's max size, we print an error and bail.
 */
void emit_track_byte(track_t& track, uint8_t byte) {
    if (track.position >= TRACK_MAX_SIZE) {
        printf("track is full\n");
        return;
    }
    if (track.position >= track.size) {
        track.size = track.position + 1;
    }
    track.data[track.position++] = byte;
}

/**
 * Gap 1
 * "is the first data written to a track during initialization. The gap originally
 * consists of 128 bytes of self-sync (0xFF), a large enough area to insure that all portions
 * of a track will contain data."
 * This was to handle slight drive speed variations.
 * We don't have that, so we just take the average gap size from Beneath Apple DOS here.
 */
void emit_gap_a(track_t& track) {
    for (int i = 0 ; i < GAP_A_SIZE; i++) {
        emit_track_byte(track, 0xFF);
    }
}

/**
 * Gap 2
 * "appears after each Address Field and before each Data Field.
 * its length varies from five to ten bytes on a normal drive".
 * If the gap is too short, the disk might spin past the data field
 * while the OS is decoding the address field.
 * We take the average gap size from Beneath Apple DOS here.
 */
void emit_gap_b(track_t& track) {
    for (int i = 0 ; i < GAP_B_SIZE; i++) {
        emit_track_byte(track, 0xFF);
    }
}

/**
 * Gap 3
 * "appears after each Data Field and before each Address field. It is longer than Gap 2 and ranges from 14
 * to 24 bytes in length. Its purpose is similar to Gap 2".
 * We take the average gap size from Beneath Apple DOS here.
 */
void emit_gap_c(track_t& track) {
    for (int i = 0 ; i < GAP_C_SIZE; i++) {
        emit_track_byte(track, 0xFF);
    }
}

/**
 * Gap 4
 * fill out the rest of the track with 0xFF
 */
void emit_gap_d(track_t& track) {
    while (track.position < TRACK_MAX_SIZE) {
        emit_track_byte(track, 0xFF);
    }
}

/**
 * Encodes a byte as two 4-4 encoded bytes. Volume, track, sector, and checksum in the address field
 * are encoded this way.
 */
void emit_encoded_44(track_t& track, uint8_t value) {
    uint8_t XX = ((value & 0b10101010)) >> 1 | 0b10101010;
    uint8_t YY = ((value & 0b01010101)) | 0b10101010;
    emit_track_byte(track, XX);
    emit_track_byte(track, YY);
}

void emit_address_prologue(track_t& track) {
    emit_track_byte(track, 0xD5);
    emit_track_byte(track, 0xAA);
    emit_track_byte(track, 0x96);
}

void emit_address_epilogue(track_t& track) {
    emit_track_byte(track, 0xDE);
    emit_track_byte(track, 0xAA);
    emit_track_byte(track, 0xEB);
}

void emit_data_prologue(track_t& track) {
    emit_track_byte(track, 0xD5);
    emit_track_byte(track, 0xAA);
    emit_track_byte(track, 0xAD);
}

void emit_data_epilogue(track_t& track) {
    emit_track_byte(track, 0xDE);
    emit_track_byte(track, 0xAA);
    emit_track_byte(track, 0xEB);
}

void emit_address_field(track_t& tr, uint8_t volume, uint8_t track, uint8_t sector) {
    emit_address_prologue(tr);

    uint8_t checksum = (volume ^ track) ^ sector;
    emit_encoded_44(tr, volume);
    emit_encoded_44(tr, track);
    emit_encoded_44(tr, sector);
    emit_encoded_44(tr, checksum);
    
    emit_address_epilogue(tr);
}


void load_sector(sector_t& sect, const char *filename) {

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("Could not open %s\n", filename);
        return;
    }

    if (fread(sect, 1, SECTOR_SIZE, fp) != SECTOR_SIZE) {
        printf("Could not read %d bytes from %s\n", SECTOR_SIZE, filename);
        fclose(fp);
        return;
    }

    fclose(fp);
}

int load_disk_image(disk_image_t& disk_image, const char *filename) {

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("Could not open %s\n", filename);
        return -1;
    }

    int sect_index = 0;
    for (int t = 0; t < TRACKS_PER_DISK; t++) {
        for (int s = 0; s < SECTORS_PER_TRACK; s++) {
            if (fread(disk_image.sectors[t][s], 1, SECTOR_SIZE, fp) != SECTOR_SIZE) {
                printf("Could not read %d bytes from %s\n", SECTOR_SIZE, filename);
                fclose(fp);
                return -1;
            }
        }
    }

    fclose(fp);
    return 0;
}

int load_nib_image(nibblized_disk_t& disk, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("Could not open %s\n", filename);
        return -1;
    }

    for (int t = 0; t < TRACKS_PER_DISK; t++) {
        fread(disk.tracks[t].data, 1, TRACK_MAX_SIZE, fp);
    }

    fclose(fp);
    return 0;
}

void write_sector_62(sector_62_t& s62, const char *filename) {
    FILE *out_fp = fopen(filename, "wb");
    if (!out_fp) {
        printf("Could not open %s for writing\n", filename);
        return;
    }

    // Write the encoded sector data
    for (int i = 0; i < sizeof(sector_62_t); i++) {
        fputc(s62[i], out_fp);
    }

    fclose(out_fp);
}



void prenibble(sector_t& buf, sector_62_t& nbuf) {
    uint8_t *nbuf1 = nbuf;
    uint8_t *nbuf2 = nbuf + 0x100;

    int c,x,a;
    uint8_t y;
  
  //printf("prenibble\n");
  prenib16:
        x = 0;
        y = 2;

  prenib1:
        //printf("x,y=%02X,%02X\n", x, y);
        y--;
        a = buf[y];
        
        c = a & 0b01;               // LSR A
        a >>=1;
        nbuf2[x] = (nbuf2[x] << 1) | c; // ROL NBUF2,X

        c = a & 0b01;               // LSR A
        a >>=1;
        nbuf2[x] = (nbuf2[x] << 1) | c; // ROL NBUF2,X

        nbuf1[y] = a;              // STA NBUF1,Y
        x++;
        if (x < 0x56) goto prenib1;
        x = 0;
        if (y != 0) goto prenib1;

    // strip just the 2nd buffer to 6 bits.
    for (x = 0x55; x >= 0; x--) {
        nbuf2[x] &= 0x3F;
    }
}

/**
 * Takes a raw sector, and nibblizes into the track's data stream.
 * Does not emit the prologue etc.
 */

void emit_nibblized_sector(track_t& track, sector_t& in) {
    sector_62_t nbuf; // don't need to preinitialize, woz prenibble does it.

    prenibble(in, nbuf);
    if (DEBUG(DEBUG_DISKII_FORMAT)) {
        printf("raw buffer out from PRENIBBLE\n");
        dump_sector_62(nbuf);
    }

    // don't need to reorder now, just emit to track.
    uint8_t last = 0;
    for (int i = 0x0155; i >= 0x0100; i--) {
        emit_track_byte(track, translate_62[nbuf[i] ^ last]);
        last = nbuf[i];
    }
    for (int i = 0x00; i <= 0xFF; i++) {
        emit_track_byte(track, translate_62[nbuf[i] ^ last]);
        last = nbuf[i];
    }
    emit_track_byte(track, translate_62[nbuf[0xFF]]);
}

/**
 * Emit a data field, which is a nibblized sector, with prologue and epilogue.
 * input: sector_t in, which is the raw sector data.
 * output: streams the nibblized data field with prologue and epilogue to the track's data stream.
 */
void emit_data_field(track_t& track, sector_t& in) {
    emit_data_prologue(track);

    emit_nibblized_sector(track, in);

    emit_data_epilogue(track);
}

/**
 * Emit a sector, which is an address field, a gap, a data field, and a gap.
 * input: sector_t in, which is the raw sector data.
 * output: streams the nibblized sector with address, data, and gap to the track's data stream.
 */
void emit_sector(track_t& tr, sector_t& in, uint8_t volume, uint8_t track, uint8_t sector) {
    // each sector after gap A is address, gap2, data, gap3
    if (DEBUG(DEBUG_DISKII)) printf("Emitting track %d, sector %d\n", track, sector);
    emit_address_field(tr, volume, track, sector);
    emit_gap_b(tr);
    emit_data_field(tr, in);
    emit_gap_c(tr);
    if (DEBUG(DEBUG_DISKII)) printf("Track position: %d\n", tr.position);
}

/**
 * Emit a track, which is a gap_a, followed by a series of sectors.
 * input: disk_image_t disk_image, which is the disk raw image data, and the track number to emit.
 * output: streams the nibblized track with gap_a, sectors to the track's data stream.
 */
void emit_track(nibblized_disk_t& disk, disk_image_t& disk_image, int volume, int track) {
    emit_gap_a(disk.tracks[track]); // gap A is only at beginning of track.
    for (int s = 0; s < SECTORS_PER_TRACK; s++) {
        // we must map the logical sector number (.do format) to the physical sector number (.nib format)
        // right now this loops in logical order.
        //emit_sector(disk.tracks[track], (sector_t&)disk_image.sectors[track][s], volume, track, disk.interleave_logical_to_phys[s]);
        // loop in physical order.
        emit_sector(disk.tracks[track], (sector_t&)disk_image.sectors[track][disk.interleave_phys_to_logical[s]], volume, track, s);
    }
    emit_gap_d(disk.tracks[track]);
}

/**
 * Emit an entire disk, which is an array of tracks.
 * input: disk_t disk, which is the nibblized disk data structure.
 * output: streams the nibblized disk to the track's data stream.
 */
void emit_disk(nibblized_disk_t& disk, disk_image_t& disk_image, int volume) {
    printf("Emitting entire disk...\n");
    for (int t = 0; t < TRACKS_PER_DISK; t++) {
        emit_track(disk, disk_image, volume, t);
    }
}

void write_nibblized_disk(nibblized_disk_t& disk, const char *filename) {
    FILE *out_fp = fopen(filename, "wb");
    if (!out_fp) {
        printf("Could not open %s for writing\n", filename);
        return;
    }

    for (int t = 0; t < TRACKS_PER_DISK; t++) {
        fwrite(disk.tracks[t].data, sizeof(uint8_t), TRACK_MAX_SIZE, out_fp);
    }

    fclose(out_fp);
}

/**
 * when handling writes, we will need to decode the nibbles back to bytes.
 * particularly, the address field, so then we know which disk_image track and sector
 * to write the data back to.
 */