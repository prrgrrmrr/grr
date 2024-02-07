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

VkVertexInputBindingDescription *
Grr_getBindingDescriptions(Grr_u32 *bindingDescriptionCount);
VkVertexInputAttributeDescription *
Grr_getAtributeDescriptions(Grr_u32 *attributeDescriptionCount);

typedef struct GrrModel {
  // Vertex data
  Grr_u32 vertexCount;
  Grr_f32 *positions;          // Vertex XYZ
  Grr_f32 *colors;             // Vertex RGB
  Grr_f32 *textureCoordinates; // Vertex UV

  // Index data
  Grr_u32 indexCount;
  Grr_u32 *indices; // Vertex indices
} GrrModel;

typedef struct GrrBufferView {
  Grr_u32 bufferIndex;
  Grr_u32 nBytes;
  Grr_u32 offset;
  Grr_i32 stride; // -1 if not defined
  Grr_u16 target;
} GrrBufferView;

typedef enum GRR_ACCESSOR_COMPONENT_TYPE {
  COMPONENT_TYPE_SIGNED_BYTE = 5120,    // 8 bits
  COMPONENT_TYPE_UNSIGNED_BYTE = 5121,  // 8 bits
  COMPONENT_TYPE_SIGNED_SHORT = 5122,   // 16 bits
  COMPONENT_TYPE_UNSIGNED_SHORT = 5123, // 16 bits
  COMPONENT_TYPE_UNSIGNED_INT = 5125,   // 32 bits
  COMPONENT_TYPE_FLOAT = 5126,          // 32 bits (signed)
} GRR_ACCESSOR_COMPONENT_TYPE;

Grr_byte
Grr_bytesPerglTFComponentType(GRR_ACCESSOR_COMPONENT_TYPE componentType);

typedef enum GRR_ACCESSOR_ELEMENT_TYPE {
  ELEMENT_TYPE_SCALAR,
  ELEMENT_TYPE_VEC2,
  ELEMENT_TYPE_VEC3,
  ELEMENT_TYPE_VEC4,
  ELEMENT_TYPE_MAT2X2,
  ELEMENT_TYPE_MAT3X3,
  ELEMENT_TYPE_MAT4X4
} GRR_ACCESSOR_ELEMENT_TYPE;

typedef struct GrrSparseAccessor {
  Grr_u32 count; //  Number of displaced elements
  // Indices: location and the component type of indices of values to be
  // replaced
  Grr_u32 indicesBufferViewIndex;
  Grr_u32 indicesByteOffset;
  GRR_ACCESSOR_COMPONENT_TYPE indicesComponentType;
  // Values: location of new values to put in the indices previously described
  Grr_u32 valuesBufferViewIndex;
  Grr_u32 valuesByteOffset;
} GrrSparseAccessor;

typedef struct GrrAccessor {
  Grr_i32 bufferViewIndex; // signed because -1 is error flag
  Grr_u32 byteOffset;      // Start in buffer view pointed to by vufferViewIndex
  Grr_u32 count;           // Element count
  GRR_ACCESSOR_ELEMENT_TYPE
  type; // Element type: "SCALAR" (1 component) "VEC2" (2 components)
        // "VEC3" (3 components) "VEC4" (4 components) "MAT2" (4
        // components) "MAT3" (9 components) "MAT4" (16 components)
  GRR_ACCESSOR_COMPONENT_TYPE
  componentType; // Component type in element
  GrrSparseAccessor
      *sparseAccessor; // Populated if accessor is sparse
                       // When accessor.bufferView is undefined,
                       // the sparse accessor is initialized as an array of
                       // zeros of size (size of the accessor element) *
                       // (accessor.count) bytes
} GrrAccessor;

typedef struct GrrMeshPrimitive {
  Grr_i32 verticesAccessorIndex; // POSITION
  Grr_i32 normalsAccessorIndex;  // NORMAL
  Grr_i32 tangentsAccessorIndex; // TANGNET
  // TODO: TEXCOORD_n, COLOR_n, JOINTS_n, WEIGHTS_n
  Grr_i32
      indicesAccessorIndex; // For indexed primitives: useful for cutting number
                            // of vertices to render by reusing their indices
  // TODO: material, mode
} GrrMeshPrimitive;

typedef struct GrrMesh {
  Grr_u32 primitiveCount; // Count of primitives that make up this mesh
  GrrMeshPrimitive *primitives;
} GrrMesh;

// glTF
typedef struct GrrAssetglTF {
  Grr_byte **buffers;         // Array of data buffers
  GrrBufferView *bufferViews; // Buffer views
  GrrAccessor *accessors;     // Buffer view accessors
  GrrMesh *meshes;            // List of meshes
  Grr_u32 scene;              // Default scene
} GrrAssetglTF;

GrrAssetglTF *Grr_glTFLoad(const Grr_string path);
// TODO: Grr_freeglTF(GrrAssetglTF *glTF);

void Grr_modelFromAsset(GrrModel *model, GrrAssetglTF *gltf, Grr_u32 meshIndex,
                        Grr_u32 primitiveIndex);

// Images
Grr_byte *Grr_loadPNG(const Grr_string path, Grr_u32 *nReadbytes, Grr_u32 *w,
                      Grr_u32 *h);

#endif