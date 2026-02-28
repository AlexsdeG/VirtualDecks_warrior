
#include "Library.h"

//==============================================================================

/**
 * Implementation of a constructor for Library
 *
 * Data members are initialized and initial configurations are applied to
 * components here. File reading occurs from a fixed path defined in the header
 * file. The retrieved value tree from reading off the xml file is used to
 * populate the elements of the trackFolders data structure.
 *
 */
Library::Library(juce::AudioFormatManager &_formatManager)
    : formatManager(_formatManager), playlist(_formatManager),
      bpmAnalysisManager(_formatManager) {
  filePath = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                 .getChildFile(".otodecks")
                 .getChildFile("Resource.xml")
                 .getFullPathName();

  juce::File file(filePath);
  if (!file.getParentDirectory().exists()) {
    file.getParentDirectory().createDirectory();
  }
  if (!file.existsAsFile()) {
    DBG("FILE DONT EXIST");
    DBG((file.create().wasOk() ? "Creation Success" : "Creation Failed"));
    std::pair<juce::String, std::vector<track>> folder;
    folder.first = "Main";
    trackFolders.push_back(folder);
  } else {
    DBG("FILE EXIST");
    juce::FileInputStream in(file);
    if (in.openedOk()) {
      auto newValueTree = juce::ValueTree::readFromStream(in);
      for (auto i = 0; i < newValueTree.getNumChildren(); ++i) {
        std::pair<juce::String, std::vector<track>> folder;
        folder.first = newValueTree.getChild(i).getProperty("name");
        for (auto j = 0; j < newValueTree.getChild(i).getNumChildren(); ++j) {
          auto song = newValueTree.getChild(i).getChild(j);
          track refSong{song.getProperty("title"), song.getProperty("length"),
                        juce::URL(song.getProperty("url").toString()),
                        song.getProperty("identity")};
          // Read fileHash and bpm if present (new fields)
          if (song.hasProperty("fileHash"))
            refSong.fileHash = song.getProperty("fileHash").toString();
          if (song.hasProperty("bpm"))
            refSong.bpm = static_cast<double>(song.getProperty("bpm"));
          folder.second.push_back(refSong);
        }
        trackFolders.push_back(folder);
      }
    }
  }

  if (trackFolders.empty()) {
    std::pair<juce::String, std::vector<track>> folder;
    folder.first = "Main";
    trackFolders.push_back(folder);
  }

  selectedFolderIndex = 0;
  playlist.setTrackTitles(trackFolders[selectedFolderIndex].second);
  addAndMakeVisible(playlist);
  playlist.setLookAndFeel(&customLookAndFeel);

  directoryComponent.getHeader().addColumn("Folders", 1, 360);
  directoryComponent.setModel(this);
  addAndMakeVisible(directoryComponent);
  directoryComponent.setColour(juce::ListBox::ColourIds::backgroundColourId,
                               juce::Colour::fromRGBA(25, 25, 25, 255));
  directoryComponent.selectRow(selectedFolderIndex);

  // Setup folder management buttons
  addAndMakeVisible(addFolderBtn);
  addAndMakeVisible(removeFolderBtn);
  addAndMakeVisible(renameFolderBtn);
  addAndMakeVisible(addFilesBtn);
  addAndMakeVisible(removeTrackBtn);

  auto buttonColour = juce::Colour::fromRGBA(50, 50, 50, 255);
  addFolderBtn.setColour(juce::TextButton::buttonColourId, buttonColour);
  removeFolderBtn.setColour(juce::TextButton::buttonColourId, buttonColour);
  renameFolderBtn.setColour(juce::TextButton::buttonColourId, buttonColour);
  addFilesBtn.setColour(juce::TextButton::buttonColourId, buttonColour);
  removeTrackBtn.setColour(juce::TextButton::buttonColourId, buttonColour);

  addAndMakeVisible(importFolderBtn);
  importFolderBtn.setColour(juce::TextButton::buttonColourId, buttonColour);

  addFolderBtn.addListener(this);
  removeFolderBtn.addListener(this);
  renameFolderBtn.addListener(this);
  addFilesBtn.addListener(this);
  removeTrackBtn.addListener(this);
  importFolderBtn.addListener(this);

  bpmAnalysisManager.addListener(this);

  // Queue BPM analysis for any tracks missing BPM data
  for (auto& folder : trackFolders)
    queueBpmAnalysis(folder.second);
}

