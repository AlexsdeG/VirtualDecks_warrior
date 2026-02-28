
#include "DJAudioPlayer.h"
#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>

//============================================================================== 

/**
 * Implementation of a constructor for DJAudioPlayer
 *
 * Initializes juce::AudioFormatManager pointer data member
 *
 */
DJAudioPlayer::DJAudioPlayer(juce::AudioFormatManager& _formatManager)
	: formatManager(_formatManager)
{
};

/**
 * Implementation of a destructor for DJAudioPlayer
 *
 */
DJAudioPlayer::~DJAudioPlayer() {};

//==============================================================================

/**
 * Implementation of prepareToPlay method for DJAudioPlayer
 *
 * Calls prepareToPlay methods on all AudioSource data members and saves the sample rate
 *
 */
void DJAudioPlayer::prepareToPlay(int samplesPerBlockExpected, double sampleRate) {
	transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
	resampleSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
	audioLPFilter.prepareToPlay(samplesPerBlockExpected, sampleRate);
	audioHPFilter.prepareToPlay(samplesPerBlockExpected, sampleRate);
	audioLBFilter.prepareToPlay(samplesPerBlockExpected, sampleRate);
	audioMBFilter.prepareToPlay(samplesPerBlockExpected, sampleRate);
	audioHBFilter.prepareToPlay(samplesPerBlockExpected, sampleRate);
	thisSampleRate = sampleRate;
};

/**
 * Implementation of getNextAudioBlock method for DJAudioPlayer
 *
 * Calls getNextAudioBlock methods on the main AudioSource data member and updates the root mean square value
 *
 */
void DJAudioPlayer::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
	audioLPFilter.getNextAudioBlock(bufferToFill);
	float rmsLevelLeft = juce::Decibels::gainToDecibels(bufferToFill.buffer->getRMSLevel(0, 0, bufferToFill.buffer->getNumSamples()));
	float rmsLevelRight = juce::Decibels::gainToDecibels(bufferToFill.buffer->getRMSLevel(1, 0, bufferToFill.buffer->getNumSamples()));
	level = (rmsLevelLeft + rmsLevelRight) / 2;
};

/**
 * Implementation of releaseResources method for DJAudioPlayer
 *
 * Calls releaseResources methods on the main AudioSource data member
 *
 */
void DJAudioPlayer::releaseResources() {
	audioLPFilter.releaseResources();
};

//============================================================================== 

/**
 * Implementation of start method for DJAudioPlayer
 *
 *  Calls the start method on the AudioTransportSource data member
 *
 */
void DJAudioPlayer::start() {
	transportSource.start();
};

/**
 * Implementation of stop method for DJAudioPlayer
 *
 *  Calls the stop method on the AudioTransportSource data member
 *
 */
void DJAudioPlayer::stop() {
	transportSource.stop();
};

/**
 * Implementation of isPlaying method for DJAudioPlayer
 *
 * Returns if the AudioTransportSource data member is playing
 *
 */
bool DJAudioPlayer::isPlaying() {
	return transportSource.isPlaying();
}

/**
 * Implementation of isLoaded method for DJAudioPlayer
 *
 * Returns the loaded data member
 *
 */
bool DJAudioPlayer::isLoaded() {
	return loaded;
}

/**
 * Implementation of returnURL method for DJAudioPlayer
 *
 * Returns the currentAudioURL data member
 *
 */
juce::URL DJAudioPlayer::returnURL() {
	return currentAudioURL;
}

/**
 * Implementation of loadURL method for DJAudioPlayer
 *
 * Creates a reader for the juce::URL and parses it into a juce::AudioFormatReaderSource
 * The AudioTransportSource data member sets it source using the juce::AudioFormatReaderSource
 *
 */
void DJAudioPlayer::loadURL(juce::URL audioURL) {
	auto* reader = formatManager.createReaderFor(audioURL.createInputStream(false));
	if (reader != nullptr) {
		std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
		transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
		readerSource.reset(newSource.release());
		DBG("real metadata size: " << reader->metadataValues.size());
		loadedFileName = audioURL.getFileName();
		loaded = true;
		currentAudioURL = audioURL;

		// Detect BPM: try metadata first, then onset-based analysis
		detectedBpm = 0.0;
		beatGrid = BeatGrid();

		juce::String filePath = audioURL.getLocalFile().getFullPathName();
		detectedBpm = readBpmFromMetadata(filePath);

		if (detectedBpm <= 0.0) {
			auto* analysisReader = formatManager.createReaderFor(audioURL.createInputStream(false));
			if (analysisReader != nullptr) {
				detectedBpm = detectBpmFromAudio(analysisReader);
				delete analysisReader;
			}
		}

		if (detectedBpm > 0.0) {
			beatGrid.bpm = detectedBpm;
			DBG("BPM detected: " << detectedBpm);
		}
	}
	else
	{
		DBG("Something went wrong loading the file ");
		loaded = false;
	}
};

