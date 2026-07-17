#include "game_manager.hpp"

namespace Game {

GameManager::GameManager() { ResetGame(); }

void GameManager::ResetGame() {
  m_gameState = GameState();
  m_scenario = Scenario();
}

GameState &GameManager::GetGameState() { return m_gameState; }

Scenario &GameManager::GetScenario() { return m_scenario; }

int GameManager::GetCurrentShift() const {
  return m_gameState.currentShift;
}

int GameManager::GetCurrentWindow() const {
  return m_gameState.currentWindow;
}

const ThreatCenterState &GameManager::GetCurrentThreatCenter() const {
  const auto &shift = m_scenario.shifts.at(static_cast<std::size_t>(m_gameState.currentShift - 1));
  const auto &window = shift.windows.at(static_cast<std::size_t>(m_gameState.currentWindow));
  return window.threatCenter;
}

const CommsState &GameManager::GetCurrentComms() const {
  const auto &shift = m_scenario.shifts.at(static_cast<std::size_t>(m_gameState.currentShift - 1));
  const auto &window = shift.windows.at(static_cast<std::size_t>(m_gameState.currentWindow));
  return window.comms;
}

CommsState &GameManager::GetCurrentComms() {
  auto &shift = m_scenario.shifts.at(static_cast<std::size_t>(m_gameState.currentShift - 1));
  auto &window = shift.windows.at(static_cast<std::size_t>(m_gameState.currentWindow));
  return window.comms;
}

const std::vector<ActionID> &GameManager::GetAvailableActions() const {
  const auto &shift = m_scenario.shifts.at(static_cast<std::size_t>(m_gameState.currentShift - 1));
  const auto &window = shift.windows.at(static_cast<std::size_t>(m_gameState.currentWindow));
  return window.availableActions;
}

bool GameManager::NextWindow() {
  const auto &shift = m_scenario.shifts.at(static_cast<std::size_t>(m_gameState.currentShift - 1));
  const int nextWindow = m_gameState.currentWindow + 1;

  if (nextWindow < static_cast<int>(shift.windows.size())) {
    m_gameState.currentWindow = nextWindow;
    return true;
  }

  return false;
}

} // namespace Game
