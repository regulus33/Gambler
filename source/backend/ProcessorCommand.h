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
 */
class ProcessorCommand {
public:
    using CommandFunction = std::function<void()>;
    
    ProcessorCommand() = default;
    explicit ProcessorCommand(CommandFunction func) : command(std::move(func)) {}
    
    void execute() const {
        if (command) {
            command();
        }
    }
    
    bool isValid() const { return command != nullptr; }
    
private:
    CommandFunction command;
};

/**
 * ProcessorCommandQueue
 * 
 * Lock-free FIFO queue for passing commands from UI thread to audio thread.
 * Uses JUCE's AbstractFifo for thread-safe ring buffer implementation.
 */
class ProcessorCommandQueue {
public:
    static constexpr int QUEUE_SIZE = 128;  // Should be enough for UI interactions
    
    ProcessorCommandQueue() : fifo(QUEUE_SIZE) {}
    
    /**
     * Push a command onto the queue (called from UI thread)
     * Returns false if queue is full
     */
    bool push(ProcessorCommand command) {
        int start1, size1, start2, size2;
        fifo.prepareToWrite(1, start1, size1, start2, size2);
        
        if (size1 > 0) {
            buffer[start1] = std::move(command);
            fifo.finishedWrite(1);
            return true;
        }
        
        return false;  // Queue full
    }
    
    /**
     * Execute all pending commands (called from audio thread)
     * Returns the number of commands executed
     */
    int executeAll() {
        int numExecuted = 0;
        int numReady = fifo.getNumReady();
        
        if (numReady > 0) {
            int start1, size1, start2, size2;
            fifo.prepareToRead(numReady, start1, size1, start2, size2);
            
            // Execute first block
            for (int i = 0; i < size1; ++i) {
                buffer[start1 + i].execute();
                numExecuted++;
            }
            
            // Execute second block (if ring buffer wrapped around)
            for (int i = 0; i < size2; ++i) {
                buffer[start2 + i].execute();
                numExecuted++;
            }
            
            fifo.finishedRead(numReady);
        }
        
        return numExecuted;
    }
    
    /**
     * Get number of commands waiting in queue
     */
    int getNumPending() const {
        return fifo.getNumReady();
    }
    
private:
    juce::AbstractFifo fifo;
    std::array<ProcessorCommand, QUEUE_SIZE> buffer;
};

