#pragma once

#include <volk.h>

#include <assert.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#define CHECK_VKRESULT(f)                                                                                                                                              \
    {                                                                                                                                                                  \
        VkResult res = (f);                                                                                                                                            \
        const char* func = #f;                                                                                                                                         \
        if (res != VK_SUCCESS) {                                                                                                                                       \
            std::cerr << "FATAL ERROR : " << func << " VkResult=\"" << sublimation::vkw::utils::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
            assert(res == VK_SUCCESS);                                                                                                                                 \
        }                                                                                                                                                              \
    }

namespace sublimation {

namespace vkw {

namespace utils {

std::string errorString(VkResult errorCode);

VkShaderModule loadShader(const std::string& filename, VkDevice device);

} // namespace utils

} // namespace vkw

} // namespace sublimation