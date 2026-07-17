#pragma once

#include <vector>
#include <string>

namespace Game {

// Action identifiers for the gameplay decision layer.
// These are intentionally lightweight and data-focused.
enum class ActionID {
  IssueWarning,
  DeployRescue,
  OpenShelter,
  CloseRoad,
  RequestAirSupport,
  EvacuateResidents
};

using ActionList = std::vector<ActionID>;

class GameState {
public:
  GameState();

  // Campaign progress.
  // Tracks how far the player has advanced through the overall campaign.
  int currentShift;
  int maxShift;

  // Time.
  // Stores the currently active operational window.
  // 0 -> 08:00, 1 -> 11:00, 2 -> 14:00, 3 -> 17:00
  int currentWindow;

  static const char *GetWindowDisplayTime(int windowIndex);

  // Resources.
  // These values change as the player manages the crisis.
  int budget;
  int rescueTeams;
  int publicTrust;

  // Shift statistics.
  // These track outcomes for the current shift only.
  int peopleSaved;
  int casualties;
  int infrastructureDamage;

  // Persistent effects.
  // Add more flags here as the gameplay grows.
  bool earlyWarningIssued;
  bool sheltersOpened;

  // Current window selection.
  // Stores the actions chosen for the active window.
  ActionList selectedActions;

  // Shift summary entries recorded when a shift completes.
  struct ShiftSummary {
    int shiftNumber = 0;
    float overallRating = 0.0f; // 0.0 - 5.0
    int peopleSaved = 0;
    int casualties = 0;
    int budgetSpent = 0;
    int publicTrust = 0;
    std::vector<std::string> actionsTaken; // optional
  };

  std::vector<ShiftSummary> decisionLog;

  // Budget snapshot at the start of the shift (used to compute spent)
  int shiftStartingBudget;

  // Resets shift-scoped values while keeping campaign progression intact.
  void ResetForNewShift();
};

} // namespace Game
