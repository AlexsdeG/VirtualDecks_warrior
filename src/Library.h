
#pragma once

#include "CustomLookAndFeel.h"
#include "PlaylistComponent.h"
#include "BpmAnalysisManager.h"
#include "FileHasher.h"
#include <JuceHeader.h>

//==============================================================================

/**
 * Definition of a Library Component
 *
 * A component to manage a library of playlist folders.
 * Functionality to select playlist folders and display
 * folder's track list. Contains folder and track add/delete
 * functionality as well as data persistance.
 *
 */
class Library : public juce::Component,
                public juce::TableListBoxModel,
                public juce::FileDragAndDropTarget,
                public juce::Button::Listener,
                public BpmAnalysisManager::Listener {
public:
  //==============================================================================

  /**
   * Class Constructor for Library, reads xml library data into class,
   * initializes member variables and configures component details.
   *
   * @param AudioFormatManager reference
   */
  Library(juce::AudioFormatManager &_formatManager);

  /**
   * Class destructor for Library, used to write library data to xml file
   */
  ~Library() override;

  //==============================================================================

  /**
   * @return if the selected cell is a valid selection
   */
  bool selectionIsValid();

  /**
   * @return selected track object
   */
  track getSelectedTrack();

  /**
   * Removes a pair element from trackFolders, or a track object from the
   * selected pair's vector
   */
  void deleteItem();

  //==============================================================================

  /**
   * Adds audio files to the currently selected folder via file chooser dialog
   */
  void addFilesToFolder();

  /**
   * Adds a new empty folder to the library
   */
  void addFolder();

  /**
   * Removes the currently selected folder from the library
   */
  void removeFolder();

  /**
   * Renames the currently selected folder
   */
  void renameFolder();

  /**
   * Removes the currently selected track from the current folder
   */
  void removeSelectedTrack();

  /**
   * Opens a directory chooser to import a folder of audio files into the library
   */
  void importFolderFromDisk();

  //==============================================================================

private:
  //==============================================================================

  /**
   * Paints the Library Component.
   *
   * @param juce::Graphics object
   */
  void paint(juce::Graphics &) override;

  /**
   * Set bounds of member components
   */
  void resized() override;

  //==============================================================================

  /**
   * @return Number of rows in the library selection
   */
  int getNumRows() override;

  /**
   * Paints the row's background of the directoryComponent member.
   *
   * @param juce::Graphics object
   * @param Row number
   * @param Row width
   * @param Row height
   * @param If the row is selected
   */
  void paintRowBackground(juce::Graphics &g, int rowNumber, int width,
                          int height, bool rowIsSelected) override;

  /**
   * Paints each cell of the directoryComponent member.
   *
   * @param juce::Graphics object
   * @param Row number
   * @param Column number
   * @param Cell width
   * @param Cell height
   * @param If the row is selected
   */
  void paintCell(juce::Graphics &g, int rowNumber, int columnId, int width,
                 int height, bool rowIsSelected) override;

  /**
   * Called when cell is clicked.
   *
   * @param Row number
   * @param Column number
   * @param juce::MouseEvent triggered by user
   */
  void cellClicked(int rowNumber, int columnId,
                   const juce::MouseEvent &e) override;

  //==============================================================================

  /**
   * Called when file is dragged over component
   *
   * @param const juce::StringArray of files being dragged over the component
   */
  bool isInterestedInFileDrag(const juce::StringArray &files) override;

  /**
   * Called when file is dropped over component
   *
   * @param const juce::StringArray of files being dropped over the component
   * @param x position of files being dropped
   * @param y position of files being dropped
   */
  void filesDropped(const juce::StringArray &files, int x, int y) override;

  //==============================================================================

  /**
   * Called when a button is clicked
   *
   * @param Button that was clicked
   */
  void buttonClicked(juce::Button *button) override;

  //==============================================================================

  /// Instance of CustomLookAndFeel class.
  CustomLookAndFeel customLookAndFeel;

  /// Instance of a PlaylistComponent Class
  PlaylistComponent playlist;

  /// Reference assigned to the AudioFormatManager passed into the constructor
  juce::AudioFormatManager &formatManager;

  /// Reader source for the audio url
  std::unique_ptr<juce::AudioFormatReader> audioReader;

  /// Reflects the trackFolders' elements
  juce::TableListBox directoryComponent;

  /// Data structure to hold playlist folders containing track objects
  std::vector<std::pair<juce::String, std::vector<track>>> trackFolders;

  /// Selected index of the directoryComponent
  int selectedFolderIndex = -1;

  /// File path to read xml data from and load the trackFolders when the
  /// application starts
  juce::String filePath;

  /// Button to add a new folder
  juce::TextButton addFolderBtn{"+ Folder"};

  /// Button to remove the selected folder
  juce::TextButton removeFolderBtn{"- Folder"};

  /// Button to rename the selected folder
  juce::TextButton renameFolderBtn{"Rename"};

  /// Button to add files to the selected folder
  juce::TextButton addFilesBtn{"+ Files"};

  /// Button to remove the selected track
  juce::TextButton removeTrackBtn{"- Track"};

  /// Button to import a folder of audio files from disk
  juce::TextButton importFolderBtn{"Import Folder"};

  /// File chooser for adding audio files
  std::unique_ptr<juce::FileChooser> fileChooser;

  /// Background BPM analysis manager
  BpmAnalysisManager bpmAnalysisManager;

  /// Queue all tracks in a folder for background BPM analysis
  void queueBpmAnalysis(std::vector<track>& tracks);

  /// Callback when background BPM analysis completes
  void bpmAnalysisComplete(const juce::String& fileHash, double bpm) override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Library)
};