//==============================================================================

/**
 * Implementation of getRMSLevel method for DJAudioPlayer
 *
 * Returns the level data member
 *
 */
float DJAudioPlayer::getRMSLevel() {
	return level;
};

/**
 * Implementation of getPositionRelative method for DJAudioPlayer
 *
 * Returns the relative position of the AudioTransportSource data member.
 * Value returned is between 0 and 1.
 *
 */
double DJAudioPlayer::getPositionRelative() {
	return (transportSource.getLengthInSeconds() == 0 ? 0 : transportSource.getCurrentPosition() / transportSource.getLengthInSeconds());
}

/**
 * Implementation of getLengthInSeconds method for DJAudioPlayer
 *
 * Returns the total length in seconds of the loaded audio source.
 */
double DJAudioPlayer::getLengthInSeconds() {
	return transportSource.getLengthInSeconds();
}

//==============================================================================

/**
 * Implementation of setGain method for DJAudioPlayer
 *
 * Checks if gain value passed in is called from a volume
 * functionality before setting the playerVolume.
 * Non volume functionality would impact the cross fader
 * volume.
 * Calls the setGain method on the AudioTransportSource data member,
 * passing in the multiplication of the player volume and cross fader volume.
 *
 */
void DJAudioPlayer::setGain(double gain, bool isVol) {
	if (isVol) {
		playerVol = gain;
	}
	else {
		crossFadeVol = gain;
	}
	if (gain < 0 || gain > 1.0) {
		DBG("DJAudioPlayer:: setGain Gain should be between 0 and 1");
	}
	else {
		transportSource.setGain(playerVol * crossFadeVol);
	}

};

/**
 * Implementation of setSpeed method for DJAudioPlayer
 *
 * If conditional acting as guard clause, ensuring resampling
 * ratio isnt set below 0 or above 100.
 * Sets ResamplingAudioSource data member's resampling ratio with
 * passed in value
 *
 */
void DJAudioPlayer::setSpeed(double ratio) {
	if (ratio < 0 || ratio > 100.0) {
		DBG("DJAudioPlayer:: setGain Gain should be between 0 and 100");
	}
	else {
		resampleSource.setResamplingRatio(ratio);
		currentSpeedRatio = ratio;
	}
};

/**
 * Implementation of setPosition method for DJAudioPlayer
 *
 * Sets the playback position by calling setPosition method
 * in the AudioTransportSource data member
 *
 */
void DJAudioPlayer::setPosition(double posInSecs) {
	transportSource.setPosition(posInSecs);
};

/**
 * Implementation of setPositionRelative method for DJAudioPlayer
 *
 * If conditional guard clause ensuring passed in value is between 0 and 1.
 * Converts value into a length in seconds and calls setPosition with converted value.
 *
 */
void DJAudioPlayer::setPositionRelative(double pos) {
	if (pos < 0 || pos > 1) {
		DBG("DJAudioPlayer:: setPositionRelative pos should be between 0 and 1");
	}
	else {
		double posInSecs = transportSource.getLengthInSeconds() * pos;
		setPosition(posInSecs);
	}
}

/**
 * Implementation of setFilter method for DJAudioPlayer
 *
 * Sets the high pass IIRCoefficients or low pass IIRCoefficients
 * on the IIRFilterAudioSource data members depending on the freq
 * value passed in
 *
 */
void DJAudioPlayer::setFilter(double freq) {
	if (freq > 0 && freq < 20000) {
		audioLPFilter.makeInactive();
		audioHPFilter.setCoefficients(juce::IIRCoefficients::makeHighPass(thisSampleRate, freq));
	}
	else if (freq < 0 && freq > -20000) {
		audioHPFilter.makeInactive();
		audioLPFilter.setCoefficients(juce::IIRCoefficients::makeLowPass(thisSampleRate, 20000 + freq));
	}
	else if (freq == 0) {
		audioHPFilter.makeInactive();
		audioLPFilter.makeInactive();
	}
}

/**
 * Implementation of setLBFilter method for DJAudioPlayer
 *
 * Sets the low shelf IIRCoefficients on the IIRFilterAudioSource
 * data member depending on the gain value passed in
 *
 */
void DJAudioPlayer::setLBFilter(double gain) {
	audioLBFilter.setCoefficients(juce::IIRCoefficients::makeLowShelf(thisSampleRate, 500, 1.0 / juce::MathConstants<double>::sqrt2, gain));
};

