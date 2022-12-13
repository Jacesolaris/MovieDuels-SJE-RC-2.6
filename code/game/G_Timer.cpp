/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include <memory>
#include "g_local.h"
#include "../Rufl/hstring.h"
#include "qcommon/ojk_saved_game_helper.h"

constexpr auto MAX_GTIMERS = 16384;

using gtimer_t = struct gtimer_s
{
	hstring id; // Use handle strings, so that things work after loading
	int time;
	gtimer_s* next; // In either free list or current list
};

gtimer_t g_timerPool[MAX_GTIMERS];
gtimer_t* g_timers[MAX_GENTITIES];
gtimer_t* g_timerFreeList;

static int TIMER_GetCount(const int num)
{
	const gtimer_t* p = g_timers[num];
	int count = 0;

	while (p)
	{
		count++;
		p = p->next;
	}

	return count;
}

/*
-------------------------
TIMER_RemoveHelper

Scans an entities timer list to remove a given
timer from the list and put it on the free list

Doesn't do much error checking, only called below
-------------------------
*/
static void TIMER_RemoveHelper(const int num, gtimer_t* timer)
{
	gtimer_t* p = g_timers[num];

	// Special case: first timer in list
	if (p == timer)
	{
		g_timers[num] = g_timers[num]->next;
		p->next = g_timerFreeList;
		g_timerFreeList = p;
		return;
	}

	// Find the predecessor
	while (p->next != timer)
	{
		p = p->next;
	}

	// Rewire
	p->next = p->next->next;
	timer->next = g_timerFreeList;
	g_timerFreeList = timer;
}

/*
-------------------------
TIMER_Clear
-------------------------
*/

void TIMER_Clear(void)
{
	int i;
	for (i = 0; i < MAX_GENTITIES; i++)
	{
		g_timers[i] = nullptr;
	}

	for (i = 0; i < MAX_GTIMERS - 1; i++)
	{
		g_timerPool[i].next = &g_timerPool[i + 1];
	}
	g_timerPool[MAX_GTIMERS - 1].next = nullptr;
	g_timerFreeList = &g_timerPool[0];
}

/*
-------------------------
TIMER_Clear
-------------------------
*/

void TIMER_Clear(const int idx)
{
	// rudimentary safety checks, might be other things to check?
	if (idx >= 0 && idx < MAX_GENTITIES)
	{
		gtimer_t* p = g_timers[idx];

		// No timers at all -> do nothing
		if (!p)
		{
			return;
		}

		// Find the end of this ents timer list
		while (p->next)
		{
			p = p->next;
		}

		// Splice the lists
		p->next = g_timerFreeList;
		g_timerFreeList = g_timers[idx];
		g_timers[idx] = nullptr;
	}
}

/*
-------------------------
TIMER_Save
-------------------------
*/

void TIMER_Save(void)
{
	int j;
	gentity_t* ent;

	ojk::SavedGameHelper saved_game(
		gi.saved_game);

	for (j = 0, ent = &g_entities[0]; j < MAX_GENTITIES; j++, ent++)
	{
		unsigned char numTimers = TIMER_GetCount(j);

		if (!ent->inuse && numTimers)
		{
			//			Com_Printf( "WARNING: ent with timers not inuse\n" );
			assert(numTimers);
			TIMER_Clear(j);
			numTimers = 0;
		}

		//Write out the timer information
		saved_game.write_chunk<uint8_t>(
			INT_ID('T', 'I', 'M', 'E'),
			numTimers);

		const gtimer_t* p = g_timers[j];
		assert(numTimers && p || !numTimers && !p);

		while (p)
		{
			const char* timerID = p->id.c_str();
			const int length = strlen(timerID) + 1;
			const int time = p->time - level.time; //convert this back to delta so we can use SET after loading

			assert(length < 1024); //This will cause problems when loading the timer if longer

			//Write out the id string
			saved_game.write_chunk(
				INT_ID('T', 'M', 'I', 'D'),
				timerID,
				length);

			//Write out the timer data
			saved_game.write_chunk<int32_t>(
				INT_ID('T', 'D', 'T', 'A'),
				time);

			p = p->next;
		}
	}
}

/*
-------------------------
TIMER_Load
-------------------------
*/

