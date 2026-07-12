#ifndef MICROS_H_
#define MICROS_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"

#ifdef _USE_HW_MICROS


bool microsInit(void);
uint32_t micros(void);
uint32_t cycles(void);
uint32_t cyclesToMicros(uint32_t cyc);


#endif


#ifdef __cplusplus
}
#endif


#endif 