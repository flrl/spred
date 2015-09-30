#ifndef STDINT_H
#define STDINT_H

typedef signed char  int8_t;
typedef signed short int16_t;
typedef signed long  int32_t;

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;

typedef int32_t  intptr_t;
typedef uint32_t uintptr_t;

typedef int32_t  intmax_t;
typedef uint32_t uintmax_t;

#define INT8_MAX	(127)
#define INT16_MAX	(32768)
#define INT32_MAX	(2147483647L)

#define INT8_MIN	(-127)
#define INT16_MIN	(-32768)
#define INT32_MIN	(-INT32_MAX-1)

#define UINT8_MAX   (0xFF)
#define UINT16_MAX	(0xFFFF)
#define UINT32_MAX	(0xFFFFFFFFL)

#define INTPTR_MIN	INT32_MIN
#define INTPTR_MAX	INT32_MAX

#define UINTPTR_MAX	UINT32_MAX

#endif