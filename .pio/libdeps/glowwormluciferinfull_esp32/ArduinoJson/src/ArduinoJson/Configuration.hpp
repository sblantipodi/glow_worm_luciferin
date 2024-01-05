// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2024, Benoit BLANCHON
// MIT License

#pragma once

// Support std::istream and std::ostream
#ifndef ARDUINOJSON_ENABLE_STD_STREAM
#  ifdef __has_include
#    if __has_include(<istream>) && \
    __has_include(<ostream>) && \
    !defined(min) && \
    !defined(max)
#      define ARDUINOJSON_ENABLE_STD_STREAM 1
#    else
#      define ARDUINOJSON_ENABLE_STD_STREAM 0
#    endif
#  else
#    ifdef ARDUINO
#      define ARDUINOJSON_ENABLE_STD_STREAM 0
#    else
#      define ARDUINOJSON_ENABLE_STD_STREAM 1
#    endif
#  endif
#endif

// Support std::string
#ifndef ARDUINOJSON_ENABLE_STD_STRING
#  ifdef __has_include
#    if __has_include(<string>) && !defined(min) && !defined(max)
#      define ARDUINOJSON_ENABLE_STD_STRING 1
#    else
#      define ARDUINOJSON_ENABLE_STD_STRING 0
#    endif
#  else
#    ifdef ARDUINO
#      define ARDUINOJSON_ENABLE_STD_STRING 0
#    else
#      define ARDUINOJSON_ENABLE_STD_STRING 1
#    endif
#  endif
#endif

// Support for std::string_view
#ifndef ARDUINOJSON_ENABLE_STRING_VIEW
#  ifdef __has_include
#    if __has_include(<string_view>) && __cplusplus >= 201703L
#      define ARDUINOJSON_ENABLE_STRING_VIEW 1
#    else
#      define ARDUINOJSON_ENABLE_STRING_VIEW 0
#    endif
#  else
#    define ARDUINOJSON_ENABLE_STRING_VIEW 0
#  endif
#endif

// Store floating-point values with float (0) or double (1)
#ifndef ARDUINOJSON_USE_DOUBLE
#  define ARDUINOJSON_USE_DOUBLE 1
#endif

// Pointer size: a heuristic to set sensible defaults
#ifndef ARDUINOJSON_SIZEOF_POINTER
#  if defined(__SIZEOF_POINTER__)
#    define ARDUINOJSON_SIZEOF_POINTER __SIZEOF_POINTER__
#  elif defined(_WIN64) && _WIN64
#    define ARDUINOJSON_SIZEOF_POINTER 8  // 64 bits
#  else
#    define ARDUINOJSON_SIZEOF_POINTER 4  // assume 32 bits otherwise
#  endif
#endif

// Store integral values with long (0) or long long (1)
#ifndef ARDUINOJSON_USE_LONG_LONG
#  if ARDUINOJSON_SIZEOF_POINTER >= 4  // 32 & 64 bits systems
#    define ARDUINOJSON_USE_LONG_LONG 1
#  else
#    define ARDUINOJSON_USE_LONG_LONG 0
#  endif
#endif

// Limit nesting as the stack is likely to be small
#ifndef ARDUINOJSON_DEFAULT_NESTING_LIMIT
#  define ARDUINOJSON_DEFAULT_NESTING_LIMIT 10
#endif

// Number of bytes to store the variant identifier
#ifndef ARDUINOJSON_SLOT_ID_SIZE
#  if ARDUINOJSON_SIZEOF_POINTER <= 2
#    define ARDUINOJSON_SLOT_ID_SIZE 1  // up to 255 slots
#  elif ARDUINOJSON_SIZEOF_POINTER == 4
#    define ARDUINOJSON_SLOT_ID_SIZE 2  // up to 65535 slots
#  else
#    define ARDUINOJSON_SLOT_ID_SIZE 4  // up to 4294967295 slots
#  endif
#endif

// Capacity of each variant pool (in slots)
#ifndef ARDUINOJSON_POOL_CAPACITY
#  if ARDUINOJSON_SIZEOF_POINTER <= 2
#    define ARDUINOJSON_POOL_CAPACITY 16  // 128 bytes
#  elif ARDUINOJSON_SIZEOF_POINTER == 4
#    define ARDUINOJSON_POOL_CAPACITY 64  // 1024 bytes
#  else
#    define ARDUINOJSON_POOL_CAPACITY 128  // 3072 bytes
#  endif
#endif

// Initial capacity of the pool list
#ifndef ARDUINOJSON_INITIAL_POOL_COUNT
#  define ARDUINOJSON_INITIAL_POOL_COUNT 4
#endif

// Automatically call shrinkToFit() from deserializeXxx()
// Disabled by default on 8-bit platforms because it's not worth the increase in
// code size
#ifndef ARDUINOJSON_AUTO_SHRINK
#  if ARDUINOJSON_SIZEOF_POINTER <= 2
#    define ARDUINOJSON_AUTO_SHRINK 0
#  else
#    define ARDUINOJSON_AUTO_SHRINK 1
#  endif
#endif

