#ifndef GRR_VULKAN_H
#define GRR_VULKAN_H

#include "assets.h"
#include "logging.h"
#include "math/linear.h"
#include "types.h"
#include "utils.h"
#include "window.h"
#include <string.h>
#include <vulkan/vulkan.h>
#if defined(GRR_PLATFORM_MACOS)
#include <vulkan/vulkan_metal.h>
#endif

// Same family could have multiple indices but we currently use one of them
typedef struct GrrQueueFamilyIndices {
  Grr_u32 graphicsFamilyIndex;
  Grr_u32 computeFamilyIndex;
  Grr_u32 transferFamilyIndex;
  Grr_u32 sparseBndingFamilyIndex;
  Grr_u32 presentFamilyIndex;
  // TODO: add other queue families: video decode, video encode, protected
  // memory management, sparse memory management, and transfer. Note some are
  // supported starting from specific versions of the vulkan API
} GrrQueueFamilyIndices;

typedef struct GrrUniformBufferObject {
  GrrMatrix4x4 model;
  GrrMatrix4x4 view;
  GrrMatrix4x4 projection;
} GrrUniformBufferObject;

void Grr_initializeVulkan();
void Grr_drawFrame();

#endif