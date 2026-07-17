#pragma once

#include <cstddef>
#include <vector>

#include "game_state.hpp"
#include "scenario.hpp"

#include <future>
#include "ai_service.hpp"

namespace Game {

class GameManager {
public:
  GameManager();

  void ResetGame();

  void Update();
  void TriggerAIUpdate();

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
  bool m_aiReportPending = false;
  std::future<AIReportResult> m_aiReportFuture;
};

} // namespace Game
