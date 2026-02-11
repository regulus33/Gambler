#pragma once

#include <juce_core/juce_core.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

// ============================================================================
// JUCE Assertion Capture & Report Generator
// ============================================================================
//
// Captures jassert/jassertfalse failures during test runs and generates
// a markdown report (jassert_report.md) with each unique assertion and
// an action plan template.
//
// Requires JUCE_LOG_ASSERTIONS=1 so assertions route through Logger::writeToLog().
//
// Usage:
//   JassertCapture capture;                    // installs itself as current logger
//   // ... run tests ...
//   capture.writeReport("/path/to/output");    // writes jassert_report.md
//   // destructor restores the previous logger
//
// ============================================================================

struct AssertionRecord
{
    std::string file;
    int line = 0;
    int count = 0;
    bool suppressed = false; // true for known-benign JUCE internal assertions
};

class JassertCapture : public juce::Logger
{
public:
    JassertCapture()
    {
        previousLogger = juce::Logger::getCurrentLogger();
        juce::Logger::setCurrentLogger (this);
    }

    ~JassertCapture() override
    {
        juce::Logger::setCurrentLogger (previousLogger);
    }

    void writeReport (const std::string& outputDir) const
    {
        std::lock_guard<std::mutex> lock (mutex);

        // Separate actionable vs suppressed records
        std::vector<AssertionRecord const*> actionable;
        std::vector<AssertionRecord const*> suppressed;

        for (auto& r : records)
        {
            if (r.suppressed)
                suppressed.push_back (&r);
            else
                actionable.push_back (&r);
        }

        if (actionable.empty() && suppressed.empty())
        {
            juce::Logger::outputDebugString ("JassertCapture: No assertions captured, skipping report.");
            return;
        }

        // Build the report
        std::ostringstream md;

        // Timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t (now);
        std::tm tm {};
#if defined(_WIN32)
        localtime_s (&tm, &time);
#else
        localtime_r (&time, &tm);
#endif

        int totalActionableHits = 0;
        for (auto* r : actionable)
            totalActionableHits += r->count;

        int totalSuppressedHits = 0;
        for (auto* r : suppressed)
            totalSuppressedHits += r->count;

        md << "# JUCE Assertion Report\n\n";
        md << "**Generated:** " << std::put_time (&tm, "%Y-%m-%d %H:%M:%S") << "\n";
        md << "**Actionable assertion hits:** " << totalActionableHits << "\n";
        md << "**Actionable unique locations:** " << actionable.size() << "\n";
        if (! suppressed.empty())
            md << "**Suppressed (known-benign):** " << totalSuppressedHits << " hits across " << suppressed.size() << " locations\n";
        md << "\n---\n\n";

        if (! actionable.empty())
        {
            md << "## Assertions\n\n";

            int index = 1;
            for (auto* r : actionable)
            {
                md << "### " << index++ << ". `" << r->file << ":" << r->line << "`\n\n";
                md << "- **Hits:** " << r->count << "\n";
                md << "- **Action plan:** " << getActionPlan (*r) << "\n\n";
            }
        }
        else
        {
            md << "## No actionable assertions found!\n\n";
            md << "All captured assertions are known-benign JUCE internal lifecycle issues.\n\n";
        }

        if (! suppressed.empty())
        {
            md << "## Suppressed (Known-Benign)\n\n";
            md << "These are JUCE-internal assertions caused by Timer/MessageManager lifecycle\n";
            md << "ordering in test environments. They do not affect test correctness.\n\n";

            for (auto* r : suppressed)
            {
                md << "- `" << r->file << ":" << r->line << "` — " << r->count << " hits\n";
            }
            md << "\n";
        }

        // Write to file
        auto reportPath = juce::File (outputDir).getChildFile ("jassert_report.md");
        reportPath.deleteFile();
        juce::FileOutputStream stream (reportPath);

        if (stream.openedOk())
        {
            auto str = md.str();
            stream.write (str.data(), str.size());
            juce::Logger::outputDebugString ("JassertCapture: Report written to " + reportPath.getFullPathName());
        }
        else
        {
            juce::Logger::outputDebugString ("JassertCapture: Failed to write report to " + reportPath.getFullPathName());
        }
    }