/**
 * Implementation of a destructor for Library
 *
 * The trackFolders data structure is used to populate a value tree before
 * writing it to the xml file at the same path.
 *
 */
Library::~Library() {
  bpmAnalysisManager.removeListener(this);

  juce::File file(filePath);
  file.deleteFile();

  juce::ValueTree main(juce::Identifier("main"));
  for (auto i = 0; i < trackFolders.size(); ++i) {
    juce::ValueTree folder(juce::Identifier(std::to_string(i)));
    folder.setProperty(juce::Identifier("name"), trackFolders[i].first,
                       nullptr);
    for (auto j = 0; j < trackFolders[i].second.size(); ++j) {
      juce::ValueTree song(juce::Identifier(std::to_string(j)));
      song.setProperty("title", trackFolders[i].second[j].title, nullptr);
      song.setProperty("length", trackFolders[i].second[j].lengthInSeconds,
                       nullptr);
      song.setProperty("url", trackFolders[i].second[j].url.toString(false),
                       nullptr);
      song.setProperty("identity", trackFolders[i].second[j].identity, nullptr);
      song.setProperty("fileHash", trackFolders[i].second[j].fileHash, nullptr);
      song.setProperty("bpm", trackFolders[i].second[j].bpm, nullptr);
      folder.addChild(song, j, nullptr);
    }
    main.addChild(folder, i, nullptr);
  }

  DBG("FILE SAVING");
  file.create();
  juce::FileOutputStream outstream(file);
  main.writeToStream(outstream);
}

//==============================================================================

/**
 * Implementation of selectionIsValid method for Library
 *
 * Returns if the folder and track selection is valid.
 * This is ensured by checking that the selectedFolderIndex respect the bounds
 * of the trackFolders size and that the playlist track is selected
 *
 */
bool Library::selectionIsValid() {
  DBG((playlist.trackIsSelected() ? "true" : "3false"));
  DBG((selectedFolderIndex >= 0 ? "true" : "1false"));
  DBG((selectedFolderIndex < trackFolders.size() ? "true" : "2false"));
  return selectedFolderIndex >= 0 &&
         selectedFolderIndex < trackFolders.size() &&
         playlist.trackIsSelected();
};

/**
 * Implementation of getSelectedTrack method for Library
 *
 * Returns the selected track from the playlist instance
 *
 */
track Library::getSelectedTrack() { return playlist.getSelectedTrack(); };

/**
 * Implementation of deleteItem method for Library
 *
 * Check if only folder is selected or both folder and track is selected.
 * In the former case, the entire folder element is erased off the trackFolders
 * data structure. In the latter case, the track element is erased off the
 * selected folder in trackFolders. Identity hash strings of tracks are compared
 * to confirm the track to be deleted.
 *
 */
void Library::deleteItem() {
  if (selectedFolderIndex >= 0 && selectedFolderIndex < trackFolders.size()) {
    if (playlist.trackIsSelected()) {
      auto &selectedPlaylist = trackFolders[selectedFolderIndex].second;
      auto selectedTrack = -1;
      for (auto i = 0; i < selectedPlaylist.size(); ++i) {
        if (selectedPlaylist[i].identity == getSelectedTrack().identity) {
          DBG("True delete match");
          selectedTrack = i;
          break;
        }
      }
      if (selectedTrack > -1) {
        selectedPlaylist.erase(selectedPlaylist.begin() + selectedTrack);
      }
      playlist.setTrackTitles(trackFolders[selectedFolderIndex].second);
    } else {
      if (trackFolders.size() > 1) {
        trackFolders.erase(trackFolders.begin() + selectedFolderIndex);
        selectedFolderIndex = 0;
        playlist.setTrackTitles(trackFolders[selectedFolderIndex].second);
        directoryComponent.selectRow(selectedFolderIndex);
      }
      directoryComponent.updateContent();
    }
  }
};

