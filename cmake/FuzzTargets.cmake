option(TIDE_ENABLE_FUZZING "Build libFuzzer-style harnesses" OFF)

function(tide_add_fuzz_target target source link_library)
  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE ${link_library})
  target_include_directories(${target} PRIVATE include src)
  tide_apply_warnings(${target})
  tide_append_sanitizers(${target})

  if(TIDE_ENABLE_FUZZING AND NOT MSVC)
    target_compile_definitions(${target} PRIVATE TIDE_LIBFUZZER=1)
    target_link_options(${target} PRIVATE -fsanitize=fuzzer)
  endif()
endfunction()
