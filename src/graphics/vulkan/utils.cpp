#include <graphics/vulkan/utils.h>

namespace sublimation {

namespace vkw {

namespace utils {

std::string errorString(VkResult errorCode) {
    switch (errorCode) {
#define STR(r)   \
    case VK_##r: \
        return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
#undef STR
        default:
            return "UNKNOWN_ERROR";
    }
}

VkShaderModule loadShader(const std::string& filename, VkDevice device) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (file.is_open()) {
        size_t filesize = file.tellg();
        std::vector<char> buf(filesize);

        file.seekg(0);
        file.read(buf.data(), filesize);
        file.close();

        VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = buf.size(),
            .pCode = (const uint32_t*)buf.data()
        };

        VkShaderModule shaderModule;
        CHECK_VKRESULT(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

        return shaderModule;
    } else {
        std::cerr << "ERROR::utils:loadSPIRVShader: failed to open file " << filename << "!\n";
        return VK_NULL_HANDLE;
    }
}

} // namespace utils

} // namespace vkw

} //namespace sublimation