//==============================================================================

/**
 * Implementation of paint method for Library
 */
void Library::paint(juce::Graphics &g) {}

/**
 * Implementation of resized method for Library
 *
 * Call setBounds method on the juce::Component data members playlist and
 * directoryComponent.
 */
void Library::resized() {
  auto buttonBarHeight = 28;
  auto dirWidth = 1.5 * getWidth() / 8;
  auto playlistX = dirWidth;
  auto playlistWidth = getWidth() - dirWidth;
  auto contentHeight = getHeight() - buttonBarHeight;

  directoryComponent.setBounds(0, 0, dirWidth, contentHeight);

  if (selectedFolderIndex != -1) {
    playlist.setBounds(playlistX, 0, playlistWidth, contentHeight);
  }

  // Folder buttons below the directory list
  auto folderBtnWidth = dirWidth / 4;
  addFolderBtn.setBounds(0, contentHeight, folderBtnWidth, buttonBarHeight);
  removeFolderBtn.setBounds(folderBtnWidth, contentHeight, folderBtnWidth, buttonBarHeight);
  renameFolderBtn.setBounds(2 * folderBtnWidth, contentHeight, folderBtnWidth, buttonBarHeight);
  importFolderBtn.setBounds(3 * folderBtnWidth, contentHeight, dirWidth - 3 * folderBtnWidth, buttonBarHeight);

  // Track buttons below the playlist
  auto trackBtnWidth = playlistWidth / 2;
  addFilesBtn.setBounds(playlistX, contentHeight, trackBtnWidth, buttonBarHeight);
  removeTrackBtn.setBounds(playlistX + trackBtnWidth, contentHeight, playlistWidth - trackBtnWidth, buttonBarHeight);
}

//==============================================================================

/**
 * Implementation of getNumRows method for Library
 *
 * Returns the size of the data structure trackFolders
 */
int Library::getNumRows() { return trackFolders.size(); };

/**
 * Implementation of paintRowBackground method for Library
 *
 * Change the colour of the row if they are selected.
 */
void Library::paintRowBackground(juce::Graphics &g, int rowNumber, int width,
                                 int height, bool rowIsSelected) {
  if (rowNumber < trackFolders.size()) {
    if (rowIsSelected) {
      g.fillAll(juce::Colour::fromRGBA(0, 125, 225, 255));
    } else {
      g.fillAll(juce::Colour::fromRGBA(100, 100, 100, 255));
    }
  }
};

/**
 * Implementation of paintCell method for Library
 *
 * Draw the text of the folder names on the rows
 *
 */
void Library::paintCell(juce::Graphics &g, int rowNumber, int columnId,
                        int width, int height, bool rowIsSelected) {
  g.setColour(juce::Colours::white);
  if (rowNumber < trackFolders.size()) {
    g.drawText(trackFolders[rowNumber].first, 2, 0, width - 4, height,
               juce::Justification::centredLeft, true);
  }
};

/**
 * Implementation of cellClicked method for Library
 *
 * Sets the selected folder index
 * Sets the playlist folder with the selected folder
 *
 */
void Library::cellClicked(int rowNumber, int columnId,
                          const juce::MouseEvent &e) {
  DBG(" PlaylistComponent::cellClicked " << rowNumber);
  selectedFolderIndex = rowNumber;
  playlist.setTrackTitles(trackFolders[selectedFolderIndex].second);

  if (e.mods.isPopupMenu()) {
    juce::PopupMenu menu;
    menu.addItem(1, "Add Folder");
    menu.addItem(2, "Rename Folder");
    menu.addItem(3, "Remove Folder", trackFolders.size() > 1);
    menu.addSeparator();
    menu.addItem(4, "Add Files to Folder");

    menu.showMenuAsync(juce::PopupMenu::Options(),
      [this](int result) {
        if (result == 1)
          addFolder();
        else if (result == 2)
          renameFolder();
        else if (result == 3)
          removeFolder();
        else if (result == 4)
          addFilesToFolder();
      });
  }
};

