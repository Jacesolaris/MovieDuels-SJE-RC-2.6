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

//
// NPC.cpp - generic functions
//

#include "b_local.h"
#include "anims.h"
#include "g_functions.h"
#include "say.h"
#include "Q3_Interface.h"
#include "g_vehicles.h"
#include "../cgame/cg_local.h"

extern vec3_t playerMins;
extern vec3_t playerMaxs;
extern void PM_SetTorsoAnimTimer(gentity_t* ent, int* torso_anim_timer, int time);
extern void PM_SetLegsAnimTimer(gentity_t* ent, int* legs_anim_timer, int time);
extern void NPC_BSNoClip(void);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speakDebounceTime);
extern void NPC_ApplyRoff(void);
extern void NPC_TempLookTarget(const gentity_t* self, int lookEntNum, int minLookTime, int maxLookTime);
extern void NPC_CheckPlayerAim(void);
extern void NPC_CheckAllClear(void);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speakDebounceTime);
extern qboolean NPC_CheckLookTarget(const gentity_t* self);
extern void NPC_SetLookTarget(const gentity_t* self, int entNum, int clearTime);
extern void Mark1_dying(gentity_t* self);
extern void NPC_BSCinematic(void);
extern int GetTime(int lastTime);
extern void G_CheckCharmed(gentity_t* self);
extern qboolean Boba_Flying(const gentity_t* self);
extern qboolean RT_Flying(const gentity_t* self);
extern qboolean jedi_cultist_destroyer(const gentity_t* self);
extern void Boba_Update();
extern bool Boba_Flee();
extern bool Boba_Tactics();
extern void BubbleShield_Update();
extern qboolean PM_LockedAnim(int anim);
extern void NPC_BSGM_Default();

extern cvar_t* g_dismemberment;
extern cvar_t* g_saberRealisticCombat;
extern cvar_t* g_corpseRemovalTime;
extern cvar_t* debug_subdivision;

//Local Variables
// ai debug cvars
cvar_t* debugNPCAI; // used to print out debug info about the bot AI
cvar_t* debugNPCFreeze; // set to disable bot ai and temporarily freeze them in place
cvar_t* debugNPCName;
cvar_t* d_saberCombat;
cvar_t* d_JediAI;
cvar_t* d_combatinfo;
cvar_t* d_noGroupAI;
cvar_t* d_asynchronousGroupAI;
cvar_t* d_slowmodeath;
cvar_t* d_slowmoaction;
cvar_t* d_SaberactionInfo;
cvar_t* d_blockinfo;
cvar_t* d_attackinfo;
cvar_t* d_saberinfo;

extern qboolean stop_icarus;

gentity_t* NPC;
gNPC_t* NPCInfo;
gclient_t* client;
usercmd_t ucmd;
visibility_t enemyVisibility;

void NPC_SetAnim(gentity_t* ent, int setAnimParts, int anim, int setAnimFlags, int iBlend);
static bState_t G_CurrentBState(gNPC_t* gNPC);

extern int eventClearTime;

extern qboolean G_EntIsBreakable(int entity_num, const gentity_t* breaker);

qboolean NPC_EntityIsBreakable(const gentity_t* ent)
{
	if (ent
		&& ent->inuse
		&& ent->takedamage
		&& ent->classname
		&& ent->classname[0]
		&& ent->s.eType != ET_INVISIBLE
		&& ent->s.eType != ET_PLAYER
		&& !ent->client
		&& G_EntIsBreakable(ent->s.number, ent)
		&& !EntIsGlass(ent)
		&& ent->health > 0
		&& !(ent->svFlags & SVF_PLAYER_USABLE))
	{
		return qtrue;
	}

	return qfalse;
}

qboolean NPC_IsAlive(const gentity_t* self, const gentity_t* npc)
{
	if (!npc)
	{
		return qfalse;
	}

	if (self && self->client && NPC_EntityIsBreakable(npc) && npc->health > 0)
	{
		return qtrue;
	}

	if (npc->s.eType == ET_PLAYER)
	{
		if (npc->health <= 0 && (npc->client && npc->client->ps.stats[STAT_HEALTH] <= 0))
		{
			return qfalse;
		}
	}
	else
	{
		return qfalse;
	}

	return qtrue;
}

extern void GM_Dying(gentity_t* self);

void CorpsePhysics(gentity_t* self)
{
	// run the bot through the server like it was a real client
	memset(&ucmd, 0, sizeof ucmd);
	ClientThink(self->s.number, &ucmd);
	VectorCopy(self->s.origin, self->s.origin2);

	if (self->client->NPC_class == CLASS_GALAKMECH)
	{
		GM_Dying(self);
	}

	//FIXME: match my pitch and roll for the slope of my groundPlane
	if (self->client->ps.groundEntityNum != ENTITYNUM_NONE && !(self->flags & FL_DISINTEGRATED))
	{
		//on the ground
		//FIXME: check 4 corners
		pitch_roll_for_slope(self);
	}

	if (eventClearTime == level.time + ALERT_CLEAR_TIME)
	{
		//events were just cleared out so add me again
		if (!(self->client->ps.eFlags & EF_NODRAW))
		{
			AddSightEvent(self->enemy, self->currentOrigin, 384, AEL_DISCOVERED);
		}
	}

	if (level.time - self->s.time > 3000)
	{
		//been dead for 3 seconds
		if (!debug_subdivision->integer && !g_saberRealisticCombat->integer)
		{
			//can't be dismembered once dead
			if (self->client->NPC_class != CLASS_PROTOCOL)
			{
				self->client->dismembered = true;
			}
		}
	}

	if (level.time - self->s.time > 5000)
	{
		//don't turn "nonsolid" until about 1 second after actual death
		if (self->client->NPC_class != CLASS_MARK1 && self->client->NPC_class != CLASS_INTERROGATOR)
			// The Mark1 & Interrogator stays solid.
		{
			self->contents = CONTENTS_CORPSE;
		}

		if (self->message)
		{
			self->contents |= CONTENTS_TRIGGER;
		}
	}
}

/*
----------------------------------------
NPC_RemoveBody

Determines when it's ok to ditch the corpse
----------------------------------------
*/
constexpr auto REMOVE_DISTANCE = 128;
#define REMOVE_DISTANCE_SQR (REMOVE_DISTANCE * REMOVE_DISTANCE)
extern qboolean InFOVFromPlayerView(const gentity_t* ent, int hFOV, int vFOV);

qboolean G_OkayToRemoveCorpse(gentity_t* self)
{
	//if we're still on a vehicle, we won't remove ourselves until we get ejected
	if (self->client && self->client->NPC_class != CLASS_VEHICLE && self->s.m_iVehicleNum != 0)
	{
		Vehicle_t* p_veh = g_entities[self->s.m_iVehicleNum].m_pVehicle;
		if (p_veh)
		{
			if (!p_veh->m_pVehicleInfo->Eject(p_veh, self, qtrue))
			{
				//dammit, still can't get off the vehicle...
				return qfalse;
			}
		}
		else
		{
			assert(0);
#ifndef FINAL_BUILD
			Com_Printf(S_COLOR_RED"ERROR: Dead pilot's vehicle removed while corpse was riding it (pilot: %s)???\n", self->targetname);
#endif
		}
	}

	if (self->message)
	{
		//I still have a key
		return qfalse;
	}

	if (IIcarusInterface::GetIcarus()->IsRunning(self->m_iIcarusID))
	{
		//still running a script
		return qfalse;
	}

	if (self->activator
		&& self->activator->client
		&& (self->activator->client->ps.eFlags & EF_HELD_BY_RANCOR
			|| self->activator->client->ps.eFlags & EF_HELD_BY_SAND_CREATURE
			|| self->activator->client->ps.eFlags & EF_HELD_BY_WAMPA))
	{
		//still holding a victim?
		return qfalse;
	}

	if (self->client
		&& (self->client->ps.eFlags & EF_HELD_BY_RANCOR
			|| self->client->ps.eFlags & EF_HELD_BY_SAND_CREATURE
			|| self->client->ps.eFlags & EF_HELD_BY_WAMPA))
	{
		//being held by a creature
		return qfalse;
	}

	if (self->client
		&& self->client->ps.heldByClient < ENTITYNUM_WORLD)
	{
		//being dragged
		return qfalse;
	}

	//okay, well okay to remove us...?
	return qtrue;
}

void NPC_RemoveBody(gentity_t* ent)
{
	ent->nextthink = level.time + FRAMETIME / 2;

	//run physics at 20fps
	CorpsePhysics(ent);

	if (ent->NPC->nextBStateThink <= level.time)
	{
		//run logic at 10 fps
		if (ent->m_iIcarusID != IIcarusInterface::ICARUS_INVALID && !stop_icarus)
		{
			IIcarusInterface::GetIcarus()->Update(ent->m_iIcarusID);
		}
		ent->NPC->nextBStateThink = level.time + FRAMETIME;

		if (!G_OkayToRemoveCorpse(ent))
		{
			return;
		}

		// I don't consider this a hack, it's creative coding . . .
		// I agree, very creative... need something like this for ATST and GALAKMECH too!
		if (ent->client->NPC_class == CLASS_MARK1)
		{
			Mark1_dying(ent);
		}

		// Since these blow up, remove the bounding box.
		if (ent->client->NPC_class == CLASS_REMOTE
			|| ent->client->NPC_class == CLASS_SENTRY
			|| ent->client->NPC_class == CLASS_PROBE
			|| ent->client->NPC_class == CLASS_INTERROGATOR
			|| ent->client->NPC_class == CLASS_PROBE
			|| ent->client->NPC_class == CLASS_MARK2)
		{
			G_FreeEntity(ent);
			return;
		}

		//FIXME: don't ever inflate back up?
		ent->maxs[2] = ent->client->renderInfo.eyePoint[2] - ent->currentOrigin[2] + 4;
		if (ent->maxs[2] < -8)
		{
			ent->maxs[2] = -8;
		}

		if (ent->NPC->aiFlags & NPCAI_HEAL_ROSH)
		{
			//kothos twins' bodies are never removed
			return;
		}

		if (ent->client->NPC_class == CLASS_GALAKMECH)
		{
			//never disappears
			return;
		}

		if (ent->NPC && ent->NPC->timeOfDeath <= level.time)
		{
			ent->NPC->timeOfDeath = level.time + 1000;

			if (ent->client->playerTeam == TEAM_ENEMY || ent->client->NPC_class == CLASS_PROTOCOL)
			{
				ent->nextthink = level.time + FRAMETIME; // try back in a second

				if (DistanceSquared(g_entities[0].currentOrigin, ent->currentOrigin) <= REMOVE_DISTANCE_SQR)
				{
					return;
				}

				if (InFOVFromPlayerView(ent, 110, 90)) // generous FOV check
				{
					if (NPC_ClearLOS(&g_entities[0], ent->currentOrigin))
					{
						return;
					}
				}
			}

			//FIXME: there are some conditions - such as heavy combat - in which we want
			//			to remove the bodies... but in other cases it's just weird, like
			//			when they're right behind you in a closed room and when they've been
			//			placed as dead NPCs by a designer...
			//			For now we just assume that a corpse with no enemy was
			//			placed in the map as a corpse
			if (ent->enemy)
			{
				if (ent->client && ent->client->ps.saberEntityNum > 0 && ent->client->ps.saberEntityNum <
					ENTITYNUM_WORLD)
				{
					gentity_t* saberent = &g_entities[ent->client->ps.saberEntityNum];
					if (saberent)
					{
						G_FreeEntity(saberent);
					}
				}
				G_FreeEntity(ent);
			}
		}
	}
}

