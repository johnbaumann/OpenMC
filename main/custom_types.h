#ifndef CUSTOM_TYPES_H
#define CUSTOM_TYPES_H

#include <stdint.h>

#define nop() __asm__ __volatile__("nop;")

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

typedef uint8_t byte;

#endif // CUSTOM_TYPES_H