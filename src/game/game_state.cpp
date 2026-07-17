#include "game_state.hpp"

namespace Game {

GameState::GameState()
    : currentShift(1),
      maxShift(4),
      currentWindow(0),
      budget(50000),
      rescueTeams(3),
      publicTrust(85),
      peopleSaved(0),
      casualties(0),
      infrastructureDamage(0),
      earlyWarningIssued(false),
      sheltersOpened(false) {}

// Initialize the shiftStartingBudget to the configured starting budget
// so UI can compute budget spent per shift without hardcoding values.
// decisionLog remains empty until shifts complete.
// Note: Keep constructor small; ResetForNewShift will set starting budget.


const char *GameState::GetWindowDisplayTime(int windowIndex) {
  static constexpr const char *kWindowTimes[] = {"08:00", "11:00", "14:00", "17:00"};

  if (windowIndex < 0 || windowIndex >= 4) {
    return "--:--";
  }

  return kWindowTimes[windowIndex];
}

void GameState::ResetForNewShift() {
  currentWindow = 0;
  budget = 50000;
  rescueTeams = 3;
  publicTrust = 85;
  peopleSaved = 0;
  casualties = 0;
  infrastructureDamage = 0;
  earlyWarningIssued = false;
  sheltersOpened = false;
  selectedActions.clear();

  // Record the budget at the start of the shift so we can compute spent.
  shiftStartingBudget = budget;
}

} // namespace Game