/*
----------------------------------------
NPC_RemoveBody

Determines when it's ok to ditch the corpse
----------------------------------------
*/

int BodyRemovalPadTime(const gentity_t* ent)
{
	const char* info = CG_ConfigString(CS_SERVERINFO);
	const char* s = Info_ValueForKey(info, "mapname");
	int time;

	if (!ent || !ent->client)
		return 0;

	// team no longer indicates species/race, so in this case we'd use NPC_class, but
	switch (ent->client->NPC_class)
	{
	case CLASS_MOUSE:
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_PROBE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_SENTRY:
	case CLASS_INTERROGATOR:
		time = 0;
		break;
	case CLASS_SBD:
	case CLASS_BATTLEDROID:
	case CLASS_DROIDEKA:
		if (strcmp(s, "md_gb_jedi") == 0
			|| strcmp(s, "md_gb_sith") == 0
			|| strcmp(s, "md_ga_jedi") == 0
			|| strcmp(s, "md_ga_sith") == 0)
		{
			time = 3000;
		}
		else
		{
			if (g_corpseRemovalTime->integer <= 0)
			{
				time = Q3_INFINITE;
			}
			else
			{
				time = g_corpseRemovalTime->integer * 1000;
			}
		}
		break;
	default:
		// never go away
		if (g_corpseRemovalTime->integer <= 0)
		{
			time = Q3_INFINITE;
		}
		else
		{
			time = g_corpseRemovalTime->integer * 1000;
		}
		break;
	}

	return time;
}

/*
----------------------------------------
NPC_RemoveBodyEffect

Effect to be applied when ditching the corpse
----------------------------------------
*/

static void NPC_RemoveBodyEffect()
{
	if (!NPC || !NPC->client || NPC->s.eFlags & EF_NODRAW)
		return;

	// team no longer indicates species/race, so in this case we'd use NPC_class, but

	// stub code
	switch (NPC->client->NPC_class)
	{
	case CLASS_PROBE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
	case CLASS_SENTRY:
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_INTERROGATOR:
	case CLASS_ATST:
	case CLASS_SBD:
	case CLASS_BATTLEDROID:
		break;
	default:
		break;
	}
}

/*
====================================================================
void pitch_roll_for_slope (edict_t *forwhom, vec3_t *slope, vec3_t storeAngles )

MG

This will adjust the pitch and roll of a monster to match
a given slope - if a non-'0 0 0' slope is passed, it will
use that value, otherwise it will use the ground underneath
the monster.  If it doesn't find a surface, it does nothinh\g
and returns.
====================================================================
*/

void pitch_roll_for_slope(gentity_t* forwhom, vec3_t pass_slope, vec3_t storeAngles, const qboolean keepPitch)
{
	vec3_t slope;
	vec3_t nvf, ovf, ovr, new_angles = { 0, 0, 0 };

	//if we don't have a slope, get one
	if (!pass_slope || VectorCompare(vec3_origin, pass_slope))
	{
		vec3_t endspot;
		vec3_t startspot;
		trace_t trace;

		VectorCopy(forwhom->currentOrigin, startspot);
		startspot[2] += forwhom->mins[2] + 4;
		VectorCopy(startspot, endspot);
		endspot[2] -= 300;
		gi.trace(&trace, forwhom->currentOrigin, vec3_origin, vec3_origin, endspot, forwhom->s.number, MASK_SOLID,
			static_cast<EG2_Collision>(0), 0);
		//		if(trace_fraction>0.05&&forwhom.movetype==MOVETYPE_STEP)
		//			forwhom.flags(-)FL_ONGROUND;

		if (trace.fraction >= 1.0)
			return;

		if (VectorCompare(vec3_origin, trace.plane.normal))
			return;

		VectorCopy(trace.plane.normal, slope);
	}
	else
	{
		VectorCopy(pass_slope, slope);
	}

	float old_pitch = 0;
	if (forwhom->client && forwhom->client->NPC_class == CLASS_VEHICLE)
	{
		//special code for vehicles
		const Vehicle_t* pVeh = forwhom->m_pVehicle;

		vec3_t tempAngles;
		tempAngles[PITCH] = tempAngles[ROLL] = 0;
		tempAngles[YAW] = pVeh->m_vOrientation[YAW];
		AngleVectors(tempAngles, ovf, ovr, nullptr);
	}
	else
	{
		old_pitch = forwhom->currentAngles[PITCH];
		AngleVectors(forwhom->currentAngles, ovf, ovr, nullptr);
	}

	vectoangles(slope, new_angles);
	float pitch = new_angles[PITCH] + 90;
	if (keepPitch)
	{
		pitch += old_pitch;
	}
	new_angles[ROLL] = new_angles[PITCH] = 0;

	AngleVectors(new_angles, nvf, nullptr, nullptr);

	float mod = DotProduct(nvf, ovr);

	if (mod < 0)
		mod = -1;
	else
		mod = 1;

	const float dot = DotProduct(nvf, ovf);

	if (storeAngles)
	{
		storeAngles[PITCH] = dot * pitch;
		storeAngles[ROLL] = (1 - Q_fabs(dot)) * pitch * mod;
	}
	else if (forwhom->client)
	{
		forwhom->client->ps.viewangles[PITCH] = dot * pitch;
		forwhom->client->ps.viewangles[ROLL] = (1 - Q_fabs(dot)) * pitch * mod;
		const float oldmins2 = forwhom->mins[2];
		forwhom->mins[2] = -24 + 12 * fabs(forwhom->client->ps.viewangles[PITCH]) / 180.0f;
		//FIXME: if it gets bigger, move up
		if (oldmins2 > forwhom->mins[2])
		{
			//our mins is now lower, need to move up
			//FIXME: trace?
			forwhom->client->ps.origin[2] += oldmins2 - forwhom->mins[2];
			forwhom->currentOrigin[2] = forwhom->client->ps.origin[2];
			gi.linkentity(forwhom);
		}
	}
	else
	{
		forwhom->currentAngles[PITCH] = dot * pitch;
		forwhom->currentAngles[ROLL] = (1 - Q_fabs(dot)) * pitch * mod;
	}
}

