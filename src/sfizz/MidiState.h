// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "CCMap.h"
#include "Range.h"
#include <array>
#include <memory>

namespace sfz
{
/**
 * @brief Holds the current "MIDI state", meaning the known state of all CCs
 * currently, as well as the note velocities that triggered the currently
 * pressed notes.
 *
 */
class MidiState
{
public:
    MidiState();

    /**
     * @brief Update the state after a note on event
     *
     * @param delay
     * @param noteNumber
     * @param velocity
     */
    void noteOnEvent(int delay, int noteNumber, float velocity) noexcept;

    /**
     * @brief Update the state after a note off event
     *
     * @param delay
     * @param noteNumber
     * @param velocity
     */
    void noteOffEvent(int delay, int noteNumber, float velocity) noexcept;

    /**
     * @brief Set all notes off
     *
     * @param delay
     */
    void allNotesOff(int delay) noexcept;

    /**
     * @brief Get the number of active notes
     */
    int getActiveNotes() const noexcept { return activeNotes; }

    /**
     * @brief Get the note duration since note on
     *
     * @param noteNumber
     * @param delay
     * @return float
     */
    float getNoteDuration(int noteNumber, int delay = 0) const;

    /**
     * @brief Set the maximum size of the blocks for the callback. The actual
     * size can be lower in each callback but should not be larger
     * than this value.
     *
     * @param samplesPerBlock
     */
    void setSamplesPerBlock(int samplesPerBlock) noexcept;
    /**
     * @brief Set the sample rate. If you do not call it it is initialized
     * to sfz::config::defaultSampleRate.
     *
     * @param sampleRate
     */
    void setSampleRate(float sampleRate) noexcept;
    /**
     * @brief Get the note on velocity for a given note
     *
     * @param noteNumber
     * @return float
     */
    float getNoteVelocity(int noteNumber) const noexcept;

    /**
     * @brief Register a pitch bend event
     *
     * @param pitchBendValue
     */
    void pitchBendEvent(int delay, float pitchBendValue) noexcept;

    /**
     * @brief Get the pitch bend status

     * @return int
     */
    float getPitchBend() const noexcept;

    /**
     * @brief Register a CC event
     *
     * @param ccNumber
     * @param ccValue
     */
    void ccEvent(int delay, int ccNumber, float ccValue) noexcept;

    /**
     * @brief Advances the internal clock of a given amount of samples.
     * You should call this at each callback. This will flush the events
     * in the midistate memory.
     *
     * @param numSamples the number of samples of clock advance
     */
    void advanceTime(int numSamples) noexcept;
    /**
     * @brief Get the CC value for CC number
     *
     * @param ccNumber
     * @return float
     */
    float getCCValue(int ccNumber) const noexcept;

    /**
     * @brief Reset the midi state (does not impact the last note on time)
     *
     */
    void reset() noexcept;

    /**
     * @brief Reset all the controllers
     */
    void resetAllControllers(int delay) noexcept;

    const EventVector& getCCEvents(int ccIdx) const noexcept;
    const EventVector& getPitchEvents() const noexcept;

    /**
     * @brief Observer of controller change events
     */
    class ControllerChangeObserver {
    public:
        virtual ~ControllerChangeObserver() {}
        virtual void onAllControllersReset() = 0;
        virtual void onControllerChange(int ccNumber, float ccValue) = 0;
    };

    /**
     * @brief Observer of controller change events, which records the
     * information in a linked list of non-duplicated controller changes.
     */
    class ControllerChangeRecorder : public ControllerChangeObserver {
    public:
        ControllerChangeRecorder();
        ~ControllerChangeRecorder();
        bool getNextControllerChange(int& ccNumber, float& ccValue) noexcept; // O(1)

    protected:
        void onAllControllersReset() override;
        void onControllerChange(int ccNumber, float ccValue) override;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

    /**
     * @brief Set the controller change observer
     */
    void setControllerChangeObserver(ControllerChangeObserver* obs)
    {
        ccObserver = obs;
    }

private:
    int activeNotes { 0 };

    /**
     * @brief Stores the note on times.
     *
     */
    MidiNoteArray<unsigned> noteOnTimes { {} };

    /**
     * @brief Stores the note off times.
     *
     */

    MidiNoteArray<unsigned> noteOffTimes { {} };

    /**
     * @brief Stores the velocity of the note ons for currently
     * depressed notes.
     *
     */
    MidiNoteArray<float> lastNoteVelocities;

    /**
     * @brief Current known values for the CCs.
     *
     */
    std::array<EventVector, config::numCCs> cc;

    /**
     * @brief Null event
     *
     */
    const EventVector nullEvent { { 0, 0.0f } };

    /**
     * @brief Pitch bend status
     */
    EventVector pitchEvents;
    float sampleRate { config::defaultSampleRate };
    int samplesPerBlock { config::defaultSamplesPerBlock };
    unsigned internalClock { 0 };

    /**
     * @brief Controller change observer
     */
    ControllerChangeObserver* ccObserver = nullptr;
};
}
