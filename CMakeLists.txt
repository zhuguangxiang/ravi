cmake_minimum_required(VERSION 3.12)
project(Ravi VERSION 1.0.4 LANGUAGES C CXX)

# Get access to CMake helpers
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

option(LLVM_JIT "Controls whether LLVM JIT compilation will be enabled, default is OFF" OFF)
option(OMR_JIT "Controls whether NanoJIT compilation will be enabled, default is OFF" OFF)
option(NO_JIT "Controls whether JIT should be disabled, default is OFF" OFF)
option(STATIC_BUILD "Build static version of Ravi, default is OFF" OFF)
option(COMPUTED_GOTO "Controls whether the interpreter switch will use computed gotos on gcc/clang, default is ON" ON)
option(LTESTS "Controls whether ltests are enabled in Debug mode; note requires Debug build" ON)

if (LLVM_JIT AND OMR_JIT)
    message(FATAL_ERROR
            "LLVM_JIT and OMR_JIT cannot be set to ON at the same time")
endif ()

# By default on X86_64 non-Windows platforms we enable MIR JIT
if (NOT WIN32
        AND NOT LLVM_JIT
        AND NOT OMR_JIT
        AND NOT NO_JIT
        AND CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    set(MIR_JIT ON)
endif ()

if (MIR_JIT)
    if (MSVC OR WIN32)
        message(FATAL_ERROR "MIR_JIT is not supported when using MSVC and/or WIN32")
    endif ()
    if (NOT (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)"))
        message(FATAL_ERROR "MIR_JIT is only supported on X86-64 platforms")
    endif ()
    set(LLVM_JIT OFF)
    set(OMR_JIT OFF)
    set(STATIC_BUILD OFF) # Because we need to expose the symbols in the library
endif ()

if (STATIC_BUILD)
    message(STATUS "STATIC library build enabled")
    set(LIBRAVI_BUILD_TYPE STATIC)
else ()
    message(STATUS "DYNAMIC library build enabled")
    set(LIBRAVI_BUILD_TYPE SHARED)
endif ()
message(STATUS "Computed goto ${COMPUTED_GOTO}")
if (COMPUTED_GOTO AND MSVC)
    message(WARNING "Computed goto is not available with MSVC")
endif ()

include_directories("${PROJECT_SOURCE_DIR}/include")

# define the Lua core source files
set(RAVI_AST_SOURCES src/ravi_ast_parse.c src/ravi_ast_print.c src/ravi_ast_typecheck.c)
set(LUA_CORE_SRCS src/lapi.c src/lcode.c src/lctype.c src/ldebug.c src/ldo.c src/ldump.c
        src/lfunc.c src/lgc.c src/llex.c src/lmem.c src/lobject.c src/lopcodes.c
        src/lparser.c src/lstate.c src/lstring.c src/ltable.c src/ltm.c src/lundump.c
        src/lvm.c src/lzio.c src/ravi_jit.c src/ltests.c src/ravi_profile.c
        src/ravi_membuf.c src/ravi_jitshared.c src/bit.c src/ravi_alloc.c
        ${RAVI_AST_SOURCES})
# define the Lua library source files
set(LUA_LIB_SRCS src/lauxlib.c src/lbaselib.c src/lbitlib.c src/lcorolib.c src/ldblib.c src/liolib.c
        src/lmathlib.c src/loslib.c src/ltablib.c src/lstrlib.c src/loadlib.c src/linit.c src/lutf8lib.c)
set(LUA_HEADERS include/lua.h include/luaconf.h include/lualib.h include/lauxlib.h)
# LLVM code gen
set(LLVM_JIT_SRCS src/ravi_llvmjit.cpp src/ravi_llvmtypes.cpp
        src/ravi_llvmcodegen.cpp src/ravi_llvmforprep.cpp src/ravi_llvmcomp.cpp
        src/ravi_llvmreturn.cpp src/ravi_llvmload.cpp src/ravi_llvmforloop.cpp
        src/ravi_llvmarith1.cpp src/ravi_llvmcall.cpp src/ravi_llvmtable.cpp
        src/ravi_llvmarith2.cpp src/ravi_llvmtforcall.cpp src/ravi_llvmrest.cpp)
# MIR sources
set(MIR_HEADERS mir/mir.h mir/mir-gen.h mir/mir-varr.h mir/mir-dlist.h mir/mir-htab.h
        mir/mir-hash.h mir/mir-bitmap.h)
set(MIR_SRCS mir/mir.c mir/mir-gen.c)
set(C2MIR_SRCS mir/c2mir/c2mir.c)
# OMR code gen
set(OMR_JIT_SRCS src/ravi_omrjit.c src/ravi_omrjitapi.c)
# MIR code gen
set(MIR_JIT_SRCS src/ravi_mirjit.c)
set(NO_JIT_SRCS src/ravi_nojit.c)
# Common stuff we need even if we don't use dmrC.
set(DMR_C_HEADERS_COMMON dmr_c/src/allocate.h dmr_c/src/port.h dmr_c/src/ptrlist.h)
set(DMR_C_SRCS_COMMON dmr_c/src/allocate.c dmr_c/src/ptrlist.c)
# dmrC Optional sources
set(RAVI_C_SYMBOL_PARSER src/ravi_dmrc_parsesymbols.c)
set(DMR_C_SRCS_OPT dmr_c/src/builtin.c dmr_c/src/char.c dmr_c/src/expression.c dmr_c/src/evaluate.c
        dmr_c/src/expand.c dmr_c/src/inline.c dmr_c/src/lib.c dmr_c/src/linearize.c
        dmr_c/src/liveness.c dmr_c/src/parse.c dmr_c/src/target.c dmr_c/src/tokenize.c
        dmr_c/src/pre-process.c dmr_c/src/scope.c dmr_c/src/show-parse.c dmr_c/src/symbol.c
        dmr_c/src/walksymbol.c)
set(DMR_C_HEADERS_OPT dmr_c/src/char.h dmr_c/src/expression.h dmr_c/src/flow.h dmr_c/src/ident-list.h
        dmr_c/src/linearize.h dmr_c/src/lib.h dmr_c/src/parse.h dmr_c/src/scope.h dmr_c/src/symbol.h
        dmr_c/src/target.h dmr_c/src/token.h dmr_c/src/walksymbol.h)
set(LUA_CMD_SRCS src/lua.c)
file(GLOB RAVI_HEADERS "${PROJECT_SOURCE_DIR}/include/*.h")

if (COMPUTED_GOTO AND NOT MSVC)
    if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang")
        set_source_files_properties(src/lvm.c PROPERTIES COMPILE_FLAGS -DRAVI_USE_COMPUTED_GOTO)
    elseif (CMAKE_C_COMPILER_ID MATCHES "GNU")
        set_source_files_properties(src/lvm.c PROPERTIES COMPILE_FLAGS "-fno-crossjumping -fno-gcse -DRAVI_USE_COMPUTED_GOTO")
    endif ()
endif ()

set(EMBEDDED_DMRC ON)
if (LLVM_JIT)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

    message(STATUS "LLVM Definitions ${LLVM_DEFINITIONS}")
    message(STATUS "LLVMJIT enabled")

    set(JIT_SRCS ${LLVM_JIT_SRCS})
    if (NOT MSVC)
        set_source_files_properties(${LLVM_JIT_SRCS} PROPERTIES
                COMPILE_FLAGS "-fno-rtti -fno-exceptions ${LLVM_DEFINITIONS}")
    endif ()
    set_property(
            SOURCE ${LLVM_JIT_SRCS}
            APPEND
            PROPERTY INCLUDE_DIRECTORIES ${LLVM_INCLUDE_DIRS}
    )
    set(DMR_C_JIT_SRCS
            dmr_c/llvm-backend/sparse-llvm.c
            )
    set(DMR_C_JIT_HEADERS
            dmr_c/llvm-backend/dmr_c.h
            )
    set(DMR_C_HEADERS ${DMR_C_HEADERS_OPT} ${DMR_C_HEADERS_COMMON})
    set(DMR_C_SRCS ${DMR_C_SRCS_OPT} ${DMR_C_SRCS_COMMON} ${RAVI_C_SYMBOL_PARSER})

    set_property(SOURCE ${DMR_C_JIT_SRCS} src/ravi_llvmcodegen.cpp ${RAVI_C_SYMBOL_PARSER}
            APPEND
            PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/dmr_c/llvm-backend")
    set_property(SOURCE ${DMR_C_SRCS} ${DMR_C_JIT_SRCS} ${RAVI_AST_SOURCES}
            APPEND
            PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/dmr_c/src")
    set_property(
            SOURCE ${RAVI_C_SYMBOL_PARSER} ${DMR_C_JIT_SRCS}
            APPEND
            PROPERTY INCLUDE_DIRECTORIES ${LLVM_INCLUDE_DIRS})
    # FIXME get rid of this dependency
    set_property(
            SOURCE ${LUA_CMD_SRCS}
            APPEND
            PROPERTY INCLUDE_DIRECTORIES ${LLVM_INCLUDE_DIRS})
elseif (OMR_JIT)
    find_package(OMRJIT REQUIRED)
    message(STATUS "OMRJIT enabled")
    set(JIT_SRCS ${OMR_JIT_SRCS})
    set(DMR_C_JIT_SRCS
            dmr_c/omrjit-backend/sparse-omrjit.c
            )
    set(DMR_C_JIT_HEADERS
            dmr_c/omrjit-backend/dmr_c.h
            )
    set(DMR_C_HEADERS ${DMR_C_HEADERS_OPT} ${DMR_C_HEADERS_COMMON})
    set(DMR_C_SRCS ${DMR_C_SRCS_OPT} ${DMR_C_SRCS_COMMON} ${RAVI_C_SYMBOL_PARSER})

    set_property(SOURCE ${DMR_C_JIT_SRCS} ${RAVI_C_SYMBOL_PARSER} ${OMR_JIT_SRCS}
            APPEND
            PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/dmr_c/omrjit-backend")
    set_property(SOURCE ${DMR_C_SRCS} ${DMR_C_JIT_SRCS} ${RAVI_AST_SOURCES}
            APPEND
            PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/dmr_c/src")
    set_property(SOURCE ${DMR_C_JIT_SRCS} ${OMR_JIT_SRCS} ${RAVI_C_SYMBOL_PARSER}
            APPEND
            PROPERTY INCLUDE_DIRECTORIES "${OMRJIT_INCLUDE_DIRS}"
            )
else ()
    # Omit all dmrC stuff except for what we need for the parser
    set(DMR_C_HEADERS ${DMR_C_HEADERS_COMMON})
    set(DMR_C_SRCS ${DMR_C_SRCS_COMMON})
    set(EMBEDDED_DMRC OFF)

    set_property(SOURCE ${DMR_C_SRCS_COMMON} ${RAVI_AST_SOURCES}
            APPEND
            PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/dmr_c/src")

    if (MIR_JIT)
        message(STATUS "MIRJIT enabled")
        set(JIT_SRCS ${MIR_SRCS} ${C2MIR_SRCS} ${MIR_JIT_SRCS})
        set_property(SOURCE ${MIR_SRCS} ${C2MIR_SRCS} ${MIR_JIT_SRCS}
                APPEND
                PROPERTY INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/mir;${CMAKE_SOURCE_DIR}/mir/c2mir")
        set_property(SOURCE ${MIR_SRCS} ${C2MIR_SRCS} ${MIR_JIT_SRCS}
                APPEND
                PROPERTY COMPILE_DEFINITIONS "x86_64;MIR_IO;MIR_SCAN")
    else()
        set(JIT_SRCS ${NO_JIT_SRCS})
    endif ()
endif ()

# Additional stuff for dmrC
if (CMAKE_COMPILER_IS_GNUCC AND EMBEDDED_DMRC)
    execute_process(COMMAND ${CMAKE_C_COMPILER} --print-file-name=
            OUTPUT_VARIABLE GCC_BASE OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -print-multiarch
            OUTPUT_VARIABLE MULTIARCH_TRIPLET ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

    set_source_files_properties(dmr_c/src/lib.c PROPERTIES COMPILE_FLAGS "-DGCC_BASE=${GCC_BASE} -DMULTIARCH_TRIPLET=${MULTIARCH_TRIPLET}")

    message(STATUS "GCC_BASE_DIR      : " ${GCC_BASE})
    message(STATUS "MULTIARCH_TRIPLET : " ${MULTIARCH_TRIPLET})
endif ()

# IDE stuff
if (MSVC OR APPLE)
    source_group("dmrC Headers" FILES ${DMR_C_HEADERS} ${DMR_C_JIT_HEADERS})
    source_group("dmrC Source Files" FILES ${DMR_C_SRCS} ${DMR_C_JIT_SRCS})
    source_group("Ravi Headers" FILES ${RAVI_HEADERS})
    source_group("Ravi Source Files" FILES ${LUA_CORE_SRCS} ${LUA_LIB_SRCS} ${JIT_SRCS})
endif ()

# Misc setup
if (MSVC OR APPLE)
    if (APPLE)
        set(EXTRA_LIBRARIES m readline)
    endif ()
elseif (NOT WIN32)
    # On Linux we need to link libdl to get access to
    # functions like dlopen()
    # ubsan can be added to get -fsanitize=undefined
    set(EXTRA_LIBRARIES m dl readline)
endif ()

# Enable minimal required LLVM components so that the
# the size of the resulting binary is manageable
if (LLVM_JIT)
    if (${LLVM_PACKAGE_VERSION} VERSION_LESS "3.8")
        set(LLVM_EXTRA_LIBS ipa)
    endif ()
    if (NOT ${LLVM_PACKAGE_VERSION} VERSION_LESS "5.0.0")
        set(LLVM_EXTRA_LIBS orcjit)
    endif ()
    message(STATUS "SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR}")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
        set(LLVM_LIBS_PROCESSOR
                X86CodeGen
                X86AsmParser
                X86Disassembler
                X86Desc
                X86Info
                X86Utils
                )
        if (${LLVM_PACKAGE_VERSION} VERSION_LESS "9.0.0")
            list(APPEND LLVM_LIBS_PROCESSOR X86AsmPrinter)
        endif ()
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
        set(LLVM_LIBS_PROCESSOR
                ARMCodeGen
                ARMAsmParser
                ARMDisassembler
                ARMAsmPrinter
                ARMDesc
                ARMInfo
                )
    endif ()
    llvm_map_components_to_libnames(LLVM_LIBS
            Analysis
            AsmParser
            AsmPrinter
            BitReader
            Core
            CodeGen
            ExecutionEngine
            InstCombine
            ${LLVM_EXTRA_LIBS}
            ipo
            MC
            MCJIT
            MCParser
            Object
            RuntimeDyld
            ScalarOpts
            Support
            Target
            TransformUtils
            ${LLVM_LIBS_PROCESSOR}
            )
    message(STATUS "LLVM_LIBS ${LLVM_LIBS}")
endif ()

set(LIBRAVI_NAME libravi)

#Main library
add_library(${LIBRAVI_NAME} ${LIBRAVI_BUILD_TYPE}
        ${RAVI_HEADERS}
        ${LUA_LIB_SRCS}
        ${LUA_CORE_SRCS}
        ${JIT_SRCS}
        ${DMR_C_HEADERS}
        ${DMR_C_SRCS}
        ${DMR_C_JIT_SRCS})
target_link_libraries(${LIBRAVI_NAME} ${EXTRA_LIBRARIES} ${LLVM_LIBS} ${OMRJIT_LIBRARIES} ${MIRJIT_LIBRARIES})

# Main Ravi executable
add_executable(ravi ${LUA_CMD_SRCS})
target_link_libraries(ravi ${LIBRAVI_NAME})

# Sources that are needed for a static NOJIT basic library
set(NOJIT_RAVI_SRCS
        ${RAVI_HEADERS}
        ${LUA_LIB_SRCS}
        ${LUA_CORE_SRCS}
        ${DMR_C_HEADERS}
        ${DMR_C_SRCS}
        ${DMR_C_JIT_SRCS}
        ${NO_JIT_SRCS})
set(RAVI_STATICEXEC_TARGET ravi_s)

# We always build a static library without JIT so that
# we can create some default executables
add_library(libravinojit_static
        ${NOJIT_RAVI_SRCS})
set_target_properties(libravinojit_static PROPERTIES PREFIX "") # As we already prefix with lib
target_link_libraries(libravinojit_static ${EXTRA_LIBRARIES})

# Create a simple NoJIT version of statically linked ravi
# This is sometimes useful in other projects that just need a Lua commandline
# but do not care about the shared library
add_executable(${RAVI_STATICEXEC_TARGET}
        ${LUA_CMD_SRCS})
target_link_libraries(ravi_s libravinojit_static)

if (NOT LTESTS)
    # Note that enabling ltests.h messes with global_State
    message(STATUS "Disabling Lua extended test harness 'ltests'")
    set_property(
            TARGET ${LIBRAVI_NAME} ravi
            APPEND
            PROPERTY COMPILE_DEFINITIONS NO_LUA_DEBUG)
    set(NO_LUA_DEBUG 1)
endif ()
if (LLVM_JIT)
    set_property(
            TARGET ${LIBRAVI_NAME} ravi
            APPEND
            PROPERTY COMPILE_DEFINITIONS "USE_LLVM=1")
    set(USE_LLVM 1)
elseif (OMR_JIT)
    set_property(
            TARGET ${LIBRAVI_NAME} ravi
            APPEND
            PROPERTY COMPILE_DEFINITIONS "USE_OMRJIT=1")
    set(USE_OMRJIT 1)
elseif (MIR_JIT)
    set_property(
            TARGET ${LIBRAVI_NAME} ravi
            APPEND
            PROPERTY COMPILE_DEFINITIONS "USE_MIRJIT=1")
    set(USE_MIRJIT 1)
endif ()
if (NOT STATIC_BUILD)
    if (WIN32)
        # enable DLL export
        set_property(
                TARGET ${LIBRAVI_NAME}
                APPEND
                PROPERTY COMPILE_DEFINITIONS LUA_BUILD_AS_DLL)
    else ()
        set_target_properties(${LIBRAVI_NAME} PROPERTIES PREFIX "")
    endif ()
else ()
    set_target_properties(${LIBRAVI_NAME} PROPERTIES PREFIX "")
endif ()
if (EMBEDDED_DMRC)
    set_property(
            TARGET ${LIBRAVI_NAME} ravi
            APPEND
            PROPERTY COMPILE_DEFINITIONS "USE_DMR_C=1")
endif ()
if (APPLE)
    set_property(
            TARGET ${LIBRAVI_NAME} libravinojit_static
            APPEND
            PROPERTY COMPILE_DEFINITIONS "LUA_USE_MACOSX=1")
elseif (UNIX)
    set_property(
            TARGET ${LIBRAVI_NAME} libravinojit_static
            APPEND
            PROPERTY COMPILE_DEFINITIONS "LUA_USE_LINUX=1")
endif ()
set_property(TARGET ${LIBRAVI_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${LIBRAVI_NAME} PROPERTY CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
configure_file(ravi-config.h.in ravi-config.h @ONLY)
target_include_directories(${LIBRAVI} ravi
        PUBLIC
        "${PROJECT_BINARY_DIR}")

# Ravi VSCode Debug adapter
set(RAVI_DEBUGGER_TARGET ravidebug)
add_executable(${RAVI_DEBUGGER_TARGET}
        vscode-debugger/src/ravidebug.c
        vscode-debugger/src/json.c
        vscode-debugger/src/protocol.c)
target_link_libraries(${RAVI_DEBUGGER_TARGET} libravinojit_static)

# Tests for VSCode Debug Adapter
add_executable(testravidebug
        vscode-debugger/src/testravidebug.c
        vscode-debugger/src/json.c
        vscode-debugger/src/protocol.c)
target_link_libraries(testravidebug libravinojit_static)

configure_file(lua-config.cmake.in lua-config.cmake @ONLY)

if (WIN32)
    configure_file(ravi-env.bat.in ravi-env.bat @ONLY)
    set(RAVI_SCRIPTS ${CMAKE_CURRENT_BINARY_DIR}/ravi-env.bat)
elseif (APPLE)
    configure_file(ravi-env.osx.sh.in ravi-env.sh @ONLY)
    set(RAVI_SCRIPTS ${CMAKE_CURRENT_BINARY_DIR}/ravi-env.sh)
else ()
    configure_file(ravi-env.linux.sh.in ravi-env.sh @ONLY)
    set(RAVI_SCRIPTS ${CMAKE_CURRENT_BINARY_DIR}/ravi-env.sh)
endif ()

install(FILES ${LUA_HEADERS}
        DESTINATION include/ravi)
install(TARGETS ${LIBRAVI_NAME} ravi ${RAVI_DEBUGGER_TARGET} ${RAVI_STATICEXEC_TARGET}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Ravi_Runtime
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Ravi_Development
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Ravi_Runtime)
install(FILES ${RAVI_SCRIPTS}
        DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lua-config.cmake
        DESTINATION cmake)
