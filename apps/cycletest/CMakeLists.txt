add_executable(cycletest main.cpp)

target_link_libraries(cycletest PRIVATE
    gs2_mmu
    gs2_cpu
    gs2_clock
    #gs2_util
    gs2_debugger
    #gs2_oldmmu
)