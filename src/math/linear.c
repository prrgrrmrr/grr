#include "linear.h"

void Grr_identity(GrrMatrix4x4 *matrix) {
  memset(matrix->data, 0, sizeof(matrix->data));
  matrix->data[0] = 1.0f;
  matrix->data[5] = 1.0f;
  matrix->data[10] = 1.0f;
  matrix->data[15] = 1.0f;
}