# Intermediate CMakeLists.txt to bridge src/CMakeLists.txt and blosc/CMakeLists.txt.
# blosc/CMakeLists.txt exports SOURCES with paths prefixed by "blosc/" (e.g. "blosc/blosc2.c"),
# which are relative to THIS directory — so target_sources() must be called from here.

set(BUILD_SHARED OFF)
set(BUILD_STATIC ON)
set(BUILD_TESTS OFF)
set(UNIFIED_TARGET_ALIAS blosc2_static)
# zmat uses miniz; disable zlib/zlib-ng in blosc2 to avoid pulling in external headers
set(DEACTIVATE_ZLIB ON)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)
if(Threads_FOUND)
  set(HAVE_THREADS TRUE)
else()
  set(HAVE_THREADS FALSE)
endif()

add_subdirectory(blosc)

# SOURCES was exported by blosc/CMakeLists.txt with paths relative to this directory
# (e.g. "blosc/blosc2.c" -> blosc2/blosc/blosc2.c).
# Convert to absolute paths to avoid CMP0076: without the NEW policy, CMake resolves
# relative paths in target_sources() relative to where the target was defined
# (blosc/CMakeLists.txt), not where target_sources() is called, giving a wrong path.
set(BLOSC2_ABS_SOURCES)
foreach(src ${SOURCES})
    if(IS_ABSOLUTE "${src}")
        list(APPEND BLOSC2_ABS_SOURCES "${src}")
    else()
        list(APPEND BLOSC2_ABS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
    endif()
endforeach()

if(BUILD_STATIC)
  target_sources(blosc2_static PRIVATE ${BLOSC2_ABS_SOURCES})
endif()
if(BUILD_SHARED)
  target_sources(blosc2_shared PRIVATE ${BLOSC2_ABS_SOURCES})
endif()
if(BUILD_TESTS)
  target_sources(blosc_testing PRIVATE ${BLOSC2_ABS_SOURCES})
endif()
