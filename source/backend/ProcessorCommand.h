#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include <array>

/**
 * ProcessorCommand
 *
 * A command pattern for thread-safe communication from UI thread to audio thread.
 * Commands are queued by the UI thread and executed by the audio thread in processBlock().
 *
 * This ensures all domain state modifications happen on the audio thread, avoiding race conditions.
 *
 * Optionally, a completion callback can be provided that will be executed on the message thread
 * after the command has been executed on the audio thread. This allows UI updates to happen
 * after the domain state has been modified, avoiding race conditions.
 */
class ProcessorCommand {
public:
    using CommandFunction = std::function<void()>;
    using CompletionCallback = std::function<void()>;

    ProcessorCommand() = default;

    /**
     * Create a command with just an execution function (no completion callback)
     */
    explicit ProcessorCommand(CommandFunction func)
        : command(std::move(func)) {}

    /**
     * Create a command with both an execution function and a completion callback.
     * The command runs on the audio thread, then the completion callback is queued
     * to run on the message thread.
     */
    ProcessorCommand(CommandFunction func, CompletionCallback completion)
        : command(std::move(func)), completionCallback(std::move(completion)) {}

    void execute() const {
        if (command) {
            command();
        }
    }

    bool isValid() const { return command != nullptr; }

    bool hasCompletionCallback() const { return completionCallback != nullptr; }

    const CompletionCallback& getCompletionCallback() const { return completionCallback; }

private:
    CommandFunction command;
    CompletionCallback completionCallback;
};

/**
 * ProcessorCommandQueue
 *
 * Lock-free FIFO queue for passing commands from UI thread to audio thread.
 * Uses JUCE's AbstractFifo for thread-safe ring buffer implementation.
 *
 * Also manages a completion callback queue for callbacks that need to run
 * on the message thread after commands have been executed on the audio thread.
 */
class ProcessorCommandQueue {
public:
    using CompletionCallback = ProcessorCommand::CompletionCallback;
    static constexpr int QUEUE_SIZE = 128;  // Should be enough for UI interactions

    ProcessorCommandQueue() : commandFifo(QUEUE_SIZE), completionFifo(QUEUE_SIZE) {}

    /**
     * Push a command onto the queue (called from UI thread)
     * Returns false if queue is full
     */
    bool push(ProcessorCommand command) {
        int start1, size1, start2, size2;
        commandFifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0) {
            commandBuffer[start1] = std::move(command);
            commandFifo.finishedWrite(1);
            return true;
        }

        return false;  // Queue full
    }

    /**
     * Execute all pending commands (called from audio thread)
     * Any completion callbacks are queued for later execution on the message thread.
     * Returns the number of commands executed
     */
    int executeAll() {
        int numExecuted = 0;
        int numReady = commandFifo.getNumReady();

        if (numReady > 0) {
            int start1, size1, start2, size2;
            commandFifo.prepareToRead(numReady, start1, size1, start2, size2);

            // Execute first block
            for (int i = 0; i < size1; ++i) {
                auto& cmd = commandBuffer[start1 + i];
                cmd.execute();

                // Queue completion callback if present
                if (cmd.hasCompletionCallback()) {
                    queueCompletionCallback(cmd.getCompletionCallback());
                }
                numExecuted++;
            }

            // Execute second block (if ring buffer wrapped around)
            for (int i = 0; i < size2; ++i) {
                auto& cmd = commandBuffer[start2 + i];
                cmd.execute();

                // Queue completion callback if present
                if (cmd.hasCompletionCallback()) {
                    queueCompletionCallback(cmd.getCompletionCallback());
                }
                numExecuted++;
            }

            commandFifo.finishedRead(numReady);
        }

        return numExecuted;
    }

    /**
     * Execute all pending completion callbacks (called from message thread)
     * Returns the number of callbacks executed
     */
    int executeCompletionCallbacks() {
        int numExecuted = 0;
        int numReady = completionFifo.getNumReady();

        if (numReady > 0) {
            int start1, size1, start2, size2;
            completionFifo.prepareToRead(numReady, start1, size1, start2, size2);

            // Execute first block
            for (int i = 0; i < size1; ++i) {
                if (completionBuffer[start1 + i]) {
                    completionBuffer[start1 + i]();
                }
                numExecuted++;
            }

            // Execute second block (if ring buffer wrapped around)
            for (int i = 0; i < size2; ++i) {
                if (completionBuffer[start2 + i]) {
                    completionBuffer[start2 + i]();
                }
                numExecuted++;
            }

            completionFifo.finishedRead(numReady);
        }

        return numExecuted;
    }

    /**
     * Check if there are pending completion callbacks
     */
    bool hasCompletionCallbacks() const {
        return completionFifo.getNumReady() > 0;
    }

    /**
     * Get number of commands waiting in queue
     */
    int getNumPending() const {
        return commandFifo.getNumReady();
    }

private:
    /**
     * Queue a completion callback (called from audio thread after command execution)
     */
    void queueCompletionCallback(const CompletionCallback& callback) {
        int start1, size1, start2, size2;
        completionFifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0) {
            completionBuffer[start1] = callback;
            completionFifo.finishedWrite(1);
        }
        // If queue is full, callback is dropped (shouldn't happen in practice)
    }

    // Command queue (UI thread -> Audio thread)
    juce::AbstractFifo commandFifo;
    std::array<ProcessorCommand, QUEUE_SIZE> commandBuffer;

    // Completion callback queue (Audio thread -> Message thread)
    juce::AbstractFifo completionFifo;
    std::array<CompletionCallback, QUEUE_SIZE> completionBuffer;
};

