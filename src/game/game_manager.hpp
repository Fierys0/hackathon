#pragma once

#include <cstddef>
#include <vector>

#include "game_state.hpp"
#include "scenario.hpp"

namespace Game {

class GameManager {
public:
  GameManager();

  void ResetGame();

  GameState &GetGameState();
  Scenario &GetScenario();

  int GetCurrentShift() const;
  int GetCurrentWindow() const;
  const ThreatCenterState &GetCurrentThreatCenter() const;
  const CommsState &GetCurrentComms() const;
  CommsState &GetCurrentComms();
  const std::vector<ActionID> &GetAvailableActions() const;

  bool NextWindow();

private:
  GameState m_gameState;
  Scenario m_scenario;
};

} // namespace Game
