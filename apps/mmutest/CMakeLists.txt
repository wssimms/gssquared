add_executable(mmutest main.cpp)

target_link_libraries(mmutest PRIVATE
    gs2_util
    gs2_cpu
    gs2_clock
    gs2_debugger
    gs2_mmu
)