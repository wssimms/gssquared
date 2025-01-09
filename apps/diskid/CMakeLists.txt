add_executable(diskid main.cpp)

target_link_libraries(diskid PRIVATE
    gs2_util
)
