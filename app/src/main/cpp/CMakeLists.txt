cmake_minimum_required(VERSION 3.22.1)
project(zjmud_android LANGUAGES C CXX)

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
set(FLUFFOS_ROOT "${REPO_ROOT}/native/fluffos/src")
set(LIBEVENT_ROOT "${REPO_ROOT}/native/third_party/libevent")
set(MUSL_CRYPT_ROOT "${REPO_ROOT}/native/third_party/musl-crypt")

set(EVENT__LIBRARY_TYPE STATIC CACHE STRING "" FORCE)
set(EVENT__DISABLE_BENCHMARK ON CACHE BOOL "" FORCE)
set(EVENT__DISABLE_MBEDTLS ON CACHE BOOL "" FORCE)
set(EVENT__DISABLE_OPENSSL ON CACHE BOOL "" FORCE)
set(EVENT__DISABLE_REGRESS ON CACHE BOOL "" FORCE)
set(EVENT__DISABLE_SAMPLES ON CACHE BOOL "" FORCE)
set(EVENT__DISABLE_TESTS ON CACHE BOOL "" FORCE)
set(EVENT__DISABLE_THREAD_SUPPORT ON CACHE BOOL "" FORCE)
add_subdirectory("${LIBEVENT_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/libevent" EXCLUDE_FROM_ALL)

if(ANDROID)
    target_compile_definitions(event_core_static PRIVATE _GNU_SOURCE)
    target_compile_definitions(event_extra_static PRIVATE _GNU_SOURCE)
    target_compile_definitions(event_static PRIVATE _GNU_SOURCE)
endif()