/*
void NPC_PostDeathThink( void )
{
	float	mostdist;
	trace_t trace1, trace2, trace3, trace4, movetrace;
	vec3_t	org, endpos, startpos, forward, right;
	int		whichtrace = 0;
	float	cornerdist[4];
	qboolean	frontbackbothclear = false;
	qboolean	rightleftbothclear = false;

	if( NPC->client->ps.groundEntityNum == ENTITYNUM_NONE || !VectorCompare( vec3_origin, NPC->client->ps.velocity ) )
	{
		if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE && NPC->client->ps.friction == 1.0 )//check avelocity?
		{
			pitch_roll_for_slope( NPC );
		}

		return;
	}

	cornerdist[FRONT] = cornerdist[BACK] = cornerdist[RIGHT] = cornerdist[LEFT] = 0.0f;

	mostdist = MIN_DROP_DIST;

	AngleVectors( NPC->currentAngles, forward, right, NULL );
	VectorCopy( NPC->currentOrigin, org );
	org[2] += NPC->mins[2];

	VectorMA( org, NPC->dead_size, forward, startpos );
	VectorCopy( startpos, endpos );
	endpos[2] -= 128;
	gi.trace( &trace1, startpos, vec3_origin, vec3_origin, endpos, NPC->s.number, MASK_SOLID, );
	if( !trace1.allsolid && !trace1.startsolid )
	{
		cornerdist[FRONT] = trace1.fraction;
		if ( trace1.fraction > mostdist )
		{
			mostdist = trace1.fraction;
			whichtrace = 1;
		}
	}

	VectorMA( org, -NPC->dead_size, forward, startpos );
	VectorCopy( startpos, endpos );
	endpos[2] -= 128;
	gi.trace( &trace2, startpos, vec3_origin, vec3_origin, endpos, NPC->s.number, MASK_SOLID );
	if( !trace2.allsolid && !trace2.startsolid )
	{
		cornerdist[BACK] = trace2.fraction;
		if ( trace2.fraction > mostdist )
		{
			mostdist = trace2.fraction;
			whichtrace = 2;
		}
	}

	VectorMA( org, NPC->dead_size/2, right, startpos );
	VectorCopy( startpos, endpos );
	endpos[2] -= 128;
	gi.trace( &trace3, startpos, vec3_origin, vec3_origin, endpos, NPC->s.number, MASK_SOLID );
	if ( !trace3.allsolid && !trace3.startsolid )
	{
		cornerdist[RIGHT] = trace3.fraction;
		if ( trace3.fraction>mostdist )
		{
			mostdist = trace3.fraction;
			whichtrace = 3;
		}
	}

	VectorMA( org, -NPC->dead_size/2, right, startpos );
	VectorCopy( startpos, endpos );
	endpos[2] -= 128;
	gi.trace( &trace4, startpos, vec3_origin, vec3_origin, endpos, NPC->s.number, MASK_SOLID );
	if ( !trace4.allsolid && !trace4.startsolid )
	{
		cornerdist[LEFT] = trace4.fraction;
		if ( trace4.fraction > mostdist )
		{
			mostdist = trace4.fraction;
			whichtrace = 4;
		}
	}

	//OK!  Now if two opposite sides are hanging, use a third if any, else, do nothing
	if ( cornerdist[FRONT] > MIN_DROP_DIST && cornerdist[BACK] > MIN_DROP_DIST )
		frontbackbothclear = true;

	if ( cornerdist[RIGHT] > MIN_DROP_DIST && cornerdist[LEFT] > MIN_DROP_DIST )
		rightleftbothclear = true;

	if ( frontbackbothclear && rightleftbothclear )
		return;

	if ( frontbackbothclear )
	{
		if ( cornerdist[RIGHT] > MIN_DROP_DIST )
			whichtrace = 3;
		else if ( cornerdist[LEFT] > MIN_DROP_DIST )
			whichtrace = 4;
		else
			return;
	}

	if ( rightleftbothclear )
	{
		if ( cornerdist[FRONT] > MIN_DROP_DIST )
			whichtrace = 1;
		else if ( cornerdist[BACK] > MIN_DROP_DIST )
			whichtrace = 2;
		else
			return;
	}

	switch ( whichtrace )
	{//check for stuck
	case 1:
		VectorMA( NPC->currentOrigin, NPC->maxs[0], forward, endpos );
		gi.trace( &movetrace, NPC->currentOrigin, NPC->mins, NPC->maxs, endpos, NPC->s.number, MASK_MONSTERSOLID );
		if ( movetrace.allsolid || movetrace.startsolid || movetrace.fraction < 1.0 )
			if ( canmove( movetrace.ent ) )
				whichtrace = -1;
			else
				whichtrace = 0;
		break;
	case 2:
		VectorMA( NPC->currentOrigin, -NPC->maxs[0], forward, endpos );
		gi.trace( &movetrace, NPC->currentOrigin, NPC->mins, NPC->maxs, endpos, NPC->s.number, MASK_MONSTERSOLID );
		if ( movetrace.allsolid || movetrace.startsolid || movetrace.fraction < 1.0 )
			if ( canmove( movetrace.ent ) )
				whichtrace = -1;
			else
				whichtrace = 0;
		break;
	case 3:
		VectorMA( NPC->currentOrigin, NPC->maxs[0], right, endpos );
		gi.trace( &movetrace, NPC->currentOrigin, NPC->mins, NPC->maxs, endpos, NPC->s.number, MASK_MONSTERSOLID );
		if ( movetrace.allsolid || movetrace.startsolid || movetrace.fraction < 1.0 )
			if ( canmove( movetrace.ent ) )
				whichtrace = -1;
			else
				whichtrace = 0;
		break;
	case 4:
		VectorMA( NPC->currentOrigin, -NPC->maxs[0], right, endpos );
		gi.trace( &movetrace, NPC->currentOrigin, NPC->mins, NPC->maxs, endpos, NPC->s.number, MASK_MONSTERSOLID );
		if (movetrace.allsolid || movetrace.startsolid || movetrace.fraction < 1.0 )
			if ( canmove( movetrace.ent ) )
				whichtrace = -1;
			else
				whichtrace = 0;
		break;
	}

	switch ( whichtrace )
	{
	case 1:
		VectorMA( NPC->client->ps.velocity, 200, forward, NPC->client->ps.velocity );
		if ( trace1.fraction >= 0.9 )
		{
//can't anymore, origin not in center of deathframe!
//			NPC->avelocity[PITCH] = -300;
			NPC->client->ps.friction = 1.0;
		}
		else
		{
			pitch_roll_for_slope( NPC, &trace1.plane.normal );
			NPC->client->ps.friction = trace1.plane.normal[2] * 0.1;
		}
		return;
		break;

	case 2:
		VectorMA( NPC->client->ps.velocity, -200, forward, NPC->client->ps.velocity );
		if(trace2.fraction >= 0.9)
		{
//can't anymore, origin not in center of deathframe!
//			NPC->avelocity[PITCH] = 300;
			NPC->client->ps.friction = 1.0;
		}
		else
		{
			pitch_roll_for_slope( NPC, &trace2.plane.normal );
			NPC->client->ps.friction = trace2.plane.normal[2] * 0.1;
		}
		return;
		break;

	case 3:
		VectorMA( NPC->client->ps.velocity, 200, right, NPC->client->ps.velocity );
		if ( trace3.fraction >= 0.9 )
		{
//can't anymore, origin not in center of deathframe!
//			NPC->avelocity[ROLL] = -300;
			NPC->client->ps.friction = 1.0;
		}
		else
		{
			pitch_roll_for_slope( NPC, &trace3.plane.normal );
			NPC->client->ps.friction = trace3.plane.normal[2] * 0.1;
		}
		return;
		break;

	case 4:
		VectorMA( NPC->client->ps.velocity, -200, right, NPC->client->ps.velocity );
		if ( trace4.fraction >= 0.9 )
		{
//can't anymore, origin not in center of deathframe!
//			NPC->avelocity[ROLL] = 300;
			NPC->client->ps.friction = 1.0;
		}
		else
		{
			pitch_roll_for_slope( NPC, &trace4.plane.normal );
			NPC->client->ps.friction = trace4.plane.normal[2] * 0.1;
		}
		return;
		break;
	}

	//on solid ground
	if ( whichtrace == -1 )
	{
		return;
	}
	NPC->client->ps.friction = 1.0;

	//VectorClear( NPC->avelocity );
	pitch_roll_for_slope( NPC );

	//gi.linkentity (NPC);
}
*/

/*
----------------------------------------
DeadThink
----------------------------------------
*/
static void DeadThink()
{
	trace_t trace;
	//HACKHACKHACKHACKHACK
	//We should really have a seperate G2 bounding box (seperate from the physics bbox) for G2 collisions only
	//FIXME: don't ever inflate back up?
	//GAH!  With Ragdoll, they get stuck in the ceiling
	const float oldMaxs2 = NPC->maxs[2];
	NPC->maxs[2] = NPC->client->renderInfo.eyePoint[2] - NPC->currentOrigin[2] + 4;
	if (NPC->maxs[2] < -8)
	{
		NPC->maxs[2] = -8;
	}
	if (NPC->maxs[2] > oldMaxs2)
	{
		//inflating maxs, make sure we're not inflating into solid
		gi.trace(&trace, NPC->currentOrigin, NPC->mins, NPC->maxs, NPC->currentOrigin, NPC->s.number, NPC->clipmask,
			static_cast<EG2_Collision>(0), 0);
		if (trace.allsolid)
		{
			//must be inflating
			NPC->maxs[2] = oldMaxs2;
		}
	}

	if (level.time >= NPCInfo->timeOfDeath + BodyRemovalPadTime(NPC))
	{
		if (NPC->client->ps.eFlags & EF_NODRAW)
		{
			if (!IIcarusInterface::GetIcarus()->IsRunning(NPC->m_iIcarusID))
			{
				NPC->e_ThinkFunc = thinkF_G_FreeEntity;
				NPC->nextthink = level.time + FRAMETIME;
			}
		}
		else
		{
			// Start the body effect first, then delay 400ms before ditching the corpse
			NPC_RemoveBodyEffect();

			NPC->e_ThinkFunc = thinkF_NPC_RemoveBody;
			NPC->nextthink = level.time + FRAMETIME / 2;

			const class_t npc_class = NPC->client->NPC_class;
			// check for droids
			if (npc_class == CLASS_ATST || npc_class == CLASS_GONK ||
				npc_class == CLASS_INTERROGATOR || npc_class == CLASS_MARK1 ||
				npc_class == CLASS_MARK2 || npc_class == CLASS_MOUSE ||
				npc_class == CLASS_PROBE || npc_class == CLASS_PROTOCOL ||
				npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 ||
				npc_class == CLASS_SEEKER || npc_class == CLASS_SENTRY ||
				npc_class == CLASS_SBD || npc_class == CLASS_BATTLEDROID ||
				npc_class == CLASS_DROIDEKA || npc_class == CLASS_OBJECT ||
				npc_class == CLASS_ASSASSIN_DROID || npc_class == CLASS_SABER_DROID)
			{
				NPC->client->ps.eFlags |= EF_NODRAW;
				NPCInfo->timeOfDeath = level.time + FRAMETIME * 8;
			}
			else
			{
				NPCInfo->timeOfDeath = level.time + FRAMETIME * 4;
			}
		}
		return;
	}

	// If the player is on the ground and the resting position contents haven't been set yet...(BounceCount tracks the contents)
	if (NPC->bounceCount < 0 && NPC->s.groundEntityNum >= 0)
	{
		// if client is in a nodrop area, make him/her nodraw
		const int contents = NPC->bounceCount = gi.pointcontents(NPC->currentOrigin, -1);

		if (contents & CONTENTS_NODROP)
		{
			NPC->client->ps.eFlags |= EF_NODRAW;
		}
	}

	CorpsePhysics(NPC);
}

/*
===============
SetNPCGlobals

local function to set globals used throughout the AI code
===============
*/
void SetNPCGlobals(gentity_t* ent)
{
	NPC = ent;
	NPCInfo = ent->NPC;
	client = ent->client;
	memset(&ucmd, 0, sizeof(usercmd_t));
}

gentity_t* saved_npc;
gNPC_t* saved_npc_info;
gclient_t* saved_client;
usercmd_t saved_ucmd;

void SaveNPCGlobals()
{
	saved_npc = NPC;
	saved_npc_info = NPCInfo;
	saved_client = client;
	memcpy(&saved_ucmd, &ucmd, sizeof(usercmd_t));
}

void RestoreNPCGlobals()
{
	NPC = saved_npc;
	NPCInfo = saved_npc_info;
	client = saved_client;
	memcpy(&ucmd, &saved_ucmd, sizeof(usercmd_t));
}