//==============================================================================

/**
 * Implementation of isInterestedInFileDrag method for Library
 *
 * Returns true
 *
 */
bool Library::isInterestedInFileDrag(const juce::StringArray &files) {
  return true;
};

/**
 * Implementation of filesDropped method for Library
 *
 * Checks if the dropped items are in the library or playlist components.
 * The addition of tracks/folders are performed on the trackFolders data
 * structure, storing all the folder/track data in the library level. Selection
 * of the folder and what the playlist displays is communicated to the playlist
 * instance using the trackFolders data. Adds tracks into the currently selected
 * folder if items are dropped on the playlist component. Adds folder of tracks
 * into the library if items are dropped on the library component.
 *
 */
void Library::filesDropped(const juce::StringArray &files, int x, int y) {
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
  auto timeString = oss.str();

  if (x > 1.5 * getWidth() / 8) {
    if (selectedFolderIndex != -1) {
      for (auto i = 0; i < files.size(); ++i) {
        DBG("YES ADDED FILE " << files.size());
        auto audioFile = juce::File{files[i]};
        audioReader.reset(formatManager.createReaderFor(audioFile));
        if (audioReader != nullptr) {
          DBG("YES ADDED FILE");
          std::hash<std::string> hasher;
          track thisTrack = {audioFile.getFileNameWithoutExtension(),
                             audioReader->lengthInSamples /
                                 audioReader->sampleRate,
                             juce::URL{audioFile}};
          size_t hash = hasher(
              thisTrack.title.toStdString() +
              std::to_string(thisTrack.lengthInSeconds) +
              thisTrack.url.toString(false).toStdString() +
              std::to_string(trackFolders[selectedFolderIndex].second.size()) +
              timeString);
          char hashString[256] = "";
          snprintf(hashString, sizeof hashString, "%zu", hash);
          DBG(hashString);
          thisTrack.identity = juce::String(hashString);
          thisTrack.fileHash = FileHasher::computeHash(audioFile);
          if (thisTrack.fileHash.isNotEmpty() && TrackDataCache::exists(thisTrack.fileHash))
            thisTrack.bpm = TrackDataCache::load(thisTrack.fileHash).detectedBpm;
          trackFolders[selectedFolderIndex].second.push_back(thisTrack);
        }
      }
    }
  } else {
    for (auto i = 0; i < files.size(); ++i) {
      auto audioFile = juce::File{files[i]};
      if (audioFile.isDirectory()) {
        std::pair<juce::String, std::vector<track>> thisFolder;
        thisFolder.first = audioFile.getFileNameWithoutExtension();
        auto folder = audioFile.findChildFiles(
            juce::File::TypesOfFileToFind::findFiles, false);
        for (auto &file : folder) {
          audioReader.reset(formatManager.createReaderFor(file));
          if (audioReader != nullptr) {
            std::hash<std::string> hasher;
            track thisTrack = {file.getFileNameWithoutExtension(),
                               audioReader->lengthInSamples /
                                   audioReader->sampleRate,
                               juce::URL{file}};
            size_t hash =
                hasher(thisTrack.title.toStdString() +
                       std::to_string(thisTrack.lengthInSeconds) +
                       thisTrack.url.toString(false).toStdString() +
                       std::to_string(
                           trackFolders[selectedFolderIndex].second.size()) +
                       timeString);
            char hashString[256] = "";
            snprintf(hashString, sizeof hashString, "%zu", hash);
            DBG(hashString);
            thisTrack.identity = juce::String(hashString);
            thisTrack.fileHash = FileHasher::computeHash(file);
            if (thisTrack.fileHash.isNotEmpty() && TrackDataCache::exists(thisTrack.fileHash))
              thisTrack.bpm = TrackDataCache::load(thisTrack.fileHash).detectedBpm;
            thisFolder.second.push_back(thisTrack);
          }
        }
        trackFolders.push_back(thisFolder);
        selectedFolderIndex = trackFolders.size() - 1;
      }
    }
  }
  if (selectedFolderIndex != -1) {
    queueBpmAnalysis(trackFolders[selectedFolderIndex].second);
    playlist.setTrackTitles(trackFolders[selectedFolderIndex].second);
  }
  directoryComponent.updateContent();
  directoryComponent.selectRow(selectedFolderIndex, true);
};

