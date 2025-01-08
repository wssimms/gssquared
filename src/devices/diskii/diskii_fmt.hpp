#pragma once

#include <stdint.h> 

/**
 * Implements a Disk II disk image nibblizer.
 * 
 * Converts a raw disk image, which is just a series of sectors in logical
 * order, into a nibblized disk image, which is a series of encoded tracks.
 * 
 * Currently this is read-only.
 */

/**
 * The key stuff for this comes from Pages 3-18 and 3-19 of Beneath Apple DOS.
 */


/**
 * This is a translation table to implement the 6-and-2 nibble encoding.
 * 
 */

// AA, D5 - Reserved Bytes, not included in the 6-and-2 encoding.

extern uint8_t translate_62[64];


/**
 * Interleave orders for various OS disk formats.
 * Our raw simulated disk data input will always be in logical order,
 * e.g. track 0 sector 0 is followed by track 0 sector 1 etc.
 * When constructing our in-memory simulated nibblized disk data,
 * we can lay it out in the interleave order.
 * That said, since there is no actual disk spinning underneath the emulator,
 * if we just put them in physical order, the disk head will magically already
 * be in the same place it was the last time RWTS was called. So, these interleave
 * orders are probably not good for performance, but, they would more accurately
 * simulate disk operation.
 */
typedef uint16_t interleave_t[0x10];

/**
 * Constants for the disk image.
 * Currently hard-coded. In the future we might want to make these configurable
 * per disk_t structure.
 */

const int TRACKS_PER_DISK = 35;
const int SECTORS_PER_TRACK = 16;
const int SECTOR_SIZE = 0x0100;
const int TRACK_MAX_SIZE = 0x1A00; // https://retrocomputing.stackexchange.com/questions/27691/apple-nibble-disk-format-specification

const int GAP_A_SIZE = 80;
const int GAP_B_SIZE = 10;
const int GAP_C_SIZE = 20;
const int ADDRESS_FIELD_SIZE = 15;
const int DATA_FIELD_SIZE = 350;

const int DEFAULT_VOLUME = 0xFE;

// one sector unencoded data

typedef uint8_t sector_t[SECTOR_SIZE];

// one sector encoded (nibblized) data without checksum.
typedef uint8_t sector_62_t[342];

// one sector encoded (nibblized) data with checksum. Only used
// for debugging.
typedef uint8_t sector_62_ondisk_t[343];

// nibblized Track data structure
typedef struct {
    uint16_t size;
    uint16_t position;
    uint8_t data[TRACK_MAX_SIZE];
} track_t;

// Nibblized Disk data structure
typedef struct {
    interleave_t interleave_phys_to_logical;
    interleave_t interleave_logical_to_phys;
    track_t tracks[35]; // They are numbered from 0 to 34, track 0 being the
                        // outer most track and track 34 being the inner most.
} disk_t;

// Disk data structure (raw, unencoded)
typedef struct {
    sector_t sectors[TRACKS_PER_DISK][SECTORS_PER_TRACK];
} disk_image_t;

extern interleave_t po_phys_to_logical;
extern interleave_t po_logical_to_phys;
extern interleave_t do_phys_to_logical;
extern interleave_t do_logical_to_phys;
extern interleave_t cpm_order;


void dump_disk_image(disk_image_t& disk_image);
int load_disk_image(disk_image_t& disk_image, const char *filename);
void emit_disk(disk_t& disk, disk_image_t& disk_image, int volume);
void write_disk(disk_t& disk, const char *filename);
void dump_disk(disk_t& disk);

