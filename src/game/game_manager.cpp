#include "game_manager.hpp"

namespace Game {

GameManager::GameManager() { ResetGame(); }

void GameManager::ResetGame() {
  m_gameState = GameState();
  // Initialize a default scenario with one shift and four time windows so
  // UI code can safely read ThreatCenterState values without crashing.
  m_scenario = Scenario();
  m_scenario.shifts.clear();
  ShiftScenario shift;
  shift.shiftNumber = 1;
  // Create four windows (08:00,11:00,14:00,17:00)
  for (int i = 0; i < 4; ++i) {
    TimeWindow w;
    // set time labels to match GameState window times
    if (i == 0) w.timeLabel = "08:00";
    else if (i == 1) w.timeLabel = "11:00";
    else if (i == 2) w.timeLabel = "14:00";
    else w.timeLabel = "17:00";
    shift.windows.push_back(w);
  }
  m_scenario.shifts.push_back(shift);
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
