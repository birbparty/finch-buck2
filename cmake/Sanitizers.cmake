# Sanitizers.cmake Provides functions to enable various sanitizers for debugging

function(enable_sanitizers target)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES
                                             ".*Clang")
    option(ENABLE_ASAN "Enable address sanitizer" ON)
    option(ENABLE_UBSAN "Enable undefined behavior sanitizer" ON)
    option(ENABLE_TSAN "Enable thread sanitizer" OFF)
    option(ENABLE_MSAN "Enable memory sanitizer" OFF)

    set(SANITIZERS "")

    if(ENABLE_ASAN)
      list(APPEND SANITIZERS "address")
    endif()

    if(ENABLE_UBSAN)
      list(APPEND SANITIZERS "undefined")
    endif()

    if(ENABLE_TSAN)
      if(ENABLE_ASAN OR ENABLE_MSAN)
        message(
          WARNING
            "Thread sanitizer is incompatible with address and memory sanitizers"
        )
      else()
        list(APPEND SANITIZERS "thread")
      endif()
    endif()

    if(ENABLE_MSAN)
      if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        if(ENABLE_ASAN OR ENABLE_TSAN)
          message(
            WARNING
              "Memory sanitizer is incompatible with address and thread sanitizers"
          )
        else()
          list(APPEND SANITIZERS "memory")
        endif()
      else()
        message(WARNING "Memory sanitizer is only supported by Clang")
      endif()
    endif()

    list(JOIN SANITIZERS "," LIST_OF_SANITIZERS)

    if(LIST_OF_SANITIZERS)
      target_compile_options(${target} PUBLIC -fsanitize=${LIST_OF_SANITIZERS})
      target_link_options(${target} PUBLIC -fsanitize=${LIST_OF_SANITIZERS})

      # Additional options for better sanitizer output
      target_compile_options(${target} PUBLIC -fno-omit-frame-pointer)
      target_compile_options(${target} PUBLIC -g)

      # For undefined behavior sanitizer, make it more strict
      if(ENABLE_UBSAN)
        target_compile_options(${target} PUBLIC -fno-sanitize-recover=all)
      endif()
    endif()
  elseif(MSVC)
    # MSVC has limited sanitizer support
    option(ENABLE_ASAN "Enable address sanitizer" ON)

    if(ENABLE_ASAN)
      target_compile_options(${target} PUBLIC /fsanitize=address)
      # MSVC requires setting environment variable
      # ASAN_OPTIONS=windows_hook_rtl_allocators=true
      message(
        STATUS
          "Address sanitizer enabled. Set environment variable: ASAN_OPTIONS=windows_hook_rtl_allocators=true"
      )
    endif()
  endif()
endfunction()

# Function to print sanitizer configuration
function(print_sanitizer_config)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES
                                             ".*Clang")
    message(STATUS "Sanitizer configuration:")
    message(STATUS "  Address Sanitizer (ASAN):    ${ENABLE_ASAN}")
    message(STATUS "  UB Sanitizer (UBSAN):        ${ENABLE_UBSAN}")
    message(STATUS "  Thread Sanitizer (TSAN):     ${ENABLE_TSAN}")
    message(STATUS "  Memory Sanitizer (MSAN):     ${ENABLE_MSAN}")
  elseif(MSVC)
    message(STATUS "Sanitizer configuration:")
    message(STATUS "  Address Sanitizer (ASAN):    ${ENABLE_ASAN}")
  endif()
endfunction()
