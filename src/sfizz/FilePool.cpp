// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand, Andrea Zanellato
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "FilePool.h"
#include "Buffer.h"
#include "AudioBuffer.h"
#include "Config.h"
#include "Debug.h"
#include "Oversampler.h"
#include "absl/types/span.h"
#include "absl/strings/match.h"
#include "absl/memory/memory.h"
#include <algorithm>
#include <memory>
#include <sndfile.hh>
#include <thread>

template <class T>
void readBaseFile(SndfileHandle& sndFile, sfz::AudioBuffer<T>& output, uint32_t numFrames, bool reverse)
{
    output.reset();
    output.resize(numFrames + sfz::config::excessFileFrames);
    output.clear();

    if (reverse)
        sndFile.seek(-static_cast<sf_count_t>(numFrames), SEEK_END);

    const unsigned channels = sndFile.channels();

    if (channels == 1) {
        output.addChannel();
        sndFile.readf(output.channelWriter(0), numFrames);
    } else if (channels == 2) {
        output.addChannel();
        output.addChannel();
        sfz::Buffer<T> tempReadBuffer { 2 * numFrames };
        sndFile.readf(tempReadBuffer.data(), numFrames);
        sfz::readInterleaved<T>(tempReadBuffer, output.getSpan(0).first(numFrames), output.getSpan(1).first(numFrames));
    }

    if (reverse) {
        for (unsigned c = 0; c < channels; ++c) {
            // TODO: consider optimizing with SIMD
            absl::Span<float> channel = output.getSpan(c).first(numFrames);
            std::reverse(channel.begin(), channel.end());
        }
    }
}

template <class T>
std::unique_ptr<sfz::AudioBuffer<T>> readFromFile(SndfileHandle& sndFile, uint32_t numFrames, sfz::Oversampling factor, bool reverse)
{
    auto baseBuffer = absl::make_unique<sfz::AudioBuffer<T>>();
    readBaseFile(sndFile, *baseBuffer, numFrames, reverse);

    if (factor == sfz::Oversampling::x1)
        return baseBuffer;

    auto outputBuffer = absl::make_unique<sfz::AudioBuffer<T>>(sndFile.channels(), numFrames * static_cast<int>(factor));
    sfz::Oversampler oversampler { factor };
    oversampler.stream(*baseBuffer, *outputBuffer);
    return outputBuffer;
}

template <class T>
void streamFromFile(SndfileHandle& sndFile, uint32_t numFrames, sfz::Oversampling factor, bool reverse, sfz::AudioBuffer<float>& output, std::atomic<size_t>* filledFrames = nullptr)
{
    if (factor == sfz::Oversampling::x1) {
        readBaseFile(sndFile, output, numFrames, reverse);
        if (filledFrames != nullptr)
            filledFrames->store(numFrames);
        return;
    }

    auto baseBuffer = readFromFile<T>(sndFile, numFrames, sfz::Oversampling::x1, reverse);
    output.reset();
    output.addChannels(baseBuffer->getNumChannels());
    output.resize(numFrames * static_cast<int>(factor) + sfz::config::excessFileFrames);
    output.clear();
    sfz::Oversampler oversampler { factor };
    oversampler.stream(*baseBuffer, output, filledFrames);
}

sfz::FilePool::FilePool(sfz::Logger& logger)
: logger(logger)
{
    FilePromise promise;
    if (!promise.dataStatus.is_lock_free())
        DBG("atomic<DataStatus> is not lock-free; could cause issues with locking");

    for (int i = 0; i < config::numBackgroundThreads; ++i)
        threadPool.emplace_back( &FilePool::loadingThread, this );

    threadPool.emplace_back( &FilePool::clearingThread, this );

    for (int i = 0; i < config::maxFilePromises; ++i)
        emptyPromises.push_back(std::make_shared<FilePromise>());
}

sfz::FilePool::~FilePool()
{
    quitThread = true;

    for (unsigned i = 0; i < threadPool.size(); ++i) {
        std::error_code ec;
        workerBarrier.post(ec);
    }

    for (auto& thread: threadPool)
        thread.join();
}

