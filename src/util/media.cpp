/* Functions to handle setting up media descriptors */
#include <iostream>
#include <sys/stat.h>

#include "gs2.hpp"
#include "cpu.hpp"
#include "media.hpp"
#include "debug.hpp"

/**
 * First goal:
 * generic routine. Given a filename, create a media descriptor struct that informs
 * device drivers how to access the media.
 */

/**
 * Identify file type. Given a filename, inspect the file and return a media_descriptor
 * 
 * The filename is the only thing that is passed in in the md.
 */

int compare_suffix(const char *filename, const char *suffix) {
    int i = strlen(filename);
    int j = strlen(suffix);
    if (i < (j+1)) return 0;
    //printf("Comparing %s with %s\n", filename + i - j, suffix);
    return strncasecmp(filename + i - j, suffix, j) == 0;
}

int get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) != 0) return -1;
    return st.st_size;
}

static inline uint32_t le32_to_cpu(const uint8_t *bytes) {
    return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

static inline uint16_t le16_to_cpu(const uint8_t *bytes) {
    return bytes[0] | (bytes[1] << 8);
}

int read_2mg_header(format_2mg_t &hdr_out, const char *filename) {
    format_2mg_raw_t raw;
    
    FILE* fp = fopen(filename, "rb");
    if (!fp) return -1;
    
    if (fread(&raw, sizeof(format_2mg_raw_t), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }
    
    // Verify magic number "2IMG"
    if (memcmp(raw.id, "2IMG", 4) != 0) {
        return -1;
    }
    
    // Copy fixed-size fields
    memcpy(hdr_out.id, raw.id, 4);
    hdr_out.id[4] = 0;          // null terminate
    memcpy(hdr_out.creator, raw.creator, 4);
    hdr_out.creator[4] = 0;     // null terminate
    
    // Convert little-endian fields
    hdr_out.header_size = le16_to_cpu(raw.header_size);
    hdr_out.version = le16_to_cpu(raw.version);
    hdr_out.flag = le32_to_cpu(raw.image_format);
    hdr_out.block_count = le32_to_cpu(raw.block_count);
    hdr_out.bytes_count = le32_to_cpu(raw.bytes_count);
    hdr_out.comment_offset = le32_to_cpu(raw.comment_offset);
    hdr_out.comment_length = le32_to_cpu(raw.comment_length);
    hdr_out.creator_data = le32_to_cpu(raw.creator_data);
    hdr_out.creator_data_length = le32_to_cpu(raw.creator_data_length);

    if (hdr_out.comment_offset != 0 && hdr_out.comment_length > 0) {
        fseek(fp, hdr_out.comment_offset, SEEK_SET);
        hdr_out.comment_content = new char[hdr_out.comment_length + 1];
        fread(hdr_out.comment_content, 1, hdr_out.comment_length, fp);
        hdr_out.comment_content[hdr_out.comment_length] = 0;
    } else hdr_out.comment_content = nullptr;

    if (hdr_out.creator_data != 0 && hdr_out.creator_data_length > 0) {
        fseek(fp, hdr_out.creator_data, SEEK_SET);
        hdr_out.creator_data_content = new char[hdr_out.creator_data_length + 1];
        fread(hdr_out.creator_data_content, 1, hdr_out.creator_data_length, fp);
        hdr_out.creator_data_content[hdr_out.creator_data_length] = 0;
    } else hdr_out.creator_data_content = nullptr;

    fclose(fp);

    return 0;
}

format_2mg_creator_data_t format_2mg_creator_list[] = {
    {"!nfc", "Asimov"},
    {"B2TR", "Bernie ][ the Rescue"},
    {"CTKG", "Catakig"},
    {"ShIm", "Sheppy's ImageMaker"},
    {"WOOF", "Sweet 16"},
    {"XGS!", "XGS"},
    {"CdrP", "CiderPress"}
};

const int num_2mg_creators = (sizeof(format_2mg_creator_list) / sizeof(format_2mg_creator_data_t));

const char *get_2mg_creator_name(char *creator_id) {
    for (int i = 0; i < num_2mg_creators; i++) {
        if (strncmp(format_2mg_creator_list[i].creator, creator_id, 4) == 0) {
            return format_2mg_creator_list[i].name;
        }
    }
    return "(Unknwon)";
}


const char * get_media_type_name(media_type_t media_type) {
    switch (media_type) {
        case MEDIA_NYBBLE: return "NYBBLE";
        case MEDIA_PRENYBBLE: return "PRE-NYBBLE";
        case MEDIA_BLK: return "BLOCK";
        default: return "UNKNOWN";
    }
}

const char * get_interleave_name(interleave_t interleave) {
    switch (interleave) {
        case INTERLEAVE_NONE: return "NONE";
        case INTERLEAVE_DO: return "DOS";
        case INTERLEAVE_PO: return "PRODOS";
        case INTERLEAVE_CPM: return "CP/M";
        default: return "UNKNOWN";
    }
}

int display_media_descriptor(media_descriptor& md) {
    std::cout << "<> Media Descriptor: " << md.filename << std::endl;
    std::cout << "  Media Type: " << get_media_type_name(md.media_type) << std::endl;
    std::cout << "  Interleave: " << get_interleave_name(md.interleave) << std::endl;
    std::cout << "  Block Size: " << md.block_size << std::endl;
    std::cout << "  Block Count: " << md.block_count << std::endl;
    std::cout << "  File Size: " << md.file_size << std::endl;
    std::cout << "  Data Offset: " << md.data_offset << std::endl;

    return 0;
}

int display_2mg_header(format_2mg_t& hdr) {
    std::cout << "<> 2MG Header: " << std::endl;
    std::cout << "  ID: " << hdr.id << std::endl;
    std::cout << "  Creator: " << hdr.creator << " (" << get_2mg_creator_name(hdr.creator) << ")" << std::endl;
    std::cout << "  Header Size: " << hdr.header_size << std::endl;
    std::cout << "  Version: " << hdr.version << std::endl;
    std::cout << "  Image Format: " << hdr.flag << std::endl;
    std::cout << "  Block Count: " << hdr.block_count << std::endl;
    std::cout << "  Bytes Count: " << hdr.bytes_count << std::endl;
    std::cout << "  Comment Offset: " << hdr.comment_offset << std::endl;
    std::cout << "  Comment Length: " << hdr.comment_length << std::endl;
    if (hdr.comment_content && hdr.comment_length > 0) {
        std::cout << "  Comment Content: " << std::endl << "---" << std::endl;
        std::cout << hdr.comment_content;
        std::cout << std::endl << "---" << std::endl;
    }
    std::cout << "  Creator Data: " << hdr.creator_data << std::endl;
    std::cout << "  Creator Data Length: " << hdr.creator_data_length << std::endl;
    if (hdr.creator_data_content && hdr.creator_data_length > 0) {
        std::cout << "  Creator Data Content: " << std::endl << "---" << std::endl;
        std::cout << hdr.creator_data_content;
        std::cout << std::endl << "---" << std::endl;
    }
    return 0;
}

int identify_media(media_descriptor& md) {
    if (compare_suffix(md.filename, ".2mg")) {
        md.media_type = MEDIA_BLK;
        format_2mg_t hdr;
        if (read_2mg_header(hdr, md.filename) != 0) {
            std::cerr << "Failed to read 2MG header: " << md.filename << std::endl;
            return -1;
        }
        display_2mg_header(hdr);
        md.file_size = hdr.bytes_count;
        md.block_count = hdr.block_count;
        md.block_size = hdr.bytes_count / hdr.block_count;
        md.interleave = INTERLEAVE_NONE;
        md.data_offset = hdr.header_size;
    } else if (compare_suffix(md.filename, ".hdv")) {
        md.media_type = MEDIA_BLK;
        // get size of file on disk
        md.file_size = get_file_size(md.filename);
        md.block_size = 256;
        md.block_count = md.file_size / md.block_size;
        md.interleave = INTERLEAVE_NONE;
        md.data_offset = 0;
    } else if (compare_suffix(md.filename, ".do") || compare_suffix(md.filename, ".dsk")) {
        md.media_type = MEDIA_NYBBLE;
        // if file size is not 143K, then error.
        md.file_size = get_file_size(md.filename);
        if (md.file_size != 560 * 256) {
            std::cerr << "File size is not 143K: " << md.filename << std::endl;
            return -1;
        }
        md.block_size = 256;
        md.block_count = md.file_size / md.block_size;
        md.interleave = INTERLEAVE_DO;
        md.data_offset = 0;
    } else if (compare_suffix(md.filename, ".po")) {
        md.media_type = MEDIA_NYBBLE;
                // if file size is not 143K, then error.
        md.file_size = get_file_size(md.filename);
        if (md.file_size != 560 * 256) {
            std::cerr << "File size is not 143K: " << md.filename << std::endl;
            return -1;
        }
        md.block_size = 256;
        md.block_count = md.file_size / md.block_size;
        md.interleave = INTERLEAVE_PO;
        md.data_offset = 0;
    } else if (compare_suffix(md.filename, ".nib")) {
        md.media_type = MEDIA_PRENYBBLE;
        md.file_size = get_file_size(md.filename);
        md.block_size = 256;
        md.block_count = 560; // assumed 560 sectors on a 143K diskette.
        md.interleave = INTERLEAVE_NONE;
        md.data_offset = 0;
    } else {
        std::cerr << "Unknown media type: " << md.filename << std::endl;
        return -1;
    }
    return 0;
}

