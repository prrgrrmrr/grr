#include "assets.h"

VkVertexInputBindingDescription *
Grr_getBindingDescriptions(Grr_u32 *bindingDescriptionCount) {
  *bindingDescriptionCount = 1; // Update if necessary

  VkVertexInputBindingDescription *bindingDescriptions =
      (VkVertexInputBindingDescription *)malloc(
          sizeof(VkVertexInputBindingDescription) * (*bindingDescriptionCount));

  // Positions
  bindingDescriptions[0].binding = 0;
  bindingDescriptions[0].stride = sizeof(Grr_f32) * 3;
  bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  // // Colors
  // bindingDescriptions[1].binding = 1;
  // bindingDescriptions[1].stride = sizeof(Grr_f32) * 3;
  // bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  // // Texture coordinates
  // bindingDescriptions[2].binding = 2;
  // bindingDescriptions[2].stride = sizeof(Grr_f32) * 2;
  // bindingDescriptions[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return bindingDescriptions;
}

VkVertexInputAttributeDescription *
Grr_getAtributeDescriptions(Grr_u32 *attributeDescriptionCount) {
  *attributeDescriptionCount = 1; // Update if necessary

  VkVertexInputAttributeDescription *attributeDescriptions =
      (VkVertexInputAttributeDescription *)malloc(
          sizeof(VkVertexInputAttributeDescription) *
          (*attributeDescriptionCount));
  if (attributeDescriptions == NULL) {
    GRR_LOG_CRITICAL("Failed to allocate memory for attribute descriptions\n");
    return NULL;
  }

  // Positions
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = 0;

  // // Colors
  // attributeDescriptions[1].binding = 1;
  // attributeDescriptions[1].location = 1;
  // attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  // attributeDescriptions[1].offset = 0;

  // // Texture coordinates
  // attributeDescriptions[2].binding = 2;
  // attributeDescriptions[2].location = 2;
  // attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
  // attributeDescriptions[2].offset = 0;

  return attributeDescriptions;
}

// PNG decompression

Grr_byte _Grr_reconA(Grr_byte *bytes, Grr_u32 r, Grr_u32 c, Grr_u32 stride,
                     Grr_u32 bytesPerPixel) {
  return (c >= bytesPerPixel ? bytes[r * stride + c - bytesPerPixel] : 0);
}

Grr_byte _Grr_reconB(Grr_byte *bytes, Grr_u32 r, Grr_u32 c, Grr_u32 stride,
                     Grr_u32 bytesPerPixel) {
  return (r > 0 ? bytes[(r - 1) * stride + c] : 0);
}

Grr_byte _Grr_reconC(Grr_byte *bytes, Grr_u32 r, Grr_u32 c, Grr_u32 stride,
                     Grr_u32 bytesPerPixel) {
  return (r > 0 && c >= bytesPerPixel
              ? bytes[(r - 1) * stride + c - bytesPerPixel]
              : 0);
}

Grr_byte PaethPredictor(Grr_byte a, Grr_byte b, Grr_byte c) {
  Grr_i32 p = -c;
  p += a + b;
  Grr_u32 pa = (p > a ? p - a : a - p);
  Grr_u32 pb = (p > b ? p - b : b - p);
  Grr_u32 pc = (p > c ? p - c : c - p);
  Grr_byte pr;
  if (pa <= pb && pa <= pc)
    pr = a;
  else if (pb <= pc)
    pr = b;
  else
    pr = c;
  return pr;
}