//We MUST do this, other funcs were using NPC illegally when "self" wasn't the global NPC
void ClearNPCGlobals()
{
	NPC = nullptr;
	NPCInfo = nullptr;
	client = nullptr;
}

//===============

extern qboolean showBBoxes;
vec3_t NPCDEBUG_RED = { 1.0, 0.0, 0.0 };
vec3_t NPCDEBUG_GREEN = { 0.0, 1.0, 0.0 };
vec3_t NPCDEBUG_BLUE = { 0.0, 0.0, 1.0 };
vec3_t NPCDEBUG_LIGHT_BLUE = { 0.3f, 0.7f, 1.0 };
extern void CG_Cube(vec3_t mins, vec3_t maxs, vec3_t color, float alpha);

void NPC_ShowDebugInfo()
{
	if (showBBoxes)
	{
		gentity_t* found = nullptr;
		vec3_t mins, maxs;

		//do player, too
		VectorAdd(player->currentOrigin, player->mins, mins);
		VectorAdd(player->currentOrigin, player->maxs, maxs);
		CG_Cube(mins, maxs, NPCDEBUG_RED, 0.25);
		//do NPCs
		while ((found = G_Find(found, FOFS(classname), "NPC")) != nullptr)
		{
			if (gi.inPVS(found->currentOrigin, g_entities[0].currentOrigin))
			{
				VectorAdd(found->currentOrigin, found->mins, mins);
				VectorAdd(found->currentOrigin, found->maxs, maxs);
				CG_Cube(mins, maxs, NPCDEBUG_RED, 0.25);
			}
		}
	}
}

void NPC_ApplyScriptFlags()
{
	if (NPCInfo->scriptFlags & SCF_CROUCHED)
	{
		if (NPCInfo->charmedTime > level.time && (ucmd.forwardmove || ucmd.rightmove))
		{
			//ugh, if charmed and moving, ignore the crouched command
		}
		else
		{
			ucmd.upmove = -127;
		}
	}

	if (NPCInfo->scriptFlags & SCF_RUNNING)
	{
		ucmd.buttons &= ~BUTTON_WALKING;
	}
	else if (NPCInfo->scriptFlags & SCF_WALKING)
	{
		if (NPCInfo->charmedTime > level.time && (ucmd.forwardmove || ucmd.rightmove))
		{
			//ugh, if charmed and moving, ignore the walking command
		}
		else
		{
			ucmd.buttons |= BUTTON_WALKING;
		}
	}
	/*
		if(NPCInfo->scriptFlags & SCF_CAREFUL)
		{
			ucmd.buttons |= BUTTON_CAREFUL;
		}
	*/
	if (NPCInfo->scriptFlags & SCF_LEAN_RIGHT)
	{
		ucmd.buttons |= BUTTON_USE;
		ucmd.rightmove = 127;
		ucmd.forwardmove = 0;
		ucmd.upmove = 0;
	}
	else if (NPCInfo->scriptFlags & SCF_LEAN_LEFT)
	{
		ucmd.buttons |= BUTTON_USE;
		ucmd.rightmove = -127;
		ucmd.forwardmove = 0;
		ucmd.upmove = 0;
	}

	if (NPCInfo->scriptFlags & SCF_ALT_FIRE && ucmd.buttons & BUTTON_ATTACK)
	{
		//Use altfire instead
		ucmd.buttons |= BUTTON_ALT_ATTACK;
	}

	// only removes NPC when it's safe too (Player is out of PVS)
	if (NPCInfo->scriptFlags & SCF_SAFE_REMOVE)
	{
		// take from BSRemove
		if (!gi.inPVS(NPC->currentOrigin, g_entities[0].currentOrigin)) //FIXME: use cg.vieworg?
		{
			G_UseTargets2(NPC, NPC, NPC->target3);
			NPC->s.eFlags |= EF_NODRAW;
			NPC->svFlags &= ~SVF_NPC;
			NPC->s.eType = ET_INVISIBLE;
			NPC->contents = 0;
			NPC->health = 0;
			NPC->targetname = nullptr;

			//Disappear in half a second
			NPC->e_ThinkFunc = thinkF_G_FreeEntity;
			NPC->nextthink = level.time + FRAMETIME;
		} //FIXME: else allow for out of FOV???
	}
}

extern qboolean JET_Flying(const gentity_t* self);
extern void JET_FlyStart(gentity_t* self);
extern void jet_fly_stop(gentity_t* self);

void NPC_HandleAIFlags()
{
	// Update Guys With Jet Packs
	//----------------------------
	if (NPCInfo->scriptFlags & SCF_FLY_WITH_JET)
	{
		bool ShouldFly = !!(NPCInfo->aiFlags & NPCAI_FLY);
		const bool IsFlying = !!JET_Flying(NPC);
		bool IsInTheAir = NPC->client->ps.groundEntityNum == ENTITYNUM_NONE;

		if (IsFlying)
		{
			// Don't Stop Flying Until Near The Ground
			//-----------------------------------------
			if (IsInTheAir)
			{
				vec3_t ground;
				trace_t trace;
				VectorCopy(NPC->currentOrigin, ground);
				ground[2] -= 60.0f;
				gi.trace(&trace, NPC->currentOrigin, nullptr, nullptr, ground, NPC->s.number, NPC->clipmask,
					static_cast<EG2_Collision>(0), 0);

				IsInTheAir = !trace.allsolid && !trace.startsolid && trace.fraction > 0.9f;
			}

			// If Flying, Remember The Last Time
			//-----------------------------------
			if (IsInTheAir)
			{
				NPC->lastInAirTime = level.time;
				ShouldFly = true;
			}

			// Auto Turn Off Jet Pack After 1 Second On The Ground
			//-----------------------------------------------------
			else if (!ShouldFly && level.time - NPC->lastInAirTime > 3000)
			{
				NPCInfo->aiFlags &= ~NPCAI_FLY;
			}
		}

		// If We Should Be Flying And Are Not, Start Er Up
		//-------------------------------------------------
		if (ShouldFly && !IsFlying)
		{
			JET_FlyStart(NPC); // EVENTUALLY, Remove All Other Calls
		}

		// Otherwise, If Needed, Shut It Off
		//-----------------------------------
		else if (!ShouldFly && IsFlying)
		{
			jet_fly_stop(NPC); // EVENTUALLY, Remove All Other Calls
		}
	}

	//FIXME: make these flags checks a function call like NPC_CheckAIFlagsAndTimers
	if (NPCInfo->aiFlags & NPCAI_LOST)
	{
		//Print that you need help!
		//FIXME: shouldn't remove this just yet if cg_draw needs it
		NPCInfo->aiFlags &= ~NPCAI_LOST;

		if (NPCInfo->goalEntity && NPCInfo->goalEntity == NPC->enemy)
		{
			//We can't nav to our enemy
			//Drop enemy and see if we should search for him
			NPC_LostEnemyDecideChase();
		}
	}

	//been told to play a victory sound after a delay
	if (NPCInfo->greetingDebounceTime && NPCInfo->greetingDebounceTime < level.time)
	{
		G_AddVoiceEvent(NPC, Q_irand(EV_VICTORY1, EV_VICTORY3), Q_irand(2000, 4000));
		NPCInfo->greetingDebounceTime = 0;
	}

	if (NPCInfo->ffireCount > 0)
	{
		if (NPCInfo->ffireFadeDebounce < level.time)
		{
			NPCInfo->ffireCount--;
			//Com_Printf( "drop: %d < %d\n", NPCInfo->ffireCount, 3+((2-g_spskill->integer)*2) );
			NPCInfo->ffireFadeDebounce = level.time + 3000;
		}
	}
}

void NPC_AvoidWallsAndCliffs()
{
	/*
		vec3_t	forward, right, testPos, angles, mins;
		trace_t	trace;
		float	fwdDist, rtDist;
		//FIXME: set things like this forward dir once at the beginning
		//of a frame instead of over and over again
		if ( NPCInfo->aiFlags & NPCAI_NO_COLL_AVOID )
		{
			return;
		}

		if ( ucmd.upmove > 0 || NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
		{//Going to jump or in the air
			return;
		}

		if ( !ucmd.forwardmove && !ucmd.rightmove )
		{
			return;
		}

		if ( fabs( AngleDelta( NPC->currentAngles[YAW], NPCInfo->desiredYaw ) ) < 5.0 )//!ucmd.angles[YAW] )
		{//Not turning much, don't do this
			//NOTE: Should this not happen only if you're not turning AT ALL?
			//	You could be turning slowly but moving fast, so that would
			//	still let you walk right off a cliff...
			//NOTE: Or maybe it is a good idea to ALWAYS do this, regardless
			//	of whether ot not we're turning?  But why would we be walking
			//  straight into a wall or off	a cliff unless we really wanted to?
			return;
		}

		VectorCopy( NPC->mins, mins );
		mins[2] += STEPSIZE;
		angles[YAW] = NPC->client->ps.viewangles[YAW];//Add ucmd.angles[YAW]?
		AngleVectors( angles, forward, right, NULL );
		fwdDist = ((float)ucmd.forwardmove)/16.0f;
		rtDist = ((float)ucmd.rightmove)/16.0f;
		VectorMA( NPC->currentOrigin, fwdDist, forward, testPos );
		VectorMA( testPos, rtDist, right, testPos );
		gi.trace( &trace, NPC->currentOrigin, mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask );
		if ( trace.allsolid || trace.startsolid || trace.fraction < 1.0 )
		{//Going to bump into something, don't move, just turn
			ucmd.forwardmove = 0;
			ucmd.rightmove = 0;
			return;
		}

		VectorCopy(trace.endpos, testPos);
		testPos[2] -= 128;

		gi.trace( &trace, trace.endpos, mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask );
		if ( trace.allsolid || trace.startsolid || trace.fraction < 1.0 )
		{//Not going off a cliff
			return;
		}

		//going to fall at least 128, don't move, just turn... is this bad, though?  What if we want them to drop off?
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		return;
	*/
}

void NPC_CheckAttackScript(void)
{
	if (!(ucmd.buttons & BUTTON_ATTACK))
	{
		return;
	}

	G_ActivateBehavior(NPC, BSET_ATTACK);
}