bool sfz::FilePool::checkSample(std::string& filename) const noexcept
{
    fs::path path { rootDirectory / filename };
    std::error_code ec;
    if (fs::exists(path, ec))
        return true;

#if WIN32
    return false;
#else
    fs::path oldPath = std::move(path);
    path = oldPath.root_path();

    static const fs::path dot { "." };
    static const fs::path dotdot { ".." };

    for (const fs::path &part : oldPath.relative_path()) {
        if (part == dot || part == dotdot) {
            path /= part;
            continue;
        }

        if (fs::exists(path / part, ec)) {
            path /= part;
            continue;
        }

        auto it = path.empty() ? fs::directory_iterator{ dot, ec } : fs::directory_iterator{ path, ec };
        if (ec) {
            DBG("Error creating a directory iterator for " << filename << " (Error code: " << ec.message() << ")");
            return false;
        }

        auto searchPredicate = [&part](const fs::directory_entry &ent) -> bool {
            return absl::EqualsIgnoreCase(
                ent.path().filename().native(), part.native());
        };

        while (it != fs::directory_iterator{} && !searchPredicate(*it))
            it.increment(ec);

        if (it == fs::directory_iterator{}) {
            DBG("File not found, could not resolve " << filename);
            return false;
        }

        path /= it->path().filename();
    }

    const auto newPath = fs::relative(path, rootDirectory, ec);
    if (ec) {
        DBG("Error extracting the new relative path for " << filename << " (Error code: " << ec.message() << ")");
        return false;
    }
    DBG("Updating " << filename << " to " << newPath.native());
    filename = newPath.string();
    return true;
#endif
}

bool sfz::FilePool::checkSampleId(FileId& fileId) const noexcept
{
    std::string filename = fileId.filename();
    bool result = checkSample(filename);
    if (result)
        fileId = FileId(std::move(filename), fileId.isReverse());
    return result;
}

absl::optional<sfz::FileInformation> sfz::FilePool::getFileInformation(const FileId& fileId) noexcept
{
    const fs::path file { rootDirectory / fileId.filename() };

    if (!fs::exists(file))
        return {};

    SndfileHandle sndFile(file.string().c_str());
    if (sndFile.channels() != 1 && sndFile.channels() != 2) {
        DBG("[sfizz] Missing logic for " << sndFile.channels() << " channels, discarding sample " << fileId);
        return {};
    }

    FileInformation returnedValue;
    returnedValue.end = static_cast<uint32_t>(sndFile.frames()) - 1;
    returnedValue.sampleRate = static_cast<double>(sndFile.samplerate());
    returnedValue.numChannels = sndFile.channels();

    if (!fileId.isReverse()) {
        SF_INSTRUMENT instrumentInfo;
        sndFile.command(SFC_GET_INSTRUMENT, &instrumentInfo, sizeof(instrumentInfo));
        if (instrumentInfo.loop_count > 0) {
            returnedValue.loopBegin = instrumentInfo.loops[0].start;
            returnedValue.loopEnd = min(returnedValue.end, instrumentInfo.loops[0].end - 1);
        }
    } else {
        // TODO loops ignored when reversed
        //   prehaps it can make use of SF_LOOP_BACKWARD?
    }

    return returnedValue;
}

bool sfz::FilePool::preloadFile(const FileId& fileId, uint32_t maxOffset) noexcept
{
    auto fileInformation = getFileInformation(fileId);
    if (!fileInformation)
        return false;

    const fs::path file { rootDirectory / fileId.filename() };
    SndfileHandle sndFile(file.string().c_str());

    // FIXME: Large offsets will require large preloading; is this OK in practice? Apparently sforzando does the same
    const auto frames = static_cast<uint32_t>(sndFile.frames());
    const auto framesToLoad = [&]() {
        if (preloadSize == 0)
            return frames;
        else
            return min(frames, maxOffset + preloadSize);
    }();

    const auto existingFile = preloadedFiles.find(fileId);
    if (existingFile != preloadedFiles.end()) {
        if (framesToLoad > existingFile->second.preloadedData->getNumFrames()) {
            preloadedFiles[fileId].preloadedData = readFromFile<float>(sndFile, framesToLoad, oversamplingFactor, fileId.isReverse());
        }
    } else {
        fileInformation->sampleRate = static_cast<float>(oversamplingFactor) * static_cast<float>(sndFile.samplerate());
        FileDataHandle handle {
            readFromFile<float>(sndFile, framesToLoad, oversamplingFactor, fileId.isReverse()),
            *fileInformation
        };
        preloadedFiles.insert_or_assign(fileId, handle);
    }
    return true;
}