Grr_byte *Grr_loadPNG(const Grr_string path, Grr_u32 *nReadBytes, Grr_u32 *w,
                      Grr_u32 *h) {
  // Parse and decode PNG data
  // glTF note: Any colorspace information (such as ICC profiles, intents,
  // gamma values, etc.) from PNG or JPEG images MUST be ignored.

  *nReadBytes = 0;

  size_t nBytes;
  Grr_byte *bytes = Grr_readBytesFromFile(path, &nBytes);
  if (bytes == NULL) {
    return NULL;
  }

  Grr_bool pngOk = true;
  Grr_byte magic[] = {0x89, 0x50, 0x4e, 0x47,
                      0x0d, 0x0a, 0x1a, 0x0a}; // First 8 bytes of PNG file

  size_t i = 0;
  for (; pngOk && i < 8; i++) {
    if (bytes[i] != magic[i]) {
      GRR_LOG_ERROR(
          "PNG magic bytes: expected byte %d to be (%d) but found (%d)\n", i,
          magic[i], bytes[i]);
      pngOk = false;
    }
  }

  Grr_u32 chunkSize;
  Grr_u32 crc;

  Grr_bool firstChunk = true;

#define CHUNK_SIZE()                                                           \
  (((Grr_u32)bytes[i] << 24) | ((Grr_u32)bytes[i + 1] << 16) |                 \
   ((Grr_u32)bytes[i + 2] << 8) | ((Grr_u32)bytes[i + 3]))

#define CRC()                                                                  \
  (((Grr_u32)bytes[i] << 24) | ((Grr_u32)bytes[i + 1] << 16) |                 \
   ((Grr_u32)bytes[i + 2] << 8) | ((Grr_u32)bytes[i + 3]))

#define WIDTH()                                                                \
  (((Grr_u32)bytes[i] << 24) | ((Grr_u32)bytes[i + 1] << 16) |                 \
   ((Grr_u32)bytes[i + 2] << 8) | ((Grr_u32)bytes[i + 3]))

#define HEIGHT()                                                               \
  (((Grr_u32)bytes[i] << 24) | ((Grr_u32)bytes[i + 1] << 16) |                 \
   ((Grr_u32)bytes[i + 2] << 8) | ((Grr_u32)bytes[i + 3]))

#define CHUNK_IS_IHDR()                                                        \
  ((bytes[i] == 73) && (bytes[i + 1] == 72) && (bytes[i + 2] == 68) &&         \
   (bytes[i + 3] == 82))

#define CHUNK_IS_IEND()                                                        \
  ((bytes[i] == 73) && (bytes[i + 1] == 69) && (bytes[i + 2] == 78) &&         \
   (bytes[i + 3] == 68))

#define CHUNK_IS_IDAT()                                                        \
  ((bytes[i] == 73) && (bytes[i + 1] == 68) && (bytes[i + 2] == 65) &&         \
   (bytes[i + 3] == 84))

  Grr_u32 width, height; // In pixels
  Grr_byte bitDepth, colorType;
  size_t compressedSize = 0;

  while (pngOk && i < nBytes) {
    // Assume remaining bytes represent sequence of chunks starting with IHDR
    // chunk and ending with an IEND chunk

    chunkSize = CHUNK_SIZE();
    i += 4; // Jump to chunk type

    if (firstChunk) {
      if (!CHUNK_IS_IHDR()) {
        GRR_LOG_ERROR("PNG: first chunk is not IHDR\n");
        pngOk = false;
        break;
      }
    }

    if (CHUNK_IS_IEND())
      break;

    Grr_bool IDATChunk = CHUNK_IS_IDAT();

    i += 4; // Jump to chunk data
    if (firstChunk) {
      firstChunk = false;
      width = WIDTH();
      *w = width;
      i += 4;
      height = HEIGHT();
      *h = height;
      i += 4;
      if (width == 0 || height == 0) {
        GRR_LOG_ERROR("PNG: image dimensions are not allowed to be zero\n");
        pngOk = false;
        break;
      }
      GRR_LOG_DEBUG("PNG width (%u) height (%u)\n", width, height);
      bitDepth = bytes[i];
      colorType = bytes[i + 1];

      if (bitDepth != 8) {
        GRR_LOG_ERROR("PNG: only bit depth 8 is supported\n");
        pngOk = false;
        break;
      }

      if (colorType != 6) {
        GRR_LOG_ERROR("PNG: only support truecolor with alpha: pixel is a RGB "
                      "triple followed by an alpha sample\n");
        pngOk = false;
        break;
      }

      if (bytes[i + 2] != 0) {
        // Compression method
        GRR_LOG_ERROR("PNG: invalid compression method\n");
        pngOk = false;
        break;
      } else if (bytes[i + 3] != 0) {
        // Filter method
        GRR_LOG_ERROR("PNG: invalid filter method\n");
        pngOk = false;
        break;
      } else if (bytes[i + 4] != 0) {
        // Adam7 interlacing if 1
        GRR_LOG_ERROR("PNG: interlacing is not suuported\n");
        pngOk = false;
        break;
      } else {
        i += chunkSize - 8;
      }
    } else if (IDATChunk) {
      // Compressed pixel data
      // for (int ii = 0; ii < chunkSize; ii++) {
      //   printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(bytes[i + ii]));
      // }
      memmove(bytes + compressedSize, bytes + i, chunkSize);
      compressedSize += chunkSize;
      i += chunkSize;
    } else
      i += chunkSize; // Jump to CRC

    crc = CRC(); // Cyclic redundancy code is calculated on chunk type and
    // chunk data fields
    // TODO perform error checking on chunk
    // [bytes+i-chunkSize,bytes+i)
    printf("CRC %u\n", crc);
    i += 4;
  }

  Grr_byte *decoded = NULL;
  if (pngOk) {
    if (!CHUNK_IS_IEND()) {
      GRR_LOG_ERROR("PNG: last chunk is not IEND\n");
    } else {
      // Decompress deflate stream
      GRR_LOG_DEBUG("DEFLATE: compressed length (%d bytes)\n", compressedSize);
      size_t decodedSize;
      decoded = Grr_inflate(bytes, compressedSize, &decodedSize);
      GRR_LOG_DEBUG("DEFLATE: decompressed length (%u bytes)\n", decodedSize);

      // Defilter
      Grr_u32 bytesPerPixel = 4;
      Grr_u32 stride = bytesPerPixel * width;
      Grr_u32 currentByte = 0;
      Grr_u32 writePosition = 0;
      Grr_byte filterType;
      for (Grr_u32 r = 0; r < height; r++) {
        filterType = decoded[currentByte++];
        for (Grr_u32 c = 0; c < stride; c++) {

          switch (filterType) {
          case 0:
            decoded[writePosition++] = decoded[currentByte++];
            break;

          case 1:
            decoded[writePosition++] =
                (decoded[currentByte++] +
                 _Grr_reconA(decoded, r, c, stride, bytesPerPixel)) &
                0xFF;
            break;

          case 2:
            decoded[writePosition++] =
                (decoded[currentByte++] +
                 _Grr_reconB(decoded, r, c, stride, bytesPerPixel)) &
                0xFF;
            break;

          case 3:
            decoded[writePosition++] =
                (decoded[currentByte++] +
                 (Grr_u32)floor(
                     (_Grr_reconA(decoded, r, c, stride, bytesPerPixel) +
                      _Grr_reconB(decoded, r, c, stride, bytesPerPixel)) /
                     2.0)) &
                0xFF;
            break;

          case 4:
            decoded[writePosition++] =
                (decoded[currentByte++] +
                 PaethPredictor(
                     _Grr_reconA(decoded, r, c, stride, bytesPerPixel),
                     _Grr_reconB(decoded, r, c, stride, bytesPerPixel),
                     _Grr_reconC(decoded, r, c, stride, bytesPerPixel))) &
                0xFF;
            break;

          default:
            GRR_LOG_ERROR("PNG: unknown filtering method (%u)\n", filterType);
            exit(EXIT_FAILURE);
            break;
          }
        }
      }
      *nReadBytes = writePosition;
    }
  }

  if (bytes != NULL)
    free(bytes);

  return decoded;

#undef CHUNK_SIZE
#undef CRC
#undef WIDTH
#undef HEIGHT
#undef CHUNK_IS_IHDR
#undef CHUNK_IS_IEND
#undef CHUNK_IS_IDA
}

// * JSON grammar: https://www.rfc-editor.org/rfc/pdfrfc/rfc8259.txt.pdf and
// https://www.json.org/json-en.html
// - JSON text encoding UTF-8: https://datatracker.ietf.org/doc/html/rfc3629
// - JSON can represent four primitive types (strings, numbers, booleans, and
// null) and two structured types (objects and arrays)
// - A string is a sequence of zero or more Unicode characters [UNICODE]
// - An object is an unordered collection of zero or more name/value pairs,
// where a name is a string and a value is a string, number, boolean, null,
// object, or array.
// - An array is an ordered sequence of zero or more values.

typedef enum GRR_JSON_TOKEN {
  // Structural characters
  LEFT_BRACE = 0x007B,
  RIGHT_BRACE = 0x007D,
  LEFT_BRACKET = 0x005B,
  RIGHT_BRACKET = 0x005D,
  COMMA = 0x002C,
  COLON = 0x003A,

  // Literals

  // true
  LOWERCASE_T = 0x0074,
  LOWERCASE_R = 0x0072,
  LOWERCASE_U = 0x0075,
  LOWERCASE_E = 0x0065,
  LOWERCASE_F = 0x0066,
  LOWERCASE_A = 0x0061,
  LOWERCASE_L = 0x006C,
  LOWERCASE_S = 0x0073,
  LOWERCASE_N = 0x006E,

  // Strings
  QUOTATION_MARK = 0x0022,
  SLASH = 0x002F,
  BACKSLASH = 0x005C,
  LOWERCASE_B = 0x0062,
  // f defined above
  // n defined above
  // r defined above
  // t defined above
  // u defined above

  // Numbers
  MINUS = 0x002D,
  PLUS = 0x002B,
  DECIMAL_DIGIT,
  NON_ZERO_DECIMAL_DIGIT,
  HEX_DIGIT,
  // Lowercase e defined above
  UPPERCASE_E = 0x0045,
  DECIMAL_POINT = 0x002E,
  UNICODE_CODEPOINT,

  // Whitespace
  SPACE = 0x0020,
  LINE_FEED = 0x000A,
  CARRIAGE_RETURN = 0x000D,
  HORIZONTAL_TAB = 0x0009

} GRR_JSON_TOKEN;

typedef enum GRR_JSON_TYPE {
  GRR_JSON_OBJECT,
  GRR_JSON_ARRAY,
  GRR_JSON_TRUE_LITERAL_T,
  GRR_JSON_TRUE_LITERAL_R,
  GRR_JSON_TRUE_LITERAL_U,
  GRR_JSON_TRUE_LITERAL_E,
  GRR_JSON_FALSE_LITERAL_F,
  GRR_JSON_FALSE_LITERAL_A,
  GRR_JSON_FALSE_LITERAL_L,
  GRR_JSON_FALSE_LITERAL_S,
  GRR_JSON_FALSE_LITERAL_E,
  GRR_JSON_NULL_LITERAL_N,
  GRR_JSON_NULL_LITERAL_U,
  GRR_JSON_NULL_LITERAL_L,
  // GRR_JSON_null_LITERAL_L
  GRR_JSON_NULL_LITERAL,
  GRR_JSON_NUMBER,
  GRR_JSON_STRING
} GRR_JSON_TYPE;