// Number of bytes to store the length of a string
#ifndef ARDUINOJSON_STRING_LENGTH_SIZE
#  if ARDUINOJSON_SIZEOF_POINTER <= 2
#    define ARDUINOJSON_STRING_LENGTH_SIZE 1  // up to 255 characters
#  else
#    define ARDUINOJSON_STRING_LENGTH_SIZE 2  // up to 65535 characters
#  endif
#endif

#ifdef ARDUINO

// Enable support for Arduino's String class
#  ifndef ARDUINOJSON_ENABLE_ARDUINO_STRING
#    define ARDUINOJSON_ENABLE_ARDUINO_STRING 1
#  endif

// Enable support for Arduino's Stream class
#  ifndef ARDUINOJSON_ENABLE_ARDUINO_STREAM
#    define ARDUINOJSON_ENABLE_ARDUINO_STREAM 1
#  endif

// Enable support for Arduino's Print class
#  ifndef ARDUINOJSON_ENABLE_ARDUINO_PRINT
#    define ARDUINOJSON_ENABLE_ARDUINO_PRINT 1
#  endif

// Enable support for PROGMEM
#  ifndef ARDUINOJSON_ENABLE_PROGMEM
#    define ARDUINOJSON_ENABLE_PROGMEM 1
#  endif

#else  // ARDUINO

// Disable support for Arduino's String class
#  ifndef ARDUINOJSON_ENABLE_ARDUINO_STRING
#    define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#  endif

// Disable support for Arduino's Stream class
#  ifndef ARDUINOJSON_ENABLE_ARDUINO_STREAM
#    define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#  endif

// Disable support for Arduino's Print class
#  ifndef ARDUINOJSON_ENABLE_ARDUINO_PRINT
#    define ARDUINOJSON_ENABLE_ARDUINO_PRINT 0
#  endif

// Enable PROGMEM support on AVR only
#  ifndef ARDUINOJSON_ENABLE_PROGMEM
#    ifdef __AVR__
#      define ARDUINOJSON_ENABLE_PROGMEM 1
#    else
#      define ARDUINOJSON_ENABLE_PROGMEM 0
#    endif
#  endif

#endif  // ARDUINO

// Convert unicode escape sequence (\u0123) to UTF-8
#ifndef ARDUINOJSON_DECODE_UNICODE
#  define ARDUINOJSON_DECODE_UNICODE 1
#endif

// Ignore comments in input
#ifndef ARDUINOJSON_ENABLE_COMMENTS
#  define ARDUINOJSON_ENABLE_COMMENTS 0
#endif

// Support NaN in JSON
#ifndef ARDUINOJSON_ENABLE_NAN
#  define ARDUINOJSON_ENABLE_NAN 0
#endif

// Support Infinity in JSON
#ifndef ARDUINOJSON_ENABLE_INFINITY
#  define ARDUINOJSON_ENABLE_INFINITY 0
#endif

// Control the exponentiation threshold for big numbers
// CAUTION: cannot be more that 1e9 !!!!
#ifndef ARDUINOJSON_POSITIVE_EXPONENTIATION_THRESHOLD
#  define ARDUINOJSON_POSITIVE_EXPONENTIATION_THRESHOLD 1e7
#endif

// Control the exponentiation threshold for small numbers
#ifndef ARDUINOJSON_NEGATIVE_EXPONENTIATION_THRESHOLD
#  define ARDUINOJSON_NEGATIVE_EXPONENTIATION_THRESHOLD 1e-5
#endif

#ifndef ARDUINOJSON_LITTLE_ENDIAN
#  if defined(_MSC_VER) ||                           \
      (defined(__BYTE_ORDER__) &&                    \
       __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
      defined(__LITTLE_ENDIAN__) || defined(__i386) || defined(__x86_64)
#    define ARDUINOJSON_LITTLE_ENDIAN 1
#  else
#    define ARDUINOJSON_LITTLE_ENDIAN 0
#  endif
#endif

#ifndef ARDUINOJSON_ENABLE_ALIGNMENT
#  if defined(__AVR)
#    define ARDUINOJSON_ENABLE_ALIGNMENT 0
#  else
#    define ARDUINOJSON_ENABLE_ALIGNMENT 1
#  endif
#endif

#ifndef ARDUINOJSON_TAB
#  define ARDUINOJSON_TAB "  "
#endif

#ifndef ARDUINOJSON_STRING_BUFFER_SIZE
#  define ARDUINOJSON_STRING_BUFFER_SIZE 32
#endif

#ifndef ARDUINOJSON_DEBUG
#  ifdef __PLATFORMIO_BUILD_DEBUG__
#    define ARDUINOJSON_DEBUG 1
#  else
#    define ARDUINOJSON_DEBUG 0
#  endif
#endif

#if defined(nullptr)
#  error nullptr is defined as a macro. Remove the faulty #define or #undef nullptr
// See https://github.com/bblanchon/ArduinoJson/issues/1355
#endif
