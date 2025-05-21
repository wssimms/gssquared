add_executable(trace main.cpp)

target_link_libraries(trace PRIVATE
    gs2_debugger
)