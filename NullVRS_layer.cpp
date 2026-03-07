#include <vulkan/vulkan.h>
#include <vulkan/vk_layer.h>
#include <vulkan/utility/vk_dispatch_table.h>

#include <assert.h>
#include <string.h>

#include <mutex>
#include <map>

#undef VK_LAYER_EXPORT
#if defined(WIN32)
#define VK_LAYER_EXPORT extern "C" __declspec(dllexport)
#else
#define VK_LAYER_EXPORT extern "C"
#endif

#define VRS_LAYERNAME "VK_LAYER_TROVLABS_nullVRS"

// single global lock, for simplicity
std::mutex global_lock;
typedef std::lock_guard<std::mutex> scoped_lock;

// use the loader's dispatch table pointer as a key for dispatch map lookups
template<typename DispatchableType>
void *GetKey(DispatchableType inst)
{
    return *(void **)inst;
}

// layer book-keeping information, to store dispatch tables by key
std::map<void *, VkuInstanceDispatchTable> instance_dispatch;
std::map<void *, VkuDeviceDispatchTable> device_dispatch;


//
// Instance functions
//

VK_LAYER_EXPORT VkResult VKAPI_CALL NullVRS_CreateInstance(const VkInstanceCreateInfo *pCreateInfo,
                                                           const VkAllocationCallbacks *pAllocator,
                                                           VkInstance *pInstance)
{	
	VkLayerInstanceCreateInfo *chain_info = (VkLayerInstanceCreateInfo *)pCreateInfo->pNext;
    while (chain_info 
            && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO 
            && chain_info->function == VK_LAYER_LINK_INFO)) {
        chain_info = (VkLayerInstanceCreateInfo *)chain_info->pNext;
    }

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = 
        chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = 
        (PFN_vkCreateInstance)fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element of the chain.
    // This ensures that the next layer gets it's layer info and not
    // the info for our current layer.
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    // Continue call down the chain
    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
        return result;

    // Init layer's dispatch table using GetInstanceProcAddr of
    // next layer in the chain.
    VkuInstanceDispatchTable instance_dispatch_table;
    vkuInitInstanceDispatchTable(*pInstance, &instance_dispatch_table, fpGetInstanceProcAddr);

    {
        scoped_lock l(global_lock);
        instance_dispatch[GetKey(*pInstance)] = instance_dispatch_table;
    }

    return VK_SUCCESS;
}


VK_LAYER_EXPORT void VKAPI_CALL NullVRS_DestroyInstance(VkInstance instance, 
                                                        const VkAllocationCallbacks* pAllocator)
{
    scoped_lock l(global_lock);
    instance_dispatch[GetKey(instance)].DestroyInstance(instance, pAllocator);
    instance_dispatch.erase(GetKey(instance));
}


VK_LAYER_EXPORT VkResult VKAPI_CALL NullVRS_EnumerateInstanceLayerProperties(
    uint32_t *pPropertyCount,
    VkLayerProperties *pProperties)
{
    if(pPropertyCount) *pPropertyCount = 1;

    if(pProperties)
    {
        strcpy(pProperties->layerName, VRS_LAYERNAME);
        strcpy(pProperties->description, "Intercepts & no-ops Variable Rate Shading commands");
        pProperties->implementationVersion = 1;
        pProperties->specVersion = VK_API_VERSION_1_3;
    }

    return VK_SUCCESS;
}


VK_LAYER_EXPORT VkResult VKAPI_CALL NullVRS_EnumerateInstanceExtensionProperties(
    const char *pLayerName, 
    uint32_t *pPropertyCount, 
    VkExtensionProperties *pProperties)
{
    if(pLayerName == NULL || strcmp(pLayerName, VRS_LAYERNAME))
        return VK_ERROR_LAYER_NOT_PRESENT;

    // don't expose any extensions
    if(pPropertyCount) *pPropertyCount = 0;
        return VK_SUCCESS;
}


//
// Device functions
//

