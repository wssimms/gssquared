add_executable(cputest main.cpp)

target_link_libraries(cputest PRIVATE
    gs2_mmu
    gs2_util
    gs2_cpu
    gs2_clock
    gs2_cpu 
    gs2_debugger
)