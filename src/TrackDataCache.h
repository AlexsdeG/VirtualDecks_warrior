#pragma once

#include <JuceHeader.h>
#include "BeatGrid.h"

/**
 * Data returned from the track data cache.
 *
 * Consolidates detected BPM, detection confidence, and the
 * beat grid (which may include manual overrides) into a single
 * structure keyed by file content hash.
 */
struct TrackData {
	/// Auto-detected BPM (0.0 if not yet analysed)
	double detectedBpm = 0.0;

	/// Confidence of the detection (0.0 to 1.0)
	double confidence = 0.0;

	/// Beat grid (may contain manual overrides)
	BeatGrid beatGrid;
};

/**
 * Persistent cache for per-track analysis data, keyed by file content hash.
 *
 * Data is stored as JSON files in ~/.otodecks/trackdata/<fileHash>.json.
 * This replaces the old identity-based BeatGridConfig for new data.
 */
class TrackDataCache {
public:
	/**
	 * Load cached track data for a given file hash.
	 *
	 * @param fileHash The content hash of the audio file
	 * @return TrackData with saved values, or defaults if no cache exists
	 */
	static TrackData load(const juce::String& fileHash);

	/**
	 * Save track data for a given file hash.
	 *
	 * @param fileHash The content hash of the audio file
	 * @param data The TrackData to persist
	 */
	static void save(const juce::String& fileHash, const TrackData& data);

	/**
	 * Check whether cached data exists for a given file hash.
	 *
	 * @param fileHash The content hash of the audio file
	 * @return true if a cache file exists on disk
	 */
	static bool exists(const juce::String& fileHash);

private:
	static juce::File getCacheDirectory();
	static juce::File getCacheFile(const juce::String& fileHash);
};
