#include "dxvk_barrier.h"

namespace dxvk {
  
  constexpr static VkAccessFlags AccessReadMask
    = VK_ACCESS_INDIRECT_COMMAND_READ_BIT
    | VK_ACCESS_INDEX_READ_BIT
    | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
    | VK_ACCESS_UNIFORM_READ_BIT
    | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT
    | VK_ACCESS_SHADER_READ_BIT
    | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
    | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
    | VK_ACCESS_TRANSFER_READ_BIT
    | VK_ACCESS_HOST_READ_BIT
    | VK_ACCESS_MEMORY_READ_BIT
    | VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT;
    
  constexpr static VkAccessFlags AccessWriteMask
    = VK_ACCESS_SHADER_WRITE_BIT
    | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    | VK_ACCESS_TRANSFER_WRITE_BIT
    | VK_ACCESS_HOST_WRITE_BIT
    | VK_ACCESS_MEMORY_WRITE_BIT
    | VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT
    | VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT;
  
  DxvkBarrierSet:: DxvkBarrierSet(DxvkCmdBuffer cmdBuffer)
  : m_cmdBuffer(cmdBuffer) {

  }


  DxvkBarrierSet::~DxvkBarrierSet() {

  }

  
  void DxvkBarrierSet::accessMemory(
          VkPipelineStageFlags      srcStages,
          VkAccessFlags             srcAccess,
          VkPipelineStageFlags      dstStages,
          VkAccessFlags             dstAccess) {
    DxvkAccessFlags access = this->getAccessTypes(srcAccess);

    m_srcStages |= srcStages;
    m_dstStages |= dstStages;
    
    m_srcAccess |= srcAccess & AccessWriteMask;

    if (access.test(DxvkAccess::Write))
      m_dstAccess |= dstAccess;
  }