//==============================================================================

/**
 * Implementation of buttonClicked method for Library
 *
 * Routes button clicks to the appropriate action method.
 */
void Library::buttonClicked(juce::Button *button) {
  if (button == &addFolderBtn)
    addFolder();
  else if (button == &removeFolderBtn)
    removeFolder();
  else if (button == &renameFolderBtn)
    renameFolder();
  else if (button == &addFilesBtn)
    addFilesToFolder();
  else if (button == &removeTrackBtn)
    removeSelectedTrack();
  else if (button == &importFolderBtn)
    importFolderFromDisk();
}

//==============================================================================

/**
 * Implementation of addFolder method for Library
 *
 * Prompts user for a folder name and creates a new empty folder.
 */
void Library::addFolder() {
  auto *editor = new juce::AlertWindow("New Folder", "Enter folder name:",
                                       juce::MessageBoxIconType::NoIcon);
  editor->addTextEditor("name", "New Folder", "Folder Name:");
  editor->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
  editor->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

  editor->enterModalState(true, juce::ModalCallbackFunction::create(
    [this, editor](int result) {
      if (result == 1) {
        auto name = editor->getTextEditorContents("name").trim();
        if (name.isNotEmpty()) {
          std::pair<juce::String, std::vector<track>> folder;
          folder.first = name;
          trackFolders.push_back(folder);
          selectedFolderIndex = static_cast<int>(trackFolders.size()) - 1;
          directoryComponent.updateContent();
          directoryComponent.selectRow(selectedFolderIndex);
          playlist.setTrackTitles(trackFolders[selectedFolderIndex].second);
        }
      }
      delete editor;
    }), true);
}

/**
 * Implementation of removeFolder method for Library
 *
 * Removes the currently selected folder (if more than one exists).
 */
void Library::removeFolder() {
  if (selectedFolderIndex >= 0 &&
      selectedFolderIndex < static_cast<int>(trackFolders.size()) &&
      trackFolders.size() > 1) {
    trackFolders.erase(trackFolders.begin() + selectedFolderIndex);
    selectedFolderIndex = 0;
    directoryComponent.updateContent();
    directoryComponent.selectRow(selectedFolderIndex);
    playlist.setTrackTitles(trackFolders[selectedFolderIndex].second);
  }
}

/**
 * Implementation of renameFolder method for Library
 *
 * Prompts user for a new name for the selected folder.
 */
void Library::renameFolder() {
  if (selectedFolderIndex < 0 ||
      selectedFolderIndex >= static_cast<int>(trackFolders.size()))
    return;

  auto currentName = trackFolders[selectedFolderIndex].first;
  auto *editor = new juce::AlertWindow("Rename Folder", "Enter new name:",
                                       juce::MessageBoxIconType::NoIcon);
  editor->addTextEditor("name", currentName, "Folder Name:");
  editor->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
  editor->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

  editor->enterModalState(true, juce::ModalCallbackFunction::create(
    [this, editor](int result) {
      if (result == 1) {
        auto name = editor->getTextEditorContents("name").trim();
        if (name.isNotEmpty() && selectedFolderIndex >= 0 &&
            selectedFolderIndex < static_cast<int>(trackFolders.size())) {
          trackFolders[selectedFolderIndex].first = name;
          directoryComponent.updateContent();
          directoryComponent.repaint();
        }
      }
      delete editor;
    }), true);
}

