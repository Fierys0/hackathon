#include "game/ai_service.hpp"
#include <iostream>

int main() {
    std::cout << "Loading API key...\n";
    std::string key = Game::AIService::LoadAPIKey();
    std::cout << "API Key starts with: " << (key.empty() ? "EMPTY" : key.substr(0, 8)) << "...\n";

    if (key.empty()) {
        std::cerr << "Error: API key is empty!\n";
        return 1;
    }

    std::cout << "Requesting dynamic report...\n";
    auto futureReport = Game::AIService::RequestDynamicReportAsync("Flood", 1, 0, 15.5f, 4.2f, 0.0f);
    
    std::cout << "Waiting for report response...\n";
    auto report = futureReport.get();
    
    std::cout << "Report Success: " << (report.success ? "Yes" : "No") << "\n";
    if (report.success) {
        std::cout << "Headline: " << report.headline << "\n";
        std::cout << "Body: " << report.body << "\n";
    } else {
        std::cerr << "Report request failed.\n";
    }

    std::cout << "\nRequesting chat response...\n";
    auto futureChat = Game::AIService::RequestChatResponseAsync("Hendro", "Are the dam gates secure?", "Rainfall: 15.5 mm/h, River Depth: 4.2 m");
    
    std::cout << "Waiting for chat response...\n";
    auto chat = futureChat.get();
    
    std::cout << "Chat Success: " << (chat.success ? "Yes" : "No") << "\n";
    if (chat.success) {
        std::cout << "Message: " << chat.message << "\n";
    } else {
        std::cerr << "Chat request failed.\n";
    }

    return 0;
}