VK_LAYER_EXPORT VkResult VKAPI_CALL NullVRS_CreateDevice(VkPhysicalDevice physicalDevice,
                                                         const VkDeviceCreateInfo* pCreateInfo,
                                                         const VkAllocationCallbacks* pAllocator,
                                                         VkDevice* pDevice)
{
	VkLayerDeviceCreateInfo *chain_info = (VkLayerDeviceCreateInfo *)pCreateInfo->pNext;
    while (chain_info 
            && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO 
            && chain_info->function == VK_LAYER_LINK_INFO)) {
        chain_info = (VkLayerDeviceCreateInfo *)chain_info->pNext;
    }

    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = 
        chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = 
        chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = 
        (PFN_vkCreateDevice)fpGetInstanceProcAddr(NULL, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain.
    // This ensures that the next layer gets it's layer info and not
    // the info for our current layer.
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Init layer's dispatch table using GetDeviceProcAddr of next layer
    VkuDeviceDispatchTable device_dispatch_table;
    vkuInitDeviceDispatchTable(*pDevice,
                               &device_dispatch_table,
                               fpGetDeviceProcAddr);

    // store the table by key
    {
        scoped_lock l(global_lock);
        device_dispatch[GetKey(*pDevice)] = device_dispatch_table;
    }

    return VK_SUCCESS;
}


VK_LAYER_EXPORT void VKAPI_CALL NullVRS_DestroyDevice(VkDevice device,
                                                      const VkAllocationCallbacks* pAllocator)
{
    scoped_lock l(global_lock);
    device_dispatch[GetKey(device)].DestroyDevice(device, pAllocator);
    device_dispatch.erase(GetKey(device));
}


VK_LAYER_EXPORT VkResult VKAPI_CALL NullVRS_EnumerateDeviceLayerProperties(
    VkPhysicalDevice physicalDevice, 
    uint32_t *pPropertyCount, 
    VkLayerProperties *pProperties)
{
    return NullVRS_EnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}


VK_LAYER_EXPORT VkResult VKAPI_CALL NullVRS_EnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice, const char *pLayerName,
    uint32_t *pPropertyCount, VkExtensionProperties *pProperties)
{
    // pass through any queries that aren't to us
    if(pLayerName == NULL || strcmp(pLayerName, VRS_LAYERNAME)) {
        if(physicalDevice == VK_NULL_HANDLE)
            return VK_SUCCESS;

        scoped_lock l(global_lock);
        return instance_dispatch[GetKey(physicalDevice)].EnumerateDeviceExtensionProperties(
                physicalDevice, 
                pLayerName, 
                pPropertyCount, 
                pProperties);
    }

    // don't expose any extensions
    if(pPropertyCount) *pPropertyCount = 0;
        return VK_SUCCESS;
}


//
// Layer intercepted functions
//

VK_LAYER_EXPORT void VKAPI_CALL NullVRS_CmdSetFragmentShadingRateKHR(
    VkCommandBuffer commandBuffer,
    const VkExtent2D* pFragmentSize,
    const VkFragmentShadingRateCombinerOpKHR combinerOps[2])
{
    // do nothing
    return;
}


//
// Dispatch chain redirect functions
//

#define GETPROCADDR(func) if(!strcmp(pName, "vk" #func)) return (PFN_vkVoidFunction)&NullVRS_##func;


VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL NullVRS_GetDeviceProcAddr(
    VkDevice device,
    const char *pName)
{
    // device chain functions we intercept
    GETPROCADDR(GetDeviceProcAddr);
    GETPROCADDR(EnumerateDeviceLayerProperties);
    GETPROCADDR(EnumerateDeviceExtensionProperties);
    GETPROCADDR(CreateDevice);
    GETPROCADDR(DestroyDevice);
    GETPROCADDR(CmdSetFragmentShadingRateKHR);
  
    {
        scoped_lock l(global_lock);
        return device_dispatch[GetKey(device)].GetDeviceProcAddr(device, pName);
    }
}


VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI_CALL NullVRS_GetInstanceProcAddr(
    VkInstance instance,
    const char *pName)
{
    // instance chain functions we intercept
    GETPROCADDR(GetInstanceProcAddr);
    GETPROCADDR(EnumerateInstanceLayerProperties);
    GETPROCADDR(EnumerateInstanceExtensionProperties);
    GETPROCADDR(CreateInstance);
    GETPROCADDR(DestroyInstance);

    // device chain functions we intercept
    GETPROCADDR(GetDeviceProcAddr);
    GETPROCADDR(EnumerateDeviceLayerProperties);
    GETPROCADDR(EnumerateDeviceExtensionProperties);
    GETPROCADDR(CreateDevice);
    GETPROCADDR(DestroyDevice);
    GETPROCADDR(CmdSetFragmentShadingRateKHR);

    {
        scoped_lock l(global_lock);
        return instance_dispatch[GetKey(instance)].GetInstanceProcAddr(instance, pName);
    }
}
