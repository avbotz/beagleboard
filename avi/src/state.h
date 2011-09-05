#ifndef __state_h
#define __state_h

#include <libconfig.h++>

#define STATE_PROP_NUM 10

class state {
public:

	//Constructor/Destructor
	state();
	~state();

	//Track whether the desired or current state of the sub has been updated
	volatile int des_updated;
	volatile int cur_updated;

	//The properties that should be tracked by the avi_state
	//These properties should be variables that are required by more than one object in different threads
	//MUST DO WHEN ADDING TO state_property:
	//	1. Change the STATE_PROP_NUM define directive at the top of state.h
	//	2. Add the new state property to the state_property enum BEFORE mission_state
	//	3. Establish a range (or appropriate enum) of values that the property can take
	//		add this range to the statePropRanges array in state.cpp
	//	4. Initialize the state property in the initialization list in state's constructor in state.cpp
	enum state_property {
		cur_heading,
		cur_depth,
		cur_power,
		des_heading,
		des_depth,
		des_power,
		kill_state,
		cam_mode,
		buoy_color,
		mission_state
	};

	//Kill switch states
	//To avoid ambiguity of "Killing the sub on"
	enum kill_vals {
		kill_dead,
		kill_alive
	};

	//All the possible mission states
	//Add new states between m_start and m_done
	enum m_state {
		m_start,
		m_gate,
		m_buoy_find,
		m_buoy_hit,
		m_buoy_next,
		m_path_find,
		m_path_center,
		m_path_follow,
		m_hedge_find,
		m_hedge_hit,
		m_hedge_next,
		m_done
	};

	//Camera modes, used to track the vision processors status
	enum cam_modes {
		cm_search,		//Camera does not have control of sub, will not change desired state
		cm_track,		//Vision processor grabs control of sub, will change desired state
		cm_done			//Vision processor is done with vision task
	};

	//Various buoy colors
	//Used to tell the vision_buoy processor what color to search for 
	enum buoy_colors {
		red_buoy,
		green_buoy,
		yellow_buoy
	};
	
	int getTimeStamp();
	
	int isDesUpdated();
	int isCurUpdated();
	int getProperty(state_property prop);
	int setProperty(state_property prop, int newValue);
	
private:
	int stateProps[STATE_PROP_NUM];
	static const int statePropRanges[STATE_PROP_NUM][2];
	
	enum {MIN, MAX};
	
	long timestamp;
	
	struct boostImpl;
	boostImpl* bimpl;
	
};

#endif
