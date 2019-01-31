// Copyright 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include "cereal/common/goldfish_vk_private_defs.h"

namespace goldfish_vk {

// Class for tracking host-side state. Currently we only care about
// tracking VkDeviceMemory to make it easier to pass the right data
// from mapped pointers to the guest, but this could get more stuff
// added to it if for instance, we want to only expose a certain set
// of physical device capabilities, or do snapshots.

// This class may be autogenerated in the future.
// Currently, it works by interfacing with VkDecoder calling on_<apicall>
// functions.
class VkDecoderGlobalState {
public:
    VkDecoderGlobalState();
    ~VkDecoderGlobalState();

    // There should only be one instance of VkDecoderGlobalState
    // per process
    static VkDecoderGlobalState* get();

    VkResult on_vkCreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkInstance* pInstance);

    // Override features
    void on_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice,
                                        VkPhysicalDeviceFeatures* pFeatures);

    // Override API version
    void on_vkGetPhysicalDeviceProperties(
            VkPhysicalDevice physicalDevice,
            VkPhysicalDeviceProperties* pProperties);

    // Override memory types advertised from host
    //
    void on_vkGetPhysicalDeviceMemoryProperties(
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceMemoryProperties* pMemoryProperties);

    VkResult on_vkCreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDevice* pDevice);

    void on_vkGetDeviceQueue(
        VkDevice device,
        uint32_t queueFamilyIndex,
        uint32_t queueIndex,
        VkQueue* pQueue);

    void on_vkDestroyDevice(
        VkDevice device,
        const VkAllocationCallbacks* pAllocator);

    VkResult on_vkCreateBuffer(VkDevice device,
                               const VkBufferCreateInfo* pCreateInfo,
                               const VkAllocationCallbacks* pAllocator,
                               VkBuffer* pBuffer);

    VkResult on_vkBindBufferMemory(VkDevice device,
                                   VkBuffer buffer,
                                   VkDeviceMemory memory,
                                   VkDeviceSize memoryOffset);

    VkResult on_vkCreateImage(
        VkDevice device,
        const VkImageCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkImage* pImage);

    void on_vkDestroyImage(
        VkDevice device,
        VkImage image,
        const VkAllocationCallbacks* pAllocator);

    VkResult on_vkCreateImageView(
        VkDevice device,
        const VkImageViewCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkImageView* pView);

    void on_vkGetImageMemoryRequirements(
        VkDevice device,
        VkImage image,
        VkMemoryRequirements* pMemoryRequirements);

    void on_vkCmdCopyBufferToImage(
        VkCommandBuffer commandBuffer,
        VkBuffer srcBuffer,
        VkImage dstImage,
        VkImageLayout dstImageLayout,
        uint32_t regionCount,
        const VkBufferImageCopy* pRegions);

    // Do we need to wrap vk(Create|Destroy)Instance to
    // update our maps of VkDevices? Spec suggests no:
    // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/vkDestroyInstance.html
    // Valid Usage
    // All child objects created using instance
    // must have been destroyed prior to destroying instance
    //
    // This suggests that we should emulate the invalid behavior by
    // not destroying our own VkDevice maps on instance destruction.

    VkResult on_vkAllocateMemory(
        VkDevice device,
        const VkMemoryAllocateInfo* pAllocateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDeviceMemory* pMemory);

    void on_vkFreeMemory(
        VkDevice device,
        VkDeviceMemory memory,
        const VkAllocationCallbacks* pAllocator);

    VkResult on_vkMapMemory(VkDevice device,
                            VkDeviceMemory memory,
                            VkDeviceSize offset,
                            VkDeviceSize size,
                            VkMemoryMapFlags flags,
                            void** ppData);

    void on_vkUnmapMemory(VkDevice device, VkDeviceMemory memory);

    uint8_t* getMappedHostPointer(VkDeviceMemory memory);
    VkDeviceSize getDeviceMemorySize(VkDeviceMemory memory);
    bool usingDirectMapping() const;

    // VK_ANDROID_native_buffer
    VkResult on_vkGetSwapchainGrallocUsageANDROID(
        VkDevice device,
        VkFormat format,
        VkImageUsageFlags imageUsage,
        int* grallocUsage);
    VkResult on_vkGetSwapchainGrallocUsage2ANDROID(
        VkDevice device,
        VkFormat format,
        VkImageUsageFlags imageUsage,
        VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
        uint64_t* grallocConsumerUsage,
        uint64_t* grallocProducerUsage);
    VkResult on_vkAcquireImageANDROID(
        VkDevice device,
        VkImage image,
        int nativeFenceFd,
        VkSemaphore semaphore,
        VkFence fence);
    VkResult on_vkQueueSignalReleaseImageANDROID(
        VkQueue queue,
        uint32_t waitSemaphoreCount,
        const VkSemaphore* pWaitSemaphores,
        VkImage image,
        int* pNativeFenceFd);

    // VK_GOOGLE_address_space
    VkResult on_vkMapMemoryIntoAddressSpaceGOOGLE(
       VkDevice device,
       VkDeviceMemory memory,
       uint64_t* pAddress);

    VkResult on_vkAllocateCommandBuffers(
            VkDevice device,
            const VkCommandBufferAllocateInfo* pAllocateInfo,
            VkCommandBuffer* pCommandBuffers);

    void on_vkCmdExecuteCommands(VkCommandBuffer commandBuffer,
                                 uint32_t commandBufferCount,
                                 const VkCommandBuffer* pCommandBuffers);

    VkResult on_vkQueueSubmit(VkQueue queue,
                          uint32_t submitCount,
                          const VkSubmitInfo* pSubmits,
                          VkFence fence);

    VkResult on_vkResetCommandBuffer(VkCommandBuffer commandBuffer,
                                     VkCommandBufferResetFlags flags);

    void on_vkFreeCommandBuffers(VkDevice device,
                                 VkCommandPool commandPool,
                                 uint32_t commandBufferCount,
                                 const VkCommandBuffer* pCommandBuffers);

    VkResult on_vkCreateCommandPool(VkDevice device,
                                    const VkCommandPoolCreateInfo* pCreateInfo,
                                    const VkAllocationCallbacks* pAllocator,
                                    VkCommandPool* pCommandPool);

    void on_vkDestroyCommandPool(VkDevice device,
                                 VkCommandPool commandPool,
                                 const VkAllocationCallbacks* pAllocator);

    VkResult on_vkResetCommandPool(VkDevice device,
                                   VkCommandPool commandPool,
                                   VkCommandPoolResetFlags flags);

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace goldfish_vk