/**
 * Implementation of setMBFilter method for DJAudioPlayer
 *
 * Sets the peak filter IIRCoefficients on the IIRFilterAudioSource
 * data member depending on the gain value passed in
 *
 */
void DJAudioPlayer::setMBFilter(double gain) {
	audioMBFilter.setCoefficients(juce::IIRCoefficients::makePeakFilter(thisSampleRate, 3250, 1.0 / juce::MathConstants<double>::sqrt2, gain));
};

/**
 * Implementation of setHBFilter method for DJAudioPlayer
 *
 * Sets the high shelf IIRCoefficients on the IIRFilterAudioSource
 * data member depending on the gain value passed in
 *
 */
void DJAudioPlayer::setHBFilter(double gain) {
	audioHBFilter.setCoefficients(juce::IIRCoefficients::makeHighShelf(thisSampleRate, 5000, 1.0 / juce::MathConstants<double>::sqrt2, gain));
};

//==============================================================================

/**
 * Implementation of getDetectedBpm method for DJAudioPlayer
 *
 * Returns the BPM from the beat grid (may be manual override or detected).
 */
double DJAudioPlayer::getDetectedBpm() const {
	return detectedBpm;
}

/**
 * Implementation of getCurrentBpm method for DJAudioPlayer
 *
 * Returns the effective BPM adjusted by the current speed ratio.
 */
double DJAudioPlayer::getCurrentBpm() const {
	return beatGrid.bpm * currentSpeedRatio;
}

/**
 * Implementation of getSpeedRatio method for DJAudioPlayer
 *
 * Returns the current resampling speed ratio.
 */
double DJAudioPlayer::getSpeedRatio() const {
	return currentSpeedRatio;
}

/**
 * Implementation of getBeatGrid method for DJAudioPlayer
 *
 * Returns the current beat grid.
 */
const BeatGrid& DJAudioPlayer::getBeatGrid() const {
	return beatGrid;
}

/**
 * Implementation of setBeatGrid method for DJAudioPlayer
 *
 * Updates the beat grid for the loaded track.
 */
void DJAudioPlayer::setBeatGrid(const BeatGrid& grid) {
	beatGrid = grid;
}

//==============================================================================

/**
 * Implementation of readBpmFromMetadata method for DJAudioPlayer
 *
 * Uses TagLib to read BPM from the audio file's metadata tags.
 * Supports ID3v2 TBPM, Vorbis BPM, and other tag formats via
 * TagLib's unified property map.
 */
double DJAudioPlayer::readBpmFromMetadata(const juce::String& filePath) {
	TagLib::FileRef fileRef(filePath.toRawUTF8());
	if (fileRef.isNull() || fileRef.tag() == nullptr)
		return 0.0;

	TagLib::PropertyMap props = fileRef.tag()->properties();
	if (props.contains("BPM")) {
		TagLib::StringList bpmList = props["BPM"];
		if (!bpmList.isEmpty()) {
			double bpm = bpmList.front().toInt();
			if (bpm > 0.0)
				return bpm;
		}
	}

	return 0.0;
}

/**
 * Implementation of detectBpmFromAudio method for DJAudioPlayer
 *
 * Performs onset-based BPM detection by analysing audio energy levels.
 * Algorithm:
 * 1. Read a segment of audio (~30s, starting 10s in to skip intros)
 * 2. Compute energy in short frames (~10ms)
 * 3. Compute spectral flux (energy difference between frames)
 * 4. Peak-pick onset positions above a threshold
 * 5. Calculate inter-onset intervals (IOIs)
 * 6. Build histogram of IOIs, find the dominant period
 * 7. Convert period to BPM with octave correction (60-180 range)
 */