/**
 * Implementation of addFilesToFolder method for Library
 *
 * Opens a file chooser dialog allowing multi-selection of audio files,
 * then adds them to the currently selected folder.
 */
void Library::addFilesToFolder() {
  if (selectedFolderIndex < 0 ||
      selectedFolderIndex >= static_cast<int>(trackFolders.size()))
    return;

  fileChooser = std::make_unique<juce::FileChooser>(
      "Select audio files to add",
      juce::File::getSpecialLocation(juce::File::userHomeDirectory),
      formatManager.getWildcardForAllFormats());

  fileChooser->launchAsync(
      juce::FileBrowserComponent::openMode |
          juce::FileBrowserComponent::canSelectFiles |
          juce::FileBrowserComponent::canSelectMultipleItems,
      [this](const juce::FileChooser &chooser) {
        auto results = chooser.getResults();
        if (results.isEmpty())
          return;

        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
        auto timeString = oss.str();

        for (auto &audioFile : results) {
          audioReader.reset(formatManager.createReaderFor(audioFile));
          if (audioReader != nullptr) {
            std::hash<std::string> hasher;
            track thisTrack = {audioFile.getFileNameWithoutExtension(),
                               audioReader->lengthInSamples /
                                   audioReader->sampleRate,
                               juce::URL{audioFile}};
            size_t hash = hasher(
                thisTrack.title.toStdString() +
                std::to_string(thisTrack.lengthInSeconds) +
                thisTrack.url.toString(false).toStdString() +
                std::to_string(
                    trackFolders[selectedFolderIndex].second.size()) +
                timeString);
            char hashString[256] = "";
            snprintf(hashString, sizeof hashString, "%zu", hash);
            thisTrack.identity = juce::String(hashString);
            thisTrack.fileHash = FileHasher::computeHash(audioFile);
            if (thisTrack.fileHash.isNotEmpty() && TrackDataCache::exists(thisTrack.fileHash))
              thisTrack.bpm = TrackDataCache::load(thisTrack.fileHash).detectedBpm;
            trackFolders[selectedFolderIndex].second.push_back(thisTrack);
          }
        }
        queueBpmAnalysis(trackFolders[selectedFolderIndex].second);
        playlist.setTrackTitles(trackFolders[selectedFolderIndex].second);
      });
}

/**
 * Implementation of importFolderFromDisk method for Library
 *
 * Opens a directory chooser, scans the selected folder for audio files,
 * and adds them as a new library folder.
 */
void Library::importFolderFromDisk() {
  fileChooser = std::make_unique<juce::FileChooser>(
      "Select a folder to import",
      juce::File::getSpecialLocation(juce::File::userHomeDirectory));

  fileChooser->launchAsync(
      juce::FileBrowserComponent::openMode |
          juce::FileBrowserComponent::canSelectDirectories,
      [this](const juce::FileChooser &chooser) {
        auto result = chooser.getResult();
        if (!result.isDirectory())
          return;

        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
        auto timeString = oss.str();

        std::pair<juce::String, std::vector<track>> thisFolder;
        thisFolder.first = result.getFileNameWithoutExtension();

        auto childFiles = result.findChildFiles(
            juce::File::TypesOfFileToFind::findFiles, false,
            formatManager.getWildcardForAllFormats());

        for (auto &file : childFiles) {
          audioReader.reset(formatManager.createReaderFor(file));
          if (audioReader != nullptr) {
            std::hash<std::string> hasher;
            track thisTrack = {file.getFileNameWithoutExtension(),
                               audioReader->lengthInSamples /
                                   audioReader->sampleRate,
                               juce::URL{file}};
            size_t hash = hasher(
                thisTrack.title.toStdString() +
                std::to_string(thisTrack.lengthInSeconds) +
                thisTrack.url.toString(false).toStdString() +
                std::to_string(thisFolder.second.size()) +
                timeString);
            char hashString[256] = "";
            snprintf(hashString, sizeof hashString, "%zu", hash);
            thisTrack.identity = juce::String(hashString);
            thisTrack.fileHash = FileHasher::computeHash(file);
            if (thisTrack.fileHash.isNotEmpty() && TrackDataCache::exists(thisTrack.fileHash))
              thisTrack.bpm = TrackDataCache::load(thisTrack.fileHash).detectedBpm;
            thisFolder.second.push_back(thisTrack);
          }
        }

        if (!thisFolder.second.empty()) {
          trackFolders.push_back(thisFolder);
          selectedFolderIndex = static_cast<int>(trackFolders.size()) - 1;
          directoryComponent.updateContent();
          directoryComponent.selectRow(selectedFolderIndex);
          queueBpmAnalysis(trackFolders[selectedFolderIndex].second);
          playlist.setTrackTitles(trackFolders[selectedFolderIndex].second);
        }
      });
}

