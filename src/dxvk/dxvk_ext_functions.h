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
   * and synchronization2 - are resolved independently here via
   * vkGetDeviceProcAddr, instead of assuming they already exist as members
   * of a class this patch cannot see or verify.
   *
   * This assumes \c vk::DeviceFn (the class behind \c DxvkDevice::vkd())
   * exposes \c vkGetDeviceProcAddr as a callable member, which is true for
   * stock DXVK's generated dispatch tables (every class of this kind needs
   * that entry point to bootstrap its own other members, so it is kept
   * public) and should hold for this fork too. If this fork's copy of
   * vulkan_loader.h names that member differently, update the one call site
   * in dxvk_ext_functions.cpp - nothing else here depends on its name.
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