float NPC_MaxDistSquaredForWeapon();

void NPC_CheckAttackHold(void)
{
	vec3_t vec;

	// If they don't have an enemy they shouldn't hold their attack anim.
	if (!NPC->enemy)
	{
		NPCInfo->attackHoldTime = 0;
		return;
	}

	//FIXME: need to tie this into AI somehow?
	VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, vec);
	if (VectorLengthSquared(vec) > NPC_MaxDistSquaredForWeapon())
	{
		NPCInfo->attackHoldTime = 0;
	}
	else if (NPCInfo->attackHoldTime && NPCInfo->attackHoldTime > level.time)
	{
		ucmd.buttons |= BUTTON_ATTACK;
	}
	else if (NPCInfo->attackHold && ucmd.buttons & BUTTON_ATTACK)
	{
		NPCInfo->attackHoldTime = level.time + NPCInfo->attackHold;
	}
	else
	{
		NPCInfo->attackHoldTime = 0;
	}
}

/*
void NPC_KeepCurrentFacing(void)

Fills in a default ucmd to keep current angles facing
*/
void NPC_KeepCurrentFacing(void)
{
	if (!ucmd.angles[YAW])
	{
		ucmd.angles[YAW] = ANGLE2SHORT(client->ps.viewangles[YAW]) - client->ps.delta_angles[YAW];
	}

	if (!ucmd.angles[PITCH])
	{
		ucmd.angles[PITCH] = ANGLE2SHORT(client->ps.viewangles[PITCH]) - client->ps.delta_angles[PITCH];
	}
}

/*
-------------------------
NPC_BehaviorSet_Charmed
-------------------------
*/

