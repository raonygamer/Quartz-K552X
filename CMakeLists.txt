cmake_minimum_required(VERSION 3.20)

add_subdirectory(protocol)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(TOOLCHAIN_PREFIX arm-none-eabi-)

# These must be selected before project() enables the languages.
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)

project(QuartzK552X LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MCPU cortex-m0)
set(FIRMWARE_TARGET quartz-k552x)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        src/*.c
        src/*.cpp
)

add_executable(${FIRMWARE_TARGET}
        mcu/startup.S
        ${SOURCES}
)

set_target_properties(${FIRMWARE_TARGET} PROPERTIES
        SUFFIX ".elf"
)

target_include_directories(${FIRMWARE_TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/include/mcu
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_options(${FIRMWARE_TARGET} PRIVATE
        -mcpu=${MCPU}
        -mthumb
        -mfloat-abi=soft

        -ffreestanding
        -fdata-sections
        -ffunction-sections

        -Wall
        -Wextra

        $<$<CONFIG:Debug>:-Og>
        $<$<CONFIG:Debug>:-g3>
        $<$<CONFIG:Release>:-O2>

        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-unwind-tables>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-asynchronous-unwind-tables>
)

target_link_options(${FIRMWARE_TARGET} PRIVATE
        -mcpu=${MCPU}
        -mthumb
        -mfloat-abi=soft

        -nostdlib
        -nostartfiles
        -static

        -T${CMAKE_SOURCE_DIR}/mcu/linker.ld

        -Wl,--gc-sections
        -Wl,--build-id=none
        -Wl,--orphan-handling=warn
        -Wl,--print-memory-usage
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_TARGET}.map
)

# Keep compiler helper routines such as 32/64-bit division available without
# pulling in libc or libstdc++.
target_link_libraries(${FIRMWARE_TARGET} PRIVATE
        gcc
        QuartzProtocol
)

set_property(TARGET ${FIRMWARE_TARGET} APPEND PROPERTY
        LINK_DEPENDS ${CMAKE_SOURCE_DIR}/mcu/linker.ld
)

set(FIRMWARE_BIN ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_TARGET}.bin)
set(FIRMWARE_HEX ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_TARGET}.hex)

add_custom_command(
        TARGET ${FIRMWARE_TARGET}
        POST_BUILD

        COMMAND ${CMAKE_OBJCOPY}
        -O binary
        --gap-fill=0xFF
        --pad-to=0x10000
        $<TARGET_FILE:${FIRMWARE_TARGET}>
        ${FIRMWARE_BIN}

        COMMAND ${CMAKE_OBJCOPY}
        -O ihex
        $<TARGET_FILE:${FIRMWARE_TARGET}>
        ${FIRMWARE_HEX}

        COMMAND ${CMAKE_SIZE}
        $<TARGET_FILE:${FIRMWARE_TARGET}>

        VERBATIM
)