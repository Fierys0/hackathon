#pragma once

#include "game_manager.hpp"

namespace Game {

/**
 * @class Simulation
 * 
 * Core backend system responsible for progressing gameplay one time window at a time.
 * 
 * This is the initial skeleton only - disaster calculations, scoring, report generation,
 * and outcome evaluation will be implemented in future steps.
 * 
 * The simulation follows a turn-based model where each turn advances to the next
 * operational window (08:00 → 11:00 → 14:00 → 17:00).
 */
class Simulation {
public:
    Simulation();
    ~Simulation() = default;

    /**
     * Execute one turn of gameplay.
     * 
     * Primary entry point for the simulation backend. Processes selected actions,
     * validates resources, and advances time windows.
     * 
     * Responsibilities:
     * 1. Read currently selected actions from GameState
     * 2. Validate that player has enough resources to perform those actions
     * 3. Placeholder: Leave TODO comments where action processing will happen later
     * 4. Advance to the next time window using GameManager
     * 
     * @param gameManager Reference to the game manager containing current state
     * @return true if the shift continues (more windows remain)
     * @return false if the current shift has finished (end of shift reached)
     */
    bool ExecuteTurn(GameManager& gameManager);

private:
    /**
     * Validate selected actions against available resources.
     * 
     * This is a placeholder implementation that will later:
     * - Check if budget is sufficient for selected actions
     * - Check if rescue teams are available
     * - Validate action combinations
     * 
     * @param gameState Current game state containing selected actions
     * @return true if all actions are valid and resources sufficient
     */
    bool ValidateActions(const GameState& gameState);
    
    /**
     * Advance to the next operational window.
     * 
     * This is a wrapper around GameManager::NextWindow() but will later
     * contain logic for processing window transitions.
     * 
     * @param gameManager Game manager to advance
     * @return true if window advanced successfully, false if shift ended
     */
    bool AdvanceWindow(GameManager& gameManager);
    
    /**
     * Clear selected actions after processing.
     * 
     * This prepares for the next turn by resetting the action queue.
     * 
     * @param gameState Game state to modify
     */
    void ClearSelectedActions(GameState& gameState);
    
    /**
     * Process the player's selected actions and apply immediate, local effects
     * to the GameState. This does NOT calculate outcomes like casualties or
     * generate reports; it only updates resources and persistent flags.
     *
     * @param gameManager Access to the live game state and scenario data
     */
    void ProcessActions(GameManager& gameManager);
    
    /**
     * Evaluate the outcomes of the processed actions and update GameState.
     * This performs simple, deterministic rule-based updates to peopleSaved,
     * casualties, infrastructureDamage, and publicTrust based on the current
     * disaster type and persistent effects.
     *
     * @param gameManager Access to scenario, threat center, and game state
     */
    void EvaluateOutcomes(GameManager& gameManager);
    void EvaluateFlood(GameState& gs, const ThreatCenterState& threat);
    void EvaluateWildfire(GameState& gs, const ThreatCenterState& threat);
    void EvaluateEarthquake(GameState& gs, const ThreatCenterState& threat);

    /**
     * Generate comms reports for the current window based on what changed
     * during this ExecuteTurn. This is event-driven: only affected report
     * slots are updated.
     */
    void GenerateComms(GameManager& gameManager);

public:
    static int GetActionBudgetCost(ActionID id, DisasterType type);
    static int GetActionRescueTeamCost(ActionID id, DisasterType type);

private:

    // Per-turn counters populated while processing actions. These are reset
    // at the start of each ExecuteTurn and used by EvaluateOutcomes.
    int m_teamsDeployedThisTurn = 0;
    int m_airSupportSpentThisTurn = 0;
    int m_roadsClosedThisTurn = 0;

    // Snapshots captured at the start of ExecuteTurn to detect changes.
    ThreatCenterState m_prevThreat;
    CommsState m_prevComms;
    int m_prevPeopleSaved = 0;
    int m_prevCasualties = 0;
    int m_prevInfrastructureDamage = 0;
    bool m_prevSheltersOpened = false;
    bool m_prevEarlyWarningIssued = false;

    // Future systems will be integrated as private members here:
    // - ActionProcessor for calculating action outcomes
    // - DisasterSimulator for dynamic threat progression
    // - ReportGenerator for comms system updates
    // - ScoringEngine for trust/budget/casualty calculations
};

} // namespace Game