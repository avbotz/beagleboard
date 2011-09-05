/*
 *  state.cpp
 *  AVI
 *
 *  Created by Daniel Naito on 7/4/10.
 *  Copyright 2010 Tchips.com. All rights reserved.
 *
 */

#include <boost/thread/mutex.hpp>
#include <sys/time.h>

#include "state.h"

struct state::boostImpl {
	boost::mutex m_mutex;
//	boost::thread m_thread; //state needs no thread, just mutex to be thread-safe
};

const int state::statePropRanges[STATE_PROP_NUM][2] = 
	{	{0, 359},					//current heading
		{0, 100},					//current depth		
		{0, 200},					//current power
		{0, 359},					//desired heading
		{0, 100},					//desired depth
		{0, 200},					//desired power
		{kill_dead, kill_alive},	//kill state
		{cm_search, cm_done},		//camera mode
		{red_buoy, yellow_buoy},	//buoy colors
		{m_start, m_done}			//mission state
	};

state::state() :
	des_updated(0),
	cur_updated(0),
	bimpl(new boostImpl)
{
	stateProps[cur_heading]		= 0;
	stateProps[cur_depth]		= 0;
	stateProps[cur_power]		= 100;
	stateProps[des_heading]		= 0;
	stateProps[des_depth]		= 0;
	stateProps[des_power]		= 100;
	stateProps[kill_state]		= kill_dead;
	stateProps[cam_mode]		= cm_search;
	stateProps[buoy_color]		= red_buoy;
	stateProps[mission_state]	= m_start;
	
	struct timeval tv;
	gettimeofday(&tv, NULL);
	timestamp = tv.tv_sec*1000 + (tv.tv_usec/1000);
	
}
state::~state() {
	
}

int state::getTimeStamp() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long ts = tv.tv_sec*1000 + (tv.tv_usec/1000);
	return ts-timestamp;
}

int state::isDesUpdated() {
	boost::mutex::scoped_lock l(bimpl->m_mutex);
	return des_updated;
}

int state::isCurUpdated() {
	boost::mutex::scoped_lock l(bimpl->m_mutex);
	return cur_updated;
}

int state::getProperty(state_property prop) {
	if (prop < 0 || prop >= STATE_PROP_NUM)
		return -1;
	
	boost::mutex::scoped_lock l(bimpl->m_mutex);
	return stateProps[prop];
}

int state::setProperty(state_property prop, int newValue) {
	if (prop < 0 || prop >= STATE_PROP_NUM)
		return -1;
	if (newValue < statePropRanges[prop][MIN] || newValue > statePropRanges[prop][MAX])
		return -1;
	if (stateProps[prop] == newValue)
		return 0;
	
	boost::mutex::scoped_lock l(bimpl->m_mutex);
	switch (prop) {
		case cur_heading:
		case cur_depth:
		case cur_power:
		case kill_state:
			cur_updated = 1;
			break;
		case des_heading:
		case des_depth:
		case des_power:
			des_updated = 1;
			break;
		default:
			break;
	}
	stateProps[prop] = newValue;
	return 0;
}