Grr_bool _Grr_isWhitespace(Grr_u32 codePoint) {
  // Whitespace
  return codePoint == SPACE || codePoint == LINE_FEED ||
         codePoint == CARRIAGE_RETURN || codePoint == HORIZONTAL_TAB;
}

Grr_bool _Grr_isDecimalDigit(Grr_u32 codePoint) {
  return codePoint >= 0x0030 && codePoint <= 0x0039; // 0-9
}

Grr_bool _Grr_isNonZeroDecimalDigit(Grr_u32 codePoint) {
  return codePoint >= 0x0031 && codePoint <= 0x0039; // 1-9
}

Grr_bool _Grr_isHexDigit(Grr_u32 codePoint) {
  return (codePoint >= 0x0030 && codePoint <= 0x0039) || // 0-9
         (codePoint >= 0x0041 && codePoint <= 0x0046) || // Uppercase A-F
         (codePoint >= 0x0061 && codePoint <= 0x0066);   // Lowercase a-f
}

// Update current string

char previousJSONString[256]; // Previous JSON string
char currentJSONString[256];  // Current JSON string
Grr_u16 lastPushed;           // Last pushed on stack
Grr_u16 previouslyPushed;     // Previous to last
Grr_u16 currentJSONStringLength;
void _Grr_beingString() {
  if (currentJSONStringLength > 0) {
    strncpy(previousJSONString, currentJSONString, currentJSONStringLength);
    previousJSONString[currentJSONStringLength] = '\0';
  }
  currentJSONString[0] = '\0';
  currentJSONStringLength = 0;
}

void _Grr_updateJSONString(Grr_u32 codePoint) {
  if (codePoint <= 0x7F) {
    currentJSONString[currentJSONStringLength++] = codePoint;
  } else {
    currentJSONString[currentJSONStringLength++] = '\\';
    currentJSONString[currentJSONStringLength++] = 'u';
    sprintf(currentJSONString + currentJSONStringLength, "%4X", codePoint);
    currentJSONStringLength += 4;
  }
}

void _Grr_endJSONString(GrrList *stack, GrrList *objStack,
                        Grr_bool isHashValue) {
  currentJSONString[currentJSONStringLength++] = '\0';

  GrrType type;
  Grr_u16 topJSONType = Grr_listGetAtIndex(stack, stack->count - 1, NULL)->u16;
  GrrHashMapValue *topJSONValue =
      Grr_listGetAtIndex(objStack, objStack->count - 1, &type);

  GrrHashMapValue stringValue;
  stringValue.string = (Grr_string)malloc(currentJSONStringLength + 1);
  strncpy(stringValue.string, currentJSONString, currentJSONStringLength);
  stringValue.string[currentJSONStringLength] = '\0';

  if (topJSONType == GRR_JSON_ARRAY) {
    assert(type == LIST);
    assert(isHashValue == false);
    Grr_listPushBack(topJSONValue->list, stringValue, STRING);
  } else if (topJSONType == GRR_JSON_OBJECT) {
    assert(type == HASH_MAP);
    if (isHashValue) {
      Grr_hashMapPut(topJSONValue->map, previousJSONString, stringValue,
                     STRING);
    }
  }
}

// JSON object handling

GrrHashMap *_Grr_beginJSONObject(GrrList *objStack) {
  GrrHashMap *obj = (GrrHashMap *)malloc(sizeof(GrrHashMap));
  Grr_initHashMap(obj);

  GrrHashMapValue value;
  value.map = obj;
  Grr_listPushBack(objStack, value, HASH_MAP);
  GRR_LOG_DEBUG("Push new hashmap (@ %p) to stack\n", obj);
  assert(Grr_listGetAtIndex(objStack, objStack->count - 1, NULL)->map == obj);

  return obj;
}

void _Grr_endJSONObject(GrrList *objStack) { Grr_listPop(objStack); }

// JSON list handling

GrrList *_Grr_beginJSONList(GrrList *objStack) {
  GrrList *list = (GrrList *)malloc(sizeof(GrrList));
  Grr_initList(list);

  GrrHashMapValue value;
  value.list = list;
  Grr_listPushBack(objStack, value, LIST);
  GRR_LOG_DEBUG("Push new list (@ %p) to stack\n", list);

  return list;
}

void _Grr_endJSONList(GrrList *objStack) { Grr_listPop(objStack); }

// JSON number handling

char number[256]; // Current nubmer string
Grr_u16 numberLength;
Grr_bool numberIsFloat;

void _Grr_beginJSONNumber() {
  numberLength = 0;
  numberIsFloat = false;
}

void _Grr_updateJSONNumber(Grr_u32 codePoint) {
  switch (codePoint) {
  case UPPERCASE_E:
  case LOWERCASE_E:
  case DECIMAL_POINT:
    numberIsFloat = true;

  default:
    break;
  }
  number[numberLength++] = codePoint;
}

void _Grr_endJSONNumber(GrrList *stack, GrrList *objStack) {

  number[numberLength++] = '\0';

  GrrType type;
  Grr_u16 topJSONType = Grr_listGetAtIndex(stack, stack->count - 1, NULL)->u16;
  GrrHashMapValue *topJSONValue =
      Grr_listGetAtIndex(objStack, objStack->count - 1, &type);

  GrrHashMapValue numberValue;
  GrrType numberType;
  if (numberIsFloat) {
    numberValue.f64 = atof(number);
    numberType = FLOAT64;
  } else {
    numberValue.i64 = atoll(number);
    numberType = INT64;
  }

  if (topJSONType == GRR_JSON_ARRAY) {
    assert(type == LIST);
    Grr_listPushBack(topJSONValue->list, numberValue, numberType);
  } else if (topJSONType == GRR_JSON_OBJECT) {
    assert(type == HASH_MAP);
    Grr_hashMapPut(topJSONValue->map, currentJSONString, numberValue,
                   numberType);
  }
}

// JSON true/false/null literal handling
void _Grr_endJSONLiteral(GrrList *stack, GrrList *objStack, Grr_byte value) {
  GrrType type;
  Grr_u16 topJSONType = Grr_listGetAtIndex(stack, stack->count - 1, NULL)->u16;
  GrrHashMapValue *topJSONValue =
      Grr_listGetAtIndex(objStack, objStack->count - 1, &type);

  GrrHashMapValue literalValue;
  GrrType literalType = BOOLEAN;

  if (value == 0)
    literalValue.boolean = false;
  else if (value == 1)
    literalValue.boolean = true;
  else
    literalType = NULL_TYPE;

  if (topJSONType == GRR_JSON_ARRAY) {
    assert(type == LIST);
    Grr_listPushBack(topJSONValue->list, literalValue, literalType);
  } else if (topJSONType == GRR_JSON_OBJECT) {
    assert(type == HASH_MAP);
    Grr_hashMapPut(topJSONValue->map, currentJSONString, literalValue,
                   literalType);
  }
}

