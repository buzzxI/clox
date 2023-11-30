#ifndef clox_complier_h
#define clox_complier_h

#include "scanner/scanner.h"
#include "chunk/chunk.h"

#define UINT16_COUNT (UINT16_MAX + 1) 

FunctionObj* compile(const char *source);

#endif