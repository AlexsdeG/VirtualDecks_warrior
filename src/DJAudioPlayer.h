
#pragma once
#include <JuceHeader.h>
#include "BeatGrid.h"

/**
 * Definition of a DJAudioplayer
 *
 * An AudioSource class that contains general player functionality.
 * Acts as an AudioSource interface that contains load, gain, playback
 * and filter functionality
 *
 */
class DJAudioPlayer : public juce::AudioSource {
public:

	//==============================================================================

	/**
		* Class Constructor for DJAudioPlayer, initializes member variables.
		*
		* @param juce::AudioFormatManager reference
	*/
	DJAudioPlayer(juce::AudioFormatManager& formatManager);

	/**
		* Class destructor for DJAudioPlayer
	*/
	~DJAudioPlayer();

	//==============================================================================

	/**
		* Prepares audio source members
		*
		* @param Expected samples in a block
		* @param Number of samples per second
	*/
	void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;

	/**
		* Called repeatedly to fetch subsequent blocks of audio data.
		*
		* @param juce::AudioSourceChannelInfo&: Buffer to be filled by audio source
	*/
	void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

	/**
		* Release resources on audio sources.
	*/
	void releaseResources() override;

	//==============================================================================

	/**
		* Start playing the file
	*/
	void start();

	/**
	   * Stop playing the file
   */
	void stop();

	/**
	   * Returns true if the DJAudioPlayer is playing on the audio source, and false otherwise
   */
	bool isPlaying();

	/**
	   * Returns true if player is loaded with an audio file
   */
	bool isLoaded();

	/**
	   * Returns the juce::URL of the loaded audio file.
   */
	juce::URL returnURL();

	/**
		* Loads URL into the transport source
		*
		* @param juce::URL of audio file to be loaded
	*/
	void loadURL(juce::URL audioURL);

	//==============================================================================

	/**
	   * Returns the rms level of the audio source via the level variable member
   */
	float getRMSLevel();

	/**
	   * Get the relative position of the playhead
   */
	double getPositionRelative();

	/**
	   * Get the total length of the loaded track in seconds
	 */
	double getLengthInSeconds();

	//==============================================================================

	/**
		* Set gain of the file
		*
		*  @param Gain of audio source, between 0 to 1
		*  @param True if called from volume functionality, false otherwise.
	*/
	void setGain(double gain, bool isVol = true);

	/**
		* Set speed of file playing by setting resampling audio source ratio
		*
		* @param Ratio of the resampling source
	*/
	void setSpeed(double ratio);

	/**
		* Set position of the file playback in seconds
		*
		* @param Position of the audio source playback in seconds.
	*/
	void setPosition(double posInSecs);

	/**
		* Set relative position of the file playback, calls setPosition
		*
		* @param Relative position of the audio source playback between 0 and 1.
	*/
	void setPositionRelative(double pos);

	/**
	   * Sets the IIR coefficients of audioLPFilter and audioHPFilter audio sources
	   *
	   * @param Frequency to perform low or high pass from -20000 to 20000.
   */
	void setFilter(double freq);

	/**
	   * Sets the IIR coefficients of audioLBFilter audio source
	   *
	   * @param Gain factor of the audio source in the low band.
   */
	void setLBFilter(double gain);

	/**
	   * Sets the IIR coefficients of audioMBFilter audio source
	   *
	   * @param Gain factor of the audio source in the mid band.
   */
	void setMBFilter(double gain);

	/**
	   * Sets the IIR coefficients of audioHBFilter audio source
	   *
	   * @param Gain factor of the audio source in the high band.
   */
	void setHBFilter(double gain);

	//==============================================================================

	/**
	 * Get the detected or manually set BPM of the loaded track.
	 * Returns 0.0 if no BPM has been determined.
	 */
	double getDetectedBpm() const;

	/**
	 * Get the effective BPM (detected BPM adjusted by current speed ratio).
	 */
	double getCurrentBpm() const;

