#pragma once

#include <string>
#include <future>

namespace Game {

struct AIReportResult {
    std::string headline;
    std::string body;
    bool success = false;
    bool quotaExceeded = false;
};

struct AIChatResult {
    std::string message;
    bool success = false;
    bool quotaExceeded = false;
};

class AIService {
public:
    // Load Gemini API key from config/api_key.txt
    static std::string LoadAPIKey();

    // Trigger an asynchronous request to Gemini API. Returns a future result.
    // Inputs:
    // - disasterType: "Flood", "Wildfire", or "Volcano"
    // - shiftNum: 1-7
    // - windowIndex: 0-3
    // - metric1, metric2, metric3: telemetry metrics depending on disaster type
    static std::future<AIReportResult> RequestDynamicReportAsync(
        const std::string& disasterType,
        int shiftNum,
        int windowIndex,
        float metric1,
        float metric2,
        float metric3
    );

    // Trigger an asynchronous chat response.
    // Inputs:
    // - characterName: "Hendro", "Budi", "Surono"
    // - userMessage: the message typed by the player
    // - threatStatus: telemetry metrics formatted as text
    static std::future<AIChatResult> RequestChatResponseAsync(
        const std::string& characterName,
        const std::string& userMessage,
        const std::string& threatStatus
    );

    // Trigger an asynchronous after-action report response.
    static std::future<AIReportResult> RequestAfterActionReportAsync(
        int shiftNum,
        int peopleSaved,
        int casualties,
        int budgetSpent,
        const std::string& actionsTakenList
    );
};

} // namespace Game
