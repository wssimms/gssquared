add_executable(gstrace main.cpp)

target_link_libraries(gstrace PRIVATE
    gs2_debugger
)