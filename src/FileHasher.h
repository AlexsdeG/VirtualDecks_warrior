#pragma once

#include <JuceHeader.h>

/**
 * Utility for computing a content-based hash of an audio file.
 *
 * Reads the first and last 64 KB of the file plus the file size
 * to produce a hex string. This allows the same file to be
 * recognised across re-imports without reading the entire file.
 */
class FileHasher {
public:
	/**
	 * Compute a partial-content hash of the given file.
	 *
	 * Uses FNV-1a (64-bit) over the first+last 64 KB and file size.
	 *
	 * @param file The audio file to hash
	 * @return Hex string of the hash, or empty string on failure
	 */
	static juce::String computeHash(const juce::File& file)
	{
		if (!file.existsAsFile())
			return {};

		auto fileSize = file.getSize();
		if (fileSize <= 0)
			return {};

		juce::MemoryBlock data;
		constexpr juce::int64 chunkSize = 65536; // 64 KB

		juce::FileInputStream stream(file);
		if (!stream.openedOk())
			return {};

		if (fileSize <= chunkSize * 2)
		{
			data.setSize(static_cast<size_t>(fileSize));
			stream.read(data.getData(), static_cast<int>(fileSize));
		}
		else
		{
			juce::MemoryBlock head(static_cast<size_t>(chunkSize));
			stream.read(head.getData(), static_cast<int>(chunkSize));

			juce::MemoryBlock tail(static_cast<size_t>(chunkSize));
			stream.setPosition(fileSize - chunkSize);
			stream.read(tail.getData(), static_cast<int>(chunkSize));

			data.append(head.getData(), head.getSize());
			data.append(tail.getData(), tail.getSize());
		}

		// Append file size for extra uniqueness
		data.append(&fileSize, sizeof(fileSize));

		// FNV-1a 64-bit hash
		const auto* bytes = static_cast<const uint8_t*>(data.getData());
		uint64_t h = 14695981039346656037ULL;
		for (size_t i = 0; i < data.getSize(); ++i)
		{
			h ^= static_cast<uint64_t>(bytes[i]);
			h *= 1099511628211ULL;
		}

		return juce::String::toHexString(static_cast<juce::int64>(h));
	}
};
