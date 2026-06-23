if (PICO_SDK_PATH)
    return()
endif()

if (DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()

if (DEFINED ENV{PICO_SDK_FETCH_FROM_GIT})
    set(PICO_SDK_FETCH_FROM_GIT $ENV{PICO_SDK_FETCH_FROM_GIT})
endif()

if (DEFINED ENV{PICO_SDK_FETCH_FROM_GIT_TAG})
    set(PICO_SDK_FETCH_FROM_GIT_TAG $ENV{PICO_SDK_FETCH_FROM_GIT_TAG})
endif()

if (NOT PICO_SDK_PATH)
    if (PICO_SDK_FETCH_FROM_GIT)
        include(FetchContent)
        FetchContent_Declare(
            pico_sdk
            GIT_REPOSITORY https://github.com/raspberrypi/pico-sdk
            GIT_TAG ${PICO_SDK_FETCH_FROM_GIT_TAG:-master}
        )
        FetchContent_MakeAvailable(pico_sdk)
    else()
        message(FATAL_ERROR "PICO_SDK_PATH not set")
    endif()
endif()

get_filename_component(PICO_SDK_PATH "${PICO_SDK_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
if (NOT EXISTS ${PICO_SDK_PATH})
    message(FATAL_ERROR "Directory '${PICO_SDK_PATH}' not found")
endif()

set(PICO_SDK_INIT_CMAKE_FILE ${PICO_SDK_PATH}/pico_sdk_init.cmake)
if (NOT EXISTS ${PICO_SDK_INIT_CMAKE_FILE})
    message(FATAL_ERROR "PICO SDK init file not found: ${PICO_SDK_INIT_CMAKE_FILE}")
endif()

include(${PICO_SDK_INIT_CMAKE_FILE})