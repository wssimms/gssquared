add_executable(speaker main.cpp)

# Link SDL3 and required frameworks
target_link_libraries(speaker PRIVATE 
    SDL3::SDL3-shared
    SDL3_image::SDL3_image-shared
)
target_link_libraries(speaker PRIVATE
    gs2_devices_speaker
    gs2_util
    gs2_mmu
)
