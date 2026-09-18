#include "dxvk_ext_functions.h"
#include "dxvk_device.h"

namespace dxvk {

  void DxvkExtDeviceFunctions::init(const Rc<vk::DeviceFn>& vkd) {
    VkDevice device = vkd->device();

    // NOTE: see the header comment - this assumes vk::DeviceFn exposes
    // vkGetDeviceProcAddr as a public member. If this fork's dispatch
    // table names it differently, this is the only line to change.
    auto load = [device, &vkd] (const char* name) {
      return vkd->vkGetDeviceProcAddr(device, name);
    };

    vkCmdBeginRenderingKHR   = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(load("vkCmdBeginRenderingKHR"));
    vkCmdEndRenderingKHR     = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(load("vkCmdEndRenderingKHR"));

    vkCmdSetCullModeEXT      = reinterpret_cast<PFN_vkCmdSetCullModeEXT>(load("vkCmdSetCullModeEXT"));
    vkCmdSetFrontFaceEXT     = reinterpret_cast<PFN_vkCmdSetFrontFaceEXT>(load("vkCmdSetFrontFaceEXT"));

    vkCmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(load("vkCmdPipelineBarrier2KHR"));
  }

}