absl::optional<sfz::FileDataHandle> sfz::FilePool::loadFile(const FileId& fileId) noexcept
{
    auto fileInformation = getFileInformation(fileId);
    if (!fileInformation)
        return {};

    const fs::path file { rootDirectory / fileId.filename() };
    SndfileHandle sndFile(file.string().c_str());

    // FIXME: Large offsets will require large preloading; is this OK in practice? Apparently sforzando does the same
    const auto frames = static_cast<uint32_t>(sndFile.frames());
    const auto existingFile = loadedFiles.find(fileId);
    if (existingFile != loadedFiles.end()) {
        return existingFile->second;
    } else {
        fileInformation->sampleRate = static_cast<float>(oversamplingFactor) * static_cast<float>(sndFile.samplerate());
        FileDataHandle handle {
            readFromFile<float>(sndFile, frames, oversamplingFactor, fileId.isReverse()),
            *fileInformation
        };
        loadedFiles.insert_or_assign(fileId, handle);
        return handle;
    }
}

sfz::FilePromisePtr sfz::FilePool::getFilePromise(const FileId& fileId) noexcept
{
    if (emptyPromises.empty()) {
        DBG("[sfizz] No empty promises left to honor the one for " << fileId);
        return {};
    }

    const auto preloaded = preloadedFiles.find(fileId);
    if (preloaded == preloadedFiles.end()) {
        DBG("[sfizz] File not found in the preloaded files: " << fileId);
        return {};
    }

    auto promise = emptyPromises.back();
    promise->fileId = preloaded->first;
    promise->preloadedData = preloaded->second.preloadedData;
    promise->sampleRate = static_cast<float>(preloaded->second.information.sampleRate);
    promise->oversamplingFactor = oversamplingFactor;
    promise->creationTime = std::chrono::high_resolution_clock::now();

    if (!promiseQueue.try_push(promise)) {
        DBG("[sfizz] Could not enqueue the promise for " << fileId << " (queue capacity " << promiseQueue.capacity() << ")");
        return {};
    }

    std::error_code ec;
    workerBarrier.post(ec);
    ASSERT(!ec);

    emptyPromises.pop_back();

    return promise;
}

void sfz::FilePool::setPreloadSize(uint32_t preloadSize) noexcept
{
    // Update all the preloaded sizes
    for (auto& preloadedFile : preloadedFiles) {
        const auto numFrames = preloadedFile.second.preloadedData->getNumFrames() / static_cast<int>(oversamplingFactor);
        const auto maxOffset = numFrames > this->preloadSize ? static_cast<uint32_t>(numFrames) - this->preloadSize : 0;
        fs::path file { rootDirectory / preloadedFile.first.filename() };
        SndfileHandle sndFile(file.string().c_str());
        preloadedFile.second.preloadedData = readFromFile<float>(sndFile, preloadSize + maxOffset, oversamplingFactor, preloadedFile.first.isReverse());
    }
    this->preloadSize = preloadSize;
}

void sfz::FilePool::tryToClearPromises()
{
    const std::lock_guard<std::mutex> promiseLock { promiseGuard };

    for (auto& promise: promisesToClear) {
        if (promise->dataStatus != FilePromise::DataStatus::Wait)
            promise->reset();
    }
}

