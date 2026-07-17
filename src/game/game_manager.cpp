#include "game_manager.hpp"
#include <chrono>
#include <iostream>

namespace Game {

GameManager::GameManager() {
  ResetGame();
  TriggerAIUpdate();
}

void GameManager::ResetGame() {
  m_gameState = GameState();
  m_scenario = Scenario();
  m_scenario.shifts.clear();

  auto setReport = [](Report &r, ReportType type, const std::string &title,
                      const std::string &preview, const std::string &full,
                      ReportPriority pri, const std::string &timestamp) {
    r.type = type;
    r.title = title;
    r.preview = preview;
    r.fullReport = full;
    r.priority = pri;
    r.timestamp = timestamp;
    r.unread = true;
  };

  // ==================== SHIFT 1: FLOOD ====================
  {
    ShiftScenario shift;
    shift.shiftNumber = 1;

    // Window 0 (08:00)
    {
      TimeWindow w;
      w.timeLabel = "08:00";
      w.objective = "Monitor river level and prepare flood response staging.";
      w.threatCenter.flood.riverDepth = 3.5f;
      w.threatCenter.flood.rainfall = 20.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Monsoon Season Begins", "City prepares for monsoons.", "Meteorologists forecast the arrival of seasonal monsoons. City authorities have placed flood response teams on standby.", ReportPriority::Info, "08:00");
      setReport(w.comms.weather, ReportType::Weather, "Light Rain Upstream", "Monsoon clouds converging.", "Light rain of 20 mm/h is reported in the highlands. River levels are expected to remain within safe thresholds for now.", ReportPriority::Info, "08:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Residents on Alert", "Local community watches river.", "Community heads in low-lying areas advise residents to monitor local streams. No flooding reported yet.", ReportPriority::Info, "08:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-A Sensor Stable", "River level: 3.5m.", "Automated telemetry at SEC-A indicates river depth is stable at 3.5 meters. Outflow rates are normal.", ReportPriority::Info, "08:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Rescue Teams Standby", "Teams prepared at base.", "Regional search and rescue teams have completed gear inspections and are on standby for any emergency dispatches.", ReportPriority::Info, "08:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Dam Capacity Stable", "Capacity at 60%.", "The main flood retention dam reports a capacity level of 60%. Gates are closed and operating within specifications.", ReportPriority::Info, "08:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::OpenShelter, ActionID::CloseRoad, ActionID::EvacuateResidents };
      shift.windows.push_back(w);
    }

    // Window 1 (11:00)
    {
      TimeWindow w;
      w.timeLabel = "11:00";
      w.objective = "Deploy precautionary assets as rainfall increases.";
      w.threatCenter.flood.riverDepth = 4.2f;
      w.threatCenter.flood.rainfall = 55.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Rain Intensifies", "Severe downpours hit Sector A.", "Torrential downpours are causing traffic gridlock in northern sectors. Municipal drainage systems are working at full capacity.", ReportPriority::Medium, "11:00");
      setReport(w.comms.weather, ReportType::Weather, "Heavy Rain Advisory", "Rainfall at 55 mm/h.", "BMKG has escalated the warning level to Orange. Rainfall has increased to 55 mm/h. Upstream runoff is rising.", ReportPriority::Medium, "11:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Ankle-deep Water", "Water rising in streets.", "Residents in Sector A report local street pooling. 'Water is ankle-deep and starting to pool near the river banks.'", ReportPriority::Medium, "11:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-A Sensor Rising", "River level: 4.2m.", "Sensor SEC-A reports river level has risen to 4.2 meters. Inflow rate is increasing.", ReportPriority::Medium, "11:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Equipment Staged", "Staging near river banks.", "Rescue units are staging inflatable boats and sandbags at vulnerable points along the northern river banks.", ReportPriority::Info, "11:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Dam Inflow Rising", "Capacity at 75%.", "Dam control reports rapid reservoir filling. Capacity is currently at 75%. Inflow remains high.", ReportPriority::Info, "11:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::OpenShelter, ActionID::CloseRoad, ActionID::EvacuateResidents };
      shift.windows.push_back(w);
    }

    // Window 2 (14:00)
    {
      TimeWindow w;
      w.timeLabel = "14:00";
      w.objective = "Perform emergency mitigations. Local sensors are failing.";
      w.threatCenter.flood.riverDepth = 4.9f;
      w.threatCenter.flood.rainfall = 85.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Minor Flooding Reported", "Low-lying areas inundated.", "Local media reports river overflows in Sector A. Several ground-floor apartments have reported minor water ingress.", ReportPriority::High, "14:00");
      setReport(w.comms.weather, ReportType::Weather, "Extreme Rain Alert", "Rainfall at 85 mm/h upstream.", "BMKG has issued a Red alert. Rainfall peaking at 85 mm/h upstream. Heavy runoff expected to peak within hours.", ReportPriority::Critical, "14:00");
      setReport(w.comms.citizen, ReportType::Citizen, "River Overflows Gardens", "Water reaching doorsteps.", "Citizen report: 'The river is overflowing into our gardens! The local sensor seems partially submerged!'", ReportPriority::High, "14:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-A SENSOR ERROR", "TELEMETRY OFFLINE.", "ALERT: Communication with Sensor SEC-A has been lost. Last recorded level was 4.9 meters. Visual checks required.", ReportPriority::High, "14:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Active Assistance", "Assisting trapped residents.", "Field teams are assisting elderly residents and moving vehicles to higher ground. Multiple calls for assistance incoming.", ReportPriority::Medium, "14:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Dam Critical Limit", "Capacity at 92%. Inflow peaks.", "Dam operator warning: Inflow is approaching critical limits. Dam capacity is at 92%. Requesting emergency permission to open floodgates.", ReportPriority::High, "14:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::OpenShelter, ActionID::CloseRoad, ActionID::EvacuateResidents };
      shift.windows.push_back(w);
    }

    // Window 3 (17:00)
    {
      TimeWindow w;
      w.timeLabel = "17:00";
      w.objective = "Evacuate high-risk zones and coordinate active search & rescue.";
      w.threatCenter.flood.riverDepth = 5.8f;
      w.threatCenter.flood.rainfall = 110.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Major Flood in Sector A", "Evacuations underway.", "Sector A is experiencing severe flooding. Evacuation routes are active. Emergency response is fully engaged.", ReportPriority::Critical, "17:00");
      setReport(w.comms.weather, ReportType::Weather, "Rainfall Peaking", "Storm front slowly passing.", "Extreme rain continues at 110 mm/h. Runoff is peaking. Heavy flooding expected to persist overnight.", ReportPriority::Medium, "17:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Evacuating to Roofs", "Seeking high ground.", "Urgent citizen post: 'Water is waist-deep! Evacuating to the second floor and roof. We need immediate evacuation support!'", ReportPriority::Critical, "17:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-A SENSOR OFFLINE", "NO DATA AVAILABLE.", "SEC-A telemetry remains offline. Emergency manual readings estimate river level at 5.8 meters.", ReportPriority::High, "17:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Evacuation Dispatches", "Evacuations active.", "Rescue boats are actively evacuating stranded residents in Sector A. All available personnel have been deployed.", ReportPriority::High, "17:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Dam Reservoir Full", "Capacity at 98%. Peak load.", "Reservoir capacity is at 98%. Water is spilling over the emergency weir. Structural integrity is monitored closely.", ReportPriority::Critical, "17:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::OpenShelter, ActionID::CloseRoad, ActionID::EvacuateResidents };
      shift.windows.push_back(w);
    }

    m_scenario.shifts.push_back(shift);
  }

  // ==================== SHIFT 2: WILDFIRE ====================
  {
    ShiftScenario shift;
    shift.shiftNumber = 2;

    // Window 0 (08:00)
    {
      TimeWindow w;
      w.timeLabel = "08:00";
      w.objective = "Monitor temperatures and forestry sensors for potential combustion.";
      w.threatCenter.wildfire.temperature = 34.0f;
      w.threatCenter.wildfire.humidity = 22.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Dry Spell Warning", "Dry season begins.", "Authorities warn of increased fire risks as the region enters a prolonged dry spell with low precipitation forecasts.", ReportPriority::Info, "08:00");
      setReport(w.comms.weather, ReportType::Weather, "High Temperature Warning", "High temp, dry air.", "BMKG warns of temperatures rising to 34C in Sector C forest area with humidity dropping to 22%.", ReportPriority::Info, "08:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Hazy Conditions", "Dust and dry air.", "Hikers report dry conditions on trails. Some agricultural smoke visible in distant hills.", ReportPriority::Info, "08:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-C Smoke Clear", "Air index: normal.", "Smoke detector SEC-C in Sector C forest reports normal particulate counts. Air quality index remains green.", ReportPriority::Info, "08:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Forest Patrol Active", "Patrols staging.", "Forestry ranger units have initiated dry-season patrols along vulnerable sector borders.", ReportPriority::Info, "08:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Grid Load Normal", "Grid operating normally.", "Electrical transmission lines crossing Sector C forest report normal operating temperatures and loads.", ReportPriority::Info, "08:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::RequestAirSupport, ActionID::EvacuateResidents, ActionID::CloseRoad };
      shift.windows.push_back(w);
    }

    // Window 1 (11:00)
    {
      TimeWindow w;
      w.timeLabel = "11:00";
      w.objective = "Preemptively close high-risk areas as dry winds accelerate.";
      w.threatCenter.wildfire.temperature = 38.0f;
      w.threatCenter.wildfire.humidity = 14.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Brush Fire Containment", "Small fire in Sector C.", "Fire units are responding to a small brush fire in the Sector C forest area. Containment is underway.", ReportPriority::Info, "11:00");
      setReport(w.comms.weather, ReportType::Weather, "Dry Winds Rising", "Wind gusts to 25 km/h.", "BMKG reports dry winds are picking up, peaking at 25 km/h, which may accelerate fire spread in dry vegetation.", ReportPriority::Medium, "11:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Smoke Smell on Trail", "Smoke seen near trails.", "Hiker report: 'Smelled burning wood near the hiking trail. Saw some small flames in the dry brush.'", ReportPriority::Medium, "11:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-C Smoke Detected", "Particulates rising.", "Sensor SEC-C reports elevated particulate counts. Smoke concentration is rising near the border zone.", ReportPriority::Medium, "11:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Fire Crews Dispatched", "Responding to fire.", "Two fire engines have been dispatched to containment lines in the Sector C forest area.", ReportPriority::Info, "11:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Traffic Slowing", "Smoke on forest road.", "Sector C local roads report speed reductions due to drifting smoke from the nearby forest.", ReportPriority::Info, "11:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::RequestAirSupport, ActionID::EvacuateResidents, ActionID::CloseRoad };
      shift.windows.push_back(w);
    }

    // Window 2 (14:00)
    {
      TimeWindow w;
      w.timeLabel = "14:00";
      w.objective = "Request air suppression support and protect residential borders.";
      w.threatCenter.wildfire.temperature = 41.0f;
      w.threatCenter.wildfire.humidity = 9.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Wildfire Expands Rapidly", "Sector C fire out of control.", "Winds have pushed the fire front across the containment line. Sector C villages are now under direct threat.", ReportPriority::High, "14:00");
      setReport(w.comms.weather, ReportType::Weather, "Gale Force Dry Winds", "Winds peaking at 45 km/h.", "BMKG has issued an extreme wind warning. Gale force dry winds are feeding the fire front, driving it south.", ReportPriority::High, "14:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Ash Falling on Roofs", "Thick smoke in village.", "Village elder reports: 'Ash is falling on our roofs! Smoke is very thick, visibility is under 50 meters!'", ReportPriority::High, "14:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-C Smoke Critical", "Smoke index critical.", "Sensor SEC-C reports critical smoke density. Ambient temperature at the station has reached 52C.", ReportPriority::High, "14:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Evacuation Request", "Need evacuation support.", "Fire commanders request urgent evacuation of Sector C villages as the fire front approaches residential lines.", ReportPriority::Critical, "14:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Grid Lines Shut Down", "Transmission lines offline.", "Power grid operator has shut down high-voltage lines in Sector C forest due to extreme heat and fire risk.", ReportPriority::High, "14:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::RequestAirSupport, ActionID::EvacuateResidents, ActionID::CloseRoad };
      shift.windows.push_back(w);
    }

    // Window 3 (17:00)
    {
      TimeWindow w;
      w.timeLabel = "17:00";
      w.objective = "Conduct defensive firefights and evacuate remaining residents.";
      w.threatCenter.wildfire.temperature = 42.0f;
      w.threatCenter.wildfire.humidity = 8.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Villages Engulfed by Fire", "Major emergency in Sector C.", "Sector C villages are experiencing active structure fires. Emergency evacuation and containment are in progress.", ReportPriority::Critical, "17:00");
      setReport(w.comms.weather, ReportType::Weather, "Fire Storm Conditions", "Extreme fire weather.", "BMKG warns of active fire storm conditions. Localized fire winds are making containment extremely hazardous.", ReportPriority::Critical, "17:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Trapped by Smoke", "Stranded near village.", "Urgent citizen message: 'Main exit road is blocked by fire! We are trapped near the community center!'", ReportPriority::Critical, "17:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-C SENSOR ERROR", "STATION MELTED.", "ALERT: Sensor SEC-C has stopped transmitting data. Station is presumed destroyed by the fire front.", ReportPriority::High, "17:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Defensive Operations", "Defending village borders.", "Crews are performing defensive operations to save homes while search and rescue units extract trapped residents.", ReportPriority::High, "17:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Forest Highway Closed", "Zero visibility on highway.", "Highway 4 crossing Sector C forest has been closed in both directions due to zero visibility and active flames.", ReportPriority::Critical, "17:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::RequestAirSupport, ActionID::EvacuateResidents, ActionID::CloseRoad };
      shift.windows.push_back(w);
    }

    m_scenario.shifts.push_back(shift);
  }

  // ==================== SHIFT 3: VOLCANO ====================
  {
    ShiftScenario shift;
    shift.shiftNumber = 3;

    // Window 0 (08:00)
    {
      TimeWindow w;
      w.timeLabel = "08:00";
      w.objective = "Monitor volcanic tremors and outgassing thresholds.";
      w.threatCenter.earthquake.tremorsPerHour = 8;
      w.threatCenter.earthquake.gasEmission = 120.0f;
      w.threatCenter.earthquake.groundDeformation = 6.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Volcanic Activity Monitored", "Mount Fumbo active.", "Geophysicists are monitoring Mount Fumbo in Sector B after a small increase in seismic activity and venting.", ReportPriority::Info, "08:00");
      setReport(w.comms.weather, ReportType::Weather, "Ash Dispersion Model", "Winds blow west.", "BMKG ash dispersion models predict that if an eruption occurs, ash plumes will disperse west towards the airport.", ReportPriority::Info, "08:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Rumbling Sounds", "Morning rumblings heard.", "Sector B residents report: 'We felt some small vibrations this morning and heard distant rumbles from the mountain.'", ReportPriority::Info, "08:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-B Seismic Active", "Tremors: 8/h, Gas: 120ppm.", "Seismograph SEC-B reports background tremor rate at 8 per hour. CO2 and SO2 outgassing is normal at 120ppm.", ReportPriority::Info, "08:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Medical Readiness", "Disaster prep reviewed.", "Medical coordination centers are reviewing stockpiles of respiratory masks and emergency shelter supplies.", ReportPriority::Info, "08:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Airport Open", "Airport operating normally.", "Sector B regional airport reports normal flight operations. No ash interference present.", ReportPriority::Info, "08:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::EvacuateResidents, ActionID::CloseRoad, ActionID::OpenShelter, ActionID::DeployRescue };
      shift.windows.push_back(w);
    }

    // Window 1 (11:00)
    {
      TimeWindow w;
      w.timeLabel = "11:00";
      w.objective = "Alert the aviation sector and stage medical rescue teams.";
      w.threatCenter.earthquake.tremorsPerHour = 15;
      w.threatCenter.earthquake.gasEmission = 280.0f;
      w.threatCenter.earthquake.groundDeformation = 22.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Seismic Activity Rises", "Micro-tremors rising.", "Seismic activity at Mount Fumbo has escalated. Geologists have raised the volcano alert level to yellow.", ReportPriority::Info, "11:00");
      setReport(w.comms.weather, ReportType::Weather, "Sulfur Plumes Noted", "Plumes rising to 1,000m.", "BMKG notes steam and gas plumes rising to 1,000 meters above the crater, drifting southwest.", ReportPriority::Medium, "11:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Sulfur Odor Reported", "Sulfur smell near mountain.", "Tourists near hiking trails report a strong sulfur smell and heating of local thermal springs.", ReportPriority::Medium, "11:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-B Tremors Rising", "Tremors: 15/h, Gas: 280ppm.", "Sensor SEC-B reports tremors have increased to 15/h and gas emission has reached 280ppm. Tiltmeters show 22mm inflation.", ReportPriority::Medium, "11:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Staging Area Active", "Evac vehicles staged.", "Rescue vehicles and emergency medical staff are establishing staging areas at the border of the danger zone.", ReportPriority::Info, "11:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Airport Flight Advisory", "Ash risk advisory.", "Airport authorities have issued flight advisories warning pilots of localized steam and gas plumes.", ReportPriority::Medium, "11:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::EvacuateResidents, ActionID::CloseRoad, ActionID::OpenShelter, ActionID::DeployRescue };
      shift.windows.push_back(w);
    }

    // Window 2 (14:00)
    {
      TimeWindow w;
      w.timeLabel = "14:00";
      w.objective = "Establish shelters and prepare for immediate evacuation of the danger radius.";
      w.threatCenter.earthquake.tremorsPerHour = 38;
      w.threatCenter.earthquake.gasEmission = 420.0f;
      w.threatCenter.earthquake.groundDeformation = 55.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Eruption Imminent Alert", "Alert raised to Level 3.", "Volcanology center warns of high probability of eruption. Magma movement is close to the surface.", ReportPriority::High, "14:00");
      setReport(w.comms.weather, ReportType::Weather, "Toxic Gas Plume", "Gas levels exceed safety limit.", "BMKG warns of toxic sulfur dioxide gas concentrations moving downwind. Mask usage is strongly recommended.", ReportPriority::High, "14:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Windows Shaking", "Strong tremors felt.", "Resident report: 'Frequent earthquakes are shaking our houses and rattling windows. We can see a dark ash column starting to rise!'", ReportPriority::High, "14:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-B Telemetry Critical", "Tremors: 38/h, Gas: 420ppm.", "Sensor SEC-B reports tremors at 38/h, gas at 420ppm, and ground deformation at 55mm. Infrasound sensors detect venting.", ReportPriority::High, "14:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Shelter Operations Active", "Shelters opening.", "Emergency services are opening public shelters in Sector B safe zones. Evacuation coordination is active.", ReportPriority::Medium, "14:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Airport Advisory", "Airport considering closure.", "Airport operator reports volcanic ash particles detected on runways. Flight cancellations are likely.", ReportPriority::High, "14:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::EvacuateResidents, ActionID::CloseRoad, ActionID::OpenShelter, ActionID::DeployRescue };
      shift.windows.push_back(w);
    }

    // Window 3 (17:00)
    {
      TimeWindow w;
      w.timeLabel = "17:00";
      w.objective = "Protect airport operations and execute emergency zone evacuation.";
      w.threatCenter.earthquake.tremorsPerHour = 72;
      w.threatCenter.earthquake.gasEmission = 850.0f;
      w.threatCenter.earthquake.groundDeformation = 140.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Mount Fumbo Erupts!", "Eruption column to 12k feet.", "Mount Fumbo has erupted, sending a massive ash column to 12,000 feet. Pyroclastic flows are moving down the northern slope.", ReportPriority::Critical, "17:00");
      setReport(w.comms.weather, ReportType::Weather, "Extreme Ash Fall", "Heavy ash fall in Sector B.", "BMKG reports heavy ash fall downwind. Visibility is near-zero and air quality is hazardous.", ReportPriority::Critical, "17:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Fleeing the Ash", "Ash falling like snow.", "Resident: 'It is pitch black outside and ash is falling like snow! We are fleeing in our cars towards the south!'", ReportPriority::Critical, "17:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-B SENSOR ERROR", "STATION MELTED.", "ALERT: Seismograph SEC-B has stopped transmitting. Heavy ash fall and heat have destroyed the station.", ReportPriority::High, "17:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Rescue Teams Active", "Active search and rescue.", "Medical and rescue teams are extracting citizens trapped by ash fall. Operations are severely restricted by visibility.", ReportPriority::High, "17:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Airport CLOSED", "All flights grounded.", "Sector B Airport is CLOSED indefinitely due to volcanic ash accumulation and safety risks.", ReportPriority::Critical, "17:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::EvacuateResidents, ActionID::CloseRoad, ActionID::OpenShelter, ActionID::DeployRescue };
      shift.windows.push_back(w);
    }

    m_scenario.shifts.push_back(shift);
  }

  // ==================== SHIFT 4: FLASH FLOOD SURGE ====================
  {
    ShiftScenario shift;
    shift.shiftNumber = 4;

    // Window 0 (08:00)
    {
      TimeWindow w;
      w.timeLabel = "08:00";
      w.objective = "Pre-stage sandbags and monitor highlands drainage flow.";
      w.threatCenter.flood.riverDepth = 3.8f;
      w.threatCenter.flood.rainfall = 30.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Highlands Drainage Watch", "Staging in Sector A.", "Highlands report sudden cloudbursts. Municipal streams are rising slowly.", ReportPriority::Info, "08:00");
      setReport(w.comms.weather, ReportType::Weather, "Cloudburst warning", "Highlands rain peaking.", "BMKG reports severe thunderstorms in the highlands upstream. Inflow expected to rise.", ReportPriority::Info, "08:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Streams Running Fast", "Water muddy and swift.", "Ranchers report local streams running fast and muddy. Trash starting to accumulate at bridges.", ReportPriority::Info, "08:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-A Telemetry Active", "River level: 3.8m.", "Telemetry station SEC-A indicates depth is 3.8 meters. Outflow rates normal.", ReportPriority::Info, "08:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Inflatables Inspected", "Pre-staging assets.", "Emergency responders are pre-staging sandbags and dispatching flood response trucks.", ReportPriority::Info, "08:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Gate Operations Ready", "Testing gate winches.", "The flood retention dam reports 65% capacity. Operator confirms winches are ready.", ReportPriority::Info, "08:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::OpenShelter, ActionID::CloseRoad, ActionID::EvacuateResidents };
      shift.windows.push_back(w);
    }

    // Window 1 (11:00)
    {
      TimeWindow w;
      w.timeLabel = "11:00";
      w.objective = "Close vulnerable bridges and coordinate staging assets.";
      w.threatCenter.flood.riverDepth = 4.5f;
      w.threatCenter.flood.rainfall = 65.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Street Flooding", "Water pooling in Sector A.", "Localized flash floods hit urban streets. Secondary roads are blocked by debris.", ReportPriority::Medium, "11:00");
      setReport(w.comms.weather, ReportType::Weather, "Rain Intensity Rises", "Rainfall at 65 mm/h.", "Downpours are peaking. Highlands streams are overflowing their channels.", ReportPriority::Medium, "11:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Bridge Inundated", "Water touching girders.", "Citizen report: 'Water is starting to touch the girders of the lower bridge!'", ReportPriority::Medium, "11:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-A Rising Rapidly", "River level: 4.5m.", "Station SEC-A telemetry indicates river depth is rising rapidly at 4.5 meters.", ReportPriority::Medium, "11:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Barricades Deployed", "Closing lower roads.", "Rescue teams have deployed warning barricades near low-lying river bank paths.", ReportPriority::Info, "11:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Dam Silt Levels Rise", "Reservoir filling.", "Reservoir capacity has reached 80%. High silt content noted in inflows.", ReportPriority::Info, "11:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::OpenShelter, ActionID::CloseRoad, ActionID::EvacuateResidents };
      shift.windows.push_back(w);
    }

    // Window 2 (14:00)
    {
      TimeWindow w;
      w.timeLabel = "14:00";
      w.objective = "Issue alerts and evacuate danger zone as sensor failure occurs.";
      w.threatCenter.flood.riverDepth = 5.2f;
      w.threatCenter.flood.rainfall = 90.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "River Breach Imminent", "Low roads submerged.", "The main river channel is breaching bank defenses. Several roads are closed.", ReportPriority::High, "14:00");
      setReport(w.comms.weather, ReportType::Weather, "Storm Warning Orange", "Highlands peak runoff.", "Highlands runoff is peaking. Massive surge wave expected to hit Sector A.", ReportPriority::High, "14:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Sensor Swept Away", "Sensor structure collapsed.", "Citizen post: 'The monitoring station structure has collapsed into the torrent!'", ReportPriority::High, "14:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-A OFFLINE", "NO SIGNAL RECEIVED.", "ALERT: Sensor SEC-A is offline. Telemetry stream is lost. Last depth: 5.2m.", ReportPriority::High, "14:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Active Evacuations", "Moving families out.", "Responders are evacuating ground-floor apartments and driving people to high ground.", ReportPriority::Critical, "14:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Dam Overflow Risk", "Capacity at 94%.", "Dam capacity at 94%. Operators prepare emergency weir discharges.", ReportPriority::High, "14:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::OpenShelter, ActionID::CloseRoad, ActionID::EvacuateResidents };
      shift.windows.push_back(w);
    }

    // Window 3 (17:00)
    {
      TimeWindow w;
      w.timeLabel = "17:00";
      w.objective = "Execute absolute zone evacuation and secure critical infrastructure.";
      w.threatCenter.flood.riverDepth = 6.2f;
      w.threatCenter.flood.rainfall = 125.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Tidal Surge Wave Hits", "Sector A inundated.", "A major surge wave has hit Sector A. Multiple districts are under water.", ReportPriority::Critical, "17:00");
      setReport(w.comms.weather, ReportType::Weather, "Rain Slowing Down", "Storm head moving east.", "Storm front passing, rain slowly subsides but floodwater remains very high.", ReportPriority::Medium, "17:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Water at Upper Floor", "Trapped in building.", "Urgent: 'Water is waist-deep on our second floor. Trapped here, need boat extraction!'", ReportPriority::Critical, "17:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-A SENSOR DEAD", "STATION DESTROYED.", "SEC-A remains offline. Emergency crews report the telemetry station was swept away.", ReportPriority::High, "17:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Full Mobilization", "Extracting citizens.", "All available search and rescue boats are deployed in Sector A for extractions.", ReportPriority::High, "17:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Dam Outflow Peak", "Spilling over spillway.", "Dam is discharging water via the emergency spillway. Inflow slowly decreasing.", ReportPriority::Critical, "17:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::OpenShelter, ActionID::CloseRoad, ActionID::EvacuateResidents };
      shift.windows.push_back(w);
    }

    m_scenario.shifts.push_back(shift);
  }

  // ==================== SHIFT 5: HIGHLANDS FOREST FIRE ====================
  {
    ShiftScenario shift;
    shift.shiftNumber = 5;

    // Window 0 (08:00)
    {
      TimeWindow w;
      w.timeLabel = "08:00";
      w.objective = "Monitor highland forest sectors for combustion risks.";
      w.threatCenter.wildfire.temperature = 35.0f;
      w.threatCenter.wildfire.humidity = 20.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Dry Gale Warning", "High wind warning.", "Dry winds from the east are expected to create extreme fire weather today.", ReportPriority::Info, "08:00");
      setReport(w.comms.weather, ReportType::Weather, "Heat Dome Intensifies", "Temps rising to 35C.", "BMKG reports dry heat dome hovering over the highlands forest sector.", ReportPriority::Info, "08:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Dry Grass Odor", "Grass extremely dry.", "Hikers report undergrowth is completely dried up and cracking underfoot.", ReportPriority::Info, "08:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-C Sensor Online", "Particulates stable.", "Telemetry station SEC-C reports normal particulate metrics.", ReportPriority::Info, "08:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Rangers Staging", "Positioned at firebreaks.", "Highland rangers are staging wildfire trucks at key access paths.", ReportPriority::Info, "08:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Power lines Monitored", "Thermal check normal.", "Electrical grid operator performs routine thermal inspections on forest lines.", ReportPriority::Info, "08:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::RequestAirSupport, ActionID::EvacuateResidents, ActionID::CloseRoad };
      shift.windows.push_back(w);
    }

    // Window 1 (11:00)
    {
      TimeWindow w;
      w.timeLabel = "11:00";
      w.objective = "Deploy containment trucks and close forest recreation zones.";
      w.threatCenter.wildfire.temperature = 39.0f;
      w.threatCenter.wildfire.humidity = 12.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Small Blaze Spreads", "Fire near trail border.", "A lightning strike has ignited a dry pine patch. Containment forces are en route.", ReportPriority::Info, "11:00");
      setReport(w.comms.weather, ReportType::Weather, "Dry Winds Rising", "Winds at 30 km/h.", "BMKG warns winds are rising to 30 km/h, blowing direct south towards villages.", ReportPriority::Medium, "11:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Spotted Flames", "Flames in tree canopy.", "Citizen report: 'Spotted active crown fire in the canopy near Sector C trail!'", ReportPriority::Medium, "11:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-C Smoke Rises", "Particulates elevated.", "Sensor SEC-C reports smoke density is rising. Air index: Orange.", ReportPriority::Medium, "11:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Ground Crews Active", "Clearing fuel loads.", "Ranger crews are clearing brush lines to prevent fire jumping the main trail.", ReportPriority::Info, "11:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Highway Smoke Alert", "Slowing on route 4.", "Route 4 reports visibility down to 200m due to crosswinds and smoke.", ReportPriority::Info, "11:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::RequestAirSupport, ActionID::EvacuateResidents, ActionID::CloseRoad };
      shift.windows.push_back(w);
    }

    // Window 2 (14:00)
    {
      TimeWindow w;
      w.timeLabel = "14:00";
      w.objective = "Execute evacuation alerts and request immediate water bombers.";
      w.threatCenter.wildfire.temperature = 42.0f;
      w.threatCenter.wildfire.humidity = 7.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Out of Control Fire", "Sector C villages threatened.", "Dry gale winds have spread the fire front across the dry valley. Fire is out of control.", ReportPriority::High, "14:00");
      setReport(w.comms.weather, ReportType::Weather, "Extreme Dry Winds", "Winds peaking at 50 km/h.", "Extreme wind warning. Dry gale winds are feeding the forest fire.", ReportPriority::High, "14:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Village Surrounded", "Smoke blocking sun.", "Village elder report: 'Sun is completely blocked. Red sky. Ash falling everywhere!'", ReportPriority::High, "14:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-C Sensor Melting", "Temp: 58C, smoke critical.", "Sensor SEC-C reports heat at 58C and critical particulate density.", ReportPriority::High, "14:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Evac Order Issued", "Evacuating village center.", "Fire commanders request absolute evacuations of Sector C villages.", ReportPriority::Critical, "14:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Grid Offline", "Main lines shut down.", "Electrical grid operator has shut down all forest lines to prevent arcing.", ReportPriority::High, "14:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::RequestAirSupport, ActionID::EvacuateResidents, ActionID::CloseRoad };
      shift.windows.push_back(w);
    }

    // Window 3 (17:00)
    {
      TimeWindow w;
      w.timeLabel = "17:00";
      w.objective = "Perform emergency structural defenses and extract stranded rangers.";
      w.threatCenter.wildfire.temperature = 44.0f;
      w.threatCenter.wildfire.humidity = 5.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Villages Overrun by Fire", "Active fires in homes.", "Forest fire has overrun Sector C village borders. Ground units are in retreat.", ReportPriority::Critical, "17:00");
      setReport(w.comms.weather, ReportType::Weather, "Fire Storm Active", "Tornado warning.", "Firestorm conditions generate local wind vortices. Suppression operations grounded.", ReportPriority::Critical, "17:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Trapped on Valley Road", "Highway exit blocked.", "Citizen report: 'Valley Road is blocked by burning pines. We are trapped in our SUV!'", ReportPriority::Critical, "17:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-C SENSOR ERROR", "STATION OFFLINE.", "ALERT: Sensor SEC-C has stopped transmitting data. Presumed destroyed by fire.", ReportPriority::High, "17:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Emergency Search", "Extrication missions.", "Rescue teams perform high-risk extraction missions to save trapped motorists.", ReportPriority::High, "17:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Highway closed", "Forest roads blocked.", "All roads through Sector C forest are closed indefinitely due to ash and fire.", ReportPriority::Critical, "17:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::DeployRescue, ActionID::RequestAirSupport, ActionID::EvacuateResidents, ActionID::CloseRoad };
      shift.windows.push_back(w);
    }

    m_scenario.shifts.push_back(shift);
  }

  // ==================== SHIFT 6: VOLCANIC TREMOR CRISIS ====================
  {
    ShiftScenario shift;
    shift.shiftNumber = 6;

    // Window 0 (08:00)
    {
      TimeWindow w;
      w.timeLabel = "08:00";
      w.objective = "Monitor volcanic tremors and outgassing thresholds.";
      w.threatCenter.earthquake.tremorsPerHour = 10;
      w.threatCenter.earthquake.gasEmission = 150.0f;
      w.threatCenter.earthquake.groundDeformation = 8.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Mount Fumbo Alert Raised", "Alert raised to level 2.", "Geologists raise alert level for Mount Fumbo following seismicity spikes.", ReportPriority::Info, "08:00");
      setReport(w.comms.weather, ReportType::Weather, "Ash Plume Winds", "Winds blowing west.", "BMKG predicts volcanic ash will move west towards airport if eruption occurs.", ReportPriority::Info, "08:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Vibrations Felt", "Felt weak tremors.", "Sector B residents report minor shaking. Steam venting visible on the peak.", ReportPriority::Info, "08:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-B Seismic Active", "Tremors: 10/h, Gas: 150ppm.", "Seismograph SEC-B reports tremor rate at 10 per hour. Outgassing is slightly elevated.", ReportPriority::Info, "08:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Disaster Staging", "Staging medical gear.", "Medical teams are checking stockpiles of emergency respirators and shelters.", ReportPriority::Info, "08:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Airport Flight Normal", "No ash on runways.", "Sector B Airport is operating normally. Ash levels remain negligible.", ReportPriority::Info, "08:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::EvacuateResidents, ActionID::CloseRoad, ActionID::OpenShelter, ActionID::DeployRescue };
      shift.windows.push_back(w);
    }

    // Window 1 (11:00)
    {
      TimeWindow w;
      w.timeLabel = "11:00";
      w.objective = "Issue warning to the aviation sector and activate shelters.";
      w.threatCenter.earthquake.tremorsPerHour = 20;
      w.threatCenter.earthquake.gasEmission = 320.0f;
      w.threatCenter.earthquake.groundDeformation = 28.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Tremors Intensify", "Alert level yellow.", "Seismic activity at Mount Fumbo has escalated. Micro-tremors are constant.", ReportPriority::Info, "11:00");
      setReport(w.comms.weather, ReportType::Weather, "Gas Venting Increases", "SO2 plume observed.", "BMKG reports sulfur dioxide venting has increased. Steam column is rising.", ReportPriority::Medium, "11:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Strong Sulfur Odor", "Strong smell in valley.", "Tourists report strong rotten egg smell in surrounding hiking areas.", ReportPriority::Medium, "11:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-B Inflation Noted", "Deformation at 28mm.", "Sensor SEC-B tiltmeter shows inflation of 28mm. Tremors have risen to 20/h.", ReportPriority::Medium, "11:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Vehicles Staged", "Evacuation trucks ready.", "Rescue vehicles and staff are establishing staging areas at sector borders.", ReportPriority::Info, "11:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Airport Ash Advisory", "Warning pilots.", "Airport authorities advise pilots of steam and gas plumes near Mount Fumbo.", ReportPriority::Medium, "11:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::EvacuateResidents, ActionID::CloseRoad, ActionID::OpenShelter, ActionID::DeployRescue };
      shift.windows.push_back(w);
    }

    // Window 2 (14:00)
    {
      TimeWindow w;
      w.timeLabel = "14:00";
      w.objective = "Perform emergency road closures and prepare danger zone evacuation.";
      w.threatCenter.earthquake.tremorsPerHour = 45;
      w.threatCenter.earthquake.gasEmission = 500.0f;
      w.threatCenter.earthquake.groundDeformation = 68.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Alert Level Raised to 3", "Eruption imminent.", "Volcanology center warns of high probability of eruption as magma ascends.", ReportPriority::High, "14:00");
      setReport(w.comms.weather, ReportType::Weather, "Plume Reaches 3,000m", "Ash drifting southwest.", "BMKG warns ash plume is reaching 3,000m. Hazardous gas concentrations moving downwind.", ReportPriority::High, "14:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Crater Glow Visible", "Rumbling shaking.", "Resident report: 'Volcanic earthquakes are rattling windows. Glow visible at night!'", ReportPriority::High, "14:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-B Telemetry Alert", "Tremors: 45/h, Gas: 500ppm.", "Sensor SEC-B tiltmeters indicate rapid swelling at 68mm. Infrasound venting is constant.", ReportPriority::High, "14:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Shelter Evacuations", "Moving evacuees to camps.", "Emergency services are coordinating evacuations from high-risk zones.", ReportPriority::Medium, "14:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Runway Ash Detected", "Aviation warning.", "Ash particles detected on runways. Flight cancellations are expected.", ReportPriority::High, "14:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::EvacuateResidents, ActionID::CloseRoad, ActionID::OpenShelter, ActionID::DeployRescue };
      shift.windows.push_back(w);
    }

    // Window 3 (17:00)
    {
      TimeWindow w;
      w.timeLabel = "17:00";
      w.objective = "Protect airport operations and execute emergency zone evacuation.";
      w.threatCenter.earthquake.tremorsPerHour = 85;
      w.threatCenter.earthquake.gasEmission = 950.0f;
      w.threatCenter.earthquake.groundDeformation = 170.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Eruptive Phase Begins!", "Crater dome collapses.", "Mount Fumbo dome has collapsed, sending pyroclastic flows down slopes.", ReportPriority::Critical, "17:00");
      setReport(w.comms.weather, ReportType::Weather, "Extreme Ash Column", "Ash column to 15,000 feet.", "BMKG warns ash plume has reached 15,000 feet. Heavy ash fall in Sector B.", ReportPriority::Critical, "17:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Pitch Black Sky", "Shaking and ash fall.", "Resident: 'It is pitch black outside. Ash is falling like snow! Fleeing south.'", ReportPriority::Critical, "17:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-B SENSOR ERROR", "STATION OFFLINE.", "ALERT: Seismograph SEC-B has stopped transmitting. Station presumed destroyed by heat.", ReportPriority::High, "17:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Rescue Dispatches Active", "Moving teams to safety.", "Rescue teams are evacuating trapped residents. Operations limited by visibility.", ReportPriority::High, "17:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Airport CLOSED", "All flights grounded.", "Sector B Airport is CLOSED indefinitely due to heavy ash accumulation.", ReportPriority::Critical, "17:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::EvacuateResidents, ActionID::CloseRoad, ActionID::OpenShelter, ActionID::DeployRescue };
      shift.windows.push_back(w);
    }

    m_scenario.shifts.push_back(shift);
  }

  // ==================== SHIFT 7: MEGA-HAZARD CASCADING COLLAPSE ====================
  {
    ShiftScenario shift;
    shift.shiftNumber = 7;

    // Window 0 (08:00)
    {
      TimeWindow w;
      w.timeLabel = "08:00";
      w.objective = "Assess multiple indicators as seismic and meteorological anomalies intersect.";
      w.threatCenter.earthquake.tremorsPerHour = 12;
      w.threatCenter.earthquake.gasEmission = 180.0f;
      w.threatCenter.earthquake.groundDeformation = 10.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Multi-Disaster Alert", "Anomalies in Sector B and C.", "NDMC issues warning as volcanic inflation coincides with high forest temperatures.", ReportPriority::Info, "08:00");
      setReport(w.comms.weather, ReportType::Weather, "Dry Storm Advisory", "Thunderstorms and heat.", "Dry lightning storms are forecast in forest areas while volcano tremors rise.", ReportPriority::Info, "08:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Heavy Tremors Felt", "Tremors shaking buildings.", "Residents report frequent shaking. Vent steam columns rising high.", ReportPriority::Info, "08:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-B Gas Inflation Rising", "Tremors: 12/h, Gas: 180ppm.", "Sensor SEC-B reports background tremors have risen to 12 per hour.", ReportPriority::Info, "08:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Joint Command Active", "Establishing emergency center.", "NDMC establishes joint command to coordinate forestry, volcano, and medical units.", ReportPriority::Info, "08:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Aviation Sector Normal", "Airport operating normally.", "Flight paths are open, but under strict meteorological observation.", ReportPriority::Info, "08:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::EvacuateResidents, ActionID::CloseRoad, ActionID::OpenShelter, ActionID::DeployRescue };
      shift.windows.push_back(w);
    }

    // Window 1 (11:00)
    {
      TimeWindow w;
      w.timeLabel = "11:00";
      w.objective = "Preemptively activate emergency shelters and airport closures.";
      w.threatCenter.earthquake.tremorsPerHour = 25;
      w.threatCenter.earthquake.gasEmission = 380.0f;
      w.threatCenter.earthquake.groundDeformation = 35.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Volcano Swelling", "Inflation is accelerating.", "Deformation at Mount Fumbo is rising. Volcanologists raise alert level to yellow.", ReportPriority::Info, "11:00");
      setReport(w.comms.weather, ReportType::Weather, "Ash Plume Forecast", "Winds blowing southwest.", "BMKG reports steam plumes rising to 1,500m, moving towards the regional runway.", ReportPriority::Medium, "11:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Ash Layer Noted", "Light ash fall.", "Hikers report light ash layers on forest leaves. Sulfur smell is very strong.", ReportPriority::Medium, "11:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-B Tremors: 25/h", "Deformation at 35mm.", "Sensor SEC-B tiltmeter shows inflation of 35mm. Tremors have risen to 25/h.", ReportPriority::Medium, "11:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Staging Stretched", "Staging multiple centers.", "Rescue units stage vehicles, but report personnel limitations due to split staging.", ReportPriority::Info, "11:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Airport flight alert", "Ash threat rising.", "Airport operator issues critical ash warnings. Cancellations expected shortly.", ReportPriority::Medium, "11:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::EvacuateResidents, ActionID::CloseRoad, ActionID::OpenShelter, ActionID::DeployRescue };
      shift.windows.push_back(w);
    }

    // Window 2 (14:00)
    {
      TimeWindow w;
      w.timeLabel = "14:00";
      w.objective = "Execute absolute danger zone evacuations and close transit highways.";
      w.threatCenter.earthquake.tremorsPerHour = 60;
      w.threatCenter.earthquake.gasEmission = 600.0f;
      w.threatCenter.earthquake.groundDeformation = 90.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Volcanic Swarm Tremors", "Alert raised to Level 3.", "Magma ascent is creating continuous swarm tremors. Eruption is imminent.", ReportPriority::High, "14:00");
      setReport(w.comms.weather, ReportType::Weather, "SO2 Venting Peaks", "SO2 exceeds safety limits.", "BMKG reports SO2 concentrations exceed safe breathing guidelines near Mount Fumbo.", ReportPriority::High, "14:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Continuous Shaking", "Frequent landslides.", "Resident report: 'Continuous shaking. Small landslides are blocking local valley roads.'", ReportPriority::High, "14:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-B Telemetry Warning", "Tremors: 60/h, Deformation: 90mm.", "Sensor SEC-B tiltmeter indicates rapid swelling at 90mm. Infrasound is constant.", ReportPriority::High, "14:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Evacuation Mobilization", "Evacuating danger zone.", "Emergency services order evacuation of the entire volcano danger radius.", ReportPriority::Critical, "14:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Runway Closed", "Ash on runway.", "Airport halts all flight departures due to runway ash accumulation.", ReportPriority::High, "14:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::EvacuateResidents, ActionID::CloseRoad, ActionID::OpenShelter, ActionID::DeployRescue };
      shift.windows.push_back(w);
    }

    // Window 3 (17:00)
    {
      TimeWindow w;
      w.timeLabel = "17:00";
      w.objective = "Coordinate active search and rescue under near-zero visibility conditions.";
      w.threatCenter.earthquake.tremorsPerHour = 99;
      w.threatCenter.earthquake.gasEmission = 1200.0f;
      w.threatCenter.earthquake.groundDeformation = 250.0f;
      setReport(w.comms.breakingNews, ReportType::BreakingNews, "Cataclysmic Eruption!", "Pyroclastic flows active.", "Mount Fumbo has experienced a major cataclysmic eruption. Ash plume has reached 25,000 feet.", ReportPriority::Critical, "17:00");
      setReport(w.comms.weather, ReportType::Weather, "Hazardous Ash Fall", "Near-zero visibility.", "BMKG warns of severe ash fall downwind. Visibility is near-zero. Breathing is hazardous.", ReportPriority::Critical, "17:00");
      setReport(w.comms.citizen, ReportType::Citizen, "Trapped by Ash and Heat", "Seeking shelter.", "Urgent: 'Our vehicle is trapped in deep ash! We are sheltering inside a concrete tunnel!'", ReportPriority::Critical, "17:00");
      setReport(w.comms.sensor, ReportType::Sensor, "SEC-B SENSOR ERROR", "STATION OBLITERATED.", "ALERT: Seismograph SEC-B has stopped transmitting. Station was obliterated by pyroclastic surge.", ReportPriority::High, "17:00");
      setReport(w.comms.rescue, ReportType::Rescue, "Emergency Search Active", "Active search & rescue.", "All search and rescue forces are deployed to extract trapped citizens near slopes.", ReportPriority::High, "17:00");
      setReport(w.comms.infrastructure, ReportType::Infrastructure, "Airport CLOSED", "Indefinite airport closure.", "Sector B Airport is CLOSED indefinitely. Structural damage to hangars reported.", ReportPriority::Critical, "17:00");
      w.availableActions = { ActionID::IssueWarning, ActionID::EvacuateResidents, ActionID::CloseRoad, ActionID::OpenShelter, ActionID::DeployRescue };
      shift.windows.push_back(w);
    }

    m_scenario.shifts.push_back(shift);
  }
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
    TriggerAIUpdate();
    return true;
  }

  return false;
}

void GameManager::TriggerAIUpdate() {
  // Determine active disaster type based on current shift
  std::string disasterType = "Flood";
  int shift = m_gameState.currentShift;
  if (shift == 1 || shift == 4) {
    disasterType = "Flood";
  } else if (shift == 2 || shift == 5) {
    disasterType = "Wildfire";
  } else {
    disasterType = "Volcano";
  }

  const auto &threat = GetCurrentThreatCenter();
  float metric1 = 0.0f;
  float metric2 = 0.0f;
  float metric3 = 0.0f;

  if (disasterType == "Flood") {
    metric1 = threat.flood.rainfall;
    metric2 = threat.flood.riverDepth;
  } else if (disasterType == "Wildfire") {
    metric1 = threat.wildfire.temperature;
    metric2 = threat.wildfire.humidity;
  } else {
    metric1 = static_cast<float>(threat.earthquake.tremorsPerHour);
    metric2 = threat.earthquake.gasEmission;
    metric3 = threat.earthquake.groundDeformation;
  }

  m_aiReportFuture = AIService::RequestDynamicReportAsync(
      disasterType, shift, m_gameState.currentWindow, metric1, metric2, metric3
  );
  m_aiReportPending = true;
}

void GameManager::Update() {
  if (!m_aiReportPending) {
    return;
  }

  // Non-blocking wait: check status
  auto status = m_aiReportFuture.wait_for(std::chrono::seconds(0));
  if (status == std::future_status::ready) {
    m_aiReportPending = false;
    AIReportResult result = m_aiReportFuture.get();
    if (result.success) {
      // Update both breaking news and citizen rumors dynamically with the AI output
      CommsState &comms = GetCurrentComms();
      
      comms.breakingNews.title = result.headline;
      comms.breakingNews.preview = result.headline;
      comms.breakingNews.fullReport = result.body;
      comms.breakingNews.unread = true;

      comms.citizen.title = "Local report: " + result.headline;
      comms.citizen.preview = "Citizen update: " + result.headline;
      comms.citizen.fullReport = "Social media and citizens are discussing: " + result.body;
      comms.citizen.unread = true;
      
      // Also update weather updates to sound like dynamic advisory
      comms.weather.title = "Meteorological Advisory - " + result.headline;
      comms.weather.preview = "Atmospheric conditions alert.";
      comms.weather.fullReport = "Scientific sensors and satellite forecasts support the following update: " + result.body;
      comms.weather.unread = true;

      std::cout << "[AIService] Successfully integrated dynamic reports from Gemini API.\n";
    } else {
      std::cout << "[AIService] Gemini API update was not successful (or key was not set); fell back to offline reports.\n";
    }
  }
}

} // namespace Game
