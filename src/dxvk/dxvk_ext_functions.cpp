#include "dxvk_ext_functions.h"
#include "dxvk_device.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace dxvk {

  void DxvkExtDeviceFunctions::init(const Rc<vk::DeviceFn>& vkd) {
    VkDevice device = vkd->device();

    // NOTE: this used to call vkd->vkGetDeviceProcAddr(device, name), on the
    // assumption that vk::DeviceFn exposes that member (true for stock
    // DXVK's generated dispatch tables). That is the same kind of assumption
    // that turned out to be wrong for vk::LibraryFn::vkEnumerateInstanceVersion
    // in dxvk_instance.cpp on this fork, so rather than guess again here,
    // this resolves vkGetDeviceProcAddr directly from vulkan-1.dll, which is
    // required by the Vulkan loader specification to export it as a plain
    // DLL symbol - this holds regardless of what this fork's own vk::DeviceFn
    // happens to expose, since m_vkl/m_vkd must already have loaded that
    // exact module to get this far (DxvkDevice::vkd() only exists after
    // vkCreateDevice succeeded).
    HMODULE vulkanModule = GetModuleHandleA("vulkan-1.dll");

    PFN_vkGetDeviceProcAddr getDeviceProcAddr = vulkanModule
      ? reinterpret_cast<PFN_vkGetDeviceProcAddr>(
          GetProcAddress(vulkanModule, "vkGetDeviceProcAddr"))
      : nullptr;

    if (!getDeviceProcAddr)
      return;

    auto load = [device, getDeviceProcAddr] (const char* name) {
      return getDeviceProcAddr(device, name);
    };

    vkCmdBeginRenderingKHR   = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(load("vkCmdBeginRenderingKHR"));
    vkCmdEndRenderingKHR     = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(load("vkCmdEndRenderingKHR"));

    vkCmdSetCullModeEXT      = reinterpret_cast<PFN_vkCmdSetCullModeEXT>(load("vkCmdSetCullModeEXT"));
    vkCmdSetFrontFaceEXT     = reinterpret_cast<PFN_vkCmdSetFrontFaceEXT>(load("vkCmdSetFrontFaceEXT"));

    vkCmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(load("vkCmdPipelineBarrier2KHR"));
  }

}
