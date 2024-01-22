#include "utils.h"

// Binary file IO

Grr_byte *Grr_readBytesFromFile(const Grr_string path, size_t *nBytes) {
  FILE *f = fopen(path, "rb");
  Grr_byte *bytes = NULL;
  *nBytes = 0;
  if (f != NULL) {
    fseek(f, 0, SEEK_END);
    size_t countBytes = ftell(f);
    rewind(f);
    bytes = (Grr_byte *)malloc(countBytes);
    if (bytes) {
      size_t countReadBytes = fread(bytes, 1, countBytes, f);
      if (countReadBytes != countBytes) {
        GRR_LOG_ERROR(
            "Read bytes (%llu) does not match available bytes (%llu)\n",
            countReadBytes, countBytes);
        free(bytes);
        bytes = NULL;
      }
    } else {
      GRR_LOG_ERROR("Failed to allocate memory for file bytes (%llu)\n",
                    countBytes);
    }
    fclose(f);
    *nBytes = countBytes;
  } else {
    GRR_LOG_ERROR("Failed to open file at path (%s)\n", path);
  }

  return bytes;
}

Grr_bool Grr_writeBytesToFile(const Grr_string path, const Grr_byte *bytes,
                              const Grr_u32 nBytes) {

  FILE *f = fopen(path, "wb");
  if (NULL == f) {
    GRR_LOG_ERROR("Failed to open file (%s) to write\n", path);
    return false;
  }
  size_t written = fwrite(bytes, 1, nBytes, f);
  fclose(f);

  return (written == nBytes);
}

#define MAX_TREE_NODES 5000 // TODO figure out max size
typedef struct GrrHuffmanTree {
  Grr_u32 left[MAX_TREE_NODES];
  Grr_u32 right[MAX_TREE_NODES];
  Grr_u32 label[MAX_TREE_NODES]; // Binary label of edge pointing to node
  Grr_u32 value[MAX_TREE_NODES]; // Alphabet value if leaf node
  Grr_u32 count;                 // Keep count of nodes in tree
} GrrHuffmanTree;

Grr_u32 _Grr_HuffmanNewNode(GrrHuffmanTree *tree) {
  if (tree->count >= MAX_TREE_NODES) {
    GRR_LOG_CRITICAL("Huffman: exhausted max nodes (%d) available for tree\n",
                     MAX_TREE_NODES);
    exit(EXIT_FAILURE);
  }
  return tree->count++;
}
#undef MAX_TREE_NODES

void _Grr_HuffmanTreeInit(GrrHuffmanTree *tree) {
  tree->count = 1;                            // Node 0 is root
  memset(tree->left, 0, sizeof(tree->left));  // 0 means no left child
  memset(tree->right, 0, sizeof(tree->left)); // 0 means no right child
}

Grr_bool _Grr_HuffmanNodeIsLeaf(GrrHuffmanTree *tree, Grr_u32 node) {
  return (!tree->left[node] && !tree->right[node]);
}

void _Grr_HuffmanTreeAdd(GrrHuffmanTree *tree, Grr_u32 code, Grr_u32 length,
                         Grr_u32 leafValue) {
  Grr_u32 currentNode = 0; // Root
  for (Grr_i32 i = length - 1; i >= 0; i--) {
    Grr_u32 bit = code & (1 << i);
    if (bit) {
      // Right
      if (!tree->right[currentNode])
        tree->right[currentNode] = _Grr_HuffmanNewNode(tree);
      tree->label[tree->right[currentNode]] = 1;
      currentNode = tree->right[currentNode];
    } else {
      // Left
      if (!tree->left[currentNode])
        tree->left[currentNode] = _Grr_HuffmanNewNode(tree);
      tree->label[tree->left[currentNode]] = 0;
      currentNode = tree->left[currentNode];
    }
  }
  tree->value[currentNode] = leafValue;
}