void sfz::FilePool::clearingThread()
{
    while (!quitThread) {
        tryToClearPromises();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void sfz::FilePool::loadingThread() noexcept
{
    FilePromisePtr promise;
    while (!quitThread) {

        if (emptyQueue) {
            while(promiseQueue.try_pop(promise)) {
                // We're just dequeuing
            }
            emptyQueue = false;
            continue;
        }

        std::error_code ec;
        workerBarrier.wait(ec);
        ASSERT(!ec);

        if (!promiseQueue.try_pop(promise)) {
            continue;
        }

        threadsLoading++;
        const auto loadStartTime = std::chrono::high_resolution_clock::now();
        const auto waitDuration = loadStartTime - promise->creationTime;

        const fs::path file { rootDirectory / promise->fileId.filename() };
        SndfileHandle sndFile(file.string().c_str());
        if (sndFile.error() != 0) {
            DBG("[sfizz] libsndfile errored for " << promise->fileId << " with message " << sndFile.strError());
            promise->dataStatus = FilePromise::DataStatus::Error;
            continue;
        }
        const auto frames = static_cast<uint32_t>(sndFile.frames());
        streamFromFile<float>(sndFile, frames, oversamplingFactor, promise->fileId.isReverse(), promise->fileData, &promise->availableFrames);
        promise->dataStatus = FilePromise::DataStatus::Ready;
        const auto loadDuration = std::chrono::high_resolution_clock::now() - loadStartTime;
        logger.logFileTime(waitDuration, loadDuration, frames, promise->fileId.filename());

        threadsLoading--;

        while (!filledPromiseQueue.try_push(promise)) {
            DBG("[sfizz] Error enqueuing the promise for " << promise->fileId << " in the filledPromiseQueue");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        promise.reset();
    }
}

void sfz::FilePool::clear()
{
    emptyFileLoadingQueues();
    preloadedFiles.clear();
    temporaryFilePromises.clear();
    promisesToClear.clear();
}

void sfz::FilePool::cleanupPromises() noexcept
{
    const std::unique_lock<std::mutex> lock { promiseGuard, std::try_to_lock };
    if (!lock.owns_lock())
        return;

    // The garbage collection cleared the data from these so we can move them
    // back to the empty queue
    auto clearedIterator = promisesToClear.begin();
    auto clearedSentinel = promisesToClear.rbegin();
    while (clearedIterator < clearedSentinel.base()) {
        if (clearedIterator->get()->dataStatus == FilePromise::DataStatus::Wait) {
            emptyPromises.push_back(*clearedIterator);
            std::iter_swap(clearedIterator, clearedSentinel);
            ++clearedSentinel;
        } else {
            ++clearedIterator;
        }
    }
    promisesToClear.resize(std::distance(promisesToClear.begin(), clearedSentinel.base()));

    FilePromisePtr promise;
    // Remove the promises from the filled queue and put them in a linear
    // storage
    while (filledPromiseQueue.try_pop(promise))
        temporaryFilePromises.push_back(promise);

    auto filledIterator = temporaryFilePromises.begin();
    auto filledSentinel = temporaryFilePromises.rbegin();
    while (filledIterator < filledSentinel.base()) {
        if (filledIterator->use_count() == 1) {
            promisesToClear.push_back(*filledIterator);
            std::iter_swap(filledIterator, filledSentinel);
            ++filledSentinel;
        } else {
            ++filledIterator;
        }
    }
    temporaryFilePromises.resize(std::distance(temporaryFilePromises.begin(), filledSentinel.base()));
}

void sfz::FilePool::setOversamplingFactor(sfz::Oversampling factor) noexcept
{
    float samplerateChange { static_cast<float>(factor) / static_cast<float>(this->oversamplingFactor) };
    for (auto& preloadedFile : preloadedFiles) {
        const auto numFrames = preloadedFile.second.preloadedData->getNumFrames() / static_cast<int>(this->oversamplingFactor);
        const uint32_t maxOffset = numFrames > this->preloadSize ? static_cast<uint32_t>(numFrames) - this->preloadSize : 0;
        fs::path file { rootDirectory / preloadedFile.first.filename() };
        SndfileHandle sndFile(file.string().c_str());
        preloadedFile.second.preloadedData = readFromFile<float>(sndFile, preloadSize + maxOffset, factor, preloadedFile.first.isReverse());
        preloadedFile.second.information.sampleRate *= samplerateChange;
    }

    this->oversamplingFactor = factor;
}

sfz::Oversampling sfz::FilePool::getOversamplingFactor() const noexcept
{
    return oversamplingFactor;
}

uint32_t sfz::FilePool::getPreloadSize() const noexcept
{
    return preloadSize;
}

void sfz::FilePool::emptyFileLoadingQueues() noexcept
{
    emptyQueue = true;
    std::error_code ec;
    workerBarrier.post(ec);
    ASSERT(!ec);

    while (emptyQueue)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void sfz::FilePool::waitForBackgroundLoading() noexcept
{
    // TODO: validate that this is enough, otherwise we will need an atomic count
    // of the files we need to load still.
    // Spinlocking on the size of the background queue
    while (!promiseQueue.was_empty()){
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // Spinlocking on the threads possibly logging in the background
    while (threadsLoading > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}
