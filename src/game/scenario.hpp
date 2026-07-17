#pragma once

#include <string>
#include <vector>

#include "game_state.hpp"
#include "threat_center.hpp"
#include "comms.hpp"

namespace Game {

enum class DisasterType {
  Flood,
  Wildfire,
  Earthquake
};

// Describes a single time window within a shift.
// It holds the scripted world state that the player observes and acts upon.
struct TimeWindow {
  ThreatCenterState threatCenter;
  CommsState comms;
  std::vector<ActionID> availableActions;
  std::string objective;
  std::string timeLabel;

  TimeWindow();
};

// Describes a full shift that contains several time windows.
struct ShiftScenario {
  int shiftNumber = 1;
  std::vector<TimeWindow> windows;

  ShiftScenario();
};

// Describes the overall scripted scenario for the campaign.
struct Scenario {
  std::string scenarioName;
  DisasterType disasterType = DisasterType::Flood;
  std::vector<ShiftScenario> shifts;

  Scenario();
};

} // namespace Game