GrrAssetglTF *_Grr_glTFFromJSON(GrrHashMap *json, Grr_string assetDir) {
  GrrAssetglTF *glTF = (GrrAssetglTF *)malloc(sizeof(GrrAssetglTF));
  if (NULL == glTF) {
    GRR_LOG_ERROR("glTF: failed to allocate memory for GrrAsset\n");
    return NULL;
  }

  GrrType type;
  GrrHashMapValue *buffersList = Grr_hashMapGet(json, "buffers", &type);
  assert(type == LIST);
  glTF->buffers =
      (Grr_byte **)malloc(sizeof(Grr_byte *) * buffersList->list->count);
  if (NULL == glTF->buffers) {
    GRR_LOG_ERROR("glTF: failed to allocate memory for buffer list\n");
    return NULL;
  }

  GrrHashMapValue *value;
  size_t nBytes;

  // Prep path
  size_t lenAssetDir = strlen(assetDir);
  size_t lenURI;
  Grr_string uri;
  char pathURI[256];
  memcpy(pathURI, assetDir, lenAssetDir);

  // Load buffer binary data
  for (Grr_u32 i = 0; i < buffersList->list->count; i++) {
    value = Grr_listGetAtIndex(buffersList->list, i, &type);
    assert(type == HASH_MAP);
    assert(Grr_hashMapGet(value->map, "byteLength", NULL) != NULL);
    assert(Grr_hashMapGet(value->map, "uri", NULL) != NULL);
    uri = Grr_hashMapGet(value->map, "uri", NULL)->string;

    GRR_LOG_DEBUG("URI %s%s\n", assetDir,
                  Grr_hashMapGet(value->map, "uri", NULL)->string);
    lenURI = strlen(uri);
    if (lenURI >= 4 && (0 == strncmp("data:", pathURI, 5))) {
      // TODO: Data URI https://www.rfc-editor.org/rfc/pdfrfc/rfc2397.txt.pdf
      GRR_LOG_CRITICAL("glTF: parsing of data URIs not implemented\n");
      return NULL;
    } else {
      // Relative path
      // TODO: Relative paths — path-noscheme or ipath-noscheme as defined by
      // RFC 3986, Section 4.2 or RFC 3987, Section 2.2 — without scheme,
      // authority, or parameters. Reserved characters (as defined by RFC 3986,
      // Section 2.2. and RFC 3987, Section 2.2.) MUST be percent-encoded.
      memcpy(pathURI + lenAssetDir, uri, lenURI);
      pathURI[lenAssetDir + lenURI] = '\0';
      glTF->buffers[i] = Grr_readBytesFromFile(pathURI, &nBytes);
      assert(nBytes == Grr_hashMapGet(value->map, "byteLength", NULL)->i64);
      GRR_LOG_DEBUG("glTF: read buffer bytes (%llu)\n", nBytes);
    }
  }

  // Buffer views
  GrrHashMapValue *bufferViewList = Grr_hashMapGet(json, "bufferViews", &type);
  glTF->bufferViews = (GrrBufferView *)malloc(sizeof(GrrBufferView) *
                                              bufferViewList->list->count);
  if (NULL != glTF->bufferViews) {
    for (Grr_u32 i = 0; i < bufferViewList->list->count; i++) {
      value = Grr_listGetAtIndex(bufferViewList->list, i, &type);
      assert(type == HASH_MAP);
      glTF->bufferViews[i].bufferIndex =
          Grr_hashMapGet(value->map, "buffer", NULL)->i64;
      glTF->bufferViews[i].nBytes =
          Grr_hashMapGet(value->map, "byteLength", NULL)->i64;
      GrrHashMapValue *offsetValue =
          Grr_hashMapGet(value->map, "byteOffset", NULL);
      glTF->bufferViews[i].offset = (offsetValue ? offsetValue->i64 : 0);
      GrrHashMapValue *strideValue =
          Grr_hashMapGet(value->map, "byteStride", NULL);
      glTF->bufferViews[i].stride = (strideValue ? strideValue->i64 : -1);
      GrrHashMapValue *targetValue = Grr_hashMapGet(value->map, "target", NULL);
      if (targetValue)
        glTF->bufferViews[i].target = targetValue->i64;
    }
  } else {
    GRR_LOG_ERROR("glTF: failed to allocate memory for buffer views\n");
  }

  // Accessors
  // An accessor defines a method for retrieving data as typed arrays from
  // within a buffer view. The accessor specifies a component type (e.g., float)
  // and a data type (e.g., VEC3 for 3D vectors), which when combined define the
  // complete data type for each data element
  GrrHashMapValue *accessorList = Grr_hashMapGet(json, "accessors", &type);
  glTF->accessors =
      (GrrAccessor *)malloc(sizeof(GrrAccessor) * accessorList->list->count);
  if (NULL != glTF->accessors) {
    Grr_string elementTypeString;
    for (Grr_u32 i = 0; i < accessorList->list->count; i++) {
      value = Grr_listGetAtIndex(accessorList->list, i, &type);
      assert(type == HASH_MAP);
      // Buffer view index
      GrrHashMapValue *bufferView =
          Grr_hashMapGet(value->map, "bufferView", NULL);
      glTF->accessors[i].bufferViewIndex = (bufferView ? bufferView->i64 : -1);
      // Offset within buffer
      GrrHashMapValue *offset = Grr_hashMapGet(value->map, "byteOffset", NULL);
      glTF->accessors[i].byteOffset = (offset ? offset->i64 : 0);
      // Element count
      glTF->accessors[i].count = Grr_hashMapGet(value->map, "count", NULL)->i64;
      // Parse element type
      elementTypeString = Grr_hashMapGet(value->map, "type", NULL)->string;
      if (0 == strcmp(elementTypeString, "SCALAR"))
        glTF->accessors[i].type = ELEMENT_TYPE_SCALAR;
      else if (0 == strcmp(elementTypeString, "VEC2"))
        glTF->accessors[i].type = ELEMENT_TYPE_VEC2;
      else if (0 == strcmp(elementTypeString, "VEC3"))
        glTF->accessors[i].type = ELEMENT_TYPE_VEC3;
      else if (0 == strcmp(elementTypeString, "VEC4"))
        glTF->accessors[i].type = ELEMENT_TYPE_VEC4;
      else if (0 == strcmp(elementTypeString, "MAT2"))
        glTF->accessors[i].type = ELEMENT_TYPE_MAT2X2;
      else if (0 == strcmp(elementTypeString, "MAT3"))
        glTF->accessors[i].type = ELEMENT_TYPE_MAT3X3;
      else if (0 == strcmp(elementTypeString, "MAT4"))
        glTF->accessors[i].type = ELEMENT_TYPE_MAT4X4;
      // Component type
      glTF->accessors[i].componentType =
          Grr_hashMapGet(value->map, "componentType", NULL)->i64;
      // Handle sparse accessor
      GrrHashMapValue *sparse = Grr_hashMapGet(value->map, "sparse", NULL);
      if (NULL != sparse) {
        GRR_LOG_WARNING("Sparse");
        glTF->accessors[i].sparseAccessor =
            (GrrSparseAccessor *)malloc(sizeof(GrrSparseAccessor));
        if (NULL != glTF->accessors[i].sparseAccessor) {
          // Required properties
          glTF->accessors[i].sparseAccessor->count =
              Grr_hashMapGet(sparse->map, "count", NULL)->i64;
          GrrHashMap *indicesObj =
              Grr_hashMapGet(sparse->map, "indices", NULL)->map;
          GrrHashMap *valuesObj =
              Grr_hashMapGet(sparse->map, "values", NULL)->map;

          // Sub-properties
          // Indices
          GrrHashMapValue *tmpValue;
          tmpValue = Grr_hashMapGet(indicesObj, "bufferView", NULL);
          glTF->accessors[i].sparseAccessor->indicesBufferViewIndex =
              tmpValue->i64;
          tmpValue = Grr_hashMapGet(indicesObj, "byteOffset", NULL);
          glTF->accessors[i].sparseAccessor->indicesByteOffset =
              (tmpValue ? tmpValue->i64 : 0);
          glTF->accessors[i].sparseAccessor->indicesComponentType =
              Grr_hashMapGet(indicesObj, "componentType", NULL)->i64;

          // Values
          tmpValue = Grr_hashMapGet(valuesObj, "bufferView", NULL);
          glTF->accessors[i].sparseAccessor->valuesBufferViewIndex =
              tmpValue->i64;
          tmpValue = Grr_hashMapGet(indicesObj, "byteOffset", NULL);
          glTF->accessors[i].sparseAccessor->valuesByteOffset =
              (tmpValue ? tmpValue->i64 : 0);
        } else {
          GRR_LOG_ERROR("glTF: failed to allocate memory for sparse accessor "
                        "properties\n");
        }
      } else {
        glTF->accessors[i].sparseAccessor = NULL;
      }
    }
  } else {
    GRR_LOG_ERROR("glTF: failed to allocate memory for accessors\n");
  }

  // List of meshes
  GrrHashMapValue *meshList = Grr_hashMapGet(json, "meshes", &type);
  glTF->meshes = (GrrMesh *)malloc(sizeof(GrrMesh) * meshList->list->count);
  if (NULL != glTF->meshes) {
    for (Grr_u32 i = 0; i < meshList->list->count; i++) {
      value = Grr_listGetAtIndex(meshList->list, i, &type);
      assert(type == HASH_MAP);
      value = Grr_hashMapGet(value->map, "primitives",
                             &type); // Reuse value here (ignoring mesh name)
      assert(type == LIST);
      glTF->meshes[i].primitiveCount = value->list->count;
      glTF->meshes[i].primitives = (GrrMeshPrimitive *)malloc(
          sizeof(GrrMeshPrimitive) * value->list->count);
      if (NULL != glTF->meshes[i].primitives) {
        for (Grr_u32 j = 0; j < value->list->count; j++) {
          GrrHashMap *primitiveObj =
              Grr_listGetAtIndex(value->list, j, NULL)->map;
          GrrHashMap *attributesObj =
              Grr_hashMapGet(primitiveObj, "attributes", NULL)->map;
          GrrHashMapValue *attribute =
              Grr_hashMapGet(attributesObj, "POSITION", NULL);
          glTF->meshes[i].primitives[j].verticesAccessorIndex =
              (attribute ? attribute->i64 : -1);
          // TODO: other attributes
          // Indexed primitive
          GrrHashMapValue *indicesValue =
              Grr_hashMapGet(primitiveObj, "indices", NULL);
          glTF->meshes[i].primitives[j].indicesAccessorIndex =
              (indicesValue ? indicesValue->i64 : -1);
          // TODO: material, mode
        }
      } else {
        GRR_LOG_ERROR("glTF: failed to allocate memory for mesh primitives\n");
      }
    }
  } else {
    GRR_LOG_ERROR("glTF: failed to allocate memory for meshes\n");
  }

  // Default scene
  value = Grr_hashMapGet(json, "scene", &type);
  if (NULL != value)
    glTF->scene = value->i64;
  else
    glTF->scene = 0; // Pick first scene if none specified

  // TODO: List of scenes

  return glTF;
}