/**
 * Implementation of removeSelectedTrack method for Library
 *
 * Removes the currently selected track from the current folder.
 */
void Library::removeSelectedTrack() {
  if (selectedFolderIndex < 0 ||
      selectedFolderIndex >= static_cast<int>(trackFolders.size()))
    return;

  if (!playlist.trackIsSelected())
    return;

  auto selectedTrackObj = playlist.getSelectedTrack();
  auto &selectedPlaylist = trackFolders[selectedFolderIndex].second;
  for (auto it = selectedPlaylist.begin(); it != selectedPlaylist.end(); ++it) {
    if (it->identity == selectedTrackObj.identity) {
      selectedPlaylist.erase(it);
      break;
    }
  }
  playlist.setTrackTitles(trackFolders[selectedFolderIndex].second);
}

//==============================================================================

/**
 * Implementation of queueBpmAnalysis method for Library
 *
 * Queues background BPM analysis for tracks that don't yet have a BPM value.
 * Computes fileHash on-the-fly if missing.
 */
void Library::queueBpmAnalysis(std::vector<track>& tracks)
{
	DBG("Library::queueBpmAnalysis - " + juce::String(static_cast<int>(tracks.size())) + " tracks");
	for (auto& t : tracks)
	{
		// Compute fileHash if not yet set
		if (t.fileHash.isEmpty())
		{
			juce::File audioFile = t.url.getLocalFile();
			DBG("  Computing hash for: " + t.title + " file=" + audioFile.getFullPathName() + " exists=" + juce::String(audioFile.existsAsFile() ? "yes" : "no"));
			if (audioFile.existsAsFile())
				t.fileHash = FileHasher::computeHash(audioFile);
		}

		// Queue analysis if BPM is unknown
		if (t.bpm <= 0.0 && t.fileHash.isNotEmpty())
		{
			juce::File audioFile = t.url.getLocalFile();
			bpmAnalysisManager.analyzeTrack(audioFile, t.fileHash);
		}
	}
}

/**
 * Implementation of bpmAnalysisComplete callback for Library
 *
 * Called on the message thread when a background BPM analysis finishes.
 * Updates all tracks with the matching fileHash and refreshes the playlist.
 */
void Library::bpmAnalysisComplete(const juce::String& fileHash, double bpm)
{
	DBG("Library::bpmAnalysisComplete - hash=" + fileHash + " bpm=" + juce::String(bpm));
	bool updated = false;
	for (auto& folder : trackFolders)
	{
		for (auto& t : folder.second)
		{
			if (t.fileHash == fileHash && t.bpm <= 0.0)
			{
				t.bpm = bpm;
				updated = true;
			}
		}
	}

	if (updated && selectedFolderIndex >= 0 &&
		selectedFolderIndex < static_cast<int>(trackFolders.size()))
	{
		playlist.setTrackTitles(trackFolders[selectedFolderIndex].second);
	}
}