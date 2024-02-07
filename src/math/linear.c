#include "linear.h"

void Grr_normalize3(GrrVector3 *v) {
  Grr_f32 d = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
  v->x /= d;
  v->y /= d;
  v->z /= d;
}

void Grr_scale3(GrrVector3 *v, Grr_f32 s) {
  v->x *= s;
  v->y *= s;
  v->y *= s;
}

void Grr_setLength3(GrrVector3 *v, Grr_f32 length) {
  Grr_normalize3(v);
  Grr_scale3(v, length);
}

void Grr_crossProduct(GrrVector3 *v, GrrVector3 *w, GrrVector3 *c) {
  c->x = v->y * w->z - v->z * w->y;
  c->y = v->z * w->x - v->x * w->z;
  c->z = v->x * w->y - v->y * w->x;
}

void Grr_identityMatrix(GrrMatrix4x4 *matrix) {
  memset(matrix->data, 0, sizeof(matrix->data));
  matrix->data[0] = 1.0f;
  matrix->data[5] = 1.0f;
  matrix->data[10] = 1.0f;
  matrix->data[15] = 1.0f;
}

// Matrix to transform from local space (mesh) to world space
// void Grr_modelMatrix(GrrMesh *mesh, GrrMatrix4x4 *matrix) {}

// Transform from world space to camera's view space
// void Grr_viewMatrix(GrrCamera *camera, GrrMatrix4x4 *matrix) {}

// Projects vertices to clip space and performs perspective divide
// Clip space uses left-to-right x-axis, downwards y-axis and the z-axis goes
// from near clipping plane to far clipping plane
void Grr_perspectiveProjectionMatrix(GrrCamera *camera, GrrMatrix4x4 *matrix) {
  memset(matrix->data, 0, sizeof(matrix->data));
  Grr_f32 t = tanf(camera->yFOV * 0.5);
  matrix->data[0] = -1.0 / (camera->aspectRatio * t);
  matrix->data[5] = -1.0 / t;
  matrix->data[10] = -camera->zFar / (camera->zNear - camera->zFar);
  matrix->data[11] = 1.0;
  matrix->data[14] =
      (-camera->zNear * camera->zFar) / (camera->zNear - camera->zFar);
}