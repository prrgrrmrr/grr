#ifndef GRR_ASSETS_H
#define GRR_ASSETS_H

#include "logging.h"
#include "types.h"
#include "utils.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

// Khronos glTF 2.0: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.pdf

//  JSON glTF files SHOULD use .gltf extension and model/gltf+json Media Type.
// * glTF files stored in GLB container SHOULD use .glb extension and
// model/gltf-binary Media Type.

// * Files representing binary buffers SHOULD use either:
// -  .bin file extension with application/octet-stream Media Type;
// -  .bin, .glbin, or .glbuf file extensions with application/gltf-buffer Media
// Type.

// * PNG images SHOULD use .png file extension with image/png Media Type;
// -  PNG images SHOULD NOT contain animations, non-square pixel ratios, or
// embedded ICC profiles. Such features, if present, MUST be ignored by client
// implementations.

// * JPEG images SHOULD use .jpeg or .jpg file extensions with image/jpeg Media
// Type
// -  JPEG images MUST be compatible with JPEG File Interchange Format.
// -  JPEG images SHOULD NOT contain embedded ICC profiles. If present, embedded
// ICC profiles MUST be ignored by client implementations.
// -  Exchangeable image file format (Exif) chunks MAY be ignored by client
// implementations.

// * JSON
// - glTF JSON data SHOULD be written with UTF-8 encoding without BOM
// - ASCII characters stored in glTF JSON SHOULD be written without JSON
// escaping
// - Non-ASCII characters stored in glTF JSON MAY be escaped. Property names
// (keys) within JSON objects SHOULD be unique. glTF client implementations
// SHOULD override lexically preceding values for the same key.
// - Some of glTF properties are defined as integers in the schema. Such values
// MAY be stored as decimals with a zero fractional part or by using exponent
// notation. Regardless of encoding, such properties MUST NOT contain any
// non-zero fractional value.
// - Non-integer numbers SHOULD be written in a way that preserves original
// values when these numbers are read back, i.e., they SHOULD NOT be altered by
// JSON serialization / deserialization roundtrip: This is typically achieved
// with algorithms like Grisu2 used by common JSON libraries.

// * ASSETS
// - Data URIs that embed binary resources in the glTF JSON as defined by
// the RFC 2397. The Data URI’s mediatype field MUST match the encoded
// content.
// - Relative paths — path-noscheme or ipath-noscheme as defined by RFC
// 3986, Section 4.2 or RFC 3987, Section 2.2 — without scheme, authority,
// or parameters. Reserved characters (as defined by RFC 3986, Section 2.2.
// and RFC 3987, Section 2.2.) MUST be percent-encoded.
// - URIs SHOULD undergo syntax-based normalization as defined by RFC 3986,
// Section 6.2.2, RFC 3987, Section 5.3.2, and applicable schema rules (e.g.,
// RFC 7230, Section 2.7.3 for HTTP) on export and/or import.

// * Coordinate system
// - glTF uses a right-handed coordinate system. glTF defines +Y as up, +Z as
// forward, and -X as right; the front of a glTF asset faces +Z.
// - The units for all linear distances are meters.
// - All angles are in radians.
// - Positive rotation is counterclockwise.

typedef struct GrrVertex {
  Grr_f32 position[3];
  Grr_f32 color[4];
  Grr_f32 textureCoordinates[2];
} GrrVertex;

VkVertexInputBindingDescription Grr_getBindingDescription();
VkVertexInputAttributeDescription *
Grr_getAtributeDescriptions(Grr_u32 *attributeDescriptionCount);

typedef struct GrrModel {
  GrrVertex *vertices;
  Grr_u32 vertexCount;
  Grr_u32 *indices;
  Grr_u32 indexCount;
} GrrModel;

// glTF
GrrHashMap *Grr_glTFLoad(const Grr_string path);

// Images
Grr_byte *Grr_loadPNG(const Grr_string path, Grr_u32 *nReadbytes, Grr_u32 *w,
                      Grr_u32 *h);

#endif