void NPC_BehaviorSet_Charmed(const int bState)
{
	switch (bState)
	{
	case BS_FOLLOW_LEADER: //# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader();
		break;
	case BS_REMOVE:
		NPC_BSRemove();
		break;
	case BS_SEARCH: //# 43: Using current waypoint as a base, search the immediate branches of waypoints for enemies
		NPC_BSSearch();
		break;
	case BS_WANDER: //# 46: Wander down random waypoint paths
		NPC_BSWander();
		break;
	case BS_FLEE:
		NPC_BSFlee();
		break;
	case BS_FOLLOW_OVERRIDE: //# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader();
		break;
	default:
	case BS_DEFAULT: //whatever
		NPC_BSDefault();
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Default
-------------------------
*/

void NPC_BehaviorSet_Default(const int bState)
{
	switch (bState)
	{
	case BS_ADVANCE_FIGHT: //head toward captureGoal, shoot anything that gets in the way
		NPC_BSAdvanceFight();
		break;
	case BS_SLEEP: //Follow a path, looking for enemies
		NPC_BSSleep();
		break;
	case BS_FOLLOW_LEADER: //# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader();
		break;
	case BS_JUMP: //41: Face navgoal and jump to it.
		NPC_BSJump();
		break;
	case BS_REMOVE:
		NPC_BSRemove();
		break;
	case BS_SEARCH: //# 43: Using current waypoint as a base, search the immediate branches of waypoints for enemies
		NPC_BSSearch();
		break;
	case BS_NOCLIP:
		NPC_BSNoClip();
		break;
	case BS_WANDER: //# 46: Wander down random waypoint paths
		NPC_BSWander();
		break;
	case BS_FLEE:
		NPC_BSFlee();
		break;
	case BS_WAIT:
		NPC_BSWait();
		break;
	case BS_CINEMATIC:
		NPC_BSCinematic();
		break;
	case BS_FOLLOW_OVERRIDE: //# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader();
		break;
	default:
	case BS_DEFAULT: //whatever
		NPC_BSDefault();
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Interrogator
-------------------------
*/
void NPC_BehaviorSet_Interrogator(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSInterrogator_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

void NPC_BSImperialProbe_Attack(void);
void NPC_BSImperialProbe_Patrol(void);
void NPC_BSImperialProbe_Wait(void);

/*
-------------------------
NPC_BehaviorSet_ImperialProbe
-------------------------
*/
void NPC_BehaviorSet_ImperialProbe(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSImperialProbe_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

void NPC_BSSeeker_Default();

/*
-------------------------
NPC_BehaviorSet_Seeker
-------------------------
*/
void NPC_BehaviorSet_Seeker(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
	default:
		NPC_BSSeeker_Default();
		break;
	}
}

void NPC_BSRemote_Default();

/*
-------------------------
NPC_BehaviorSet_Remote
-------------------------
*/
void NPC_BehaviorSet_Remote(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSRemote_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

void NPC_BSSentry_Default(void);

/*
-------------------------
NPC_BehaviorSet_Sentry
-------------------------
*/
void NPC_BehaviorSet_Sentry(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSentry_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Grenadier
-------------------------
*/
void NPC_BehaviorSet_Grenadier(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSGrenadier_Default();
		break;

	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Tusken
-------------------------
*/
void NPC_BehaviorSet_Tusken(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSTusken_Default();
		break;

	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Sniper
-------------------------
*/
void NPC_BehaviorSet_Sniper(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSniper_Default();
		break;

	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Stormtrooper
-------------------------
*/

void NPC_BehaviorSet_Stormtrooper(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSST_Default();
		break;

	case BS_INVESTIGATE:
		NPC_BSST_Investigate();
		break;

	case BS_SLEEP:
		NPC_BSST_Sleep();
		break;

	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

void NPC_BehaviorSet_Object(int bState)
{
}

/*
-------------------------
NPC_BehaviorSet_Jedi
-------------------------
*/

void NPC_BehaviorSet_Jedi(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_INVESTIGATE: //WTF???!!
	case BS_DEFAULT:
		npc_bs_jedi_default();
		break;

	case BS_FOLLOW_LEADER:
		npc_bs_jedi_follow_leader();
		break;
	case BS_FOLLOW_OVERRIDE: //# 40: Follow your leader and shoot any enemies you come across
		npc_bs_jedi_follow_leader();
		break;

	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Grogu
-------------------------
*/

void NPC_BehaviorSet_Grogu(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		npc_bs_jedi_default();
		break;

	case BS_FOLLOW_LEADER:
		NPC_BSFollowLeader();
		break;
	case BS_FOLLOW_OVERRIDE: //# 40: Follow your leader and shoot any enemies you come across
		NPC_BSFollowLeader();
		break;

	case BS_INVESTIGATE:
		NPC_BSST_Investigate();
		break;

	case BS_SLEEP:
		NPC_BSST_Sleep();
		break;

	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

qboolean G_JediInNormalAI(const gentity_t* ent)
{
	//NOTE: should match above func's switch!
	//check our bState
	const bState_t bState = G_CurrentBState(ent->NPC);
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_INVESTIGATE:
	case BS_DEFAULT:
	case BS_FOLLOW_LEADER:
	case BS_FOLLOW_OVERRIDE:
		return qtrue;
	default:
		break;
	}
	return qfalse;
}

/*
-------------------------
NPC_BehaviorSet_Droid
-------------------------
*/
void NPC_BehaviorSet_Droid(const int bState)
{
	switch (bState)
	{
	case BS_DEFAULT:
	case BS_STAND_GUARD:
	case BS_PATROL:
		NPC_BSDroid_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Mark1
-------------------------
*/
void NPC_BehaviorSet_Mark1(const int bState)
{
	switch (bState)
	{
	case BS_DEFAULT:
	case BS_STAND_GUARD:
	case BS_PATROL:
		NPC_BSMark1_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Mark2
-------------------------
*/
void NPC_BehaviorSet_Mark2(const int bState)
{
	switch (bState)
	{
	case BS_DEFAULT:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
		NPC_BSMark2_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_ATST
-------------------------
*/
void NPC_BehaviorSet_ATST(const int bState)
{
	switch (bState)
	{
	case BS_DEFAULT:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
		NPC_BSATST_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_MineMonster
-------------------------
*/
void NPC_BehaviorSet_MineMonster(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSMineMonster_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Howler
-------------------------
*/
void NPC_BehaviorSet_Howler(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSHowler_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Rancor
-------------------------
*/
void NPC_BehaviorSet_Rancor(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSRancor_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Wampa
-------------------------
*/
void NPC_BehaviorSet_Wampa(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSWampa_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_SandCreature
-------------------------
*/
void NPC_BehaviorSet_SandCreature(const int bState)
{
	switch (bState)
	{
	case BS_STAND_GUARD:
	case BS_PATROL:
	case BS_STAND_AND_SHOOT:
	case BS_HUNT_AND_KILL:
	case BS_DEFAULT:
		NPC_BSSandCreature_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_BehaviorSet_Droid
-------------------------
*/
// Added 01/21/03 by AReis.
void NPC_BehaviorSet_Animal(const int bState)
{
	switch (bState)
	{
	case BS_DEFAULT:
	case BS_STAND_GUARD:
	case BS_PATROL:
		NPC_BSAnimal_Default();

		//NPC_BSDroid_Default();
		break;
	default:
		NPC_BehaviorSet_Default(bState);
		break;
	}
}

/*
-------------------------
NPC_RunBehavior
-------------------------
*/
extern void NPC_BSEmplaced(void);
extern qboolean NPC_CheckSurrender(void);
extern void NPC_BSRT_Default(void);
extern void NPC_BSCivilian_Default(int bState);
extern void NPC_BSSD_Default();
extern void NPC_BehaviorSet_Trooper(int bState);
extern bool NPC_IsTrooper(const gentity_t* ent);
extern bool Pilot_MasterUpdate();

void NPC_RunBehavior(const int team, const int bState)
{
	//
	//
	if (bState == BS_FOLLOW_OVERRIDE)
	{
		NPC_BSFollowLeader();
	}
	if (bState == BS_CINEMATIC)
	{
		NPC_BSCinematic();
	}
	else if (NPCInfo->scriptFlags & SCF_PILOT && Pilot_MasterUpdate())
	{
	}
	else if (NPC_JumpBackingUp())
	{
	}
	else if (!TIMER_Done(NPC, "DEMP2_StunTime"))
	{
		NPC_UpdateAngles(qtrue, qtrue);
	}
	else if (NPC->client->ps.weapon == WP_EMPLACED_GUN)
	{
		NPC_BSEmplaced();
		G_CheckCharmed(NPC);
	}
	else if (NPC->client->NPC_class == CLASS_HOWLER)
	{
		NPC_BehaviorSet_Howler(bState);
	}
	else if (jedi_cultist_destroyer(NPC))
	{
		npc_bs_jedi_default();
	}
	else if (NPC->client->NPC_class == CLASS_SABER_DROID)
	{
		//saber droid
		NPC_BSSD_Default();
	}
	else if (NPC->client->ps.weapon == WP_SABER)
	{
		//jedi
		NPC_BehaviorSet_Jedi(bState);
	}
	else if (NPC->client->NPC_class == CLASS_REBORN && NPC->client->ps.weapon == WP_MELEE)
	{
		//force-only reborn
		NPC_BehaviorSet_Jedi(bState);
	}
	else if (NPC->client->NPC_class == CLASS_SITHLORD && NPC->client->ps.weapon == WP_MELEE)
	{
		//force-only reborn
		NPC_BehaviorSet_Jedi(bState);
	}
	else if (NPC->client->NPC_class == CLASS_SITHLORD && NPC->client->ps.weapon == WP_NONE)
	{
		//force-only reborn
		NPC_BehaviorSet_Jedi(bState);
	}
	else if (NPC->client->NPC_class == CLASS_GROGU)
	{
		//force-only reborn
		NPC_BehaviorSet_Grogu(bState);
	}
	else if (NPC->client->NPC_class == CLASS_BOBAFETT || NPC->client->NPC_class == CLASS_MANDALORIAN || NPC->client->
		NPC_class == CLASS_JANGO || NPC->client->NPC_class == CLASS_JANGODUAL)
	{
		Boba_Update();
		if (NPCInfo->surrenderTime)
		{
			Boba_Flee();
		}
		else
		{
			if (!Boba_Tactics())
			{
				if (Boba_Flying(NPC))
				{
					NPC_BehaviorSet_Seeker(bState);
				}
				else
				{
					NPC_BehaviorSet_Jedi(bState);
				}
			}
		}
	}
	else if (NPC->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		//bounty hunter
		if (RT_Flying(NPC) || NPC->enemy != nullptr)
		{
			NPC_BSRT_Default();
		}
		else
		{
			NPC_BehaviorSet_Stormtrooper(bState);
		}
		G_CheckCharmed(NPC);
	}
	else if (NPC->client->NPC_class == CLASS_RANCOR)
	{
		NPC_BehaviorSet_Rancor(bState);
	}
	else if (NPC->client->NPC_class == CLASS_SAND_CREATURE)
	{
		NPC_BehaviorSet_SandCreature(bState);
	}
	else if (NPC->client->NPC_class == CLASS_WAMPA)
	{
		NPC_BehaviorSet_Wampa(bState);
		G_CheckCharmed(NPC);
	}
	else if (NPCInfo->scriptFlags & SCF_FORCED_MARCH)
	{
		//being forced to march
		NPC_BSDefault();
	}
	else if (NPC->client->ps.weapon == WP_TUSKEN_RIFLE)
	{
		if (NPCInfo->scriptFlags & SCF_ALT_FIRE)
		{
			NPC_BehaviorSet_Sniper(bState);
			G_CheckCharmed(NPC);
			return;
		}
		NPC_BehaviorSet_Tusken(bState);
		G_CheckCharmed(NPC);
	}
	else if (NPC->client->ps.weapon == WP_TUSKEN_STAFF)
	{
		NPC_BehaviorSet_Tusken(bState);
		G_CheckCharmed(NPC);
	}
	else if (NPC->client->ps.weapon == WP_NOGHRI_STICK)
	{
		NPC_BehaviorSet_Stormtrooper(bState);
		G_CheckCharmed(NPC);
	}
	else if (NPC->client->NPC_class == CLASS_OBJECT)
	{
		NPC_BehaviorSet_Object(bState);
	}
	else
	{
		switch (team)
		{
			// not sure if TEAM_ENEMY is appropriate here, I think I should be using NPC_class to check for behavior - dmv
		case TEAM_PROJECTION:
		case TEAM_ENEMY:
			// special cases for enemy droids
			switch (NPC->client->NPC_class)
			{
			case CLASS_ATST:
				NPC_BehaviorSet_ATST(bState);
				return;
			case CLASS_PROBE:
				NPC_BehaviorSet_ImperialProbe(bState);
				return;
			case CLASS_REMOTE:
				NPC_BehaviorSet_Remote(bState);
				return;
			case CLASS_SENTRY:
				NPC_BehaviorSet_Sentry(bState);
				return;
			case CLASS_INTERROGATOR:
				NPC_BehaviorSet_Interrogator(bState);
				return;
			case CLASS_MINEMONSTER:
				NPC_BehaviorSet_MineMonster(bState);
				return;
			case CLASS_HOWLER:
				NPC_BehaviorSet_Howler(bState);
				return;
			case CLASS_RANCOR:
				NPC_BehaviorSet_Rancor(bState);
				return;
			case CLASS_SAND_CREATURE:
				NPC_BehaviorSet_SandCreature(bState);
				return;
			case CLASS_MARK1:
				NPC_BehaviorSet_Mark1(bState);
				return;
			case CLASS_MARK2:
				NPC_BehaviorSet_Mark2(bState);
				return;
			case CLASS_GALAKMECH:
				NPC_BSGM_Default();
				return;
			default:
				break;
			}

			if (NPC->client->NPC_class == CLASS_ASSASSIN_DROID)
			{
				BubbleShield_Update();
			}

			if (NPC_IsTrooper(NPC))
			{
				NPC_BehaviorSet_Trooper(bState);
				return;
			}

			if (NPC->enemy && NPC->client->ps.weapon == WP_NONE && bState != BS_HUNT_AND_KILL && !Q3_TaskIDPending(
				NPC, TID_MOVE_NAV))
			{
				//if in battle and have no weapon, run away, fixme: when in BS_HUNT_AND_KILL, they just stand there
				if (bState != BS_FLEE)
				{
					NPC_StartFlee(NPC->enemy, NPC->enemy->currentOrigin, AEL_DANGER_GREAT, 5000, 10000);
				}
				else
				{
					NPC_BSFlee();
				}
				return;
			}
			if (NPC->client->ps.weapon == WP_SABER)
			{
				//special melee exception
				NPC_BehaviorSet_Default(bState);
				return;
			}
			if (NPC->client->ps.weapon == WP_DISRUPTOR && NPCInfo->scriptFlags & SCF_ALT_FIRE)
			{
				//a sniper
				NPC_BehaviorSet_Sniper(bState);
				return;
			}
			if (NPC->client->ps.weapon == WP_THERMAL
				|| NPC->client->ps.weapon == WP_MELEE) //FIXME: separate AI for melee fighters
			{
				//a grenadier
				NPC_BehaviorSet_Grenadier(bState);
				return;
			}
			if (NPC_CheckSurrender())
			{
				return;
			}
			NPC_BehaviorSet_Stormtrooper(bState);
			break;
		case TEAM_NEUTRAL:

			// special cases for enemy droids
			if (NPC->client->NPC_class == CLASS_PROTOCOL)
			{
				NPC_BehaviorSet_Default(bState);
			}
			else if (NPC->client->NPC_class == CLASS_UGNAUGHT || NPC->client->NPC_class == CLASS_JAWA)
			{
				//others, too?
				NPC_BSCivilian_Default(bState);
			}
			// Add special vehicle behavior here.
			else if (NPC->client->NPC_class == CLASS_VEHICLE)
			{
				const Vehicle_t* pVehicle = NPC->m_pVehicle;
				if (!pVehicle->m_pPilot && pVehicle->m_iBoarding == 0)
				{
					if (pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
					{
						NPC_BehaviorSet_Animal(bState);
					}
				}
			}
			else
			{
				// Just one of the average droids
				NPC_BehaviorSet_Droid(bState);
			}
			break;

		default:
			if (NPC->client->NPC_class == CLASS_SEEKER)
			{
				NPC_BehaviorSet_Seeker(bState);
			}
			else
			{
				if (NPCInfo->charmedTime > level.time)
				{
					NPC_BehaviorSet_Charmed(bState);
				}
				else
				{
					NPC_BehaviorSet_Default(bState);
				}
				G_CheckCharmed(NPC);
			}
			break;
		}
	}
}

static bState_t G_CurrentBState(gNPC_t* gNPC)
{
	if (gNPC->tempBehavior != BS_DEFAULT)
	{
		//Overrides normal behavior until cleared
		return gNPC->tempBehavior;
	}

	if (gNPC->behaviorState == BS_DEFAULT)
	{
		gNPC->behaviorState = gNPC->defaultBehavior;
	}

	return gNPC->behaviorState;
}

/*
===============
NPC_ExecuteBState

  MCG

NPC Behavior state thinking

===============
*/
void NPC_ExecuteBState(const gentity_t* self) //, int msec )
{
	NPC_HandleAIFlags();

	//FIXME: these next three bits could be a function call, some sort of setup/cleanup func
	//Lookmode must be reset every think cycle
	if (NPC->delayScriptTime && NPC->delayScriptTime <= level.time)
	{
		G_ActivateBehavior(NPC, BSET_DELAYED);
		NPC->delayScriptTime = 0;
	}

	//Clear this and let bState set it itself, so it automatically handles changing bStates... but we need a set bState wrapper func
	NPCInfo->combatMove = qfalse;

	//Execute our bState
	const bState_t bState = G_CurrentBState(NPCInfo);

	//Pick the proper bstate for us and run it
	NPC_RunBehavior(self->client->playerTeam, bState);

	//	if(bState != BS_POINT_COMBAT && NPCInfo->combatPoint != -1)
	//	{
	//level.combatPoints[NPCInfo->combatPoint].occupied = qfalse;
	//NPCInfo->combatPoint = -1;
	//	}

	//Here we need to see what the scripted stuff told us to do
	//Only process snapshot if independant and in combat mode- this would pick enemies and go after needed items
	//	ProcessSnapshot();

	//Ignore my needs if I'm under script control- this would set needs for items
	//	CheckSelf();

	//Back to normal?  All decisions made?

	//FIXME: don't walk off ledges unless we can get to our goal faster that way, or that's our goal's surface
	//NPCPredict();

	if (NPC->enemy)
	{
		if (!NPC->enemy->inuse)
		{
			//just in case bState doesn't catch this
			G_ClearEnemy(NPC);
		}
	}

	if (NPC->client->ps.saberLockTime && NPC->client->ps.saberLockEnemy != ENTITYNUM_NONE)
	{
		NPC_SetLookTarget(NPC, NPC->client->ps.saberLockEnemy, level.time + 1000);
	}
	else if (!NPC_CheckLookTarget(NPC))
	{
		if (NPC->enemy)
		{
			NPC_SetLookTarget(NPC, NPC->enemy->s.number, 0);
		}
	}

	if (NPC->enemy)
	{
		if (NPC->enemy->flags & FL_DONT_SHOOT)
		{
			ucmd.buttons &= ~BUTTON_ATTACK;
			ucmd.buttons &= ~BUTTON_ALT_ATTACK;
		}
		else if (NPC->client->playerTeam != TEAM_ENEMY //not an enemy
			&& (NPC->client->playerTeam != TEAM_FREE || NPC->client->NPC_class == CLASS_TUSKEN && Q_irand(0, 4))
			//not a rampaging creature or I'm a tusken and I feel generous (temporarily)
			&& NPC->enemy->NPC
			&& (NPC->enemy->NPC->surrenderTime > level.time || NPC->enemy->NPC->scriptFlags & SCF_FORCED_MARCH))
		{
			//don't shoot someone who's surrendering if you're a good guy
			ucmd.buttons &= ~BUTTON_ATTACK;
			ucmd.buttons &= ~BUTTON_ALT_ATTACK;
		}

		if (client->ps.weaponstate == WEAPON_IDLE)
		{
			client->ps.weaponstate = WEAPON_READY;
		}
	}
	else
	{
		if (client->ps.weaponstate == WEAPON_READY)
		{
			client->ps.weaponstate = WEAPON_IDLE;
		}
	}

	if (!(ucmd.buttons & BUTTON_ATTACK) && NPC->attackDebounceTime > level.time)
	{
		//We just shot but aren't still shooting, so hold the gun up for a while
		if (client->ps.weapon == WP_SABER)
		{
			//One-handed
			NPC_SetAnim(NPC, SETANIM_TORSO, TORSO_WEAPONREADY1, SETANIM_FLAG_NORMAL);
		}
		else if (client->ps.weapon == WP_BRYAR_PISTOL)
		{
			//Sniper pose
			NPC_SetAnim(NPC, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL);
		}
		else if (client->ps.weapon == WP_SBD_BLASTER)
		{
			//Sniper pose
			NPC_SetAnim(NPC, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL);
		}
		else if (client->ps.weapon == WP_JAWA)
		{
			//Sniper pose
			NPC_SetAnim(NPC, SETANIM_TORSO, TORSO_WEAPONREADY3, SETANIM_FLAG_NORMAL);
		}
	}

	NPC_CheckAttackHold();
	NPC_ApplyScriptFlags();

	//cliff and wall avoidance
	NPC_AvoidWallsAndCliffs();

	// run the bot through the server like it was a real client
	//=== Save the ucmd for the second no-think Pmove ============================
	ucmd.serverTime = level.time - 50;
	memcpy(&NPCInfo->last_ucmd, &ucmd, sizeof(usercmd_t));
	if (!NPCInfo->attackHoldTime)
	{
		NPCInfo->last_ucmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS);
		//so we don't fire twice in one think
	}
	//============================================================================
	NPC_CheckAttackScript();
	NPC_KeepCurrentFacing();

	if (!NPC->next_roff_time || NPC->next_roff_time < level.time)
	{
		//If we were following a roff, we don't do normal pmoves.
		ClientThink(NPC->s.number, &ucmd);
	}
	else
	{
		NPC_ApplyRoff();
	}

	// end of thinking cleanup
	NPCInfo->touchedByPlayer = nullptr;

	NPC_CheckPlayerAim();
	NPC_CheckAllClear();
}

void NPC_CheckInSolid()
{
	trace_t trace;
	vec3_t point;
	VectorCopy(NPC->currentOrigin, point);
	point[2] -= 0.25;

	gi.trace(&trace, NPC->currentOrigin, NPC->mins, NPC->maxs, point, NPC->s.number, NPC->clipmask,
		static_cast<EG2_Collision>(0), 0);
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy(NPC->currentOrigin, NPCInfo->lastClearOrigin);
	}
	else
	{
		if (VectorLengthSquared(NPCInfo->lastClearOrigin))
		{
			//			gi.Printf("%s stuck in solid at %s: fixing...\n", NPC->script_targetname, vtos(NPC->currentOrigin));
			G_SetOrigin(NPC, NPCInfo->lastClearOrigin);
			gi.linkentity(NPC);
		}
	}
}

/*
===============
NPC_Think

Main NPC AI - called once per frame
===============
*/
#if	AI_TIMERS
extern int AITime;
#endif//	AI_TIMERS
void NPC_Think(gentity_t* ent) //, int msec )
{
	vec3_t oldMoveDir;

	ent->nextthink = level.time + FRAMETIME / 2;

	SetNPCGlobals(ent);

	memset(&ucmd, 0, sizeof ucmd);

	VectorCopy(ent->client->ps.moveDir, oldMoveDir);
	VectorClear(ent->client->ps.moveDir);

	if (ent->client->playerTeam == g_entities[0].client->playerTeam &&
		Distance(ent->currentOrigin, g_entities[0].currentOrigin) <= 1024.0f)
	{
		g_entities[0].nearAllies = ent->s.number;
	}

	// see if NPC ai is frozen
	if (debugNPCFreeze->integer || NPC->svFlags & SVF_ICARUS_FREEZE || ent && ent->client && ent->client->ps.
		stasisTime > level.time)
	{
		NPC_UpdateAngles(qtrue, qtrue);
		ClientThink(ent->s.number, &ucmd);
		VectorCopy(ent->s.origin, ent->s.origin2);
		return;
	}
	if (debugNPCFreeze->integer || NPC->svFlags & SVF_ICARUS_FREEZE || ent && ent->client && ent->client->ps.
		stasisJediTime > level.time)
	{
		NPC_UpdateAngles(qtrue, qtrue);
		ClientThink(ent->s.number, &ucmd);
		VectorCopy(ent->s.origin, ent->s.origin2);
		return;
	}

	if (!ent || !ent->NPC || !ent->client)
	{
		return;
	}

	// dead NPCs have a special think, don't run scripts (for now)
	//FIXME: this breaks deathscripts
	if (ent->health <= 0)
	{
		DeadThink();
		if (NPCInfo->nextBStateThink <= level.time)
		{
			if (ent->m_iIcarusID != IIcarusInterface::ICARUS_INVALID && !stop_icarus)
			{
				IIcarusInterface::GetIcarus()->Update(ent->m_iIcarusID);
			}
		}
		return;
	}

	// TODO! Tauntaun's (and other creature vehicles?) think, we'll need to make an exception here to allow that.

	if (ent->client
		&& ent->client->NPC_class == CLASS_VEHICLE
		//&& self->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER && self->m_pVehicle->m_pVehicleInfo->type != VH_WALKER && self->m_pVehicle->m_pVehicleInfo->type != VH_ANIMAL
		&& ent->NPC_type
		&& !ent->m_pVehicle->m_pVehicleInfo->Inhabited(ent->m_pVehicle))
	{
		//empty swoop logic
		if (ent->owner)
		{
			//still have attached owner, check and see if can forget him (so he can use me later)
			vec3_t dir2_owner;
			VectorSubtract(ent->owner->currentOrigin, ent->currentOrigin, dir2_owner);

			gentity_t* old_owner = ent->owner;
			ent->owner = nullptr; //clear here for that SpotWouldTelefrag check...?

			if (VectorLengthSquared(dir2_owner) > 128 * 128
				|| !(ent->clipmask & old_owner->clipmask)
				|| DotProduct(ent->client->ps.velocity, old_owner->client->ps.velocity) < -200.0f && !G_BoundsOverlap(
					ent->absmin, ent->absmin, old_owner->absmin, old_owner->absmax))
			{
				//all clear, become solid to our owner now
				gi.linkentity(ent);
			}
			else
			{
				//blocked, retain owner
				ent->owner = old_owner;
			}
		}
	}
	if (player->client->ps.viewEntity == ent->s.number)
	{
		//being controlled by player
		if (ent->client)
		{
			//make the noises
			if (TIMER_Done(ent, "patrolNoise") && !Q_irand(0, 20))
			{
				switch (ent->client->NPC_class)
				{
				case CLASS_R2D2: // droid
					G_SoundOnEnt(ent, CHAN_AUTO, va("sound/chars/r2d2/misc/r2d2talk0%d.wav", Q_irand(1, 3)));
					break;
				case CLASS_R5D2: // droid
					G_SoundOnEnt(ent, CHAN_AUTO, va("sound/chars/r5d2/misc/r5talk%d.wav", Q_irand(1, 4)));
					break;
				case CLASS_PROBE: // droid
					G_SoundOnEnt(ent, CHAN_AUTO, va("sound/chars/probe/misc/probetalk%d.wav", Q_irand(1, 3)));
					break;
				case CLASS_MOUSE: // droid
					G_SoundOnEnt(ent, CHAN_AUTO, va("sound/chars/mouse/misc/mousego%d.wav", Q_irand(1, 3)));
					break;
				case CLASS_GONK: // droid
					G_SoundOnEnt(ent, CHAN_AUTO, va("sound/chars/gonk/misc/gonktalk%d.wav", Q_irand(1, 2)));
					break;
				default:
					break;
				}
				TIMER_Set(ent, "patrolNoise", Q_irand(2000, 4000));
			}
		}
		//FIXME: might want to at least make sounds or something?
		//NPC_UpdateAngles(qtrue, qtrue);
		//Which ucmd should we send?  Does it matter, since it gets overridden anyway?
		NPCInfo->last_ucmd.serverTime = level.time - 50;
		ClientThink(NPC->s.number, &ucmd);
		VectorCopy(ent->s.origin, ent->s.origin2);
		return;
	}

	if (NPCInfo->nextBStateThink <= level.time)
	{
#if	AI_TIMERS
		int	startTime = GetTime(0);
#endif//	AI_TIMERS
		if (NPC->s.eType != ET_PLAYER)
		{
			//Something drastic happened in our script
			return;
		}

		if (NPC->s.weapon == WP_SABER && NPC->client->ps.SaberActive())
		{
			//Jedi think faster
			NPCInfo->nextBStateThink = level.time + FRAMETIME / 2;
		}
		else
		{
			//Maybe even 200 ms?
			//NPCInfo->nextBStateThink = level.time + FRAMETIME;
			NPCInfo->nextBStateThink = level.time + FRAMETIME / 2;
		}

		//nextthink is set before this so something in here can override it
		NPC_ExecuteBState(ent);

#if	AI_TIMERS
		int addTime = GetTime(startTime);
		if (addTime > 50)
		{
			gi.Printf(S_COLOR_RED"ERROR: NPC number %d, %s %s at %s, weaponnum: %d, using %d of AI time!!!\n", NPC->s.number, NPC->NPC_type, NPC->targetname, vtos(NPC->currentOrigin), NPC->s.weapon, addTime);
		}
		AITime += addTime;
#endif//	AI_TIMERS
	}
	else
	{
		if (NPC->client
			&& NPC->client->NPC_class == CLASS_ROCKETTROOPER
			&& (NPC->client->ps.eFlags & EF_FORCE_GRIPPED || NPC->client->ps.eFlags & EF_FORCE_GRASPED)
			&& NPC->client->moveType == MT_FLYSWIM
			&& NPC->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			//reduce velocity
			VectorScale(NPC->client->ps.velocity, 0.75f, NPC->client->ps.velocity);
		}
		VectorCopy(oldMoveDir, ent->client->ps.moveDir);
		//or use client->pers.lastCommand?
		NPCInfo->last_ucmd.serverTime = level.time - 50;
		if (!NPC->next_roff_time || NPC->next_roff_time < level.time)
		{
			//If we were following a roff, we don't do normal pmoves.
			//FIXME: firing angles (no aim offset) or regular angles?
			NPC_UpdateAngles(qtrue, qtrue);
			memcpy(&ucmd, &NPCInfo->last_ucmd, sizeof(usercmd_t));
			ClientThink(NPC->s.number, &ucmd);
		}
		else
		{
			NPC_ApplyRoff();
		}
		VectorCopy(ent->s.origin, ent->s.origin2);
	}
	//must update icarus *every* frame because of certain animation completions in the pmove stuff that can leave a 50ms gap between ICARUS animation commands
	if (ent->m_iIcarusID != IIcarusInterface::ICARUS_INVALID && !stop_icarus)
	{
		IIcarusInterface::GetIcarus()->Update(ent->m_iIcarusID);
	}
}

void NPC_InitAI(void)
{
	debugNPCAI = gi.cvar("d_npcai", "0", CVAR_CHEAT);
	debugNPCFreeze = gi.cvar("d_npcfreeze", "0", CVAR_CHEAT);
	d_JediAI = gi.cvar("d_JediAI", "0", CVAR_CHEAT);
	d_noGroupAI = gi.cvar("d_noGroupAI", "0", CVAR_CHEAT);
	d_asynchronousGroupAI = gi.cvar("d_asynchronousGroupAI", "1", CVAR_CHEAT);
	d_combatinfo = gi.cvar("d_combatinfo", "0", CVAR_ARCHIVE);
	d_SaberactionInfo = gi.cvar("d_SaberactionInfo", "0", CVAR_ARCHIVE);
	d_blockinfo = gi.cvar("d_blockinfo", "0", CVAR_ARCHIVE);
	d_attackinfo = gi.cvar("d_attackinfo", "0", CVAR_ARCHIVE);
	d_saberinfo = gi.cvar("d_saberinfo", "0", CVAR_ARCHIVE);

	//0 = never (BORING)
	//1 = kyle only
	//2 = kyle and last enemy jedi
	//3 = kyle and any enemy jedi
	//4 = kyle and last enemy in a group, special kicks
	//5 = kyle and any enemy
	//6 = also when kyle takes pain or enemy jedi dodges player saber swing or does an acrobatic evasion
	// NOTE : I also create this in UI_Init()
	d_slowmodeath = gi.cvar("d_slowmodeath", "3", CVAR_ARCHIVE); //save this setting

	d_saberCombat = gi.cvar("d_saberCombat", "0", CVAR_CHEAT);

	d_slowmoaction = gi.cvar("d_slowmoaction", "0", CVAR_ARCHIVE); //save this setting
}

/*
==================================
void NPC_InitAnimTable( void )

  Need to initialize this table.
  If someone tried to play an anim
  before table is filled in with
  values, causes tasks that wait for
  anim completion to never finish.
  (frameLerp of 0 * numFrames of 0 = 0)
==================================
*/
void NPC_InitAnimTable(void)
{
	for (auto& knownAnimFileSet : level.knownAnimFileSets)
	{
		for (auto& animation : knownAnimFileSet.animations)
		{
			animation.firstFrame = 0;
			animation.frameLerp = 100;
			//			level.knownAnimFileSets[i].animations[j].initialLerp = 100;
			animation.numFrames = 0;
		}
	}
}

extern int G_ParseAnimFileSet(const char* skeletonName, const char* modelName = nullptr);

void NPC_InitGame()
{
	//	globals.NPCs = (gNPC_t *) gi.TagMalloc(game.maxclients * sizeof(game.bots[0]), TAG_GAME);
	debugNPCName = gi.cvar("d_npc", "", 0);
	NPC_LoadParms();
	NPC_InitAI();
	NPC_InitAnimTable();
	G_ParseAnimFileSet("_humanoid"); //GET THIS CACHED NOW BEFORE CGAME STARTS
	/*
	ResetTeamCounters();
	for ( int team = TEAM_FREE; team < TEAM_NUM_TEAMS; team++ )
	{
		teamLastEnemyTime[team] = -10000;
	}
	*/
}

void NPC_SetAnim(gentity_t* ent, int setAnimParts, const int anim, const int setAnimFlags, const int iBlend)
{
	// FIXME : once torsoAnim and legsAnim are in the same structure for NCP and Players
	// rename PM_SETAnimFinal to PM_SetAnim and have both NCP and Players call PM_SetAnim

	if (!ent)
	{
		return;
	}

	if (ent->health > 0)
	{
		//don't lock anims if the guy is dead
		if (ent->client->ps.torsoAnimTimer
			&& PM_LockedAnim(ent->client->ps.torsoAnim)
			&& !PM_LockedAnim(anim))
		{
			//nothing can override these special anims
			setAnimParts &= ~SETANIM_TORSO;
		}

		if (ent->client->ps.legsAnimTimer
			&& PM_LockedAnim(ent->client->ps.legsAnim)
			&& !PM_LockedAnim(anim))
		{
			//nothing can override these special anims
			setAnimParts &= ~SETANIM_LEGS;
		}
	}

	if (!setAnimParts)
	{
		return;
	}

	if (ent->client->ps.stasisTime > level.time)
	{
		return;
	}

	if (ent->client->ps.stasisJediTime > level.time)
	{
		return;
	}

	if (ent->client)
	{
		//Players, NPCs
		if (setAnimFlags & SETANIM_FLAG_OVERRIDE)
		{
			if (setAnimParts & SETANIM_TORSO)
			{
				if (setAnimFlags & SETANIM_FLAG_RESTART || ent->client->ps.torsoAnim != anim)
				{
					PM_SetTorsoAnimTimer(ent, &ent->client->ps.torsoAnimTimer, 0);
				}
			}
			if (setAnimParts & SETANIM_LEGS)
			{
				if (setAnimFlags & SETANIM_FLAG_RESTART || ent->client->ps.legsAnim != anim)
				{
					PM_SetLegsAnimTimer(ent, &ent->client->ps.legsAnimTimer, 0);
				}
			}
		}

		PM_SetAnimFinal(&ent->client->ps.torsoAnim, &ent->client->ps.legsAnim, setAnimParts, anim, setAnimFlags,
			&ent->client->ps.torsoAnimTimer, &ent->client->ps.legsAnimTimer, ent, iBlend);
	}
	else
	{
		//bodies, etc.
		if (setAnimFlags & SETANIM_FLAG_OVERRIDE)
		{
			if (setAnimParts & SETANIM_TORSO)
			{
				if (setAnimFlags & SETANIM_FLAG_RESTART || ent->s.torsoAnim != anim)
				{
					PM_SetTorsoAnimTimer(ent, &ent->s.torsoAnimTimer, 0);
				}
			}
			if (setAnimParts & SETANIM_LEGS)
			{
				if (setAnimFlags & SETANIM_FLAG_RESTART || ent->s.legsAnim != anim)
				{
					PM_SetLegsAnimTimer(ent, &ent->s.legsAnimTimer, 0);
				}
			}
		}

		PM_SetAnimFinal(&ent->s.torsoAnim, &ent->s.legsAnim, setAnimParts, anim, setAnimFlags,
			&ent->s.torsoAnimTimer, &ent->s.legsAnimTimer, ent);
	}
}

qboolean npc_is_mando(const gentity_t* self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_ROCKETTROOPER:
	case CLASS_BOBAFETT:
	case CLASS_JANGO:
	case CLASS_JANGODUAL:
	case CLASS_MANDALORIAN:
		// Is Jedi...
		return qtrue;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}

qboolean NPC_IsOversized(const gentity_t* self)
{
	switch (self->client->NPC_class)
	{
	case CLASS_WOOKIE:
	case CLASS_SBD:
	case CLASS_ASSASSIN_DROID:
	case CLASS_HAZARD_TROOPER:
	case CLASS_ROCKETTROOPER:
	case CLASS_SABER_DROID:
	case CLASS_VADER:
		// Is Jedi...
		return qtrue;
	default:
		// NOT Jedi...
		break;
	}

	return qfalse;
}