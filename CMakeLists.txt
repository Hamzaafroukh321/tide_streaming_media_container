cmake_minimum_required(VERSION 3.24)

project(tide_streaming_media_container
  VERSION 0.1.0
  DESCRIPTION "TIDE streaming media container toolkit"
  LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)
include(cmake/Warnings.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/FuzzTargets.cmake)

set(TIDE_PUBLIC_HEADERS
  include/tide/tide.h
  include/tide/error.h
  include/tide/limits.h
  include/tide/decoder.h
  include/tide/demux.h
  include/tide/mux.h
  include/tide/repair.h
  include/tide/source.h)

set(TIDE_CORE_SOURCES
  src/common/checked.c
  src/common/error.c
  src/common/limits.c
  src/format/crc32c.c
  src/format/decoder.c
  src/format/reader.c
  src/format/records.c
  src/format/writer.c
  src/source/source.c
  src/source/memory_source.c
  src/model/lease.c
  src/model/time.c
  src/model/edit.c
  src/model/packet.c
  src/index/reorder.c
  src/index/index_builder.c
  src/demux/demux.c
  src/mux/mux.c
  src/repair/scanner.c)

add_library(tide_core ${TIDE_PUBLIC_HEADERS} ${TIDE_CORE_SOURCES})
target_include_directories(tide_core PUBLIC include PRIVATE src)
tide_apply_warnings(tide_core)
tide_append_sanitizers(tide_core)

if(WIN32 AND CMAKE_C_COMPILER_ID MATCHES "Clang")
  get_filename_component(TIDE_CLANG_BIN_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
  file(GLOB_RECURSE TIDE_CLANG_BUILTINS_CANDIDATES
    "${TIDE_CLANG_BIN_DIR}/../lib/clang/*/lib/windows/clang_rt.builtins-x86_64.lib")
  list(LENGTH TIDE_CLANG_BUILTINS_CANDIDATES TIDE_CLANG_BUILTINS_COUNT)
  if(TIDE_CLANG_BUILTINS_COUNT GREATER 0)
    list(GET TIDE_CLANG_BUILTINS_CANDIDATES 0 TIDE_CLANG_BUILTINS)
    target_link_libraries(tide_core PUBLIC "${TIDE_CLANG_BUILTINS}")
  endif()
endif()

add_executable(tide src/cli/main.c)
target_link_libraries(tide PRIVATE tide_core)
tide_apply_warnings(tide)
tide_append_sanitizers(tide)

add_executable(tide_dump tools/tide_dump.c)
target_link_libraries(tide_dump PRIVATE tide_core)
tide_apply_warnings(tide_dump)
tide_append_sanitizers(tide_dump)

add_executable(tide_index_dump tools/tide_index_dump.c)
target_link_libraries(tide_index_dump PRIVATE tide_core)
tide_apply_warnings(tide_index_dump)
tide_append_sanitizers(tide_index_dump)

add_executable(example_write_capture examples/write_capture.c)
target_link_libraries(example_write_capture PRIVATE tide_core)
tide_apply_warnings(example_write_capture)
tide_append_sanitizers(example_write_capture)

add_executable(example_stream_demux examples/stream_demux.c)
target_link_libraries(example_stream_demux PRIVATE tide_core)
tide_apply_warnings(example_stream_demux)
tide_append_sanitizers(example_stream_demux)

if(BUILD_TESTING)
  add_executable(tide_tests
    tests/unit/reader_test.c
    tests/unit/record_semantics_test.c
    tests/unit/time_test.c
    tests/unit/edit_test.c
    tests/unit/reorder_test.c
    tests/integration/partial_file_test.c
    tests/integration/remux_test.c
    tests/integration/repair_test.c
    tests/test_main.c)
  target_link_libraries(tide_tests PRIVATE tide_core)
  target_include_directories(tide_tests PRIVATE tests tests/integration src)
  tide_apply_warnings(tide_tests)
  tide_append_sanitizers(tide_tests)
  add_test(NAME tide_tests COMMAND tide_tests)
endif()

tide_add_fuzz_target(fuzz_decoder fuzz/fuzz_decoder.c tide_core)
tide_add_fuzz_target(fuzz_chunk_boundaries fuzz/fuzz_chunk_boundaries.c tide_core)
tide_add_fuzz_target(fuzz_demux_remux fuzz/fuzz_demux_remux.c tide_core)

install(TARGETS tide_core tide tide_dump tide_index_dump
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(DIRECTORY include/tide DESTINATION include)
