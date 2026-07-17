#include "ai_service.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#ifdef HAS_CURL
#include <curl/curl.h>
#endif

namespace Game {

std::string AIService::LoadAPIKey() {
    std::ifstream file("config/api_key.txt");
    if (!file.is_open()) {
        file.open("../config/api_key.txt");
    }
    if (!file.is_open()) {
        file.open("../../config/api_key.txt");
    }
    if (!file.is_open()) {
        file.open("build/hackathon/config/api_key.txt");
    }
    if (!file.is_open()) {
        std::cerr << "[AIService] ERROR: Could not open config/api_key.txt in any location.\n";
        return "";
    }
    std::string key;
    std::getline(file, key);
    // Trim whitespace and newlines
    key.erase(std::remove(key.begin(), key.end(), '\r'), key.end());
    key.erase(std::remove(key.begin(), key.end(), '\n'), key.end());
    key.erase(key.find_last_not_of(" \t") + 1);
    key.erase(0, key.find_first_not_of(" \t"));
    
    if (key == "YOUR_GEMINI_API_KEY_HERE" || key.empty() || key[0] == '#') {
        std::cerr << "[AIService] API key is placeholder or empty.\n";
        return "";
    }
    return key;
}

#ifdef HAS_CURL
struct CurlResponse {
    std::string data;
};

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    CurlResponse* resp = static_cast<CurlResponse*>(userp);
    resp->data.append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

static std::string ExtractTextResponse(const std::string& str);

// Simple JSON parser scanning for "headline" and "body"
static AIReportResult ParseResponse(const std::string& rawResponse) {
    AIReportResult result;
    std::string textContent = ExtractTextResponse(rawResponse);
    if (textContent.empty()) {
        result.success = false;
        return result;
    }
    
    auto extractValue = [](const std::string& str, const std::string& key) -> std::string {
        std::string pattern = "\"" + key + "\"";
        std::size_t keyPos = str.find(pattern);
        if (keyPos == std::string::npos) return "";
        
        std::size_t colonPos = str.find(":", keyPos + pattern.length());
        if (colonPos == std::string::npos) return "";
        
        std::size_t startQuote = str.find("\"", colonPos + 1);
        if (startQuote == std::string::npos) return "";
        
        std::size_t endQuote = startQuote + 1;
        while (endQuote < str.length()) {
            if (str[endQuote] == '\"' && str[endQuote - 1] != '\\') {
                break;
            }
            endQuote++;
        }
        if (endQuote >= str.length()) return "";
        
        std::string val = str.substr(startQuote + 1, endQuote - startQuote - 1);
        // Unescape escaped quotes and newlines
        std::size_t pos = 0;
        while ((pos = val.find("\\\"", pos)) != std::string::npos) {
            val.replace(pos, 2, "\"");
            pos += 1;
        }
        pos = 0;
        while ((pos = val.find("\\n", pos)) != std::string::npos) {
            val.replace(pos, 2, "\n");
            pos += 1;
        }
        return val;
    };

    result.headline = extractValue(textContent, "headline");
    result.body = extractValue(textContent, "body");
    result.success = !result.headline.empty() && !result.body.empty();
    return result;
}
#endif

std::future<AIReportResult> AIService::RequestDynamicReportAsync(
    const std::string& disasterType,
    int shiftNum,
    int windowIndex,
    float metric1,
    float metric2,
    float metric3
) {
#ifndef HAS_CURL
    // Return a dummy future with failure if curl is not linked
    return std::async(std::launch::deferred, []() {
        AIReportResult res;
        res.success = false;
        return res;
    });
#else
    std::string key = LoadAPIKey();
    if (key.empty()) {
        return std::async(std::launch::deferred, []() {
            AIReportResult res;
            res.success = false;
            return res;
        });
    }

    // Run HTTP query in background thread
    return std::async(std::launch::async, [key, disasterType, shiftNum, windowIndex, metric1, metric2, metric3]() {
        AIReportResult result;
        CURL* curl = curl_easy_init();
        if (!curl) {
            result.success = false;
            return result;
        }

        // Use X-goog-api-key header auth (matches the Google AI Studio curl format)
        std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent";
        
        // Construct system context prompt and parameters
        std::stringstream promptStream;
        promptStream << "You are an automated disaster notification system for a Windows 95 serious game. "
                     << "Write a realistic, thematic alert or report (maximum 30 words) for the citizen and media reports slot. "
                     << "Generate a corresponding short header (maximum 6 words) for the alert headline. "
                     << "Current environmental conditions:\n"
                     << "- Location: National Monitoring Center - Sector A (River), Sector B (Seismic), Sector C (Forest)\n"
                     << "- Incident Shift " << shiftNum << " / Day: " << shiftNum << "\n"
                     << "- Incident Category: " << disasterType << "\n";
        
        if (disasterType == "Flood") {
            promptStream << "- Rainfall: " << metric1 << " mm/h\n"
                         << "- River Depth: " << metric2 << " m\n"
                         << "- Dam Capacity: 80%\n";
        } else if (disasterType == "Wildfire") {
            promptStream << "- Temperature: " << metric1 << "C\n"
                         << "- Humidity: " << metric2 << "%\n"
                         << "- Wind Speed: " << metric3 << " km/h\n";
        } else {
            promptStream << "- Tremors Per Hour: " << metric1 << "\n"
                         << "- Gas Emission: " << metric2 << " ppm\n"
                         << "- Ground Deformation: " << metric3 << " mm\n";
        }

        promptStream << "Respond ONLY with a JSON object containing:\n"
                     << "{\n"
                     << "  \"headline\": \"(max 6 words)\",\n"
                     << "  \"body\": \"(max 30 words)\"\n"
                     << "}\n"
                     << "Example output:\n"
                     << "{\n"
                     << "  \"headline\": \"River Rising in Sector A\",\n"
                     << "  \"body\": \"Residents near the northern river channels report street flooding as water levels reach 4.5m.\"\n"
                     << "}\n"
                     << "Do not wrap in markdown ```json blocks. Print raw JSON.";

        std::string prompt = promptStream.str();
        
        // Escape prompt for JSON body
        std::string escapedPrompt;
        for (char c : prompt) {
            if (c == '\"') escapedPrompt += "\\\"";
            else if (c == '\\') escapedPrompt += "\\\\";
            else if (c == '\n') escapedPrompt += "\\n";
            else if (c == '\r') escapedPrompt += "\\r";
            else if (c == '\t') escapedPrompt += "\\t";
            else escapedPrompt += c;
        }

        // Build Gemini POST payload
        std::string jsonPayload = "{\"contents\":[{\"parts\":[{\"text\":\"" + escapedPrompt + "\"}]}]}";

        CurlResponse response;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string apiKeyHeader = "X-goog-api-key: " + key;
        headers = curl_slist_append(headers, apiKeyHeader.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // Timeouts: connect fast, allow up to 15s for a response
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if (res == CURLE_OK && !response.data.empty()) {
            result = ParseResponse(response.data);
            if (!result.success) {
                std::cerr << "[AIService ERROR] Failed to parse dynamic report response. Raw response:\n" << response.data << "\n";
            }
        } else {
            std::cerr << "[AIService ERROR] curl_easy_perform failed or empty response. Code: " << res << ", Error: " << curl_easy_strerror(res) << "\n";
            result.success = false;
        }
        return result;
    });
#endif
}

#ifdef HAS_CURL
static std::string ExtractTextResponse(const std::string& str) {
    std::string pattern = "\"text\"";
    std::size_t keyPos = str.find(pattern);
    if (keyPos == std::string::npos) return "";
    
    std::size_t colonPos = str.find(":", keyPos + pattern.length());
    if (colonPos == std::string::npos) return "";
    
    std::size_t startQuote = str.find("\"", colonPos + 1);
    if (startQuote == std::string::npos) return "";
    
    std::size_t endQuote = startQuote + 1;
    while (endQuote < str.length()) {
        if (str[endQuote] == '\"' && str[endQuote - 1] != '\\') {
            break;
        }
        endQuote++;
    }
    if (endQuote >= str.length()) return "";
    
    std::string val = str.substr(startQuote + 1, endQuote - startQuote - 1);
    // Unescape escaped quotes and newlines
    std::size_t pos = 0;
    while ((pos = val.find("\\\"", pos)) != std::string::npos) {
        val.replace(pos, 2, "\"");
        pos += 1;
    }
    pos = 0;
    while ((pos = val.find("\\n", pos)) != std::string::npos) {
        val.replace(pos, 2, "\n");
        pos += 1;
    }
    return val;
}
#endif

std::future<AIChatResult> AIService::RequestChatResponseAsync(
    const std::string& characterName,
    const std::string& userMessage,
    const std::string& threatStatus
) {
#ifndef HAS_CURL
    return std::async(std::launch::deferred, []() {
        AIChatResult res;
        res.success = false;
        return res;
    });
#else
    std::string key = LoadAPIKey();
    if (key.empty()) {
        return std::async(std::launch::deferred, []() {
            AIChatResult res;
            res.success = false;
            return res;
        });
    }

    return std::async(std::launch::async, [key, characterName, userMessage, threatStatus]() {
        AIChatResult result;
        CURL* curl = curl_easy_init();
        if (!curl) {
            result.success = false;
            return result;
        }

        // Use X-goog-api-key header auth (matches the Google AI Studio curl format)
        std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent";

        std::stringstream promptStream;
        promptStream << "You are roleplaying as " << characterName << " inside a serious disaster management game. "
                     << "Here is your background character description:\n";
        
        if (characterName == "Hendro") {
            promptStream << "- Name: Operator Hendro (Dam Warden & Hydrology Supervisor)\n"
                         << "- Tone: Professional, slightly anxious, worried about structural integrity of the retention gates.\n";
        } else if (characterName == "Budi") {
            promptStream << "- Name: Captain Budi (Urban Wildfire and Rescue Commander)\n"
                         << "- Tone: Gritty, practical, emergency first-responder style. Stressed about fire spread and winds.\n";
        } else {
            promptStream << "- Name: Dr. Surono (Chief Volcanologist / Geoscientist)\n"
                         << "- Tone: Calm, analytical, focused on ground sensors, tremor frequencies, and gas measurements.\n";
        }

        promptStream << "\nActive disaster telemetry context:\n" << threatStatus << "\n"
                     << "Respond to this message from the NDMC monitoring officer:\n"
                     << "\"" << userMessage << "\"\n\n"
                     << "Rules:\n"
                     << "1. Stay in character at all times.\n"
                     << "2. Give a short response (maximum 35 words) suitable for a VHF radio transmission.\n"
                     << "3. Mention relevant details from the active telemetry if appropriate.\n"
                     << "4. Do not prefix with your name. Just speak your transmission text.";

        std::string prompt = promptStream.str();

        std::string escapedPrompt;
        for (char c : prompt) {
            if (c == '\"') escapedPrompt += "\\\"";
            else if (c == '\\') escapedPrompt += "\\\\";
            else if (c == '\n') escapedPrompt += "\\n";
            else if (c == '\r') escapedPrompt += "\\r";
            else if (c == '\t') escapedPrompt += "\\t";
            else escapedPrompt += c;
        }

        std::string jsonPayload = "{\"contents\":[{\"parts\":[{\"text\":\"" + escapedPrompt + "\"}]}]}";

        CurlResponse response;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string apiKeyHeader = "X-goog-api-key: " + key;
        headers = curl_slist_append(headers, apiKeyHeader.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // Timeouts: connect fast, allow up to 15s for a response
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if (res == CURLE_OK && !response.data.empty()) {
            std::string textResponse = ExtractTextResponse(response.data);
            if (!textResponse.empty()) {
                result.message = textResponse;
                result.success = true;
            } else {
                std::cerr << "[AIService ERROR] Failed to extract text from chat response. Raw response:\n" << response.data << "\n";
                result.success = false;
            }
        } else {
            std::cerr << "[AIService ERROR] curl_easy_perform failed or empty response in chat. Code: " << res << ", Error: " << curl_easy_strerror(res) << "\n";
            result.success = false;
        }
        return result;
    });
#endif
}

} // namespace Game
