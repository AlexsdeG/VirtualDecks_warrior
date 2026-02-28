---
mode: "ask"
description: "Debug and fix a bug in the DJ application"
---

# Fix Bug

You are diagnosing and fixing a bug in DJDecks.

## Context Gathering Strategy

1. **Read `.github/general-instructions.md`** to understand the architecture and signal flow.
2. **Read the file(s)** the user mentions or that are likely involved based on the symptom.
3. **Trace the data flow**: Most bugs involve the chain `UI event → DeckGUI callback → DJAudioPlayer method → audio chain`. Trace which links are involved.
4. **Check thread boundaries**: If the issue involves audio glitches or crashes, check whether UI-thread code is being called from the audio thread or vice versa.

## Common Bug Categories

### Audio Issues
- **No sound**: Check `loadURL()` success, `transportSource.start()` call, gain values (playerVol × crossFadeVol), filter states
- **Crackling/glitches**: Check for allocations in `getNextAudioBlock()`, check sample rate mismatches
- **Filter not working**: Check `thisSampleRate` is set before filter coefficients are computed (must happen after `prepareToPlay`)

### UI Issues  
- **Component not visible**: Check `addAndMakeVisible()`, check `setBounds()` in `resized()`, check paint order
- **Slider not responding**: Check `addListener(this)` and that the correct slider pointer is compared in `sliderValueChanged()`
- **Waveform not displaying**: Check `audioThumb` loading, check `changeListenerCallback` is connected

### Library/Data Issues
- **Tracks not saving**: Check XML write in Library destructor, check file path creation
- **Track not loading into deck**: Check `Library::selectionIsValid()` conditions, check `DeckGUI::loadDeck()` flow

## Fix Approach

1. Identify the minimal reproduction path
2. Read all involved source files
3. Make the smallest possible fix
4. Verify the fix doesn't break the audio chain order or thread safety
5. Build and test: `cmake --build build`
