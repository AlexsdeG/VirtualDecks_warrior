#include "BpmAnalysisManager.h"

//==============================================================================

BpmAnalysisManager::BpmAnalysisManager(juce::AudioFormatManager& formatManager, int numThreads)
	: formatManager(formatManager), threadPool(numThreads)
{
}

BpmAnalysisManager::~BpmAnalysisManager()
{
	threadPool.removeAllJobs(true, 10000);
}

//==============================================================================

void BpmAnalysisManager::analyzeTrack(const juce::File& audioFile, const juce::String& fileHash)
{
	if (fileHash.isEmpty() || !audioFile.existsAsFile())
	{
		DBG("BpmAnalysisManager::analyzeTrack - skipped: hash=" + fileHash + " exists=" + juce::String(audioFile.existsAsFile() ? "yes" : "no") + " path=" + audioFile.getFullPathName());
		return;
	}

	DBG("BpmAnalysisManager::analyzeTrack - queuing: " + audioFile.getFileName() + " hash=" + fileHash);

	// If cached analysis exists with a valid BPM, notify immediately
	if (TrackDataCache::exists(fileHash))
	{
		TrackData cached = TrackDataCache::load(fileHash);
		if (cached.detectedBpm > 0.0)
		{
			auto hash = fileHash;
			auto bpm = cached.detectedBpm;
			auto* self = this;
			juce::MessageManager::callAsync([self, hash, bpm]() {
				self->notifyListeners(hash, bpm);
			});
			return;
		}
	}

	// Try metadata first (fast, on calling thread is fine for a quick check)
	juce::String filePath = audioFile.getFullPathName();
	double metadataBpm = BpmDetector::readBpmFromMetadata(filePath);
	if (metadataBpm > 0.0)
	{
		TrackData data;
		data.detectedBpm = metadataBpm;
		data.confidence = 1.0;
		data.beatGrid.bpm = metadataBpm;
		TrackDataCache::save(fileHash, data);

		auto hash = fileHash;
		auto* self = this;
		juce::MessageManager::callAsync([self, hash, metadataBpm]() {
			self->notifyListeners(hash, metadataBpm);
		});
		return;
	}

	// Queue background audio analysis
	threadPool.addJob(new AnalysisJob(*this, audioFile, fileHash), true);
}

//==============================================================================

void BpmAnalysisManager::addListener(Listener* listener)
{
	listeners.add(listener);
}

void BpmAnalysisManager::removeListener(Listener* listener)
{
	listeners.remove(listener);
}

void BpmAnalysisManager::notifyListeners(const juce::String& fileHash, double bpm)
{
	listeners.call([&](Listener& l) { l.bpmAnalysisComplete(fileHash, bpm); });
}

//==============================================================================

BpmAnalysisManager::AnalysisJob::AnalysisJob(BpmAnalysisManager& owner, const juce::File& file, const juce::String& hash)
	: juce::ThreadPoolJob("BPM-" + hash.substring(0, 8)),
	  owner(owner),
	  audioFile(file),
	  fileHash(hash)
{
}

juce::ThreadPoolJob::JobStatus BpmAnalysisManager::AnalysisJob::runJob()
{
	// Create a thread-local AudioFormatManager since the shared one
	// is not thread-safe for concurrent createReaderFor() calls.
	juce::AudioFormatManager localFormatManager;
	localFormatManager.registerBasicFormats();

	std::unique_ptr<juce::AudioFormatReader> reader(
		localFormatManager.createReaderFor(audioFile));

	if (reader == nullptr)
	{
		DBG("BpmAnalysisManager::AnalysisJob - no reader for: " + audioFile.getFileName());
		return jobHasFinished;
	}

	BpmResult bpmResult = BpmDetector::analyze(reader.get());
	DBG("BpmAnalysisManager::AnalysisJob - result bpm=" + juce::String(bpmResult.bpm) + " confidence=" + juce::String(bpmResult.confidence) + " for: " + audioFile.getFileName());

	// Save to cache
	TrackData data;
	data.detectedBpm = bpmResult.bpm;
	data.confidence = bpmResult.confidence;
	data.beatGrid.bpm = bpmResult.bpm;
	TrackDataCache::save(fileHash, data);

	// Notify on message thread — capture by value since the job
	// is deleted by the pool after runJob() returns.
	double detectedBpm = bpmResult.bpm;
	auto* mgr = &owner;
	auto hash = fileHash;
	juce::MessageManager::callAsync([mgr, hash, detectedBpm]() {
		mgr->notifyListeners(hash, detectedBpm);
	});

	return jobHasFinished;
}
