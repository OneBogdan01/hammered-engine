
#pragma once
#include "types_vk.hpp"
namespace vkutil
{

void transition_image(VkCommandBuffer cmd, VkImage image,
                      VkImageLayout currentLayout, VkImageLayout newLayout);
void copy_image_to_image(VkCommandBuffer cmd, VkImage source,
                         VkImage destination, VkExtent2D srcSize,
                         VkExtent2D dstSize);
}

; // namespace vkutil