  void DxvkBarrierSet::accessBuffer(
    const DxvkBufferSliceHandle&    bufSlice,
          VkPipelineStageFlags      srcStages,
          VkAccessFlags             srcAccess,
          VkPipelineStageFlags      dstStages,
          VkAccessFlags             dstAccess) {
    DxvkAccessFlags access = this->getAccessTypes(srcAccess);
    
    if (srcStages == VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
     || dstStages == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
      access.set(DxvkAccess::Write);
    
    m_srcStages |= srcStages;
    m_dstStages |= dstStages;
    
    m_srcAccess |= srcAccess & AccessWriteMask;

    if (access.test(DxvkAccess::Write))
      m_dstAccess |= dstAccess;

    m_bufSlices.insert(bufSlice.handle,
      DxvkBarrierBufferSlice(bufSlice.offset, bufSlice.length, access));
  }
  
  
  void DxvkBarrierSet::accessImage(
    const Rc<DxvkImage>&            image,
    const VkImageSubresourceRange&  subresources,
          VkImageLayout             srcLayout,
          VkPipelineStageFlags      srcStages,
          VkAccessFlags             srcAccess,
          VkImageLayout             dstLayout,
          VkPipelineStageFlags      dstStages,
          VkAccessFlags             dstAccess) {
    DxvkAccessFlags access = this->getAccessTypes(srcAccess);

    if (srcStages == VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
     || dstStages == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
     || srcLayout != dstLayout)
      access.set(DxvkAccess::Write);
    
    m_srcStages |= srcStages;
    m_dstStages |= dstStages;
    
    if (srcLayout == dstLayout) {
      m_srcAccess |= srcAccess & AccessWriteMask;

      if (access.test(DxvkAccess::Write))
        m_dstAccess |= dstAccess;
    } else {
      VkImageMemoryBarrier barrier;
      barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.pNext                       = nullptr;
      barrier.srcAccessMask               = srcAccess & AccessWriteMask;
      barrier.dstAccessMask               = dstAccess;
      barrier.oldLayout                   = srcLayout;
      barrier.newLayout                   = dstLayout;
      barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
      barrier.image                       = image->handle();
      barrier.subresourceRange            = subresources;
      barrier.subresourceRange.aspectMask = image->formatInfo()->aspectMask;
      m_imgBarriers.push_back(barrier);
    }

    m_imgSlices.insert(image->handle(),
      DxvkBarrierImageSlice(subresources, access));
  }


  void DxvkBarrierSet::releaseBuffer(
          DxvkBarrierSet&           acquire,
    const DxvkBufferSliceHandle&    bufSlice,
          uint32_t                  srcQueue,
          VkPipelineStageFlags      srcStages,
          VkAccessFlags             srcAccess,
          uint32_t                  dstQueue,
          VkPipelineStageFlags      dstStages,
          VkAccessFlags             dstAccess) {
    auto& release = *this;

    release.m_srcStages |= srcStages;
    acquire.m_dstStages |= dstStages;

    VkBufferMemoryBarrier barrier;
    barrier.sType                       = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext                       = nullptr;
    barrier.srcAccessMask               = srcAccess & AccessWriteMask;
    barrier.dstAccessMask               = 0;
    barrier.srcQueueFamilyIndex         = srcQueue;
    barrier.dstQueueFamilyIndex         = dstQueue;
    barrier.buffer                      = bufSlice.handle;
    barrier.offset                      = bufSlice.offset;
    barrier.size                        = bufSlice.length;
    release.m_bufBarriers.push_back(barrier);

    barrier.srcAccessMask               = 0;
    barrier.dstAccessMask               = dstAccess;
    acquire.m_bufBarriers.push_back(barrier);

    DxvkAccessFlags access(DxvkAccess::Read, DxvkAccess::Write);
    release.m_bufSlices.insert(bufSlice.handle,
      DxvkBarrierBufferSlice(bufSlice.offset, bufSlice.length, access));
    acquire.m_bufSlices.insert(bufSlice.handle,
      DxvkBarrierBufferSlice(bufSlice.offset, bufSlice.length, access));
  }


  void DxvkBarrierSet::releaseImage(
          DxvkBarrierSet&           acquire,
    const Rc<DxvkImage>&            image,
    const VkImageSubresourceRange&  subresources,
          uint32_t                  srcQueue,
          VkImageLayout             srcLayout,
          VkPipelineStageFlags      srcStages,
          VkAccessFlags             srcAccess,
          uint32_t                  dstQueue,
          VkImageLayout             dstLayout,
          VkPipelineStageFlags      dstStages,
          VkAccessFlags             dstAccess) {
    auto& release = *this;

    release.m_srcStages |= srcStages;
    acquire.m_dstStages |= dstStages;

    VkImageMemoryBarrier barrier;
    barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext                       = nullptr;
    barrier.srcAccessMask               = srcAccess & AccessWriteMask;
    barrier.dstAccessMask               = 0;
    barrier.oldLayout                   = srcLayout;
    barrier.newLayout                   = dstLayout;
    barrier.srcQueueFamilyIndex         = srcQueue;
    barrier.dstQueueFamilyIndex         = dstQueue;
    barrier.image                       = image->handle();
    barrier.subresourceRange            = subresources;
    barrier.subresourceRange.aspectMask = image->formatInfo()->aspectMask;
    release.m_imgBarriers.push_back(barrier);

    if (srcQueue == dstQueue)
      barrier.oldLayout = dstLayout;

    barrier.srcAccessMask               = 0;
    barrier.dstAccessMask               = dstAccess;
    acquire.m_imgBarriers.push_back(barrier);

    DxvkAccessFlags access(DxvkAccess::Read, DxvkAccess::Write);
    release.m_imgSlices.insert(image->handle(),
      DxvkBarrierImageSlice(subresources, access));
    acquire.m_imgSlices.insert(image->handle(),
      DxvkBarrierImageSlice(subresources, access));
  }


  bool DxvkBarrierSet::isBufferDirty(
    const DxvkBufferSliceHandle&    bufSlice,
          DxvkAccessFlags           bufAccess) {
    return m_bufSlices.isDirty(bufSlice.handle,
      DxvkBarrierBufferSlice(bufSlice.offset, bufSlice.length, bufAccess));
  }


  bool DxvkBarrierSet::isImageDirty(
    const Rc<DxvkImage>&            image,
    const VkImageSubresourceRange&  imgSubres,
          DxvkAccessFlags           imgAccess) {
    return m_imgSlices.isDirty(image->handle(),
      DxvkBarrierImageSlice(imgSubres, imgAccess));
  }


  DxvkAccessFlags DxvkBarrierSet::getBufferAccess(
    const DxvkBufferSliceHandle&    bufSlice) {
    return m_bufSlices.getAccess(bufSlice.handle,
      DxvkBarrierBufferSlice(bufSlice.offset, bufSlice.length, 0));
  }

  
  DxvkAccessFlags DxvkBarrierSet::getImageAccess(
    const Rc<DxvkImage>&            image,
    const VkImageSubresourceRange&  imgSubres) {
    return m_imgSlices.getAccess(image->handle(),
      DxvkBarrierImageSlice(imgSubres, 0));
  }


  void DxvkBarrierSet::recordCommands(const Rc<DxvkCommandList>& commandList) {
    if (m_srcStages | m_dstStages) {
      VkPipelineStageFlags srcFlags = m_srcStages;
      VkPipelineStageFlags dstFlags = m_dstStages;
      
      if (!srcFlags) srcFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      if (!dstFlags) dstFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

      if (commandList->canUseSynchronization2()) {
        // VK_KHR_synchronization2 path. The accumulated 32-bit stage/access
        // masks above apply to every barrier in this batch either way (that
        // is exactly what the classic vkCmdPipelineBarrier call below also
        // does - one global srcStageMask/dstStageMask for the whole batch),
        // so converting is a direct 1:1 widen of each VkXxxMemoryBarrier
        // into a VkXxxMemoryBarrier2 with those same two stage masks
        // attached per-barrier, as VkDependencyInfo requires. Every classic
        // VK_ACCESS_*_BIT / VK_PIPELINE_STAGE_*_BIT constant has the same
        // numeric value in the 64-bit VkAccessFlags2/VkPipelineStageFlags2
        // types by design, so this widen is exact, not approximate.
        VkMemoryBarrier2KHR memBarrier2;
        memBarrier2.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR;
        memBarrier2.pNext = nullptr;
        memBarrier2.srcStageMask  = srcFlags;
        memBarrier2.srcAccessMask = m_srcAccess;
        memBarrier2.dstStageMask  = dstFlags;
        memBarrier2.dstAccessMask = m_dstAccess;

        std::vector<VkBufferMemoryBarrier2KHR> bufBarriers2(m_bufBarriers.size());
        for (size_t i = 0; i < m_bufBarriers.size(); i++) {
          const VkBufferMemoryBarrier& src = m_bufBarriers[i];
          VkBufferMemoryBarrier2KHR& dst = bufBarriers2[i];
          dst.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
          dst.pNext               = nullptr;
          dst.srcStageMask        = srcFlags;
          dst.srcAccessMask       = src.srcAccessMask;
          dst.dstStageMask        = dstFlags;
          dst.dstAccessMask       = src.dstAccessMask;
          dst.srcQueueFamilyIndex = src.srcQueueFamilyIndex;
          dst.dstQueueFamilyIndex = src.dstQueueFamilyIndex;
          dst.buffer              = src.buffer;
          dst.offset               = src.offset;
          dst.size                 = src.size;
        }

        std::vector<VkImageMemoryBarrier2KHR> imgBarriers2(m_imgBarriers.size());
        for (size_t i = 0; i < m_imgBarriers.size(); i++) {
          const VkImageMemoryBarrier& src = m_imgBarriers[i];
          VkImageMemoryBarrier2KHR& dst = imgBarriers2[i];
          dst.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
          dst.pNext               = nullptr;
          dst.srcStageMask        = srcFlags;
          dst.srcAccessMask       = src.srcAccessMask;
          dst.dstStageMask        = dstFlags;
          dst.dstAccessMask       = src.dstAccessMask;
          dst.oldLayout           = src.oldLayout;
          dst.newLayout           = src.newLayout;
          dst.srcQueueFamilyIndex = src.srcQueueFamilyIndex;
          dst.dstQueueFamilyIndex = src.dstQueueFamilyIndex;
          dst.image               = src.image;
          dst.subresourceRange    = src.subresourceRange;
        }

        VkDependencyInfoKHR depInfo;
        depInfo.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
        depInfo.pNext                    = nullptr;
        depInfo.dependencyFlags          = 0;
        depInfo.memoryBarrierCount       = (m_srcAccess | m_dstAccess) ? 1 : 0;
        depInfo.pMemoryBarriers          = &memBarrier2;
        depInfo.bufferMemoryBarrierCount = bufBarriers2.size();
        depInfo.pBufferMemoryBarriers    = bufBarriers2.data();
        depInfo.imageMemoryBarrierCount  = imgBarriers2.size();
        depInfo.pImageMemoryBarriers     = imgBarriers2.data();

        commandList->cmdPipelineBarrier2(m_cmdBuffer, &depInfo);
      } else {
        VkMemoryBarrier memBarrier;
        memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memBarrier.pNext = nullptr;
        memBarrier.srcAccessMask = m_srcAccess;
        memBarrier.dstAccessMask = m_dstAccess;

        VkMemoryBarrier* pMemBarrier = nullptr;
        if (m_srcAccess | m_dstAccess)
          pMemBarrier = &memBarrier;

        commandList->cmdPipelineBarrier(
          m_cmdBuffer, srcFlags, dstFlags, 0,
          pMemBarrier ? 1 : 0, pMemBarrier,
          m_bufBarriers.size(),
          m_bufBarriers.data(),
          m_imgBarriers.size(),
          m_imgBarriers.data());
      }

      commandList->addStatCtr(DxvkStatCounter::CmdBarrierCount, 1);

      this->reset();
    }
  }
  
  
  void DxvkBarrierSet::reset() {
    m_srcStages = 0;
    m_dstStages = 0;

    m_srcAccess = 0;
    m_dstAccess = 0;
    
    m_bufBarriers.resize(0);
    m_imgBarriers.resize(0);

    m_bufSlices.clear();
    m_imgSlices.clear();
  }
  
  
  DxvkAccessFlags DxvkBarrierSet::getAccessTypes(VkAccessFlags flags) {
    DxvkAccessFlags result;
    if (flags & AccessReadMask)  result.set(DxvkAccess::Read);
    if (flags & AccessWriteMask) result.set(DxvkAccess::Write);
    return result;
  }
  
}
