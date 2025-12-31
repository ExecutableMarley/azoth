#pragma once

#include <string>
#include <stdint.h>
#include <vector>

namespace Azoth
{

class ProcessImage;

bool retrieveProcessIDByName(const std::string& procName, std::uint32_t& out_process_id);

bool retrieveProcessIDByName(const std::string& procName, std::vector<std::uint32_t>& out_process_ids);

bool retrieveProcessIDByNameFallback(const std::string& process_name, std::uint32_t& out_process_id);

bool retrieveProcessIDByNameFallback(const std::string& process_name, std::vector<std::uint32_t>& out_process_ids);

bool retrieveProcessIDByWindowName(const std::string& windowName, std::uint32_t& out_process_id);

//

bool retrieveProcessMainImage(uint32_t pid, ProcessImage& procImage);

bool retrieveProcessImage(uint32_t pid, const std::string& imageName, ProcessImage& procImage);

bool retrieveAllProcessImages(uint32_t pid, std::vector<ProcessImage>& procImages);

}