	/**
	 * Get the current speed/resampling ratio.
	 */
	double getSpeedRatio() const;

	/**
	 * Jump forward or backward by a given number of beats.
	 * Uses the current beat grid BPM to calculate the distance.
	 *
	 * @param beats Number of beats to jump (negative = backward)
	 */
	void beatJump(int beats);

	//==============================================================================

	/**
	 * Set the loop-in point at the current playback position.
	 */
	void setLoopIn();

	/**
	 * Set the loop-out point at the current playback position and activate looping.
	 */
	void setLoopOut();

	/**
	 * Toggle loop on/off. If loop points are set, re-enable or disable looping.
	 */
	void toggleReloop();

	/**
	 * Halve the loop length by moving the loop-out point closer to loop-in.
	 */
	void halveLoop();

	/**
	 * Double the loop length by moving the loop-out point further from loop-in.
	 */
	void doubleLoop();

	/**
	 * Clear all loop points and deactivate looping.
	 */
	void clearLoop();

	/**
	 * @return True if a loop is currently active.
	 */
	bool isLooping() const;

	/**
	 * Get the loop-in position as a relative value (0 to 1).
	 * Returns -1.0 if not set.
	 */
	double getLoopInRelative() const;

	/**
	 * Get the loop-out position as a relative value (0 to 1).
	 * Returns -1.0 if not set.
	 */
	double getLoopOutRelative() const;

	//==============================================================================

	/**
	 * Get the current beat grid for the loaded track.
	 */
	const BeatGrid& getBeatGrid() const;

	/**
	 * Set the beat grid for the loaded track.
	 *
	 * @param grid The BeatGrid to apply
	 */
	void setBeatGrid(const BeatGrid& grid);

	//==============================================================================

private:

	/// Reference assigned to the AudioFormatManager passed into the constructor
	juce::AudioFormatManager& formatManager;

	/// Reader source for the audio url
	std::unique_ptr<juce::AudioFormatReaderSource> readerSource;

	/// AudioTransportSource to manage basic gain and playback controls.
	juce::AudioTransportSource transportSource;

	/// ResamplingAudioSource to manage resampling ratio controls
	juce::ResamplingAudioSource resampleSource{ &transportSource, false, 2 };

	/// IIRFilterAudioSource to manage low band filter controls
	juce::IIRFilterAudioSource audioLBFilter{ &resampleSource , false };

	/// IIRFilterAudioSource to manage mid band filter controls
	juce::IIRFilterAudioSource audioMBFilter{ &audioLBFilter , false };

	/// IIRFilterAudioSource to manage high band filter controls
	juce::IIRFilterAudioSource audioHBFilter{ &audioMBFilter , false };

	/// IIRFilterAudioSource to manage high pass filter controls
	juce::IIRFilterAudioSource audioHPFilter{ &audioHBFilter , false };

	/// IIRFilterAudioSource to manage low pass filter controls
	juce::IIRFilterAudioSource audioLPFilter{ &audioHPFilter , false };

	/// juce::String to store the file name of the loaded url
	juce::String loadedFileName;

	/// double to store the sample rate
	double thisSampleRate;

	/// boolean to determine if the player is loaded
	bool loaded = false;

	/// double to store the DeckGUI player volume
	double playerVol = 1;

	/// double to store the cross fader volume
	double crossFadeVol = 1;

	/// juce::URL to store the current loaded audio file's URL
	juce::URL currentAudioURL;

	/// float to store the audio source RMS level
	float level;

	/// Detected BPM from metadata or onset analysis
	double detectedBpm = 0.0;

	/// Current speed/resampling ratio
	double currentSpeedRatio = 1.0;

	/// Beat grid for the loaded track
	BeatGrid beatGrid;

	/// Loop-in position in seconds (-1.0 = not set)
	double loopInSecs = -1.0;

	/// Loop-out position in seconds (-1.0 = not set)
	double loopOutSecs = -1.0;

	/// Whether the loop is currently active
	bool loopActive = false;
};