void _Grr_HuffmanTreeFromCodeLengths(GrrHuffmanTree *tree, Grr_u32 lengths[],
                                     Grr_u32 countLengths,
                                     Grr_u32 lengthsOffset, Grr_u32 freqs[],
                                     Grr_u32 alphabet[]) {
  Grr_u32 maxLength = 0;
  for (Grr_u32 i = 0; i < countLengths; i++)
    if (maxLength < lengths[i + lengthsOffset])
      maxLength = lengths[i + lengthsOffset];

  _Grr_HuffmanTreeInit(tree);

  Grr_u32 code = 0;
  Grr_u32 nextCode[20];
  nextCode[0] = 0;
  if (freqs[0] != 0) {
    GRR_LOG_ERROR("Huffman: code length 0 has non-zero frequency\n");
    freqs[0] = 0;
  }
  for (Grr_u32 i = 1; i <= maxLength; i++) {
    code = (code + freqs[i - 1]) << 1; // Base for codes of length i
    nextCode[i] = code;
  }

  for (Grr_u32 i = 0; i < countLengths; i++) {
    if (lengths[i + lengthsOffset] > 0) {
      _Grr_HuffmanTreeAdd(tree, nextCode[lengths[i + lengthsOffset]],
                          lengths[i + lengthsOffset], alphabet[i]);
      nextCode[lengths[i + lengthsOffset]] += 1;
    }
  }
}

#define STEP()                                                                 \
  if (currentBit < 7)                                                          \
    ++currentBit;                                                              \
  else {                                                                       \
    currentBit = 0;                                                            \
    ++currentByte;                                                             \
  }

#define NEXT_BIT() ((bytes[currentByte] & (1 << currentBit)) >> currentBit)

// b must be a variable
#define NEXT_BITS(nBits, b)                                                    \
  {                                                                            \
    b = 0;                                                                     \
    for (Grr_u32 i = 0; i < nBits; i++) {                                      \
      b |= (NEXT_BIT() << i);                                                  \
      STEP()                                                                   \
    }                                                                          \
  }

Grr_u32 _Grr_HuffmanTreeWalk(GrrHuffmanTree *tree, Grr_byte *bytes,
                             size_t *pCurrentByte, size_t *pCurrentBit) {
  Grr_u32 currentNode = 0; // Root
  Grr_byte b;              // Next bit
  size_t currentByte = *pCurrentByte;
  size_t currentBit = *pCurrentBit;
  do {
    NEXT_BITS(1, b)
    if (b == 0)
      currentNode = tree->left[currentNode];
    else
      currentNode = tree->right[currentNode];
  } while (!_Grr_HuffmanNodeIsLeaf(tree, currentNode));
  *pCurrentByte = currentByte;
  *pCurrentBit = currentBit;
  return tree->value[currentNode];
}

void _Grr_HuffmanTreeDebug(GrrHuffmanTree *tree, Grr_u32 node) {
  if (_Grr_HuffmanNodeIsLeaf(tree, node)) {
    printf("Leaf (%u) ", tree->value[node]);
  } else {
    if (tree->left[node]) {
      printf("<-(%u)- ", tree->label[tree->left[node]]);
      _Grr_HuffmanTreeDebug(tree, tree->left[node]);
    }
    if (tree->right[node]) {
      printf("-(%u)-> ", tree->label[tree->right[node]]);
      _Grr_HuffmanTreeDebug(tree, tree->right[node]);
    }
  }
  fflush(stdout);
}

