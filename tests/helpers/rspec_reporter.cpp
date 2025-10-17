
#include <catch2/reporters/catch_reporter_registrars.hpp>

#pragma once

#include <catch2/reporters/catch_reporter_streaming_base.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/catch_section_info.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// A lightweight RSpec-style reporter for Catch2 v3 that prints a
// nested, colored summary after each test case finishes.
//
// Example output (green on success, red on failure):
//   APVTS Integration
//       ✓ Makes the thing work some way
//           ✓ Makes the MIDI note work some way
//
// Implementation notes:
// - We build a stable section tree during execution, de-duplicating sections
//   by file+line+name so parent sections only appear once even if visited
//   multiple times across different paths.
// - We track assertion failures via assertionEnded() and mark a section as
//   failed if any assertion fails while that section is active (including
//   within nested sections).
// - At the end of the test case, we render the tree top-down with indentation.

namespace rspec_style_detail {
struct Node {
    std::string name;
    std::string key; // unique key: name@file:line
    bool success = true; // aggregated over all visits

    std::vector<std::unique_ptr<Node>> children;
    std::unordered_map<std::string, Node*> index; // key -> Node*
};
}

class RSpecStyleReporter : public Catch::StreamingReporterBase {
public:
    using StreamingReporterBase::StreamingReporterBase;

    static std::string getDescription() {
        return "RSpec-style nested reporter with colored output";
    }

    void testRunStarting(Catch::TestRunInfo const& testRunInfo) override {
        StreamingReporterBase::testRunStarting(testRunInfo);
    }

    void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
        // Reset state per test case
        root = std::make_unique<rspec_style_detail::Node>();
        sectionStack.clear();
        failureCountStack.clear();
        totalFailures = 0;
        testName = testInfo.name;
    }

    void assertionEnded(Catch::AssertionStats const& assertionStats) override {
        if (!assertionStats.assertionResult.isOk()) {
            ++totalFailures;
        }
    }

    void sectionStarting(Catch::SectionInfo const& sectionInfo) override {
        // Construct key for de-duplication
        std::string key = makeKey(sectionInfo);
        rspec_style_detail::Node* parent = currentParent();
        rspec_style_detail::Node* child = nullptr;

        auto it = parent->index.find(key);
        if (it == parent->index.end()) {
            auto node = std::make_unique<rspec_style_detail::Node>();
            node->name = sectionInfo.name;
            node->key = key;
            child = node.get();
            parent->index.emplace(key, child);
            parent->children.emplace_back(std::move(node));
        } else {
            child = it->second;
        }

        sectionStack.push_back(child);
        failureCountStack.push_back(totalFailures);
    }

    void sectionEnded(Catch::SectionStats const& /*sectionStats*/) override {
        if (sectionStack.empty()) return;
        auto* node = sectionStack.back();
        sectionStack.pop_back();

        // Determine if any new failures occurred while this section was active
        const std::size_t baselineFailures = failureCountStack.back();
        failureCountStack.pop_back();
        const bool sectionFailed = (totalFailures > baselineFailures);

        if (sectionFailed) node->success = false; // aggregate over visits
    }

    void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override {
        const bool testPassed = testCaseStats.totals.assertions.failed == 0;

        // Render tree
        m_stream << (testPassed ? GREEN : RED) << testName << RESET << '\n';
        for (auto const& child : root->children) {
            renderNode(*child, 1);
        }
        m_stream << '\n' << std::flush;
    }

    void testRunEnded(Catch::TestRunStats const& testRunStats) override {
        m_stream << "\n========================================\n";
        m_stream << "Total tests: " << testRunStats.totals.testCases.total() << '\n';
        m_stream << "  Passed: " << GREEN << (testRunStats.totals.testCases.total() - testRunStats.totals.testCases.failed) << RESET << '\n';
        if (testRunStats.totals.testCases.failed > 0) {
            m_stream << "  Failed: " << RED << testRunStats.totals.testCases.failed << RESET << '\n';
        }
        m_stream << "========================================\n" << std::flush;
    }

private:
    static constexpr const char* GREEN = "\x1b[32m";
    static constexpr const char* RED   = "\x1b[31m";
    static constexpr const char* RESET = "\x1b[0m";

    std::unique_ptr<rspec_style_detail::Node> root = std::make_unique<rspec_style_detail::Node>();
    std::vector<rspec_style_detail::Node*> sectionStack; // active path
    std::vector<std::size_t> failureCountStack;          // baseline failures per active section
    std::size_t totalFailures = 0;                       // monotonically increasing
    std::string testName;

    rspec_style_detail::Node* currentParent() {
        return sectionStack.empty() ? root.get() : sectionStack.back();
    }

    static std::string makeKey(Catch::SectionInfo const& info) {
        // key = name@file:line to ensure uniqueness across files
        return info.name + "@" + info.lineInfo.file + ":" + std::to_string(info.lineInfo.line);
    }

    static std::string indent(int depth) {
        return std::string(static_cast<std::size_t>(depth) * 4, ' ');
    }

    void renderNode(rspec_style_detail::Node const& node, int depth) {
        m_stream << indent(depth)
                 << (node.success ? GREEN : RED)
                 << (node.success ? "✓ " : "✗ ")
                 << node.name
                 << RESET << '\n';
        for (auto const& child : node.children) {
            renderNode(*child, depth + 1);
        }
    }
};

// Register the reporter


// CATCH_REGISTER_LISTENER(RSpecStyleListener);
CATCH_REGISTER_REPORTER("rspec", RSpecStyleReporter)