void TIMER_Load(void)
{
	int j;
	gentity_t* ent;

	ojk::SavedGameHelper saved_game(
		gi.saved_game);

	for (j = 0, ent = &g_entities[0]; j < MAX_GENTITIES; j++, ent++)
	{
		unsigned char numTimers = 0;

		saved_game.read_chunk<uint8_t>(
			INT_ID('T', 'I', 'M', 'E'),
			numTimers);

		//Read back all entries
		for (int i = 0; i < numTimers; i++)
		{
			int time = 0;
			char tempBuffer[1024]; // Still ugly. Setting ourselves up for 007 AUF all over again. =)

			assert(sizeof g_timers[0]->time == sizeof time); //make sure we're reading the same size as we wrote

			//Read the id string and time
			saved_game.read_chunk(
				INT_ID('T', 'M', 'I', 'D'));

			const auto sg_buffer_data = static_cast<const char*>(
				saved_game.get_buffer_data());

			const int sg_buffer_size = saved_game.get_buffer_size();

			if (sg_buffer_size < 0 || static_cast<size_t>(sg_buffer_size) >= sizeof tempBuffer)
			{
				G_Error("invalid length for TMID string in saved game: %d\n", sg_buffer_size);
			}

			std::uninitialized_copy_n(
				sg_buffer_data,
				sg_buffer_size,
				tempBuffer);

			tempBuffer[sg_buffer_size] = '\0';

			saved_game.read_chunk<int32_t>(
				INT_ID('T', 'D', 'T', 'A'),
				time);

			//this is odd, we saved all the timers in the autosave, but not all the ents are spawned yet from an auto load, so skip it
			if (ent->inuse)
			{
				//Restore it
				TIMER_Set(ent, tempBuffer, time);
			}
		}
	}
}

static gtimer_t* TIMER_GetNew(const int num, const char* identifier)
{
	assert(num < ENTITYNUM_MAX_NORMAL); //don't want timers on NONE or the WORLD
	gtimer_t* p = g_timers[num];

	// Search for an existing timer with this name
	while (p)
	{
		if (p->id == identifier)
		{
			// Found it
			return p;
		}

		p = p->next;
	}

	// No existing timer with this name was found, so grab one from the free list
	if (!g_timerFreeList)
	{
		//oh no, none free!
		assert(g_timerFreeList);
		return nullptr;
	}

	p = g_timerFreeList;
	g_timerFreeList = g_timerFreeList->next;
	p->next = g_timers[num];
	g_timers[num] = p;
	return p;
}

gtimer_t* TIMER_GetExisting(const int num, const char* identifier)
{
	gtimer_t* p = g_timers[num];

	while (p)
	{
		if (p->id == identifier)
		{
			// Found it
			return p;
		}

		p = p->next;
	}

	return nullptr;
}

/*
-------------------------
TIMER_Set
-------------------------
*/

void TIMER_Set(const gentity_t* ent, const char* identifier, const int duration)
{
	assert(ent->inuse);
	gtimer_t* timer = TIMER_GetNew(ent->s.number, identifier);

	if (timer)
	{
		timer->id = identifier;
		timer->time = level.time + duration;
	}
}

/*
-------------------------
TIMER_Get
-------------------------
*/

int TIMER_Get(const gentity_t* ent, const char* identifier)
{
	const gtimer_t* timer = TIMER_GetExisting(ent->s.number, identifier);

	if (!timer)
	{
		return -1;
	}

	return timer->time;
}

/*
-------------------------
TIMER_Done
-------------------------
*/

qboolean TIMER_Done(const gentity_t* ent, const char* identifier)
{
	const gtimer_t* timer = TIMER_GetExisting(ent->s.number, identifier);

	if (!timer)
	{
		return qtrue;
	}

	return static_cast<qboolean>(timer->time < level.time);
}

/*
-------------------------
TIMER_Done2

Returns false if timer has been
started but is not done...or if
timer was never started
-------------------------
*/

qboolean TIMER_Done2(const gentity_t* ent, const char* identifier, const qboolean remove)
{
	gtimer_t* timer = TIMER_GetExisting(ent->s.number, identifier);

	if (!timer)
	{
		return qfalse;
	}

	const auto res = static_cast<qboolean>(timer->time < level.time);

	if (res && remove)
	{
		// Put it back on the free list
		TIMER_RemoveHelper(ent->s.number, timer);
	}

	return res;
}

/*
-------------------------
TIMER_Exists
-------------------------
*/
qboolean TIMER_Exists(const gentity_t* ent, const char* identifier)
{
	return static_cast<qboolean>(TIMER_GetExisting(ent->s.number, identifier) != nullptr);
}

/*
-------------------------
TIMER_Remove
Utility to get rid of any timer
-------------------------
*/
void TIMER_Remove(const gentity_t* ent, const char* identifier)
{
	gtimer_t* timer = TIMER_GetExisting(ent->s.number, identifier);

	if (!timer)
	{
		return;
	}

	// Put it back on the free list
	TIMER_RemoveHelper(ent->s.number, timer);
}

/*
-------------------------
TIMER_Start
-------------------------
*/

qboolean TIMER_Start(const gentity_t* self, const char* identifier, const int duration)
{
	if (TIMER_Done(self, identifier))
	{
		TIMER_Set(self, identifier, duration);
		return qtrue;
	}
	return qfalse;
}

/*
-------------------------
TIMER_List
-------------------------
*/
std::vector<std::pair<std::string, int>> TIMER_List(const gentity_t* ent)
{
	std::vector<std::pair<std::string, int>> returnValue;
	const gtimer_t* p = g_timers[ent->s.number];

	while (p)
	{
		std::pair<std::string, int> pair = std::make_pair(p->id.c_str(), p->time - level.time);
		returnValue.push_back(pair);
		p = p->next;
	}

	return returnValue;
}