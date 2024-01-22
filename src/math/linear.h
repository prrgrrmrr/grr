#ifndef GRR_MATH_H
#define GRR_MATH_H

#include "string.h" // memcpy
#include "types.h"

// GLSL types as defined in the spec:
// https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.pdf

typedef struct GrrMatrix4x4 {
  Grr_f32
      data[16]; // Column-major order: first 4 floats represent first column...
} GrrMatrix4x4;

void Grr_identity(GrrMatrix4x4 *matrix);

#endif