#pragma once

#include <stdint.h>
#include <string>

#include "../Types/Address.hpp"
#include "../Types/MemoryRange.hpp"

namespace Azoth
{


/**
 * @brief Represents a loaded executable image or shared library in a process.
 *
 * A ProcessImage describes a contiguous memory range that corresponds to
 * a mapped binary image (e.g. executable or shared object), along with
 * its identifying metadata.
 *
 * This class is purely descriptive and does not imply ownership or lifetime
 * management of the underlying memory.
 */
class ProcessImage
{
public:
	/**
     * @brief Construct an invalid / empty process image.
     */
	ProcessImage() : baseAddress(0), size(0) {}

	/**
     * @brief Construct a process image with full metadata.
     *
     * @param baseAddr Base address where the image is mapped.
     * @param size     Size of the mapped image in bytes.
     * @param name     File name of the image (without path).
     * @param path     Full file system path of the image.
     */
	ProcessImage(uint64_t baseAddr, size_t size, std::string name, std::string path)
		: baseAddress(baseAddr), size(size), name(std::move(name)), 
		path(std::move(path)) { }

	/**
     * @brief Construct a process image with full metadata.
     *
     * @param baseAddr Base address where the image is mapped.
     * @param size     Size of the mapped image in bytes.
     * @param path     Full file system path of the image.
	 * 
	 * The image name is then derived from the path by the implementation.
     */
	ProcessImage(uint64_t baseAddr, size_t size, std::string path);

	uint64_t baseAddress; ///< Base address of the image mapping
	size_t size;          ///< Size of the image in bytes
	std::string name;     ///< Image file name (without directory)
	std::string path;     ///< Full file system path to the image

	/**
     * @brief Check whether the image represents a valid mapping.
     *
     * @return True if baseAddress is non-zero and size is non-zero.
     */
	bool valid() const noexcept { return baseAddress != 0 && size != 0; }

	/**
     * @brief Boolean conversion indicating validity.
     */
	operator bool() const { return valid(); }

	/**
     * @brief Convert the image to its corresponding memory range.
     *
     * @return MemoryRange covering the entire image mapping.
     */
	operator MemoryRange() const { return MemoryRange(baseAddress, baseAddress + size); };
};

class ImageSymbol
{
	ImageSymbol() : address(0), index(0) {}
	ImageSymbol(uint64_t addr, uint32_t idx, std::string name)
		: address(addr), index(idx), name(std::move(name)) {}

	uint64_t address;
	uint32_t index;
	std::string name;

	bool valid() const noexcept { return address != 0; }
};


}