GrrAssetglTF *Grr_glTFLoad(const Grr_string path) {
  // Load file as binary
  size_t nBytes;
  Grr_byte *bytes = Grr_readBytesFromFile(path, &nBytes);
  if (bytes == NULL || nBytes == 0) {
    GRR_LOG_ERROR("glTF: No bytes read from file\n");
    return false;
  }
  GRR_LOG_DEBUG("glTF: read %d bytes from file\n", nBytes);

  Grr_u32 i = 0;              // Current byte index
  Grr_byte byte;              // Current byte
  Grr_u32 codePoint;          // Last parsed UNICODE code point
  Grr_u32 previousCodePoint;  // Second to last UNICODE code point
  Grr_bool utf8Ok = true;     // Byte array is valid UTF-8
  Grr_bool fullyASCII = true; // Byte array consists of ASCII characters only
  Grr_bool jsonOk = true;     // JSON parses correctly

  // Detect presence of byte order mark at beginning of file, ignore if
  // present instead of reporting error
  if (nBytes >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF) {
    GRR_LOG_WARNING("BOM character present at beginning of file\n");
    i += 2;
  }

  // Stack of current JSON parse
  GrrList stack;           // JSON types
  GrrList objStack;        // JSON data
  GrrHashMap *json = NULL; // JSON result

  Grr_initList(&stack);
  Grr_initList(&objStack);

#define STACK_IS_EMPTY() (stack.count == 0)
#define STACK_TOP_IS(what)                                                     \
  (Grr_listGetAtIndex(&stack, stack.count - 1, NULL)->u16 == what)
#define STACK_POP() Grr_listPop(&stack)->u16;
#define STACK_PUSH(what)                                                       \
  {                                                                            \
    GrrHashMapValue value;                                                     \
    value.u16 = what;                                                          \
    Grr_listPushBack(&stack, value, UNSIGNED16);                               \
    previouslyPushed = lastPushed;                                             \
    lastPushed = what;                                                         \
  }

  while (jsonOk && i < nBytes) {
    byte = bytes[i];

    // The octet values C0, C1, F5 to FF never appear
    if (byte == 0xC0 || byte == 0xC1 || (byte >= 0xF5 && byte <= 0xFF)) {
      GRR_LOG_ERROR("UTF-8: octet values C0, C1, F5 to FF should never "
                    "appear (byte position %d)\n",
                    i);
      utf8Ok = false;
      break;
    }

    // US-ASCII character
    if (byte <= 0x7F) {
      codePoint = (Grr_u32)byte;
      i += 1;
    }

    // 2-byte character
    else if ((byte & 0xE0) == 0xC0) {
      if ((bytes[i + 1] & 0xC0) != 0x80) {
        GRR_LOG_ERROR("UTF-8: second byte in 2-byte character does not "
                      "look like 10xxxxxx");
        utf8Ok = false;
        break;
      }
      codePoint =
          ((Grr_u32)(byte & 0x1F) << 6) | (Grr_u32)(bytes[i + 1] & 0x3F);
      if (codePoint < (Grr_u32)0x0080 || codePoint > (Grr_u32)0x07FF) {
        GRR_LOG_ERROR(
            "UTF-8: code point (%u) is out of 2-byte character range\n",
            codePoint);
        utf8Ok = false;
        break;
      }
      i += 2;
      fullyASCII = false;
    }

    // 3-byte character
    else if ((byte & 0xF0) == 0xE0) {
      if ((bytes[i + 1] & 0xC0) != 0x80) {
        GRR_LOG_ERROR("UTF-8: second byte in 3-byte character does not "
                      "look like 10xxxxxx");
        utf8Ok = false;
        break;
      }

      if ((bytes[i + 2] & 0xC0) != 0x80) {
        GRR_LOG_ERROR("UTF-8: third byte in 3-byte character does not "
                      "look like 10xxxxxx");
        utf8Ok = false;
        break;
      }

      codePoint = ((Grr_u32)(byte & 0x0F) << 12) |
                  ((Grr_u32)(bytes[i + 1] & 0x3F) << 6) |
                  (Grr_u32)(bytes[i + 2] & 0x3F);

      if (codePoint < 0x0800 || codePoint > (Grr_u32)0xFFFF) {
        GRR_LOG_ERROR(
            "UTF-8: code point (%u) is out of 3-byte character range\n",
            codePoint);
        utf8Ok = false;
        break;
      }

      i += 3;
      fullyASCII = false;
    }

    // 4-byte character
    else if ((byte & 0xF8) == 0xF0) {
      if ((bytes[i + 1] & 0xC0) != 0x80) {
        GRR_LOG_ERROR("UTF-8: second byte in 4-byte character does not "
                      "look like 10xxxxxx");
        utf8Ok = false;
        break;
      }

      if ((bytes[i + 2] & 0xC0) != 0x80) {
        GRR_LOG_ERROR("UTF-8: third byte in 4-byte character does not "
                      "look like 10xxxxxx");
        utf8Ok = false;
        break;
      }

      if ((bytes[i + 3] & 0xC0) != 0x80) {
        GRR_LOG_ERROR("UTF-8: fourth byte in 4-byte character does not "
                      "look like 10xxxxxx");
        utf8Ok = false;
        break;
      }

      codePoint = ((Grr_u32)(byte & 0x07) << 18) |
                  ((Grr_u32)(bytes[i + 1] & 0x3F) << 12) |
                  ((Grr_u32)(bytes[i + 2] & 0x3F) << 6) |
                  (Grr_u32)(bytes[i + 3] & 0x3F);

      if (codePoint < (Grr_u32)0x00010000 || codePoint > (Grr_u32)0x0010FFFF) {
        GRR_LOG_ERROR(
            "UTF-8: code point (%u) is out of 4-byte character range\n",
            codePoint);
        utf8Ok = false;
        break;
      }

      i += 4;
      fullyASCII = false;
    } else {
      GRR_LOG_ERROR(
          "UTF-8: byte (%x) does not encode start of UTF-8 byte sequence\n",
          byte);
      utf8Ok = false;
      fullyASCII = false;
      break;
    }

    // Process code point
    if (false == _Grr_isWhitespace(codePoint)) {
      switch (codePoint) {
      case LEFT_BRACE:
        if (STACK_IS_EMPTY() || !STACK_TOP_IS(GRR_JSON_STRING)) {
          if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NUMBER)) {
            STACK_POP();
            GRR_LOG_DEBUG("End JSON number\n");
            _Grr_endJSONNumber(&stack, &objStack);
          }
          GRR_LOG_DEBUG("Begin JSON object\n");
          if (NULL == json)
            json = _Grr_beginJSONObject(&objStack);
          else if (STACK_TOP_IS(GRR_JSON_ARRAY)) {
            GrrType type;
            GrrHashMapValue *value =
                Grr_listGetAtIndex(&objStack, objStack.count - 1, &type);
            assert(type == LIST);
            GrrHashMapValue objValue;
            objValue.map = _Grr_beginJSONObject(&objStack);
            Grr_listPushBack(value->list, objValue, HASH_MAP);
          } else if (STACK_TOP_IS(GRR_JSON_OBJECT)) {
            GrrType type;
            GrrHashMapValue *value =
                Grr_listGetAtIndex(&objStack, objStack.count - 1, &type);
            assert(type == HASH_MAP);
            GrrHashMapValue objValue;
            objValue.map = _Grr_beginJSONObject(&objStack);
            Grr_hashMapPut(value->map, currentJSONString, objValue, HASH_MAP);
          }

          STACK_PUSH(GRR_JSON_OBJECT);
        } else {
          // Inside string
          _Grr_updateJSONString(codePoint);
        }
        break;

      case RIGHT_BRACE:
        if (!STACK_IS_EMPTY() && !STACK_TOP_IS(GRR_JSON_STRING)) {
          if (STACK_TOP_IS(GRR_JSON_NUMBER)) {
            STACK_POP();
            GRR_LOG_DEBUG("End JSON number\n");
            _Grr_endJSONNumber(&stack, &objStack);
          }
          if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_OBJECT)) {
            STACK_POP(); // End object
            GRR_LOG_DEBUG("End JSON object\n");
            _Grr_endJSONObject(&objStack);
          } else {
            GRR_LOG_ERROR("JSON is not valid: expected object\n"); // Error
            jsonOk = false;
          }
        } else if (!STACK_IS_EMPTY()) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR(
              "JSON is not valid: expected objet or string\n"); // Error
          jsonOk = false;
        }
        break;

      case LEFT_BRACKET:
        if (STACK_IS_EMPTY() || !STACK_TOP_IS(GRR_JSON_STRING)) {
          if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NUMBER)) {
            STACK_POP();
            GRR_LOG_DEBUG("End JSON number\n");
            _Grr_endJSONNumber(&stack, &objStack);
          }
          GRR_LOG_DEBUG("Begin JSON array\n");
          if (STACK_TOP_IS(GRR_JSON_ARRAY)) {
            GrrType type;
            GrrHashMapValue *value =
                Grr_listGetAtIndex(&objStack, objStack.count - 1, &type);
            assert(type == LIST);
            GrrHashMapValue objValue;
            objValue.list = _Grr_beginJSONList(&objStack);
            Grr_listPushBack(value->list, objValue, LIST);
          } else if (STACK_TOP_IS(GRR_JSON_OBJECT)) {
            GrrType type;
            GrrHashMapValue *value =
                Grr_listGetAtIndex(&objStack, objStack.count - 1, &type);
            assert(type == HASH_MAP);
            GrrHashMapValue objValue;
            objValue.list = _Grr_beginJSONList(&objStack);
            Grr_hashMapPut(value->map, currentJSONString, objValue, LIST);
          }
          STACK_PUSH(GRR_JSON_ARRAY);
        } else {
          // Inside string
          _Grr_updateJSONString(codePoint);
        }
        break;

      case RIGHT_BRACKET:
        if (!STACK_IS_EMPTY() && !STACK_TOP_IS(GRR_JSON_STRING)) {
          if (STACK_TOP_IS(GRR_JSON_NUMBER)) {
            STACK_POP();
            GRR_LOG_DEBUG("End JSON number\n");
            _Grr_endJSONNumber(&stack, &objStack);
          }
          if (STACK_TOP_IS(GRR_JSON_ARRAY)) {
            STACK_POP(); // End array
            GRR_LOG_DEBUG("End JSON array\n");
            _Grr_endJSONList(&objStack);
          } else {
            GRR_LOG_ERROR("JSON is not valid: expected array\n"); // Error
            jsonOk = false;
          }
        } else if (!STACK_IS_EMPTY()) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR(
              "JSON is not valid: expected array or string\n"); // Error
          jsonOk = false;
        }
        break;

      case QUOTATION_MARK:
        if (STACK_IS_EMPTY() || !(STACK_TOP_IS(GRR_JSON_STRING))) {
          if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NUMBER)) {
            STACK_POP();
            GRR_LOG_DEBUG("End JSON number\n");
            _Grr_endJSONNumber(&stack, &objStack);
          }
          STACK_PUSH(GRR_JSON_STRING); // Begin string
          GRR_LOG_DEBUG("Begin JSON string\n");
          _Grr_beingString();
        } else if (!STACK_IS_EMPTY()) {
          if (previousCodePoint != BACKSLASH) {
            STACK_POP(); // End string
            GRR_LOG_DEBUG("End JSON string\n");
            Grr_bool isHashValue =
                (lastPushed == GRR_JSON_STRING) && (previouslyPushed == COLON);
            _Grr_endJSONString(&stack, &objStack, isHashValue);
          } else {
            // Inside string
            _Grr_updateJSONString(codePoint);
          }
        } else {
          GRR_LOG_ERROR("JSON is not valid: expected string\n"); // Error
          jsonOk = false;
        }
        break;

      case LOWERCASE_T:
        if (STACK_IS_EMPTY() || !STACK_TOP_IS(GRR_JSON_STRING)) {
          if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NUMBER)) {
            STACK_POP();
            GRR_LOG_DEBUG("End JSON number\n");
            _Grr_endJSONNumber(&stack, &objStack);
          }
          STACK_PUSH(GRR_JSON_TRUE_LITERAL_T); // Begin true literal
          GRR_LOG_DEBUG("Begin JSON true literal\n");
        } else if (!STACK_IS_EMPTY()) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR("JSON is not valid: expected true literal or "
                        "string\n"); // Error
          jsonOk = false;
        }
        break;

      case LOWERCASE_R:
        if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_TRUE_LITERAL_T)) {
          STACK_POP();
          STACK_PUSH(GRR_JSON_TRUE_LITERAL_R); // Update stack top
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_STRING)) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR("JSON is not valid: expected true literal or "
                        "string\n"); // Error
          jsonOk = false;
        }
        break;

      case LOWERCASE_U:
        if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_TRUE_LITERAL_R)) {
          STACK_POP();
          STACK_PUSH(GRR_JSON_TRUE_LITERAL_U); // Update stack top
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NULL_LITERAL_N)) {
          STACK_POP();
          STACK_PUSH(GRR_JSON_NULL_LITERAL_U); // Update stack top
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_STRING)) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR("JSON is not valid: expected true/null literal or "
                        "string\n"); // Error
        }
        break;

      case LOWERCASE_E:
        if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_TRUE_LITERAL_U)) {
          STACK_POP(); // End JSON true literal
          GRR_LOG_DEBUG("End JSON true literal\n");
          _Grr_endJSONLiteral(&stack, &objStack, 1);
        } else if (!STACK_IS_EMPTY() &&
                   STACK_TOP_IS(GRR_JSON_FALSE_LITERAL_S)) {
          STACK_POP(); // End JSON false literal
          GRR_LOG_DEBUG("End JSON false literal\n");
          _Grr_endJSONLiteral(&stack, &objStack, 0);
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_STRING)) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NUMBER)) {
          // Inside number
          _Grr_updateJSONNumber(codePoint);
        } else {
          GRR_LOG_ERROR("JSON is not valid: expected boolean literal, string "
                        "or number\n"); // Error
          jsonOk = false;
        }
        break;

      case UPPERCASE_E:
        if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NUMBER)) {
          // Inside number
          _Grr_updateJSONNumber(codePoint);
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_STRING)) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR(
              "JSON is not valid: expected string or number\n"); // Error
          jsonOk = false;
        }
        break;

      case LOWERCASE_F:
        if (STACK_IS_EMPTY() || !STACK_TOP_IS(GRR_JSON_STRING)) {
          if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NUMBER)) {
            STACK_POP(); // End number
            GRR_LOG_DEBUG("End JSON number\n");
            _Grr_endJSONNumber(&stack, &objStack);
          }
          STACK_PUSH(GRR_JSON_FALSE_LITERAL_F); // Begin false literal
          GRR_LOG_DEBUG("Begin JSON false literal\n");
        } else if (!STACK_IS_EMPTY()) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR("JSON is not valid: expected false literal, number or "
                        "string\n"); // Error
          jsonOk = false;
        }
        break;

      case LOWERCASE_A:
        if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_FALSE_LITERAL_F)) {
          STACK_POP();
          STACK_PUSH(GRR_JSON_FALSE_LITERAL_A); // Update stack top
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_STRING)) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR("JSON is not valid: expected false literal or "
                        "string\n"); // Error
          jsonOk = false;
        }
        break;

      case LOWERCASE_L:
        if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_FALSE_LITERAL_A)) {
          STACK_POP();
          STACK_PUSH(GRR_JSON_FALSE_LITERAL_L); // Update stack top
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NULL_LITERAL_U)) {
          STACK_POP();
          STACK_PUSH(GRR_JSON_NULL_LITERAL_L); // Update stack top
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NULL_LITERAL_L)) {
          STACK_POP(); // End JSON null literal
          GRR_LOG_DEBUG("End JSON null literal\n");
          _Grr_endJSONLiteral(&stack, &objStack, 2);
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_STRING)) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR("JSON is not valid: expected false/null literal or "
                        "string\n"); // Error
          jsonOk = false;
        }
        break;

      case LOWERCASE_S:
        if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_FALSE_LITERAL_L)) {
          STACK_POP();
          STACK_PUSH(GRR_JSON_FALSE_LITERAL_S); // Update stack top
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_STRING)) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR("JSON is not valid: expected false literal or "
                        "string\n"); // Error
          jsonOk = false;
        }
        break;

      case LOWERCASE_N:
        if (STACK_IS_EMPTY() || !STACK_TOP_IS(GRR_JSON_STRING)) {
          if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NUMBER)) {
            STACK_POP();
            GRR_LOG_DEBUG("End JSON number\n");
            _Grr_endJSONNumber(&stack, &objStack);
          }
          STACK_PUSH(GRR_JSON_NULL_LITERAL_N); // Begin null literal
          GRR_LOG_DEBUG("Begin JSON null literal\n");
        } else if (!STACK_IS_EMPTY()) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR("JSON is not valid: expected null literal or "
                        "string\n"); // Error
          jsonOk = false;
        }
        break;

      case MINUS:
        if (STACK_IS_EMPTY() || !STACK_TOP_IS(GRR_JSON_STRING)) {
          if (STACK_IS_EMPTY() || !STACK_TOP_IS(GRR_JSON_NUMBER)) {
            STACK_PUSH(GRR_JSON_NUMBER); // Begin number
            GRR_LOG_DEBUG("Begin JSON number\n");
            _Grr_beginJSONNumber();
            _Grr_updateJSONNumber(codePoint);
          } else if (!STACK_IS_EMPTY()) {
            if (previousCodePoint != LOWERCASE_E &&
                previousCodePoint != UPPERCASE_E) {
              GRR_LOG_ERROR(
                  "JSON is not valid: minus sign at middle of number is not "
                  "preceeded by exponent\n");
              jsonOk = false;
            } else {
              // Inside number
              _Grr_updateJSONNumber(codePoint);
            }
          }
        } else {
          // Inside string
          _Grr_updateJSONString(codePoint);
        }
        break;

      case PLUS:
        if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NUMBER)) {
          if (previousCodePoint != LOWERCASE_E &&
              previousCodePoint != UPPERCASE_E) {
            GRR_LOG_ERROR(
                "JSON is not valid: plus sign at middle of number is not "
                "preceeded by exponent\n");
            jsonOk = false;
          } else {
            // Inside number
            _Grr_updateJSONNumber(codePoint);
          }
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_STRING)) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR(
              "JSON is not valid: expected number or string\n"); // Error
          jsonOk = false;
        }
        break;

      case DECIMAL_POINT:
        if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_NUMBER)) {
          if (!_Grr_isDecimalDigit(previousCodePoint)) {
            GRR_LOG_ERROR("JSON is not valid: decimal point not preceeded by "
                          "decimal digit\n");
            jsonOk = false;
          } else {
            // Inside number
            _Grr_updateJSONNumber(codePoint);
          }
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_STRING)) {
          // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR(
              "JSON is not valid: expected number or string\n"); // Error
          jsonOk = false;
        }
        break;

      case COMMA:
        if (!STACK_IS_EMPTY() &&
            (STACK_TOP_IS(GRR_JSON_OBJECT) || STACK_TOP_IS(GRR_JSON_ARRAY) ||
             STACK_TOP_IS(GRR_JSON_NUMBER))) {
          if (STACK_TOP_IS(GRR_JSON_NUMBER)) {
            STACK_POP(); // End number
            GRR_LOG_DEBUG("End JSON number\n");
            _Grr_endJSONNumber(&stack, &objStack);
          }
          if (STACK_TOP_IS(GRR_JSON_OBJECT)) {
            GRR_LOG_DEBUG("Begin JSON comma (object)\n");
            STACK_PUSH(COMMA);
            GRR_LOG_DEBUG("End JSON comma (object)\n");
            STACK_POP()
          } else if (STACK_TOP_IS(GRR_JSON_ARRAY)) {
            GRR_LOG_DEBUG("Begin JSON comma (array)\n");
            STACK_PUSH(COMMA);
            GRR_LOG_DEBUG("End JSON comma (array)\n");
            STACK_POP()
          } else {
            GRR_LOG_WARNING("JSON: this message is supposed to be unreachable. "
                            "Contact developers.\n");
          }
        } else if (!STACK_IS_EMPTY() &&
                   STACK_TOP_IS(GRR_JSON_STRING)) { // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR("JSON is not valid: expected object, array, number or "
                        "string\n"); // Error
          jsonOk = false;
        }
        break;

      case COLON:
        if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_OBJECT)) {
          GRR_LOG_DEBUG("Begin JSON colon\n");
          STACK_PUSH(COLON)
          GRR_LOG_DEBUG("End JSON colon\n");
          STACK_POP()
        } else if (!STACK_IS_EMPTY() &&
                   STACK_TOP_IS(GRR_JSON_STRING)) { // Inside string
          _Grr_updateJSONString(codePoint);
        } else {
          GRR_LOG_ERROR(
              "JSON is not valid: expected colon or string\n"); // Error
          jsonOk = false;
        }
        break;

      default:
        if (_Grr_isDecimalDigit(codePoint)) {
          if (STACK_IS_EMPTY() || !STACK_TOP_IS(GRR_JSON_STRING)) {
            if (STACK_IS_EMPTY() || !STACK_TOP_IS(GRR_JSON_NUMBER)) {
              STACK_PUSH(GRR_JSON_NUMBER); // Begin number
              GRR_LOG_DEBUG("Begin JSON number\n");
              _Grr_beginJSONNumber();
              _Grr_updateJSONNumber(codePoint);
            } else if (!STACK_IS_EMPTY()) {
              // Inside Number
              _Grr_updateJSONNumber(codePoint);
            }
          } else { // Inside string
            _Grr_updateJSONString(codePoint);
          }
        } else if (!STACK_IS_EMPTY() && STACK_TOP_IS(GRR_JSON_STRING)) {
          // Inside string
          _Grr_updateJSONString(codePoint);

        } else {
          GRR_LOG_ERROR("JSON is not valid: expected number or string\n");
          utf8Ok = false;
        }
        break;
      }

      previousCodePoint = codePoint;
    }
  }

