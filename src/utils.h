#ifndef GRR_UTILS_H
#define GRR_UTILS_H

#include "logging.h"
#include "string.h"
#include "types.h"
#include <stdlib.h>

// Binary IO
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                   \
  ((byte) & 0x80 ? '1' : '0'), ((byte) & 0x40 ? '1' : '0'),                    \
      ((byte) & 0x20 ? '1' : '0'), ((byte) & 0x10 ? '1' : '0'),                \
      ((byte) & 0x08 ? '1' : '0'), ((byte) & 0x04 ? '1' : '0'),                \
      ((byte) & 0x02 ? '1' : '0'), ((byte) & 0x01 ? '1' : '0')

Grr_byte *Grr_readBytesFromFile(const Grr_string path, size_t *nBytes);
Grr_bool Grr_writeBytesToFile(const Grr_string path, const Grr_byte *bytes,
                              const Grr_u32 nBytes);

// Decompression
Grr_byte *Grr_inflate(Grr_byte *bytes, size_t nBytes, size_t *outputSize);

// Hash table & List
#define HASH_MAP_MAX 65536
#define LIST_CHUNK_MAX 65536

struct GrrHashMap;
union GrrHashMapValue;

typedef enum GrrType {
  NULL_TYPE,
  BOOLEAN,
  BYTE,
  INT16,
  INT32,
  INT64,
  UNSIGNED16,
  UNSIGNED32,
  UNSIGNED64,
  FLOAT32,
  FLOAT64,
  STRING,
  LIST,
  HASH_MAP
} GrrType;

struct GrrListChunk;
typedef struct GrrListChunk {
  GrrType *types;
  union GrrHashMapValue *values;
  struct GrrListChunk *next;
} GrrListChunk;

typedef struct GrrList {
  Grr_u32 count;
  GrrListChunk *chunk;
} GrrList;

void Grr_initList(GrrList *list);
void Grr_listPushBack(GrrList *list, union GrrHashMapValue value, GrrType type);
union GrrHashMapValue *Grr_listGetAtIndex(GrrList *list, Grr_u32 index,
                                          GrrType *type);
union GrrHashMapValue *Grr_listPop(GrrList *list);
void Grr_freeList(GrrList *list);

typedef union GrrHashMapValue {
  Grr_bool boolean;
  Grr_byte byte;

  Grr_i32 i16;
  Grr_i32 i32;
  Grr_i64 i64;

  Grr_u16 u16;
  Grr_u32 u32;
  Grr_u64 u64;

  Grr_f32 f32;
  Grr_f64 f64;

  Grr_string string;
  GrrList *list;
  struct GrrHashMap *map;
} GrrHashMapValue;

typedef struct GrrHashMapEntry {
  Grr_bool empty;
  Grr_string key;
  GrrType type;
  GrrHashMapValue value;
} GrrHashMapEntry;

typedef struct GrrHashMap {
  Grr_u32 count;
  GrrHashMapEntry entries[HASH_MAP_MAX];
} GrrHashMap;

void Grr_initHashMap(GrrHashMap *map);
void Grr_hashMapPut(GrrHashMap *map, const Grr_string key,
                    const GrrHashMapValue value, const GrrType type);
GrrHashMapValue *Grr_hashMapGet(GrrHashMap *map, const Grr_string key,
                                GrrType *type);
void Grr_freeHashMap(GrrHashMap *map);

// JSON debug
void Grr_writeJSONToFile(GrrHashMap *json, const Grr_string path);

#endif