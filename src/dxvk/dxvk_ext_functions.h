#pragma once

#include "dxvk_include.h"

namespace dxvk {

  /**
   * \brief Extra device-level Vulkan entry points
   *
   * src/dxvk normally calls into Vulkan through the generated \c vk::DeviceFn
   * dispatch table in src/vulkan/vulkan_loader.h. That file was not part of
   * the source dump this patch was built from (only src/dxvk was provided),
   * so the handful of Vulkan 1.3-era commands this patch adds calls to -
   * dynamic rendering, the extended_dynamic_state cull/front-face setters,
   * and synchronization2 - are resolved independently here, instead of
   * assuming they already exist as members of a class this patch cannot
   * see or verify.
   *
   * dxvk_ext_functions.cpp resolves vkGetDeviceProcAddr straight from
   * vulkan-1.dll (a plain DLL export per the Vulkan loader spec, so this
   * does not depend on any particular member of vk::DeviceFn/LibraryFn
   * existing under a given name - an earlier version of this file assumed
   * vk::DeviceFn exposed vkGetDeviceProcAddr as a member, which is the same
   * kind of assumption that failed to compile for vk::LibraryFn::
   * vkEnumerateInstanceVersion elsewhere in this patch).
   *
   * If a driver does not support the extension a given pointer belongs to,
   * that pointer stays \c nullptr. Every call site in this patch checks the
   * corresponding DxvkDeviceFeatures bit before calling through one of
   * these, so a null pointer here is never dereferenced; this struct does
   * not re-implement that gating.
   */
  struct DxvkExtDeviceFunctions {
    // VK_KHR_dynamic_rendering
    PFN_vkCmdBeginRenderingKHR      vkCmdBeginRenderingKHR      = nullptr;
    PFN_vkCmdEndRenderingKHR        vkCmdEndRenderingKHR        = nullptr;

    // VK_EXT_extended_dynamic_state (the two sub-states this patch wires up
    // end-to-end; the remaining EDS1/2/3 setters follow the same pattern -
    // see the note in dxvk_context.cpp next to setRasterizerState)
    PFN_vkCmdSetCullModeEXT         vkCmdSetCullModeEXT         = nullptr;
    PFN_vkCmdSetFrontFaceEXT        vkCmdSetFrontFaceEXT        = nullptr;

    // VK_KHR_synchronization2
    PFN_vkCmdPipelineBarrier2KHR    vkCmdPipelineBarrier2KHR    = nullptr;

    /**
     * \brief Resolves every pointer above via vkGetDeviceProcAddr
     *
     * Safe to call even when the owning extension is not supported/enabled:
     * vkGetDeviceProcAddr returns \c nullptr for an unknown name rather than
     * failing, so unsupported entries are simply left null.
     * \param [in] vkd Device dispatch table (already-created VkDevice)
     */
    void init(const Rc<vk::DeviceFn>& vkd);
  };

}