#undef STACK_IS_EMPTY
#undef STACK_TOP_IS
#undef STACK_POP
#undef STACK_PUSH

  GRR_LOG_DEBUG("All bytes processed? %d\n", i == nBytes);
  GRR_LOG_DEBUG("UTF-8 ok? %d\n", utf8Ok);
  GRR_LOG_DEBUG("Fully ASCII? %d\n", fullyASCII);

  Grr_freeList(&stack); // Free stack of JSON types
  free(bytes);

  if (utf8Ok && jsonOk) {
    // Use json hashmap to construct model
    Grr_string assetDir = Grr_dirFromFilePath(path);
    GRR_LOG_DEBUG("Asset dir %s\n", assetDir);
    GrrAssetglTF *glTF = _Grr_glTFFromJSON(json, assetDir);
    if (assetDir)
      free(assetDir);
    // Grr_freeHashMap(json); // Frees JSON recursively
    return glTF;
  }
  return NULL;
}

Grr_byte
Grr_bytesPerglTFComponentType(GRR_ACCESSOR_COMPONENT_TYPE componentType) {
  switch (componentType) {
  case COMPONENT_TYPE_SIGNED_BYTE:
  case COMPONENT_TYPE_UNSIGNED_BYTE:
    return 1;
  case COMPONENT_TYPE_SIGNED_SHORT:
  case COMPONENT_TYPE_UNSIGNED_SHORT:
    return 2;
  case COMPONENT_TYPE_UNSIGNED_INT:
  case COMPONENT_TYPE_FLOAT:
    return 3;
  default:
    GRR_LOG_ERROR("Unknown glTF component type %u\n", componentType);
    return 0;
  }
}

