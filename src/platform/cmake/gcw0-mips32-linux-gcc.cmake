set(CMAKE_SYSTEM_NAME "Linux")
set(TOOLCHAIN_PREFIX "/opt/gcw0-toolchain/usr")
if("${CROSS}" STREQUAL "")
  set(CROSS mipsel-linux-)
endif ()

set(MIPS_CFLAGS "${MIPS_CFLAGS} -mips32r2 -fomit-frame-pointer -fno-builtin -fno-common -fpermissive \
                                -fno-PIC -mno-check-zero-division -mplt -mno-shared -ffast-math \
								-funswitch-loops -fno-strict-aliasing -Wno-sign-compare -DGCW_ZERO")
								
set(TOOL_OS_SUFFIX "")
if(CMAKE_HOST_WIN32)
 set(TOOL_OS_SUFFIX ".exe")
endif()

execute_process(COMMAND "${CROSS}gcc${TOOL_OS_SUFFIX}" -v RESULT_VARIABLE C_CHECK_RESULT OUTPUT_QUIET ERROR_QUIET)
if(NOT C_CHECK_RESULT EQUAL 0)
  if("${TOOLCHAIN_PREFIX}" STREQUAL "")
    set(TOOLCHAIN_PREFIX "/opt/${MODEL}-toolchain")
  endif()
  set(TOOLCHAIN_BINARY_DIR "${TOOLCHAIN_PREFIX}/bin/")
endif()

set(CMAKE_SYSTEM_PROCESSOR "mipsel")
set(CMAKE_C_COMPILER   "${TOOLCHAIN_BINARY_DIR}${CROSS}gcc${TOOL_OS_SUFFIX}"     CACHE PATH "C compiler")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_BINARY_DIR}${CROSS}g++${TOOL_OS_SUFFIX}"     CACHE PATH "C++ compiler")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_BINARY_DIR}${CROSS}gcc${TOOL_OS_SUFFIX}"     CACHE PATH "assembler")
set(CMAKE_STRIP        "${TOOLCHAIN_BINARY_DIR}${CROSS}strip${TOOL_OS_SUFFIX}"   CACHE PATH "strip")
set(CMAKE_AR           "${TOOLCHAIN_BINARY_DIR}${CROSS}ar${TOOL_OS_SUFFIX}"      CACHE PATH "archive")
set(CMAKE_LINKER       "${TOOLCHAIN_BINARY_DIR}${CROSS}ld${TOOL_OS_SUFFIX}"      CACHE PATH "linker")
set(CMAKE_NM           "${TOOLCHAIN_BINARY_DIR}${CROSS}nm${TOOL_OS_SUFFIX}"      CACHE PATH "nm")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_BINARY_DIR}${CROSS}objcopy${TOOL_OS_SUFFIX}" CACHE PATH "objcopy")
set(CMAKE_OBJDUMP      "${TOOLCHAIN_BINARY_DIR}${CROSS}objdump${TOOL_OS_SUFFIX}" CACHE PATH "objdump")
set(CMAKE_RANLIB       "${TOOLCHAIN_BINARY_DIR}${CROSS}ranlib${TOOL_OS_SUFFIX}"  CACHE PATH "ranlib")

set(CMAKE_C_FLAGS          "${CMAKE_C_FLAGS} ${MIPS_CFLAGS}"                     CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${MIPS_CFLAGS} ${MIPS_CXXFLAGS}"  CACHE STRING "C++ flags")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed -Wl,--gc-sections" CACHE STRING "Executable linker flags")

# No runtime cpu detect for mipsel-linux-gcc.
set(CONFIG_RUNTIME_CPU_DETECT 0 CACHE BOOL "")

execute_process(COMMAND ${CMAKE_C_COMPILER} -print-sysroot OUTPUT_VARIABLE MIPS_SYSROOT_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CMAKE_FIND_ROOT_PATH "${MIPS_SYSROOT_PATH}")

if(NOT CMAKE_FIND_ROOT_PATH_MODE_LIBRARY)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
endif()
if(NOT CMAKE_FIND_ROOT_PATH_MODE_INCLUDE)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()
