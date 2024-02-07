#ifndef GRR_MATH_H
#define GRR_MATH_H

#include "string.h" // memcpy
#include "types.h"
#include <math.h>

// GLSL types as defined in the spec:
// https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.pdf

typedef struct GrrVector2 {
  Grr_f32 x;
  Grr_f32 y;
} GrrVector2;

typedef struct GrrVector3 {
  Grr_f32 x;
  Grr_f32 y;
  Grr_f32 z;
} GrrVector3;

typedef struct GrrVector4 {
  Grr_f32 x;
  Grr_f32 y;
  Grr_f32 z;
  Grr_f32 w;
} GrrVector4;

typedef struct GrrMatrix4x4 {
  Grr_f32
      data[16]; // Column-major order: first 4 floats represent first column...
} GrrMatrix4x4;

//  Camera uses right-handed coordinate system (facing the negative z axis)
typedef struct GrrCamera {
  GrrVector3 origin;    // Camera location
  GrrVector3 direction; // The -z axis of the camera coordinate system
  GrrVector3 upHint;    // Camera y-axis hint: any vector with positive camera y
                        // component, this will be used to calculate the true
                        // camera y-axis
  Grr_f32 zNear;        // Positive distance (in meters) to near clipping plane
  Grr_f32 zFar;         // Positive distance (in meters) to far clipping plane
  Grr_f32 aspectRatio;  // width:height ratio
  Grr_f32 yFOV;         // Vertical field of view in radians (< 3.14159...)
} GrrCamera;

void Grr_normalize3(GrrVector3 *v);
void Grr_scale3(GrrVector3 *v, Grr_f32 s);
void Grr_setLength3(GrrVector3 *v, Grr_f32 length);
void Grr_crossProduct(GrrVector3 *v, GrrVector3 *w, GrrVector3 *c);
void Grr_identityMatrix(GrrMatrix4x4 *matrix);
void Grr_perspectiveProjectionMatrix(GrrCamera *camera, GrrMatrix4x4 *matrix);

#endif