void Grr_modelFromAsset(GrrModel *model, GrrAssetglTF *gltf, Grr_u32 meshIndex,
                        Grr_u32 primitiveIndex) {
  GrrMesh *mesh = &gltf->meshes[meshIndex];
  GrrMeshPrimitive *primitive = &mesh->primitives[primitiveIndex];

  // TODO: what if vertex data is not tightly packed ?

  // Vertices
  GrrAccessor verticesAccessor =
      gltf->accessors[primitive->verticesAccessorIndex];
  if (NULL == verticesAccessor.sparseAccessor) {
    model->vertexCount = verticesAccessor.count;
    GrrBufferView bufferView =
        gltf->bufferViews[verticesAccessor.bufferViewIndex];
    model->positions =
        (Grr_f32 *)(gltf->buffers[bufferView.bufferIndex] +
                    verticesAccessor.byteOffset + bufferView.offset);
    model->colors = NULL;             // TODO
    model->textureCoordinates = NULL; // TODO
  } else {
    // TODO: handle sparse accessor
    GRR_LOG_CRITICAL("Sparse accessors not supported!");
    exit(EXIT_FAILURE);
  }

  // Indices
  if (primitive->indicesAccessorIndex == -1) {
    // Not indexed TODO
    // When indices property is not defined, attribute accessors' count
    // indicates the number of vertices to render
  } else {
    // Indexed
    // When indices property is defined, the number of vertex indices to render
    // is defined by count of accessor referred to by indices
    GrrAccessor indicesAccessor =
        gltf->accessors[primitive->indicesAccessorIndex];
    if (NULL == indicesAccessor.sparseAccessor) {
      model->indexCount = indicesAccessor.count;
      GrrBufferView bufferView =
          gltf->bufferViews[indicesAccessor.bufferViewIndex];
      if (indicesAccessor.componentType == COMPONENT_TYPE_UNSIGNED_INT) {
        model->indices =
            (Grr_u32 *)(gltf->buffers[bufferView.bufferIndex] +
                        indicesAccessor.byteOffset + bufferView.offset);
      } else {
        // Convert to COMPONENT_TYPE_UNSIGNED_INT
        model->indices = (Grr_u32 *)malloc(
            sizeof(Grr_u32) * model->indexCount); // TODO: where to free ?
        size_t bytesPerIndex =
            Grr_bytesPerglTFComponentType(indicesAccessor.componentType);
        if (bytesPerIndex == 1) {
          Grr_byte *indices = gltf->buffers[bufferView.bufferIndex] +
                              indicesAccessor.byteOffset + bufferView.offset;
          for (Grr_u32 i = 0; i < model->indexCount; i++) {
            model->indices[i] = (Grr_u32)indices[i];
          }
        } else if (bytesPerIndex == 2) {
          Grr_u16 *indices =
              (Grr_u16 *)(gltf->buffers[bufferView.bufferIndex] +
                          indicesAccessor.byteOffset + bufferView.offset);
          for (Grr_u32 i = 0; i < model->indexCount; i++) {
            model->indices[i] = (Grr_u32)indices[i];
          }
        }
      }
    } else {
      // TODO: handle sparse accessor
      GRR_LOG_CRITICAL("Sparse accessors not supported!");
      exit(EXIT_FAILURE);
    }
  }
}