double DJAudioPlayer::detectBpmFromAudio(juce::AudioFormatReader* reader) {
	if (reader == nullptr || reader->lengthInSamples == 0)
		return 0.0;

	double sampleRate = reader->sampleRate;
	int numChannels = static_cast<int>(reader->numChannels);

	// Analysis window: start 10s in (or from start if track is short), read up to 30s
	juce::int64 startSample = static_cast<juce::int64>(std::min(10.0, reader->lengthInSamples / sampleRate * 0.1) * sampleRate);
	juce::int64 samplesToRead = static_cast<juce::int64>(std::min(30.0 * sampleRate, static_cast<double>(reader->lengthInSamples - startSample)));

	if (samplesToRead < static_cast<juce::int64>(sampleRate * 2))
		return 0.0; // Need at least 2 seconds

	// Read audio into buffer
	juce::AudioBuffer<float> buffer(numChannels, static_cast<int>(samplesToRead));
	reader->read(&buffer, 0, static_cast<int>(samplesToRead), startSample, true, true);

	// Mix to mono
	std::vector<float> mono(static_cast<size_t>(samplesToRead), 0.0f);
	for (int ch = 0; ch < numChannels; ++ch) {
		const float* channelData = buffer.getReadPointer(ch);
		for (size_t i = 0; i < mono.size(); ++i)
			mono[i] += channelData[i];
	}
	if (numChannels > 1) {
		float scale = 1.0f / static_cast<float>(numChannels);
		for (auto& s : mono)
			s *= scale;
	}

	// Compute energy per frame (~10ms frames)
	int frameSize = static_cast<int>(sampleRate * 0.01);
	int numFrames = static_cast<int>(mono.size()) / frameSize;
	if (numFrames < 10)
		return 0.0;

	std::vector<float> energy(static_cast<size_t>(numFrames), 0.0f);
	for (int f = 0; f < numFrames; ++f) {
		float sum = 0.0f;
		int offset = f * frameSize;
		for (int s = 0; s < frameSize; ++s)
			sum += mono[static_cast<size_t>(offset + s)] * mono[static_cast<size_t>(offset + s)];
		energy[static_cast<size_t>(f)] = sum / static_cast<float>(frameSize);
	}

	// Compute spectral flux (positive differences)
	std::vector<float> flux(static_cast<size_t>(numFrames), 0.0f);
	for (int f = 1; f < numFrames; ++f) {
		float diff = energy[static_cast<size_t>(f)] - energy[static_cast<size_t>(f - 1)];
		flux[static_cast<size_t>(f)] = std::max(0.0f, diff);
	}

	// Adaptive threshold: running mean over ~0.5s window
	int windowSize = static_cast<int>(0.5 / 0.01); // 50 frames
	std::vector<float> threshold(static_cast<size_t>(numFrames), 0.0f);
	for (int f = 0; f < numFrames; ++f) {
		int start = std::max(0, f - windowSize / 2);
		int end = std::min(numFrames, f + windowSize / 2);
		float sum = 0.0f;
		for (int i = start; i < end; ++i)
			sum += flux[static_cast<size_t>(i)];
		threshold[static_cast<size_t>(f)] = 1.5f * sum / static_cast<float>(end - start);
	}

	// Peak-pick onsets
	std::vector<int> onsets;
	for (int f = 1; f < numFrames - 1; ++f) {
		if (flux[static_cast<size_t>(f)] > threshold[static_cast<size_t>(f)] &&
			flux[static_cast<size_t>(f)] > flux[static_cast<size_t>(f - 1)] &&
			flux[static_cast<size_t>(f)] > flux[static_cast<size_t>(f + 1)]) {
			onsets.push_back(f);
		}
	}

	if (onsets.size() < 4)
		return 0.0;

	// Calculate inter-onset intervals in seconds
	std::vector<double> iois;
	for (size_t i = 1; i < onsets.size(); ++i) {
		double ioi = (onsets[i] - onsets[i - 1]) * 0.01; // frame duration is 10ms
		if (ioi > 0.2 && ioi < 2.0) // 30-300 BPM range
			iois.push_back(ioi);
	}

	if (iois.size() < 3)
		return 0.0;

	// Build histogram: bin IOIs into 1ms resolution (200-2000ms range)
	const int numBins = 1800;
	const double binOffset = 0.2; // start at 200ms
	const double binWidth = 0.001; // 1ms per bin
	std::vector<int> histogram(static_cast<size_t>(numBins), 0);

	for (double ioi : iois) {
		int bin = static_cast<int>((ioi - binOffset) / binWidth);
		if (bin >= 0 && bin < numBins)
			histogram[static_cast<size_t>(bin)]++;
	}

	// Find peak in histogram
	int maxBin = 0;
	int maxCount = 0;
	for (int i = 0; i < numBins; ++i) {
		if (histogram[static_cast<size_t>(i)] > maxCount) {
			maxCount = histogram[static_cast<size_t>(i)];
			maxBin = i;
		}
	}

	if (maxCount < 2)
		return 0.0;

	// Convert dominant IOI to BPM
	double dominantIOI = binOffset + maxBin * binWidth;
	double bpm = 60.0 / dominantIOI;

	// Octave correction: bring into 60-180 BPM range
	while (bpm > 180.0) bpm /= 2.0;
	while (bpm < 60.0) bpm *= 2.0;

	// Round to 1 decimal
	bpm = std::round(bpm * 10.0) / 10.0;

	return bpm;
}

//==============================================================================

