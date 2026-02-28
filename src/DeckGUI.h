
#pragma once

#include <JuceHeader.h>
#include "DJAudioPlayer.h"
#include "WaveformDisplay.h"
#include "ZoomedWaveform.h"
#include "JogWheel.h"

#include "CustomLookAndFeel.h"
#include "Library.h"
#include "TrackDataCache.h"
//==============================================================================

/**
 * Definition of a DeckGUI component
 *
 * A graphics component that performs as a DJ Deck and contains multiple components
 * to control audio functionality on the DJAudioPlayer instance. Being a FileDragAndDropTarget,
 * the component has track loading functionality via file drag and drop. Track loading
 * functionality is also included in an add button, communicating with a Library instance to
 * load selected tracks.
 *
 */
class DeckGUI : public juce::Component,
	public juce::Button::Listener,
	public juce::Slider::Listener,
	public juce::FileDragAndDropTarget,
	public juce::Timer
{
public:
	//==============================================================================

	/**
		* Class Constructor for DeckGUI, initializes member variables and configures component details.
		*
		* @param juce::AudioFormatManager reference that manages audio formats
		* @param AudioThumbnailCache reference that manages a cache of juce::AudioThumbnail objects
		* @param ZoomedWaveform pointer
		* @param Library reference to load track selections
		* @param juce::Colour that defines the theme colour of the component
	*/
	DeckGUI(DJAudioPlayer* player, juce::AudioFormatManager& formatManagerToUse, juce::AudioThumbnailCache& cacheToUse, ZoomedWaveform* _zoomedDisplay, Library& _library, juce::Colour _colour);

	/**
		* Class Destructor for DeckGUI, clears dynamically allocated variables.
	*/
	~DeckGUI() override;

	//==============================================================================

private:

	//==============================================================================

	/**
		* Paints the DeckGUI Component.
		*
		* @param juce::Graphics object for the component to draw itself on
	*/
	void paint(juce::Graphics&) override;

	/**
		* Set bounds of member components
	*/
	void resized() override;

	/**
		* Called on mouse down to handle right-click context menus on cue buttons.
		*
		* @param juce::MouseEvent object
	*/
	void mouseDown(const juce::MouseEvent& event) override;

	//==============================================================================

	/**
		* Called when button in DeckGUI listener is clicked.
		*
		* @param juce::Button object that has added this component as its listener
	*/
	void buttonClicked(juce::Button* button) override;

	//==============================================================================

	/**
		* Called when slider knob in DeckGUI listener is dragged.
		*
		* @param juce::Slider object that has added this component as its listener
	*/
	void sliderValueChanged(juce::Slider* slider)override;

	//==============================================================================

	/**
		* Called when file is dragged into DeckGUI.
		*
		* @param juce::StringArray object containing the files dragged over the component
		* @return true
	*/
	bool isInterestedInFileDrag(const juce::StringArray& files) override;

	/**
		* Called when file is dropped into DeckGUI.
		*
		* @param juce::StringArray object containing the files dragged over the component
		* @param x position of file dropped
		* @param y position of file dropped
	*/
	void filesDropped(const juce::StringArray& files, int x, int y) override;

	//==============================================================================

	/**
		* Called at specific intervals defined in startTimer() params.
	*/
	void timerCallback() override;

	//==============================================================================

	/**
		* Loads the track object into the DeckGUI.
		*
		* @param track object to be loaded into the component
	*/
	void loadDeck(track track);

	//==============================================================================

	/// Pointer to Library component.
	Library* library;

	/// Pointer to DJAudioPlayer instance.
	DJAudioPlayer* player;

	/// Instance of CustomLookAndFeel class.
	CustomLookAndFeel customLookAndFeel;

	/// Unique pointer to juce::Drawable storing the stop button image.
	std::unique_ptr<juce::Drawable> stopButtonImage;

	/// Unique pointer to juce::Drawable storing the hovered stop button image.
	std::unique_ptr<juce::Drawable> stopButtonHoverImage;

	/// Unique pointer to juce::Drawable storing the play button image.
	std::unique_ptr<juce::Drawable> playButtonImage;

	/// Unique pointer to juce::Drawable storing the hovered play button image.
	std::unique_ptr<juce::Drawable>  playButtonHoverImage;

	/// Unique pointer to juce::Drawable storing the load button image.
	std::unique_ptr<juce::Drawable> loadButtonImage;

	/// Unique pointer to juce::Drawable storing the hovered load button image.
	std::unique_ptr<juce::Drawable>  loadButtonHoverImage;

	/// juce::DrawableButton for the play button component
	juce::DrawableButton playButton{ "Play", juce::DrawableButton::ButtonStyle::ImageFitted };

	/// juce::DrawableButton for the load button component
	juce::DrawableButton loadButton{ "Load", juce::DrawableButton::ButtonStyle::ImageFitted };

	/// juce::Colour to define the theme of the DeckGUI
	juce::Colour theme;

	/// juce::Label to label the volume slider
	juce::Label volLabel{ "VOLUME", "VOLUME" };

	/// juce::Slider to adjust the gain of the audio source.
	juce::Slider volSlider{ juce::Slider::SliderStyle::LinearVertical , juce::Slider::TextEntryBoxPosition::NoTextBox };

	/// juce::Label to label the BPM slider
	juce::Label speedLabel{ "SPEED", "SPEED" };

	/// Slider subclass that ignores right-click mouse events so right-click only opens the context menu.
	struct SpeedSlider : public juce::Slider {
		SpeedSlider() : juce::Slider(juce::Slider::SliderStyle::LinearVertical, juce::Slider::TextEntryBoxPosition::NoTextBox) {}
		void mouseDown(const juce::MouseEvent& e) override { if (!e.mods.isPopupMenu()) juce::Slider::mouseDown(e); }
		void mouseDrag(const juce::MouseEvent& e) override { if (!e.mods.isPopupMenu()) juce::Slider::mouseDrag(e); }
	};

	/// SpeedSlider to adjust the resampled speed(BPM) of the audio source.
	SpeedSlider speedSlider;

	/// juce::Label to display the current BPM value
	juce::Label bpmValueLabel{ "BPM_VAL", "---" };

	/// juce::Label to display the speed % deviation
	juce::Label bpmPercentLabel{ "BPM_PCT", "" };

	/// juce::Slider to adjust the low pass/high pass filter on the audio source.
	juce::Slider filter{ juce::Slider::SliderStyle::RotaryVerticalDrag , juce::Slider::TextEntryBoxPosition::NoTextBox };

	/// juce::Label to label the filter slider
	juce::Label filterLabel{ "FILTER", "FILTER" };

	/// juce::Slider to adjust the low band filter on the audio source.
	juce::Slider lowBandFilter{ juce::Slider::SliderStyle::RotaryVerticalDrag , juce::Slider::TextEntryBoxPosition::NoTextBox };

	/// juce::Label to label the low band slider
	juce::Label lbLabel{ "LOW", "LOW" };

	/// juce::Slider to adjust the mid band filter on the audio source.
	juce::Slider midBandFilter{ juce::Slider::SliderStyle::RotaryVerticalDrag , juce::Slider::TextEntryBoxPosition::NoTextBox };

	/// juce::Label to label the mid band slider
	juce::Label mbLabel{ "MID", "MID" };

	/// juce::Slider to adjust the high band filter on the audio source.
	juce::Slider highBandFilter{ juce::Slider::SliderStyle::RotaryVerticalDrag , juce::Slider::TextEntryBoxPosition::NoTextBox };

	/// juce::Label to label the high band slider
	juce::Label hbLabel{ "HIGH", "HIGH" };

	/// Instance of WaveformDisplay class.
	WaveformDisplay waveformDisplay;

	/// Instance of JogWheel class.
	JogWheel jogWheel;

	/// Pointer to ZoomedWaveform component.
	ZoomedWaveform* zoomedDisplay;

	/// Vector of WaveformDisplay pointers consisting of all Waveform references in DeckGUI.
	std::vector<WaveformDisplay*> displays{ &waveformDisplay , zoomedDisplay, &jogWheel };

	/// Vector of juce::TextButton pointers for cue buttons
	std::vector<juce::TextButton*> cues;

	/// Map of juce::TextButton pointers to std::pair of double and floats. Maps cue buttons to a pair containing double for audio position and float for hue colour of cue button.
	std::map<juce::TextButton*, std::pair<double, float>> cueTargets;

	/// Previous audio player position used to compare to the current player position.
	double prevPlayerPos;

	/// Determines if the player can continue playing
	bool canContinue = true;

	/// Determines if the deck playing mode.
	bool modeIsPlaying = false;

	/// Determines the WaveformDisplay object being dragged in displays vector.
	int draggedIndex;

	/// Determines if cue buttons should be lit up with their hue colours in cueTargets map.
	bool flash;

	/// Simple counter that increments every call to timerCallback
	int counter;

	/// Determines the average root mean square value derived from the DJAudioPlayer
	float volRMS;

	//==============================================================================
	// Tab mode: Hot Cues vs Beat Grid

	/// Enum for the cue/grid tab mode
	enum class CueGridMode { HotCues, BeatGrid };

	/// Current tab mode
	CueGridMode cueGridMode = CueGridMode::HotCues;

	/// Tab button for hot cues
	juce::TextButton cueTabButton{ "CUES" };

	/// Tab button for beat grid controls
	juce::TextButton gridTabButton{ "GRID" };

	//==============================================================================
	// Beat Grid Controls

	/// Label for the grid BPM editor
	juce::Label gridBpmLabel{ "GRID_BPM", "BPM:" };

	/// Editable text field for BPM override
	juce::TextEditor gridBpmEditor;

	/// Button to nudge grid offset earlier
	juce::TextButton gridNudgeLeftBtn{ "<" };

	/// Button to nudge grid offset later
	juce::TextButton gridNudgeRightBtn{ ">" };

	/// Label for nudge buttons
	juce::Label gridOffsetLabel{ "GRID_OFF", "OFFSET" };

	/// Tap tempo button
	juce::TextButton tapTempoBtn{ "TAP" };

	/// Button to reset grid to detected values
	juce::TextButton gridResetBtn{ "RESET" };

	/// Timestamps of tap tempo presses
	std::vector<double> tapTimes;

	/// Identity hash of the currently loaded track (legacy)
	juce::String currentTrackIdentity;

	/// Content-based file hash of the currently loaded track
	juce::String currentFileHash;

	/// Helper to save current beat grid + detected BPM to the track data cache
	void saveTrackData(const BeatGrid& grid);

	/// Sets visibility of cue buttons
	void setCueButtonsVisible(bool visible);

	/// Sets visibility of grid controls
	void setGridControlsVisible(bool visible);

	/// Updates the grid BPM editor text from the player
	void updateGridBpmDisplay();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeckGUI);
};