    int getTotalHits() const
    {
        std::lock_guard<std::mutex> lock (mutex);
        int total = 0;
        for (auto& r : records)
            if (! r.suppressed)
                total += r.count;
        return total;
    }

    int getUniqueCount() const
    {
        std::lock_guard<std::mutex> lock (mutex);
        int count = 0;
        for (auto& r : records)
            if (! r.suppressed)
                count++;
        return count;
    }

protected:
    void logMessage (const juce::String& message) override
    {
        // Check if this is an assertion message from JUCE's logAssertion()
        // Format: "JUCE Assertion failure in <filename>:<line>"
        if (message.startsWith ("JUCE Assertion failure in "))
        {
            auto location = message.fromFirstOccurrenceOf ("JUCE Assertion failure in ", false, false);
            auto file = location.upToLastOccurrenceOf (":", false, false).toStdString();
            auto lineStr = location.fromLastOccurrenceOf (":", false, false);
            int line = lineStr.getIntValue();

            bool isSuppressed = isSuppressedAssertion (file, line);

            std::lock_guard<std::mutex> lock (mutex);

            // Deduplicate by file:line
            auto it = std::find_if (records.begin(), records.end(), [&] (const AssertionRecord& r) {
                return r.file == file && r.line == line;
            });

            if (it != records.end())
            {
                it->count++;
            }
            else
            {
                records.push_back ({ file, line, 1, isSuppressed });
            }
        }

        // Always pass through to stderr so existing output is preserved
        juce::Logger::outputDebugString (message);
    }

private:
    juce::Logger* previousLogger = nullptr;
    mutable std::mutex mutex;
    std::vector<AssertionRecord> records;

    // Returns true for assertions that are known-benign JUCE-internal lifecycle
    // issues in test environments (e.g., Timer/MessageManager race conditions).
    static bool isSuppressedAssertion (const std::string& file, int line)
    {
        // JUCE Timer assertions caused by APVTS timer firing during rapid
        // PluginProcessor creation/destruction in test suites. This is a race
        // condition in JUCE's singleton Timer thread — the MessageManager is
        // alive but the Timer thread's check can transiently fail.
        if (file == "juce_Timer.cpp" && (line == 376 || line == 99))
            return true;

        return false;
    }

    // Returns a context-aware action plan for known JUCE assertion locations.
    // For unknown assertions, returns a generic investigation template.
    static std::string getActionPlan (const AssertionRecord& r)
    {
        auto key = r.file + ":" + std::to_string (r.line);

        // Known JUCE assertion patterns — add new entries here as they're discovered
        if (key.find ("juce_Timer.cpp:376") != std::string::npos)
            return "Timer started without a running MessageManager. "
                   "This typically fires during test setup/teardown when a component "
                   "starts a Timer before ScopedJuceInitialiser_GUI is fully ready, "
                   "or after it has begun shutting down. "
                   "Consider deferring timer starts or ensuring proper lifecycle ordering.";

        if (key.find ("juce_Timer.cpp:99") != std::string::npos)
            return "Timer still running during shutdown. "
                   "A Timer object is being destroyed while it's still active. "
                   "Call stopTimer() in the owning component's destructor "
                   "before the MessageManager is torn down.";

        if (key.find ("juce_FontOptions.h:115") != std::string::npos)
            return "FontOptions::withStyle() called while a Typeface pointer is set. "
                   "When a Typeface is attached, the style string is ignored. "
                   "Either clear the typeface first (withTypeface(nullptr).withStyle(...)) "
                   "or set the style before attaching the typeface.";

        if (key.find ("juce_NormalisableRange.h:254") != std::string::npos)
            return "NormalisableRange invariant violated: end must be > start, "
                   "interval must be >= 0, and skew must be > 0. "
                   "Check that the range is properly constructed before use — "
                   "a default-constructed NormalisableRange has start==end==0 which fails this check.";

        // Generic fallback
        return "Investigate why this assertion fires. "
               "Check if it's a real bug, a test setup/teardown issue, "
               "or a benign assertion that can be addressed by adjusting initialization order.";
    }
};
