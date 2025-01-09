
#include <iostream>
#include "util/media.hpp"

int main(int argc, char *argv[]) {
    media_descriptor md;
    
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    
    md.filename = argv[1];
    if (identify_media(md) != 0) {
        std::cerr << "Failed to identify media: " << md.filename << std::endl;
        return 1;
    }
    display_media_descriptor(md);
    return 0;
}