Grr_byte *Grr_inflate(Grr_byte *bytes, size_t nBytes, size_t *outputSize) {
  // Performs Huffman decoding followed by L7ZZ decoding
  // bytes assumed to be a DEFLATE stream wrapped in a zlib container
  // DEFALTE: https://www.ietf.org/rfc/rfc1951.txt

  size_t currentByte;  // Current byte index
  size_t currentBit;   // Bit in current byte
  Grr_bool finalBlock; // Final block flag
  Grr_byte blockType;  // 2-bit code type
  Grr_u16 len;         // Number of data bytes in the block
  Grr_u16 nLen;        // One's complement of LEN

  // Allocate a big enough array for decompression
  Grr_byte *decoded =
      (Grr_byte *)malloc(sizeof(Grr_byte) * 20000000); // TODO: figure out max ?
  size_t decodedSize = 0;

  *outputSize = 0;
  currentByte = 0;
  currentBit = 0;
  finalBlock = false;

  // ZIP header header
  Grr_byte compressionMethod;
  NEXT_BITS(4, compressionMethod)
  if (compressionMethod != 8) {
    GRR_LOG_ERROR("ZIP header: unsupported compression method (%u)\n",
                  compressionMethod);
    return NULL;
  }

  Grr_byte compressionInfo;
  NEXT_BITS(4, compressionInfo);
  if (compressionInfo > 7) {
    GRR_LOG_ERROR("ZIP header: compression info too long (%u)\n",
                  compressionInfo);
    return NULL;
  }
  Grr_u32 windowSize = (1 << (compressionInfo + 8));
  GRR_LOG_DEBUG("DEFLATE: window size (%u)\n", windowSize);

  currentByte = 2; // Skip to DEFLATE byte stream (ignoring byte 1 flags)

  // Huffman trees for decompression
  GrrHuffmanTree literalsAndLengthsTree;
  GrrHuffmanTree distancesTree;
  GrrHuffmanTree codeLengthTree;
  Grr_u32 lengths[1000]; // Code lengths
  Grr_u32 freqs[10];     // Frequency of each length
  Grr_u32 alphabet[286]; // Alphabet

  for (Grr_u32 i = 0; i < 286; i++)
    alphabet[i] = i;

  do {
    // Header bits
    NEXT_BITS(1, finalBlock)
    NEXT_BITS(2, blockType)

    if (blockType == 0) {
      // No compression
      GRR_LOG_DEBUG("Start of DEFLATE block with no compression\n");
      // skip any remaining bits in current partially processed byte
      currentByte += 1;
      currentBit = 0;
      // Read LEN and NLEN
      len = ((Grr_u16)bytes[currentByte + 1] << 8) | bytes[currentByte];
      nLen = ((Grr_u16)bytes[currentByte + 3] << 8) | bytes[currentByte + 2];
      if (len != (~nLen & 0xFFFF)) {
        GRR_LOG_ERROR("DEFLATE: block's NLEN is not equal to the one's "
                      "complement of LEN\n");
        decodedSize = 0;
        break;
      }
      currentByte += 4;
      // Copy LEN bytes of data to output
      memcpy(decoded + decodedSize, bytes + currentByte, len);
      decodedSize += len;
      currentByte += len;
    } else {
      if (blockType == 3) {
        GRR_LOG_ERROR("DEFLATE: block type 11 encountered\n");
        decodedSize = 0;
        break;
      }

      if (blockType == 2) {
        GRR_LOG_DEBUG("Start of DEFLATE block with dynamic Huffman codes\n");
        // The Huffman codes for the two alphabets appear in the block
        // immediately after the header bits and before the actual compressed
        // data

        // The number of literal/length codes
        Grr_u32 countLiteralLengthCodes;
        NEXT_BITS(5, countLiteralLengthCodes)
        countLiteralLengthCodes += 257;
        GRR_LOG_DEBUG("DEFALTE: number of literal/length codes (%u)\n",
                      countLiteralLengthCodes);

        // The number of distance codes
        Grr_u32 countDistanceCodes;
        NEXT_BITS(5, countDistanceCodes)
        countDistanceCodes += 1;
        GRR_LOG_DEBUG("DEFALTE: number of distance codes (%u)\n",
                      countDistanceCodes);

        // The number of code length codes
        Grr_u32 countCodeLengthCodes;
        NEXT_BITS(4, countCodeLengthCodes)
        countCodeLengthCodes += 4;
        GRR_LOG_DEBUG("DEFALTE: number of 'code length' codes (%u)\n",
                      countCodeLengthCodes);

        // Positions of code lengths for the code length alphabet
        Grr_u32 codeLengthPosition[] = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                        11, 4,  12, 3, 13, 2, 14, 1, 15};

        // Lengths
        for (Grr_u32 i = 0; i < 19; i++)
          lengths[i] = 0;

        Grr_u32 bits;
        for (Grr_u32 i = 0; i < countCodeLengthCodes; i++) {
          NEXT_BITS(3, bits);
          lengths[codeLengthPosition[i]] = bits;
        }

        // Frequencies
        for (Grr_u32 i = 0; i < 10; i++)
          freqs[i] = 0;
        for (Grr_u32 i = 0; i < 19; i++) {
          if (lengths[i] > 0)
            freqs[lengths[i]] += 1;
        }

        // Construct code length Huffman tree
        _Grr_HuffmanTreeFromCodeLengths(&codeLengthTree, lengths, 19, 0, freqs,
                                        alphabet);

        // Read literal/length + distance code length list
        Grr_u32 count = 0;
        Grr_u32 prevCodeLength;
        Grr_u32 repeatLength;

        Grr_u32 litFreqs[16];
        Grr_u32 distanceFreqs[16];
        for (Grr_u32 i = 0; i < 16; i++) {
          litFreqs[i] = 0;
          distanceFreqs[i] = 0;
        }

        while (count < countLiteralLengthCodes + countDistanceCodes) {
          Grr_u32 sym = _Grr_HuffmanTreeWalk(&codeLengthTree, bytes,
                                             &currentByte, &currentBit);
          if (sym < 16) {
            //  Literal value
            lengths[count++] = sym;
            if (sym > 0) {
              if (count <= countLiteralLengthCodes) {
                litFreqs[sym] += 1;
              } else {
                distanceFreqs[sym] += 1;
              }
            }
          } else if (sym == 16) {
            // Copy the previous code length 3..6 times
            // The next 2 bits indicate repeat length
            prevCodeLength = lengths[count - 1];
            NEXT_BITS(2, repeatLength)
            repeatLength += 3;
            while (repeatLength--) {
              lengths[count++] = prevCodeLength;
              if (prevCodeLength > 0) {
                if (count <= countLiteralLengthCodes) {
                  litFreqs[prevCodeLength] += 1;
                } else {
                  distanceFreqs[prevCodeLength] += 1;
                }
              }
            }
          } else if (sym == 17) {
            // Repeat code length 0 for 3..10 times
            // The next 3 bits indicate repeat length
            NEXT_BITS(3, repeatLength)
            repeatLength += 3;
            while (repeatLength--) {
              lengths[count++] = 0;
            }
          } else if (sym == 18) {
            // repeat code length 0 for 11..138 times
            // The next 7 bits indicate repeat length
            NEXT_BITS(7, repeatLength)
            repeatLength += 11;
            while (repeatLength--) {
              lengths[count++] = 0;
            }
          } else {
            GRR_LOG_CRITICAL("Huffman: invalid symbol (%u)\n", sym);
            exit(EXIT_FAILURE);
          }
        }

        // Construct trees
        _Grr_HuffmanTreeFromCodeLengths(&literalsAndLengthsTree, lengths,
                                        countLiteralLengthCodes, 0, litFreqs,
                                        alphabet);
        _Grr_HuffmanTreeFromCodeLengths(
            &distancesTree, lengths, count - countLiteralLengthCodes,
            countLiteralLengthCodes, distanceFreqs, alphabet);

      } else {
        // Fixed Huffman codes
        GRR_LOG_DEBUG("Start of DEFLATE block with fixed Huffman codes\n");

        // Literal/Length tree

        // Lengths
        Grr_u32 codeLengths[] = {
            8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
            8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
            8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
            8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
            8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
            8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
            8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
            9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
            9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
            9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
            9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
            9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
            7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8};

        // Frequencies
        for (Grr_u32 i = 0; i < 7; i++)
          freqs[i] = 0;
        freqs[7] = 24;
        freqs[8] = 152;
        freqs[9] = 112;

        _Grr_HuffmanTreeFromCodeLengths(&literalsAndLengthsTree, codeLengths,
                                        286, 0, freqs, alphabet);

        // Distance tree

        // Lengths
        for (Grr_u32 i = 0; i < 30; i++)
          lengths[i] = 5;

        // Frequencies
        freqs[5] = 30; // Other entries already zero from previous tree freqs

        _Grr_HuffmanTreeFromCodeLengths(&distancesTree, lengths, 30, 0, freqs,
                                        alphabet);
      }

      // Decompress block using built Huffman trees
      Grr_bool blockEnd = false;
      Grr_u32 symbol;
      Grr_u32 length;
      Grr_u32 distance;

      Grr_u32 lengthExtra[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                               2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
      Grr_u32 lengthBase[] = {3,  4,  5,  6,   7,   8,   9,   10,  11, 13,
                              15, 17, 19, 23,  27,  31,  35,  43,  51, 59,
                              67, 83, 99, 115, 131, 163, 195, 227, 258};
      Grr_u32 distanceExtra[] = {0, 0, 0,  0,  1,  1,  2,  2,  3,  3,
                                 4, 4, 5,  5,  6,  6,  7,  7,  8,  8,
                                 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
      Grr_u32 distanceBase[] = {
          1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
          33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
          1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

      do {
        // Decode value from input stream
        symbol = _Grr_HuffmanTreeWalk(&literalsAndLengthsTree, bytes,
                                      &currentByte, &currentBit);
        if (symbol < 256) {
          // Copy value (literal byte) to output stream
          decoded[decodedSize++] = symbol;
        } else if (symbol == 256) {
          blockEnd = true;
        } else {
          // (value = 257..285) decode (length, distance) from input stream
          NEXT_BITS(lengthExtra[symbol - 257], length)
          length += lengthBase[symbol - 257];

          symbol = _Grr_HuffmanTreeWalk(&distancesTree, bytes, &currentByte,
                                        &currentBit);
          NEXT_BITS(distanceExtra[symbol], distance)
          distance += distanceBase[symbol];
          while (length--) {
            decoded[decodedSize] = decoded[decodedSize - distance];
            decodedSize += 1;
          }
        }
      } while (!blockEnd);
    }
  } while (!finalBlock);

  Grr_u32 adler32; // Adler-32 checksum over the original uncompressed data
  NEXT_BITS(4 * 8, adler32)
  GRR_LOG_DEBUG("Adler-32 checksum (%u)\n", adler32);

  if (nBytes != currentByte + 1) {
    GRR_LOG_ERROR("ZIP header: wrong number of Adler-32 checksum bytes\n");
  }

#undef STEP
#undef NEXT_BIT
#undef NEXT_BITS

  *outputSize = decodedSize;
  return decoded;
}

// List

void Grr_initList(GrrList *list) {
  list->count = 0;
  list->chunk = (GrrListChunk *)malloc(sizeof(GrrListChunk));
  list->chunk->types = (GrrType *)malloc(sizeof(GrrType) * LIST_CHUNK_MAX);
  list->chunk->values =
      (GrrHashMapValue *)malloc(sizeof(GrrHashMapValue) * LIST_CHUNK_MAX);
  list->chunk->next = NULL;
}

void Grr_listPushBack(GrrList *list, GrrHashMapValue value, GrrType type) {
  Grr_u32 chunkIndex = list->count / LIST_CHUNK_MAX;
  Grr_u32 index = list->count % LIST_CHUNK_MAX;

  GrrListChunk *c = list->chunk;
  while (chunkIndex) {
    if (NULL == c->next) {
      c->next = (GrrListChunk *)malloc(sizeof(GrrListChunk));
      c->next->types = (GrrType *)malloc(sizeof(GrrType) * LIST_CHUNK_MAX);
      c->next->values =
          (GrrHashMapValue *)malloc(sizeof(GrrHashMapValue) * LIST_CHUNK_MAX);
      c->next->next = NULL;
    }
    c = c->next;
    chunkIndex -= 1;
  }
  c->types[index] = type;
  c->values[index] = value;
  list->count += 1;
}

GrrHashMapValue *Grr_listGetAtIndex(GrrList *list, Grr_u32 index,
                                    GrrType *type) {
  Grr_u32 chunkIndex = index / LIST_CHUNK_MAX;
  index = index % LIST_CHUNK_MAX;

  GrrListChunk *c = list->chunk;
  while (chunkIndex) {
    c = c->next;
    if (NULL == c) {
      GRR_LOG_ERROR("List index (%u) out of range\n", index);
      return NULL;
    }
    chunkIndex -= 1;
  }

  if (index >= list->count % LIST_CHUNK_MAX) {
    GRR_LOG_ERROR("List index (%u) out of range\n", index);
    return NULL;
  }

  if (NULL != type)
    *type = c->types[index];
  return &(c->values[index]);
}

GrrHashMapValue *Grr_listPop(GrrList *list) {
  GrrHashMapValue *value = Grr_listGetAtIndex(list, list->count - 1, NULL);
  list->count -= 1;
  return value;
}

void Grr_freeList(GrrList *list) {
  GRR_LOG_DEBUG("Free list\n");
  GrrListChunk *c = list->chunk;
  for (Grr_u32 i = 0; i < list->count; i++) {
    if (i >= LIST_CHUNK_MAX) {
      GrrListChunk *tmp = c;
      c = c->next;
      i = 0;
      free(tmp->types);
      free(tmp->values);
      free(tmp);
    }
    switch (c->types[i]) {
    case STRING:
      free(c->values[i].string);
      break;

    case LIST:
      Grr_freeList(c->values[i].list);
      break;

    case HASH_MAP:
      Grr_freeHashMap(c->values[i].map);
      break;

    default:
      break;
    }
  }
  list->count = 0;
  list->chunk = NULL;
}

// Hash table
void Grr_initHashMap(GrrHashMap *map) {
  for (Grr_u32 i = 0; i < HASH_MAP_MAX; i++) {
    map->entries[i].empty = true;
  }
  map->count = 0;
}

Grr_u32 _Grr_hash(const Grr_string key) {
  Grr_u32 h = 0;
  for (char *c = key; *c; c++) {
    h = (h * 11 + *c) % HASH_MAP_MAX;
  }
  return h;
}

void Grr_hashMapPut(GrrHashMap *map, const Grr_string key,
                    const GrrHashMapValue value, const GrrType type) {

  GRR_LOG_DEBUG("Hashmap @ %p: put key/value pair with key ('%s')", map, key);

  Grr_u32 h = _Grr_hash(key);
  for (Grr_u32 i = 0; i < HASH_MAP_MAX; i++) {
    if (map->entries[h].empty) {
      // New key
      map->entries[h].empty = false;
      size_t len = strlen(key);
      map->entries[h].key = (Grr_string)malloc(len + 1);
      strncpy(map->entries[h].key, key, len);
      map->entries[h].key[len] = '\0';
      map->entries[h].value = value;
      map->entries[h].type = type;
      map->count += 1;
      return;
    } else if (0 == strcmp(map->entries[h].key, key)) {
      // Key exists
      GRR_LOG_WARNING(
          "Hashmap: entry with key ('%s') already exists. Updating value\n",
          key);
      size_t len = strlen(key);
      map->entries[h].key = (Grr_string)realloc(map->entries[h].key, len + 1);
      strncpy(map->entries[h].key, key, len);
      map->entries[h].key[len] = '\0';
      map->entries[h].value = value;
      map->entries[h].type = type;
      return;
    } else {
      h += 1;
      if (h >= HASH_MAP_MAX)
        h = 0;
    }
  }
}

GrrHashMapValue *Grr_hashMapGet(GrrHashMap *map, const Grr_string key,
                                GrrType *type) {
  Grr_u32 h = _Grr_hash(key);
  for (Grr_u32 i = 0; i < HASH_MAP_MAX; i++) {
    if (0 == strcmp(map->entries[h].key, key)) {
      *type = map->entries[h].type;
      return &(map->entries[h].value);
    }
    h += 1;
    if (h >= HASH_MAP_MAX)
      h = 0;
  }

  return NULL;
}

void Grr_freeHashMap(GrrHashMap *map) {
  GRR_LOG_DEBUG("Free hashmap\n");
  for (Grr_u32 i = 0; i < HASH_MAP_MAX; i++) {
    if (map->entries[i].empty)
      continue;

    free(map->entries[i].key);

    switch (map->entries[i].type) {
    case STRING:
      free(map->entries[i].value.string);
      break;

    case LIST:
      Grr_freeList(map->entries[i].value.list);
      break;

    case HASH_MAP:
      Grr_freeHashMap(map->entries[i].value.map);
      break;

    default:
      break;
    }
  }
}

// JSON debug

#define INDENT(d, f)                                                           \
  {                                                                            \
    for (Grr_u16 i = 0; i < d; i++)                                            \
      fprintf(f, "\t");                                                        \
  }

void _Grr_writeJSONToFileRecursive(GrrHashMapValue value, GrrType type,
                                   Grr_u16 depth, Grr_bool indentFlag, FILE *f);
void _Grr_writeJSONObjectToFile(GrrHashMap *json, FILE *f, Grr_u16 depth,
                                Grr_bool indentFlag) {
  if (indentFlag)
    INDENT(depth, f);
  fputs("{\n", f);

  // Calc last index
  Grr_u32 lastIndex;
  for (Grr_u32 i = 0; i < HASH_MAP_MAX; i++) {
    if (false == json->entries[i].empty)
      lastIndex = i;
  }

  // Recurse
  for (Grr_u32 i = 0; i < HASH_MAP_MAX; i++) {
    if (json->entries[i].empty)
      continue;
    INDENT(depth + 1, f);
    fprintf(f, "\"%s\": ", json->entries[i].key);
    _Grr_writeJSONToFileRecursive(json->entries[i].value, json->entries[i].type,
                                  depth + 1, false, f);
    if (i < lastIndex)
      fputs(",\n", f);
    else
      fputs("\n", f);
  }
  INDENT(depth, f);
  fputs("}", f);
}

void _Grr_writeJSONListToFile(GrrList *list, FILE *f, Grr_u16 depth,
                              Grr_bool indentFlag) {
  if (indentFlag)
    INDENT(depth, f);
  fputs("[\n", f);

  // Recurse
  GrrType type;
  for (Grr_u32 i = 0; i < list->count; i++) {
    GrrHashMapValue *value = Grr_listGetAtIndex(list, i, &type);
    _Grr_writeJSONToFileRecursive(*value, type, depth + 1, true, f);
    if (i < list->count - 1)
      fputs(",\n", f);
    else
      fputs("\n", f);
  }
  INDENT(depth, f);
  fputs("]", f);
}

void _Grr_writeJSONToFileRecursive(GrrHashMapValue value, GrrType type,
                                   Grr_u16 depth, Grr_bool indentFlag,
                                   FILE *f) {
  switch (type) {
  case FLOAT64:
    if (indentFlag)
      INDENT(depth, f);
    fprintf(f, "%.10f", value.f64);
    break;

  case INT64:
    if (indentFlag)
      INDENT(depth, f);
    fprintf(f, "%lld", value.i64);
    break;

  case STRING:
    if (indentFlag)
      INDENT(depth, f);
    fprintf(f, "\"%s\"", value.string);
    break;

  case NULL_TYPE:
    if (indentFlag)
      INDENT(depth, f);
    fprintf(f, "null");
    break;

  case BOOLEAN:
    if (indentFlag)
      INDENT(depth, f);
    fprintf(f, value.boolean ? "true" : "false");
    break;

  case LIST:
    _Grr_writeJSONListToFile(value.list, f, depth, indentFlag);
    break;

  case HASH_MAP:
    _Grr_writeJSONObjectToFile(value.map, f, depth, indentFlag);
    break;

  default:
    break;
  }
}

void Grr_writeJSONToFile(GrrHashMap *json, const Grr_string path) {
  FILE *f = fopen(path, "w");
  if (NULL != f) {
    _Grr_writeJSONObjectToFile(json, f, 0, false);
    fclose(f);
  } else {
    GRR_LOG_ERROR("Failed to open file (%s) to write\n", path);
  }
}