set(FLUFFOS_SOURCES
    ${MUSL_CRYPT_ROOT}/crypt_des.c
    ${MUSL_CRYPT_ROOT}/crypt_md5.c
    ${FLUFFOS_ROOT}/backend.cc
    ${FLUFFOS_ROOT}/comm.cc
    ${FLUFFOS_ROOT}/fliconv.cc
    ${FLUFFOS_ROOT}/main.cc
    ${FLUFFOS_ROOT}/mainlib.cc
    ${FLUFFOS_ROOT}/user.cc
    ${FLUFFOS_ROOT}/net/telnet.cc
    ${FLUFFOS_ROOT}/thirdparty/libtelnet/libtelnet.c

    ${FLUFFOS_ROOT}/base/internal/debugmalloc.cc
    ${FLUFFOS_ROOT}/base/internal/external_port.cc
    ${FLUFFOS_ROOT}/base/internal/file.cc
    ${FLUFFOS_ROOT}/base/internal/hash.cc
    ${FLUFFOS_ROOT}/base/internal/log.cc
    ${FLUFFOS_ROOT}/base/internal/md.cc
    ${FLUFFOS_ROOT}/base/internal/outbuf.cc
    ${FLUFFOS_ROOT}/base/internal/port.cc
    ${FLUFFOS_ROOT}/base/internal/rc.cc
    ${FLUFFOS_ROOT}/base/internal/stats.cc
    ${FLUFFOS_ROOT}/base/internal/stralloc.cc
    ${FLUFFOS_ROOT}/base/internal/strput.cc

    ${FLUFFOS_ROOT}/vm/internal/applies_table.autogen.cc
    ${FLUFFOS_ROOT}/vm/internal/efuns.autogen.cc
    ${FLUFFOS_ROOT}/vm/internal/apply.cc
    ${FLUFFOS_ROOT}/vm/internal/eval_limit.cc
    ${FLUFFOS_ROOT}/vm/internal/master.cc
    ${FLUFFOS_ROOT}/vm/internal/otable.cc
    ${FLUFFOS_ROOT}/vm/internal/simul_efun.cc
    ${FLUFFOS_ROOT}/vm/internal/simulate.cc
    ${FLUFFOS_ROOT}/vm/internal/trace.cc
    ${FLUFFOS_ROOT}/vm/internal/vm.cc

    ${FLUFFOS_ROOT}/vm/internal/base/apply_cache.cc
    ${FLUFFOS_ROOT}/vm/internal/base/array.cc
    ${FLUFFOS_ROOT}/vm/internal/base/buffer.cc
    ${FLUFFOS_ROOT}/vm/internal/base/class.cc
    ${FLUFFOS_ROOT}/vm/internal/base/function.cc
    ${FLUFFOS_ROOT}/vm/internal/base/interpret.cc
    ${FLUFFOS_ROOT}/vm/internal/base/mapping.cc
    ${FLUFFOS_ROOT}/vm/internal/base/object.cc
    ${FLUFFOS_ROOT}/vm/internal/base/program.cc
    ${FLUFFOS_ROOT}/vm/internal/base/svalue.cc

    ${FLUFFOS_ROOT}/vm/internal/compiler/compiler.cc
    ${FLUFFOS_ROOT}/vm/internal/compiler/icode.cc
    ${FLUFFOS_ROOT}/vm/internal/compiler/generate.cc
    ${FLUFFOS_ROOT}/vm/internal/compiler/grammar.autogen.cc
    ${FLUFFOS_ROOT}/vm/internal/compiler/lex.cc
    ${FLUFFOS_ROOT}/vm/internal/compiler/scratchpad.cc
    ${FLUFFOS_ROOT}/vm/internal/compiler/trees.cc

    ${FLUFFOS_ROOT}/packages/contrib/contrib.cc
    ${FLUFFOS_ROOT}/packages/core/add_action.cc
    ${FLUFFOS_ROOT}/packages/core/call_out.cc
    ${FLUFFOS_ROOT}/packages/core/crc32.cc
    ${FLUFFOS_ROOT}/packages/core/custom_crypt.cc
    ${FLUFFOS_ROOT}/packages/core/debug.cc
    ${FLUFFOS_ROOT}/packages/core/dns.cc
    ${FLUFFOS_ROOT}/packages/core/dumpstat.cc
    ${FLUFFOS_ROOT}/packages/core/ed.cc
    ${FLUFFOS_ROOT}/packages/core/efuns_main.cc
    ${FLUFFOS_ROOT}/packages/core/file.cc
    ${FLUFFOS_ROOT}/packages/core/heartbeat.cc
    ${FLUFFOS_ROOT}/packages/core/mssp.cc
    ${FLUFFOS_ROOT}/packages/core/outbuf.cc
    ${FLUFFOS_ROOT}/packages/core/rc.cc
    ${FLUFFOS_ROOT}/packages/core/reclaim.cc
    ${FLUFFOS_ROOT}/packages/core/regexp.cc
    ${FLUFFOS_ROOT}/packages/core/replace_program.cc
    ${FLUFFOS_ROOT}/packages/core/save.cc
    ${FLUFFOS_ROOT}/packages/core/sprintf.cc
    ${FLUFFOS_ROOT}/packages/core/telnet_ext.cc
    ${FLUFFOS_ROOT}/packages/develop/checkmemory.cc
    ${FLUFFOS_ROOT}/packages/develop/develop.cc
    ${FLUFFOS_ROOT}/packages/develop/disassembler.cc
    ${FLUFFOS_ROOT}/packages/math/math.cc
    ${FLUFFOS_ROOT}/packages/mudlib_stats/mudlib_stats.cc
    ${FLUFFOS_ROOT}/packages/ops/ops.cc
    ${FLUFFOS_ROOT}/packages/ops/parse.cc
    ${FLUFFOS_ROOT}/packages/trim/trim.cc
    ${FLUFFOS_ROOT}/packages/uids/uids.cc
)

set_source_files_properties(${FLUFFOS_ROOT}/main.cc PROPERTIES COMPILE_DEFINITIONS "main=fluffos_main")
set_source_files_properties(
    ${FLUFFOS_ROOT}/thirdparty/libtelnet/libtelnet.c
    PROPERTIES LANGUAGE CXX COMPILE_DEFINITIONS HAVE_ZLIB
)

add_library(zjmud_native SHARED native_bridge.cpp ${FLUFFOS_SOURCES})

target_compile_features(zjmud_native PRIVATE cxx_std_17)
target_compile_definitions(zjmud_native PRIVATE _GNU_SOURCE)
target_compile_options(zjmud_native PRIVATE
    -fno-omit-frame-pointer
    -iquote${FLUFFOS_ROOT}
    -Wno-c++11-narrowing
    -Wno-deprecated-register
    -Wno-format-security
    -Wno-implicit-function-declaration
    -Wno-inconsistent-missing-override
    -Wno-logical-op-parentheses
    -Wno-missing-field-initializers
    -Wno-parentheses
    -Wno-pointer-bool-conversion
    -Wno-register
    -Wno-return-type
    -Wno-sign-compare
    -Wno-unused-parameter
    -Wno-write-strings
)
target_include_directories(zjmud_native PRIVATE
    ${FLUFFOS_ROOT}
    ${MUSL_CRYPT_ROOT}
    ${LIBEVENT_ROOT}/include
    ${CMAKE_CURRENT_BINARY_DIR}/libevent/include
)
target_link_libraries(zjmud_native PRIVATE event_static z log android dl m)
