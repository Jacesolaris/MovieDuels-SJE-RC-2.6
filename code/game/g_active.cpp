/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

#include "g_local.h"
#include "g_functions.h"
#include "../cgame/cg_local.h"
#include "Q3_Interface.h"
#include "wp_saber.h"
#include "g_vehicles.h"
#include "b_local.h"

#ifdef _DEBUG
#include <float.h>
#endif //_DEBUG

constexpr auto SLOWDOWN_DIST = 128.0f;
constexpr auto MIN_NPC_SPEED = 16.0f;

extern qboolean Q3_TaskIDPending(const gentity_t* ent, taskID_t taskType);
extern qboolean Manual_NPCKickAbsorbing(const gentity_t* defender);
extern void TryUse(gentity_t* ent);
extern void ChangeWeapon(const gentity_t* ent, int newWeapon);
extern void ScoreBoardReset(void);
extern void G_Stumble(gentity_t* hit_ent);
extern void WP_SaberUpdateMD(gentity_t* self, const usercmd_t* ucmd);
extern void WP_SaberUpdateJKA(gentity_t* self, const usercmd_t* ucmd);
extern void wp_saber_start_missile_block_check(gentity_t* self, const usercmd_t* ucmd);
extern void wp_saber_start_missile_block_check_md(gentity_t* self, const usercmd_t* ucmd);
extern void WP_ForcePowersUpdate(gentity_t* self, usercmd_t* ucmd);
extern qboolean PM_SaberInMassiveBounce(int move);
extern qboolean InFront(vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold = 0.0f);
extern float DotToSpot(vec3_t spot, vec3_t from, vec3_t fromAngles);
extern void NPC_SetLookTarget(const gentity_t* self, int entNum, int clearTime);
extern qboolean PM_LockAngles(gentity_t* ent, usercmd_t* ucmd);
extern qboolean PM_AdjustAnglesToGripper(gentity_t* gent, usercmd_t* cmd);
extern qboolean PM_AdjustAnglesToPuller(gentity_t* ent, const gentity_t* puller, usercmd_t* ucmd, qboolean face_away);
extern qboolean PM_AdjustAngleForWallRun(gentity_t* ent, usercmd_t* ucmd, qboolean do_move);
extern qboolean PM_AdjustAngleForWallRunUp(gentity_t* ent, usercmd_t* ucmd, qboolean do_move);
extern qboolean PM_AdjustAnglesForSpinningFlip(gentity_t* ent, usercmd_t* ucmd, qboolean angles_only);
extern qboolean PM_AdjustAnglesForBackAttack(gentity_t* ent, usercmd_t* ucmd);
extern qboolean PM_AdjustAnglesForSaberLock(gentity_t* ent, usercmd_t* ucmd);
extern qboolean PM_AdjustAnglesForKnockdown(gentity_t* ent, usercmd_t* ucmd, qboolean angle_clamp_only);
extern qboolean PM_AdjustAnglesForDualJumpAttack(gentity_t* ent, usercmd_t* ucmd);
extern qboolean PM_AdjustAnglesForLongJump(gentity_t* ent, usercmd_t* ucmd);
extern qboolean PM_AdjustAnglesForGrapple(gentity_t* ent, usercmd_t* ucmd);
extern qboolean PM_AdjustAngleForWallJump(gentity_t* ent, usercmd_t* ucmd, qboolean do_move);
extern qboolean PM_AdjustAnglesForBFKick(gentity_t* ent, usercmd_t* ucmd, vec3_t fwd_angs, qboolean aim_front);
extern qboolean PM_AdjustAnglesForStabDown(gentity_t* ent, usercmd_t* ucmd);
extern qboolean PM_AdjustAnglesForSpinProtect(gentity_t* ent, usercmd_t* ucmd);
extern qboolean PM_AdjustAnglesForWallRunUpFlipAlt(gentity_t* ent, usercmd_t* ucmd);
extern qboolean PM_AdjustAnglesForHeldByMonster(gentity_t* ent, const gentity_t* monster, usercmd_t* ucmd);
extern qboolean PM_HasAnimation(const gentity_t* ent, int animation);
extern qboolean PM_LeapingSaberAnim(int anim);
extern qboolean PM_SpinningSaberAnim(int anim);
extern qboolean PM_WalkingOrRunningAnim(int anim);
extern qboolean PM_SaberInAttack(int move);
extern qboolean PM_KickingAnim(int anim);
extern int PM_AnimLength(int index, animNumber_t anim);
extern qboolean PM_InKnockDown(const playerState_t* ps);
extern qboolean PM_InGetUp(const playerState_t* ps);
extern qboolean PM_InRoll(const playerState_t* ps);
extern void PM_CmdForRoll(playerState_t* ps, usercmd_t* pCmd);
extern qboolean PM_InAttackRoll(int anim);
extern qboolean PM_CrouchAnim(int anim);
extern qboolean PM_FlippingAnim(int anim);
extern qboolean PM_InCartwheel(int anim);
extern qboolean In_LedgeIdle(int anim);
extern qboolean PM_StandingAnim(int anim);
extern qboolean PM_InForceGetUp(const playerState_t* ps);
extern qboolean PM_GetupAnimNoMove(int legsAnim);
extern qboolean PM_SuperBreakLoseAnim(int anim);
extern qboolean PM_SuperBreakWinAnim(int anim);
extern qboolean PM_CanRollFromSoulCal(const playerState_t* ps);
extern qboolean BG_FullBodyTauntAnim(int anim);
extern qboolean FlyingCreature(const gentity_t* ent);
extern Vehicle_t* G_IsRidingVehicle(const gentity_t* pEnt);
extern void G_AttachToVehicle(gentity_t* ent, usercmd_t** ucmd);
extern void G_GetBoltPosition(gentity_t* self, int boltIndex, vec3_t pos, int modelIndex = 0);
extern void G_UpdateEmplacedWeaponData(gentity_t* ent);
extern void RunEmplacedWeapon(gentity_t* ent, usercmd_t** ucmd);
extern qboolean G_PointInBounds(const vec3_t point, const vec3_t mins, const vec3_t maxs);
extern void NPC_SetPainEvent(gentity_t* self);
extern qboolean G_HasKnockdownAnims(const gentity_t* ent);
extern int G_GetEntsNearBolt(gentity_t* self, gentity_t** radiusEnts, float radius, int boltIndex, vec3_t boltOrg);
extern qboolean PM_InOnGroundAnim(playerState_t* ps);
extern qboolean PM_LockedAnim(int anim);
extern qboolean WP_SabersCheckLock2(gentity_t* attacker, gentity_t* defender, sabersLockMode_t lock_mode);
extern qboolean G_JediInNormalAI(const gentity_t* ent);
extern void WP_ForcePowerDrain(const gentity_t* self, forcePowers_t force_power, int override_amt);
extern qboolean manual_saberblocking(const gentity_t* defender);
extern qboolean Manual_RunningAndSaberblocking(const gentity_t* defender);
extern qboolean Manual_NPCSaberblocking(const gentity_t* defender);
extern qboolean NPC_Can_Do_Blocking_stances_In_SJE_Mode(const gentity_t* defender);
extern qboolean Manual_JKAMode_NPCSaberblocking(const gentity_t* defender);
extern qboolean BG_InSlowBounce(const playerState_t* ps);
extern int BG_InGrappleMove(int move);
extern qboolean WalkCheck(const gentity_t* self);
extern qboolean PM_SaberInBrokenParry(int move);
extern void SabBeh_AnimateSlowBounce(gentity_t* self);
extern void G_Slapdown(gentity_t* self, gentity_t* attacker, const vec3_t pushDir, float strength,
	qboolean breakSaberLock);
extern qboolean PM_SaberInAttackPure(int move);
extern qboolean PM_SaberInTransitionAny(int move);
extern qboolean PM_Saberinstab(int move);
extern qboolean BG_FullBodyEmoteAnim(int anim);
extern qboolean BG_FullBodyCowerAnim(int anim);
extern qboolean PM_SaberInBashedAnim(int anim);
extern void WP_BlockPointsUpdate(const gentity_t* self);
extern qboolean PM_SaberInTransition(int move);
extern int IsPressingDestructButton(const gentity_t* self);
extern qboolean PM_Dyinganim(const playerState_t* ps);
extern qboolean npc_is_projected(const gentity_t* self);
extern qboolean Manual_MeleeDodging(const gentity_t* defender);
extern cvar_t* g_DebugSaberCombat;
extern int G_FindLocalInterestPoint(gentity_t* self);
extern float G_CanJumpToEnemyVeh(Vehicle_t* pVeh, const usercmd_t* pUmcd);
extern qboolean PM_RunningAnim(int anim);
extern qboolean PM_WalkingAnim(int anim);
extern qboolean PM_RollingAnim(int anim);
extern cvar_t* g_VaderBreath;
extern qboolean PM_LungeAnim(int anim);
extern qboolean BG_SprintAnim(int anim);
extern qboolean BG_SaberSprintAnim(int anim);
extern qboolean BG_WeaponSprintAnim(int anim);
extern void Weapon_GrapplingHook_Fire(gentity_t* ent);
extern qboolean JET_Flying(const gentity_t* self);
extern cvar_t* g_allowcallout;
extern qboolean PM_KickMove(int move);
extern void G_AddVoiceEvent(const gentity_t* self, int event, int speakDebounceTime);
extern void AddFatigueMeleeBonus(const gentity_t* attacker, const gentity_t* victim);
extern void WP_BlockPointsDrain(const gentity_t* self, int fatigue);
extern qboolean IsSurrendering(const gentity_t* self);
extern qboolean IsRespecting(const gentity_t* self);
extern qboolean IsCowering(const gentity_t* self);
extern qboolean is_anim_requires_responce(const gentity_t* self);
extern int IsPressingKickButton(const gentity_t* self);
extern qboolean IsSurrenderingAnimRequiresResponce(const gentity_t* self);
extern int BotCanAbsorbKick(const gentity_t* defender, const vec3_t push_dir);
extern qboolean SaberAttacking(const gentity_t* self);

extern qboolean BG_InKnockDown(int anim);
extern qboolean PM_SaberInSpecialAttack(int anim);
extern qboolean PM_SaberInBounce(int move);
extern qboolean PM_SaberInKnockaway(int move);
extern void G_Stagger(gentity_t* hit_ent);
extern void G_MawStagger(gentity_t* hit_ent);
extern void WP_DeactivateSaber(const gentity_t* self, qboolean clear_length = qfalse);
extern void G_StartStasisEffect_FORCE_LEVEL_1(const gentity_t* ent, int meFlags = 0, int length = 1000,
	float timeScale = 0.0f, int spinTime = 0);
extern cvar_t* d_slowmoaction;
extern int PlayerCanAbsorbKick(const gentity_t* defender, const vec3_t push_dir); //Can the player absorb a kick
extern qboolean G_InScriptedCinematicSaberAnim(const gentity_t* self);
extern qboolean jedi_is_kick_resistant(gentity_t* self);
extern void jedi_play_blocked_push_sound(const gentity_t* self);

extern qboolean PlayerAffectedByStasis();
extern qboolean PM_DodgeAnim(int anim);
extern qboolean PM_DodgeHoldAnim(int anim);
extern void Player_CheckBurn(const gentity_t* self);
extern void Player_CheckFreeze(gentity_t* self);
extern cvar_t* g_SerenityJediEngineMode;
extern int IsPressingDashButton(const gentity_t* self);
extern cvar_t* g_SaberPerfectBlockingTimerEasy;
extern cvar_t* g_SaberPerfectBlockingTimerNormal;
extern cvar_t* g_SaberPerfectBlockingTimerHard;
extern cvar_t* g_SaberMBlockingTimerwaitEasy;
extern cvar_t* g_SaberMBlockingTimerwaitNormal;
extern cvar_t* g_SaberMBlockingTimerwaitHard;

extern bool in_camera;
extern qboolean player_locked;
extern qboolean stop_icarus;
extern qboolean MatrixMode;

extern cvar_t* g_spskill;
extern cvar_t* g_timescale;
extern cvar_t* g_saberMoveSpeed;
extern cvar_t* g_speederControlScheme;
extern cvar_t* d_slowmodeath;
extern vmCvar_t cg_thirdPersonAlpha;
extern vmCvar_t cg_thirdPersonAutoAlpha;
extern qboolean PM_InForceFall();

void ClientEndPowerUps(const gentity_t* ent);

int G_FindLookItem(gentity_t* self)
{
	int bestEntNum = ENTITYNUM_NONE;
	gentity_t* entityList[MAX_GENTITIES];
	vec3_t center, mins, maxs, fwdangles, forward;
	constexpr float radius = 256;
	float bestRating = 0.0f;

	//FIXME: no need to do this in 1st person?
	fwdangles[1] = self->client->ps.viewangles[1];
	AngleVectors(fwdangles, forward, nullptr, nullptr);

	VectorCopy(self->currentOrigin, center);

	for (int i = 0; i < 3; i++)
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}
	const int numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	if (!numListedEntities)
	{
		return ENTITYNUM_NONE;
	}

	for (int e = 0; e < numListedEntities; e++)
	{
		vec3_t dir;
		const gentity_t* ent = entityList[e];

		if (!ent->item)
		{
			continue;
		}
		if (ent->s.eFlags & EF_NODRAW)
		{
			continue;
		}
		if (ent->spawnflags & 4/*ITMSF_MONSTER*/)
		{
			//NPCs only
			continue;
		}
		if (!BG_CanItemBeGrabbed(&ent->s, &self->client->ps))
		{
			//don't need it
			continue;
		}
		if (!gi.inPVS(self->currentOrigin, ent->currentOrigin))
		{
			//not even potentially visible
			continue;
		}

		if (!G_ClearLOS(self, self->client->renderInfo.eyePoint, ent))
		{
			//can't see him
			continue;
		}
		if (ent->item->giType == IT_WEAPON
			&& ent->item->giTag == WP_SABER)
		{
			//a weapon_saber pickup
			if (self->client->ps.dualSabers //using 2 sabers already
				|| self->client->ps.saber[0].saberFlags & SFL_TWO_HANDED) //using a 2-handed saber
			{
				//hands are full already, don't look at saber pickups
				continue;
			}
		}
		//rate him based on how close & how in front he is
		VectorSubtract(ent->currentOrigin, center, dir);
		float rating = 1.0f - VectorNormalize(dir) / radius;
		rating *= DotProduct(forward, dir);
		if (ent->item->giType == IT_HOLDABLE && ent->item->giTag == INV_SECURITY_KEY)
		{
			//security keys are of the highest importance
			rating *= 2.0f;
		}
		if (rating > bestRating)
		{
			bestEntNum = ent->s.number;
			bestRating = rating;
		}
	}
	return bestEntNum;
}

extern void CG_SetClientViewAngles(vec3_t angles, qboolean overrideViewEnt);

qboolean G_ClearViewEntity(gentity_t* ent)
{
	if (!ent->client->ps.viewEntity)
		return qfalse;

	if (ent->client->ps.viewEntity > 0 && ent->client->ps.viewEntity < ENTITYNUM_NONE)
	{
		if (&g_entities[ent->client->ps.viewEntity])
		{
			g_entities[ent->client->ps.viewEntity].svFlags &= ~SVF_BROADCAST;
			if (g_entities[ent->client->ps.viewEntity].NPC)
			{
				g_entities[ent->client->ps.viewEntity].NPC->controlledTime = 0;
				SetClientViewAngle(&g_entities[ent->client->ps.viewEntity],
					g_entities[ent->client->ps.viewEntity].currentAngles);
				G_SetAngles(&g_entities[ent->client->ps.viewEntity],
					g_entities[ent->client->ps.viewEntity].currentAngles);
				VectorCopy(g_entities[ent->client->ps.viewEntity].currentAngles,
					g_entities[ent->client->ps.viewEntity].NPC->lastPathAngles);
				g_entities[ent->client->ps.viewEntity].NPC->desiredYaw = g_entities[ent->client->ps.viewEntity].
					currentAngles[YAW];
			}
		}
		CG_SetClientViewAngles(ent->pos4, qtrue);
		SetClientViewAngle(ent, ent->pos4);
	}
	ent->client->ps.viewEntity = 0;
	return qtrue;
}

void G_SetViewEntity(gentity_t* self, gentity_t* viewEntity)
{
	if (!self || !self->client || !viewEntity)
	{
		return;
	}

	if (self->s.number == 0 && cg.zoomMode)
	{
		// yeah, it should really toggle them so it plays the end sound....
		cg.zoomMode = 0;
	}
	if (viewEntity->s.number == self->client->ps.viewEntity)
	{
		return;
	}
	//clear old one first
	G_ClearViewEntity(self);
	//set new one
	self->client->ps.viewEntity = viewEntity->s.number;
	viewEntity->svFlags |= SVF_BROADCAST;
	//remember current angles
	VectorCopy(self->client->ps.viewangles, self->pos4);
	if (viewEntity->client)
	{
		//vec3_t	clear = {0,0,0};
		CG_SetClientViewAngles(viewEntity->client->ps.viewangles, qtrue);
	}
	if (!self->s.number)
	{
		CG_CenterPrint("@SP_INGAME_EXIT_VIEW", SCREEN_HEIGHT * 0.95);
	}
}

qboolean G_ControlledByPlayer(const gentity_t* self)
{
	if (self && self->NPC && self->NPC->controlledTime > level.time)
	{
		//being controlled
		const gentity_t* controller = &g_entities[0];

		if (controller->client && controller->client->ps.viewEntity == self->s.number)
		{
			//we're the player's viewEntity
			return qtrue;
		}
	}
	return qfalse;
}

qboolean G_ControlledByNPC(const gentity_t* self)
{
	if (self->NPC && self->s.weapon == WP_SABER && self->client->ps.SaberActive())
	{
		//being controlled
		return qtrue;
	}
	return qfalse;
}

qboolean G_ValidateLookEnemy(gentity_t* self, gentity_t* enemy)
{
	if (!enemy)
	{
		return qfalse;
	}

	if (enemy->flags & FL_NOTARGET)
	{
		return qfalse;
	}

	if (enemy->s.eFlags & EF_NODRAW)
	{
		return qfalse;
	}

	if (!enemy || enemy == self || !enemy->inuse)
	{
		return qfalse;
	}
	if (!enemy->client || !enemy->NPC)
	{
		//not valid
		if (enemy->svFlags & SVF_NONNPC_ENEMY
			&& enemy->s.weapon == WP_TURRET
			&& enemy->noDamageTeam != self->client->playerTeam
			&& enemy->health > 0)
		{
			//a turret
			//return qtrue;
		}
		else
		{
			return qfalse;
		}
	}
	else
	{
		if (self->client->playerTeam != TEAM_FREE //evil player hates everybody
			&& enemy->client->playerTeam == self->client->playerTeam)
		{
			//on same team
			return qfalse;
		}

		const Vehicle_t* pVeh = G_IsRidingVehicle(self);
		if (pVeh && pVeh == enemy->m_pVehicle)
		{
			return qfalse;
		}

		if (enemy->health <= 0 && (level.time - enemy->s.time > 3000 || !InFront(
			enemy->currentOrigin, self->currentOrigin, self->client->ps.viewangles, 0.2f) || DistanceHorizontal(
				enemy->currentOrigin, self->currentOrigin) > 16384)) //>128
		{
			//corpse, been dead too long or too out of sight to be interesting
			if (!enemy->message)
			{
				return qfalse;
			}
		}
	}

	if ((!InFront(enemy->currentOrigin, self->currentOrigin, self->client->ps.viewangles, 0.0f) || !G_ClearLOS(
		self, self->client->renderInfo.eyePoint, enemy))
		&& (DistanceHorizontalSquared(enemy->currentOrigin, self->currentOrigin) > 65536 || fabs(
			enemy->currentOrigin[2] - self->currentOrigin[2]) > 384))
	{
		//(not in front or not clear LOS) & greater than 256 away
		return qfalse;
	}

	//LOS?

	return qtrue;
}

void G_ChooseLookEnemy(gentity_t* self, const usercmd_t* ucmd)
{
	//FIXME: should be a more intelligent way of doing this, like auto aim?
	//closest, most in front... did damage to... took damage from?  How do we know who the player is focusing on?
	gentity_t* bestEnt = nullptr;
	gentity_t* entityList[MAX_GENTITIES];
	vec3_t center, mins, maxs, fwdangles, forward;
	constexpr float radius = 256;
	float bestRating = 0.0f;

	//FIXME: no need to do this in 1st person?
	fwdangles[0] = 0; //Must initialize data!
	fwdangles[1] = self->client->ps.viewangles[1];
	fwdangles[2] = 0;
	AngleVectors(fwdangles, forward, nullptr, nullptr);

	VectorCopy(self->currentOrigin, center);

	for (int i = 0; i < 3; i++)
	{
		mins[i] = center[i] - radius;
		maxs[i] = center[i] + radius;
	}
	const int numListedEntities = gi.EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

	if (!numListedEntities)
	{
		//should we clear the enemy?
		return;
	}

	for (int e = 0; e < numListedEntities; e++)
	{
		vec3_t dir;
		gentity_t* ent = entityList[e];

		if (!gi.inPVS(self->currentOrigin, ent->currentOrigin))
		{
			//not even potentially visible
			continue;
		}

		if (!G_ValidateLookEnemy(self, ent))
		{
			//doesn't meet criteria of valid look enemy (don't check current since we would have done that before this func's call
			continue;
		}

		if (!G_ClearLOS(self, self->client->renderInfo.eyePoint, ent))
		{
			//can't see him
			continue;
		}
		//rate him based on how close & how in front he is
		VectorSubtract(ent->currentOrigin, center, dir);
		float rating = 1.0f - VectorNormalize(dir) / radius;
		rating *= DotProduct(forward, dir) + 1.0f;
		if (ent->health <= 0)
		{
			if (ucmd->buttons & BUTTON_ATTACK
				|| ucmd->buttons & BUTTON_ALT_ATTACK
				|| ucmd->buttons & BUTTON_FORCE_FOCUS)
			{
				//if attacking, don't consider dead enemies
				continue;
			}
			if (ent->message)
			{
				//keyholder
				rating *= 0.5f;
			}
			else
			{
				rating *= 0.1f;
			}
		}
		if (ent->s.weapon == WP_SABER)
		{
			rating *= 2.0f;
		}
		if (ent->enemy == self)
		{
			//he's mad at me, he's more important
			rating *= 2.0f;
		}
		else if (ent->NPC && ent->NPC->blockedSpeechDebounceTime > level.time - 6000)
		{
			//he's detected me, he's more important
			if (ent->NPC->blockedSpeechDebounceTime > level.time + 4000)
			{
				rating *= 1.5f;
			}
			else
			{
				//from 1.0f to 1.5f
				rating += rating * (static_cast<float>(ent->NPC->blockedSpeechDebounceTime - level.time) + 6000.0f) /
					20000.0f;
			}
		}
		/*
		if ( g_crosshairEntNum == ent && !self->enemy )
		{//we don't have an enemy and we are aiming at this guy
			rading *= 2.0f;
		}
		*/
		if (rating > bestRating)
		{
			bestEnt = ent;
			bestRating = rating;
		}
	}
	if (bestEnt)
	{
		self->enemy = bestEnt;
	}
}

/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback(const gentity_t* player)
{
	gclient_t* client = player->client;
	if (client->ps.pm_type == PM_DEAD)
	{
		return;
	}

	// total points of damage shot at the player this frame
	float count = client->damage_blood + client->damage_armor;
	if (count == 0)
	{
		return; // didn't take any damage
	}

	if (count > 255)
	{
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if (client->damage_fromWorld)
	{
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = false;
	}
	else
	{
		vec3_t angles;
		vectoangles(client->damage_from, angles);
		client->ps.damagePitch = angles[PITCH] / 360.0 * 256;
		client->ps.damageYaw = angles[YAW] / 360.0 * 256;
	}

	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
}

/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
extern qboolean PM_InAmputateMove(int anim);
extern void WP_ForcePowerStart(gentity_t* self, forcePowers_t force_power, int override_amt);

void P_WorldEffects(gentity_t* ent)
{
	int mouthContents = 0;

	if (ent->client->noclip)
	{
		ent->client->airOutTime = level.time + 12000; // don't need air
		return;
	}

	if (!in_camera)
	{
		if (gi.totalMapContents() & (CONTENTS_WATER | CONTENTS_SLIME))
		{
			mouthContents = gi.pointcontents(ent->client->renderInfo.eyePoint, ent->s.number);
		}
	}
	//
	// check for drowning
	//

	if (mouthContents & (CONTENTS_WATER | CONTENTS_SLIME))
	{
		if (ent->client->NPC_class == CLASS_SWAMPTROOPER)
		{
			//they have air tanks
			ent->client->airOutTime = level.time + 12000; // don't need air
			ent->damage = 2;
		}
		else if (ent->client->airOutTime < level.time)
		{
			// if out of air, start drowning
			// drown!
			ent->client->airOutTime += 1000;
			if (ent->health > 0)
			{
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// play a gurp sound instead of a normal pain sound
				if (ent->health <= ent->damage)
				{
					G_AddEvent(ent, EV_WATER_DROWN, 0);
				}
				else
				{
					G_AddEvent(ent, Q_irand(EV_WATER_GURP1, EV_WATER_GURP2), 0);
				}

				// don't play a normal pain sound
				ent->painDebounceTime = level.time + 200;

				G_Damage(ent, nullptr, nullptr, nullptr, nullptr,
					ent->damage, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	}
	else
	{
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (ent->waterlevel &&
		ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME))
	{
		if (ent->health > 0
			&& ent->painDebounceTime < level.time)
		{
			if (ent->watertype & CONTENTS_LAVA)
			{
				G_Damage(ent, nullptr, nullptr, nullptr, nullptr,
					15 * ent->waterlevel, 0, MOD_LAVA);
			}

			if (ent->watertype & CONTENTS_SLIME)
			{
				G_Damage(ent, nullptr, nullptr, nullptr, nullptr,
					1, 0, MOD_SLIME);
			}
		}
	}

	if (ent->health > 0 &&
		ent->painDebounceTime < level.time &&
		gi.WE_IsOutsideCausingPain(ent->currentOrigin) &&
		TIMER_Done(ent, "AcidPainDebounce"))
	{
		if (ent->NPC && ent->client && ent->client->ps.forcePowersKnown & 1 << FP_PROTECT)
		{
			if (!(ent->client->ps.forcePowersActive & 1 << FP_PROTECT))
			{
				WP_ForcePowerStart(ent, FP_PROTECT, 0);
			}
		}
		else
		{
			G_Damage(ent, nullptr, nullptr, nullptr, nullptr, 1, 0, MOD_SLIME);
		}
	}

	// Poisoned?
	if (ent->client->poisonDamage && ent->client->poisonTime < level.time)
	{
		ent->client->poisonDamage -= 2;
		ent->client->poisonTime = level.time + 1000;
		G_Damage(ent, nullptr, nullptr, nullptr, nullptr, 2, DAMAGE_NO_KNOCKBACK | DAMAGE_NO_ARMOR, MOD_UNKNOWN);

		if (ent->client->poisonDamage < 0)
		{
			ent->client->poisonDamage = 0;
		}
	}

	// stunned?
	if (ent->client->stunDamage && ent->client->stunTime < level.time)
	{
		ent->client->stunDamage -= 3;
		ent->client->stunTime = level.time + 1000;
		G_Damage(ent, nullptr, nullptr, nullptr, nullptr, 3, DAMAGE_NO_KNOCKBACK | DAMAGE_NO_ARMOR, MOD_ELECTROCUTE);

		if (ent->client->stunDamage < 0)
		{
			ent->client->stunDamage = 0;
		}
	}

	if (ent->client->AmputateDamage && ent->client->AmputateTime < level.time)
	{
		ent->client->AmputateDamage -= 2;
		ent->client->AmputateTime = level.time + 1000;

		if (ent->s.number && ent->health > 100)
		{
			G_Damage(ent, nullptr, nullptr, nullptr, nullptr, 200, DAMAGE_NO_KNOCKBACK | DAMAGE_NO_ARMOR, MOD_UNKNOWN);
		}
		else
		{
			G_Damage(ent, nullptr, nullptr, nullptr, nullptr, 25, DAMAGE_NO_KNOCKBACK | DAMAGE_NO_ARMOR, MOD_UNKNOWN);
		}

		if (ent->client->AmputateDamage < 0)
		{
			ent->client->AmputateDamage = 0;
		}
	}

	//in space?
	if (ent->client->inSpaceIndex && ent->client->inSpaceIndex != ENTITYNUM_NONE)
	{
		//we're in space, check for suffocating and for exiting
		gentity_t* spacetrigger = &g_entities[ent->client->inSpaceIndex];

		if (!spacetrigger->inuse ||
			!G_PointInBounds(ent->client->ps.origin, spacetrigger->absmin, spacetrigger->absmax))
		{
			//no longer in space then I suppose
			ent->client->inSpaceIndex = 0;
		}
		else
		{
			//check for suffocation
			if (ent->client->inSpaceSuffocation < level.time)
			{
				//suffocate!
				if (ent->health > 0 && ent->takedamage)
				{
					//if they're still alive..
					G_Damage(ent, spacetrigger, spacetrigger, nullptr, ent->client->ps.origin, Q_irand(20, 40),
						DAMAGE_NO_ARMOR, MOD_SUICIDE);

					if (ent->health > 0)
					{
						//did that last one kill them?
						//play the choking sound
						G_SoundOnEnt(ent, CHAN_VOICE, va("*choke%d.wav", Q_irand(1, 3)));

						if (ent->NPC && ent->health < 50)
						{
							if (ent->client->NPC_class == CLASS_OBJECT)
							{
								//
							}
							else
							{
								G_SoundOnEnt(ent, CHAN_BODY, "sound/weapons/force/gripdeath.mp3");
							}
						}

						if (ent->health > 70)
						{
							//make them grasp their throat
							NPC_SetAnim(ent, SETANIM_BOTH, BOTH_CHOKE4, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else if (ent->health > 50)
						{
							//make them grasp their throat
							NPC_SetAnim(ent, SETANIM_BOTH, BOTH_CHOKE3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							//make them grasp their throat
							NPC_SetAnim(ent, SETANIM_BOTH, BOTH_CHOKE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
				}

				ent->client->inSpaceSuffocation = level.time + Q_irand(1000, 2000);
			}
		}
	}
}

/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound(gentity_t* ent)
{
	//	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) )
	//		ent->s.loopSound = G_SoundIndex("sound/weapons/stasis/electricloop.wav");

	//	else
	//		ent->s.loopSound = 0;
}

//==============================================================
extern void G_Knockdown(gentity_t* self, gentity_t* attacker, const vec3_t pushDir, float strength,
	qboolean breakSaberLock);
extern void G_StartMatrixEffect(const gentity_t* ent, int meFlags = 0, int length = 1000, float timeScale = 0.0f,
	int spinTime = 0);

void G_GetMassAndVelocityForEnt(const gentity_t* ent, float* mass, vec3_t velocity)
{
	if (ent->client)
	{
		VectorCopy(ent->client->ps.velocity, velocity);
		*mass = ent->mass;
	}
	else
	{
		VectorCopy(ent->s.pos.trDelta, velocity);
		if (ent->s.pos.trType == TR_GRAVITY)
		{
			velocity[2] -= 0.25f * g_gravity->value;
		}
		if (!ent->mass)
		{
			*mass = 1;
		}
		else if (ent->mass <= 10)
		{
			*mass = 10;
		}
		else
		{
			*mass = ent->mass; ///10;
		}
	}
}

extern cvar_t* com_outcast;

void DoImpact(gentity_t* self, gentity_t* other, qboolean damageSelf, const trace_t* trace)
{
	float magnitude, my_mass;
	bool thrown = false;
	vec3_t velocity;
	const char* info = CG_ConfigString(CS_SERVERINFO);
	const char* s = Info_ValueForKey(info, "mapname");

	Vehicle_t* pSelfVeh = nullptr;
	const Vehicle_t* pOtherVeh = nullptr;

	// See if either of these guys are vehicles, if so, keep a pointer to the vehicle npc.
	if (self->client && self->client->NPC_class == CLASS_VEHICLE)
	{
		pSelfVeh = self->m_pVehicle;
	}
	if (other->client && other->client->NPC_class == CLASS_VEHICLE)
	{
		pOtherVeh = other->m_pVehicle;
	}

	G_GetMassAndVelocityForEnt(self, &my_mass, velocity);

	if (pSelfVeh)
	{
		magnitude = VectorLength(velocity) * pSelfVeh->m_pVehicleInfo->mass / 50.0f;
	}
	else
	{
		magnitude = VectorLength(velocity) * my_mass / 50;
	}

	// Check For Vehicle On Vehicle Impact (Ramming)
	//-----------------------------------------------
	if (pSelfVeh &&
		pSelfVeh->m_pVehicleInfo->type != VH_ANIMAL &&
		pSelfVeh->m_pVehicleInfo->type != VH_FIGHTER &&
		pOtherVeh &&
		pSelfVeh->m_pVehicleInfo == pOtherVeh->m_pVehicleInfo)
	{
		const gentity_t* attacker = self;
		const Vehicle_t* attackerVeh = pSelfVeh;
		gentity_t* victim = other;
		const Vehicle_t* victimVeh = pOtherVeh;

		// Is The Attacker Actually Not Attacking?
		//-----------------------------------------
		if (!(attackerVeh->m_ulFlags & VEH_STRAFERAM))
		{
			// Ok, So Is The Victim Actually Attacking?
			//------------------------------------------
			if (victimVeh->m_ulFlags & VEH_STRAFERAM)
			{
				// Ah, Ok.  Swap Who Is The Attacker Then
				//----------------------------------------
				attacker = other;
				attackerVeh = pOtherVeh;
				victim = self;
				victimVeh = pSelfVeh;
			}
			else
			{
				// No Attackers, So Stop
				//-----------------------
				attacker = victim = nullptr;
			}
		}

		if (attacker && victim)
		{
			//	float		maxMoveSpeed	= pSelfVeh->m_pVehicleInfo->speedMax;
			//	float		minLockingSpeed = maxMoveSpeed * 0.75;

			vec3_t attackerMoveDir;

			vec3_t victimMoveDir;
			vec3_t victimTowardAttacker;
			vec3_t victimRight;

			VectorCopy(attacker->client->ps.velocity, attackerMoveDir);
			VectorCopy(victim->client->ps.velocity, victimMoveDir);

			AngleVectors(victim->currentAngles, nullptr, victimRight, nullptr);

			VectorSubtract(victim->currentOrigin, attacker->currentOrigin, victimTowardAttacker);
			/*victimTowardAttackerDistance = */
			VectorNormalize(victimTowardAttacker);

			const float victimRightAccuracy = DotProduct(victimTowardAttacker, victimRight);

			if (
				fabsf(victimRightAccuracy) > 0.25 // Must Be Exactly Right Or Left
				//	&& victimTowardAttackerDistance<100.0f 					// Must Be Close Enough
				//	&& attackerMoveSpeed>minLockingSpeed					// Must be moving fast enough
				//	&& fabsf(attackerMoveSpeed - victimMoveSpeed)<100	 	// Both must be going about the same speed
				)
			{
				thrown = true;

				vec3_t vec_out;
				vec3_t victimAngles;
				VectorCopy(victim->currentAngles, victimAngles);
				victimAngles[2] = 0;
				AngleVectors(victimAngles, nullptr, vec_out, nullptr);

				if (attackerVeh->m_fStrafeTime < 0)
				{
					VectorScale(vec_out, -1.0f, vec_out);
				}
				if (!(victim->flags & FL_NO_KNOCKBACK))
				{
					G_Throw(victim, vec_out, 250);
				}
				//				if (false)
				//				{
				//					VectorMA(victim->currentOrigin, 250.0f, victimRight, victimRight);
				//					CG_DrawEdge(victim->currentOrigin, victimRight, EDGE_IMPACT_POSSIBLE);
				//				}
				if (victimVeh->m_pVehicleInfo->iImpactFX)
				{
					G_PlayEffect(victimVeh->m_pVehicleInfo->iImpactFX, victim->currentOrigin, trace->plane.normal);
				}
			}
		}
	}

	if (!self->client || self->client->ps.lastOnGround + 300 < level.time || self->client->ps.lastOnGround + 100 <
		level.time)
	{
		float force = 0;
		qboolean vehicleHitOwner = qfalse;

		if (other->material == MAT_GLASS || other->material == MAT_GLASS_METAL || other->material == MAT_GRATE1 || other->svFlags & SVF_BBRUSH && other->spawnflags & 8/*THIN*/)
			//(other->absmax[0]-other->absmin[0]<=32||other->absmax[1]-other->absmin[1]<=32||other->absmax[2]-other->absmin[2]<=32)) )
		{
			//glass and thin breakable brushes (axially aligned only, unfortunately) take more impact damage
			magnitude *= 2;
		}

		// See if the vehicle has crashed into the ground.
		if (pSelfVeh && pSelfVeh->m_pVehicleInfo->type != VH_ANIMAL)
		{
			if (magnitude >= 80 && self->painDebounceTime < level.time)
			{
				// Setup Some Variables
				//----------------------
				vec3_t vehFwd;
				VectorCopy(velocity, vehFwd);
				const float vehSpeed = VectorNormalize(vehFwd);
				float vehToughnessAgainstOther = pSelfVeh->m_pVehicleInfo->toughness;
				const float vehHitPercent = fabsf(DotProduct(vehFwd, trace->plane.normal));
				int vehDFlags = DAMAGE_NO_ARMOR;
				const bool vehPilotedByPlayer = pSelfVeh->m_pPilot && pSelfVeh->m_pPilot->s.number < MAX_CLIENTS;
				const bool vehInTurbo = pSelfVeh->m_iTurboTime > level.time;

				self->painDebounceTime = level.time + 200;

				// Modify Magnitude By Hit Percent And Toughness Against Other
				//-------------------------------------------------------------
				if (pSelfVeh->m_ulFlags & VEH_OUTOFCONTROL)
				{
					vehToughnessAgainstOther *= 0.01f; // If Out Of Control, No Damage Resistance
				}
				else
				{
					if (vehPilotedByPlayer)
					{
						vehToughnessAgainstOther *= 1.5f;
					}
					if (other && other->client)
					{
						vehToughnessAgainstOther *= 15.0f; // Very Tough against other clients (NPCS, Player, etc)
					}
				}
				if (vehToughnessAgainstOther > 0.0f)
				{
					magnitude *= vehHitPercent / vehToughnessAgainstOther;
				}
				else
				{
					magnitude *= vehHitPercent;
				}

				// If We Hit Architecture
				//------------------------
				if (!other || !other->client)
				{
					// Turbo Hurts
					//-------------
					if (vehInTurbo)
					{
						magnitude *= 5.0f;
					}

					else if (trace->plane.normal[2] > 0.75f && vehHitPercent < 0.2f)
					{
						magnitude /= 10.0f;
					}

					// If No Pilot, Blow This Thing Now
					//----------------------------------
					if (vehHitPercent > 0.9f && !pSelfVeh->m_pPilot && vehSpeed > 1000.0f)
					{
						vehDFlags |= DAMAGE_IMPACT_DIE;
					}

					// If Out Of Control, And We Hit A Wall Or Landed Or Head On
					//------------------------------------------------------------
					if (pSelfVeh->m_ulFlags & VEH_OUTOFCONTROL && (vehHitPercent > 0.5f || trace->plane.normal[2] <
						0.5f || velocity[2] < -50.0f))
					{
						vehDFlags |= DAMAGE_IMPACT_DIE;
					}

					// If This Is A Direct Impact (Debounced By 4 Seconds)
					//-----------------------------------------------------
					if (vehHitPercent > 0.9f && level.time - self->lastImpact > 2000 && vehSpeed > 300.0f)
					{
						self->lastImpact = level.time;

						// The Player Has Harder Requirements to Explode
						//-----------------------------------------------
						if (vehPilotedByPlayer)
						{
							if (vehHitPercent > 0.99f && vehSpeed > 1000.0f && !Q_irand(0, 30) ||
								vehHitPercent > 0.999f && vehInTurbo)
							{
								vehDFlags |= DAMAGE_IMPACT_DIE;
							}
						}
						else if (player && G_IsRidingVehicle(player) &&
							Distance(self->currentOrigin, player->currentOrigin) < 800.0f &&
							(vehInTurbo || !Q_irand(0, 1) || vehHitPercent > 0.999f))
						{
							vehDFlags |= DAMAGE_IMPACT_DIE;
						}
					}

					// Make Sure He Dies This Time.  I will accept no excuses.
					//---------------------------------------------------------
					if (vehDFlags & DAMAGE_IMPACT_DIE)
					{
						// If close enough To The PLayer
						if (player &&
							G_IsRidingVehicle(player) &&
							self->owner &&
							Distance(self->currentOrigin, player->currentOrigin) < 500.0f)
						{
							player->lastEnemy = self->owner;
							G_StartMatrixEffect(
								player, MEF_LOOK_AT_ENEMY | MEF_NO_RANGEVAR | MEF_NO_VERTBOB | MEF_NO_SPIN, 1000);
						}
						magnitude = 100000.0f;
					}
				}

				if (magnitude > 10.0f)
				{
					// Play The Impact Effect
					//------------------------
					if (pSelfVeh->m_pVehicleInfo->iImpactFX && vehSpeed > 100.0f)
					{
						G_PlayEffect(pSelfVeh->m_pVehicleInfo->iImpactFX, self->currentOrigin, trace->plane.normal);
					}

					// Set The Crashing Flag And Pain Debounce Time
					//----------------------------------------------
					pSelfVeh->m_ulFlags |= VEH_CRASHING;
				}

				if (PM_InForceFall())
				{
					// no forcefall damage
				}
				else
				{
					G_Damage(self, player, player, nullptr, self->currentOrigin, magnitude, vehDFlags, MOD_FALLING);
				}
			}

			if (self->owner == other || self->activator == other)
			{
				//hit owner/activator
				if (self->m_pVehicle && !self->m_pVehicle->m_pVehicleInfo->Inhabited(self->m_pVehicle))
				{
					//empty swoop
					if (self->client->respawnTime - level.time < 1000)
					{
						//just spawned in a second ago
						//don't actually damage or throw him...
						vehicleHitOwner = qtrue;
					}
				}
			}
			//if 2 vehicles on same side hit each other, tone it down
			//NOTE: we do this here because we still want the impact effect
			if (pOtherVeh)
			{
				if (self->client->playerTeam == other->client->playerTeam)
				{
					magnitude /= 25;
				}
			}
		}
		else if (self->client
			&& (PM_InKnockDown(&self->client->ps) || self->client->ps.eFlags & EF_FORCE_GRIPPED)
			&& magnitude >= 120)
		{
			//FORCE-SMACKED into something
			if (TIMER_Done(self, "impactEffect"))
			{
				G_PlayEffect(G_EffectIndex("env/impact_dustonly"), trace->endpos, trace->plane.normal);
				G_Sound(self, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
				TIMER_Set(self, "impactEffect", 1000);
			}
		}
		else if (self->client
			&& (PM_InKnockDown(&self->client->ps) || self->client->ps.eFlags & EF_FORCE_GRASPED)
			&& magnitude >= 120)
		{
			//FORCE-SMACKED into something
			if (TIMER_Done(self, "impactEffect"))
			{
				G_PlayEffect(G_EffectIndex("env/impact_dustonly"), trace->endpos, trace->plane.normal);
				G_Sound(self, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
				TIMER_Set(self, "impactEffect", 1000);
			}
		}

		//damage them
		if (magnitude >= 100 && other->s.number < ENTITYNUM_WORLD)
		{
			vec3_t dir2;
			vec3_t dir1;
			VectorCopy(velocity, dir1);
			VectorNormalize(dir1);
			if (VectorCompare(other->currentOrigin, vec3_origin))
			{
				//a brush with no origin
				VectorCopy(dir1, dir2);
			}
			else
			{
				VectorSubtract(other->currentOrigin, self->currentOrigin, dir2);
				VectorNormalize(dir2);
			}

			const float dot = DotProduct(dir1, dir2);

			if (dot >= 0.2)
			{
				force = dot;
			}
			else
			{
				force = 0;
			}

			force *= magnitude / 50;

			const int cont = gi.pointcontents(other->absmax, other->s.number);
			if (cont & CONTENTS_WATER)
			{
				//water absorbs 2/3 velocity
				force *= 0.33333f;
			}

			if (self->NPC && other->s.number == ENTITYNUM_WORLD)
			{
				//NPCs take less damage
				force *= 0.5f;
			}

			if (self->s.number >= MAX_CLIENTS && self->client && (PM_InKnockDown(&self->client->ps) || self->client->ps.
				eFlags & EF_FORCE_GRIPPED || self->client->ps.eFlags & EF_FORCE_GRASPED))
			{
				//NPC: I was knocked down or being gripped, impact should be harder
				//FIXME: what if I was just thrown - force pushed/pulled or thrown from a grip?
				force *= 10;
			}

			//FIXME: certain NPCs/entities should be TOUGH - like Ion Cannons, AT-STs, Mark1 droids, etc.
			if (pOtherVeh)
			{
				//if hit another vehicle, take their toughness into account, too
				force /= pOtherVeh->m_pVehicleInfo->toughness;
			}

			if ((force >= 1 || pSelfVeh) && other->s.number >= MAX_CLIENTS || force >= 10)
			{
				if (other->svFlags & SVF_GLASS_BRUSH)
				{
					other->splashRadius = (self->maxs[0] - self->mins[0]) / 4.0f;
				}

				if (pSelfVeh)
				{
					//if in a vehicle when land on someone, always knockdown, throw, damage
					if (!vehicleHitOwner)
					{
						//didn't hit owner
						// If the player was hit don't make the damage so bad...
						if (other && other->s.number < MAX_CLIENTS)
						{
							force *= 0.5f;
						}
						//Hmm, maybe knockdown?
						if (!(other->flags & FL_NO_KNOCKBACK))
						{
							G_Throw(other, dir2, force);
						}
						G_Knockdown(other, self, dir2, force, qtrue);
						G_Damage(other, self, self, velocity, self->currentOrigin, force,
							DAMAGE_NO_ARMOR | DAMAGE_EXTRA_KNOCKBACK, MOD_IMPACT);
					}
				}
				else if (self->forcePushTime > level.time - 1000 //was force pushed/pulled in the last 1600 milliseconds
					&& self->forcePuller == other->s.number) //hit the person who pushed/pulled me
				{
					//ignore the impact
				}
				else if (other->takedamage)
				{
					if (!self->client || other->s.number < MAX_CLIENTS || !other->client)
					{
						//aw, fuck it, clients no longer take impact damage from other clients, unless you're the player
						if (other->client //he's a client
							&& self->client //I'm a client
							&& other->client->ps.forceGripEntityNum == self->s.number) //he's force-gripping me
						{
							//don't damage the other guy if he's gripping me
						}
						else
						{
							if (PM_InForceFall())
							{
								// no forcefall damage
							}
							else
							{
								G_Damage(other, self, self, velocity, self->currentOrigin, floor(force),
									DAMAGE_NO_ARMOR, MOD_IMPACT);
							}
						}
					}
					else
					{
						if (PM_InForceFall())
						{
							// no forcefall damage
						}
						else
						{
							GEntity_PainFunc(other, self, self, self->currentOrigin, force, MOD_IMPACT);
						}
						//Hmm, maybe knockdown?
						if (!thrown)
						{
							if (!(other->flags & FL_NO_KNOCKBACK))
							{
								G_Throw(other, dir2, force);
							}
						}
					}
					if (other->health > 0)
					{
						//still alive?
						// if someone was thrown through the air (in a knockdown or being gripped)
						//		and they hit me hard enough, knock me down
						if (self->client)
						{
							if (PM_InKnockDown(&self->client->ps) || self->client->ps.eFlags & EF_FORCE_GRIPPED ||
								self->client->ps.eFlags & EF_FORCE_GRASPED)
							{
								G_Knockdown(other, self, dir2, Q_irand(200, 400), qtrue);
							}
						}
						else if (self->forcePuller != ENTITYNUM_NONE
							&& g_entities[self->forcePuller].client
							&& self->mass > Q_irand(50, 100))
						{
							G_Knockdown(other, &g_entities[self->forcePuller], dir2, Q_irand(200, 400), qtrue);
						}
					}
				}
				else
				{
					//Hmm, maybe knockdown?
					if (!thrown)
					{
						if (!(other->flags & FL_NO_KNOCKBACK))
						{
							G_Throw(other, dir2, force);
						}
					}
				}
			}
		}

		if (damageSelf && self->takedamage && !(self->flags & FL_NO_IMPACT_DMG))
		{
			if (pSelfVeh && self->client->ps.forceJumpZStart)
			{
				//we were force-jumping
				if (self->currentOrigin[2] >= self->client->ps.forceJumpZStart)
				{
					//we landed at same height or higher than we landed
					magnitude = 0;
				}
				else
				{
					//FIXME: take off some of it, at least?
					magnitude = (self->client->ps.forceJumpZStart - self->currentOrigin[2]) / 3;
				}
			}

			if (strcmp(s, "doom_shields") == 0) //playing outcast
			{
				if (magnitude >= 100 + self->health && self->s.number != 0 && self->s.weapon != WP_SABER || magnitude
					>= 700)
				{
					if ((self->s.weapon == WP_SABER || self->s.number == 0) && self->client && self->client->ps.
						groundEntityNum < ENTITYNUM_NONE && magnitude < 1000)
					{
						//players and jedi take less impact damage
						magnitude /= 2;
					}
					magnitude /= 40;

					if (other->bmodel && other->material != MAT_GLASS)
					{
						//take off only a little because we broke architecture (not including glass), that should hurt
						magnitude = magnitude - force / 8;
					}
					else
					{
						//take off half damage we did to it
						magnitude = magnitude - force / 2;
					}

					if (magnitude >= 1)
					{
						if (self->NPC && self->s.weapon == WP_SABER)
						{
							magnitude = 0;
						}

						if (PM_InForceFall())
						{
							// no forcefall damage
						}
						else
						{
							G_Damage(self, nullptr, nullptr, nullptr, self->currentOrigin, magnitude / 2,
								DAMAGE_NO_ARMOR, MOD_FALLING);
						}
					}
				}
			}
			else
			{
				if (magnitude >= 100 + self->health
					&& self->s.number >= MAX_CLIENTS
					&& self->s.weapon != WP_SABER
					|| self->client->NPC_class == CLASS_VEHICLE
					|| magnitude >= 700) //health here is used to simulate structural integrity
				{
					if ((self->s.weapon == WP_SABER || self->s.number < MAX_CLIENTS || self->client && (self->client->
						NPC_class == CLASS_BOBAFETT || self->client->NPC_class == CLASS_MANDALORIAN || self->client
						->
						NPC_class == CLASS_JANGO || self->client->NPC_class == CLASS_JANGODUAL || self->client->
						NPC_class == CLASS_ROCKETTROOPER)) && self->client && self->client->ps.groundEntityNum <
						ENTITYNUM_NONE && magnitude < 1000)
					{
						//players and jedi take less impact damage
						//allow for some lenience on high falls
						magnitude /= 2;
					}
					//drop it some (magic number... sigh)
					magnitude /= 40;
					//If damage other, subtract half of that damage off of own injury
					if (other->bmodel && other->material != MAT_GLASS)
					{
						//take off only a little because we broke architecture (not including glass), that should hurt
						magnitude = magnitude - force / 8;
					}
					else
					{
						//take off half damage we did to it
						magnitude = magnitude - force / 2;
					}

					if (pSelfVeh)
					{
						magnitude /= pSelfVeh->m_pVehicleInfo->toughness * 1000.0f;
						if (other->bmodel && other->material != MAT_GLASS)
						{
							//broke through some architecture, take a good amount of damage
						}
						else if (pOtherVeh)
						{
							//they're tougher
						}
						else
						{
							//take some off because of give
							magnitude /= 10.0f;
						}
						if (magnitude < 1.0f)
						{
							magnitude = 0;
						}
					}
					if (magnitude >= 1)
					{
						if (self->NPC && self->s.weapon == WP_SABER)
						{
							//FIXME: for now Jedi take no falling damage, but really they should if pushed off?
							magnitude = 0;
						}

						if (PM_InForceFall())
						{
							// no forcefall damage
						}
						else
						{
							G_Damage(self, nullptr, nullptr, nullptr, self->currentOrigin, magnitude / 2,
								DAMAGE_NO_ARMOR, MOD_FALLING);
						}
					}
				}
			}
		}
	}
}

/*
==============
ClientImpacts
==============
*/
void ClientImpacts(gentity_t* ent, const pmove_t* pm)
{
	int j;
	trace_t trace;

	memset(&trace, 0, sizeof trace);
	for (int i = 0; i < pm->numtouch; i++)
	{
		for (j = 0; j < i; j++)
		{
			if (pm->touchents[j] == pm->touchents[i])
			{
				break;
			}
		}
		if (j != i)
		{
			continue; // duplicated
		}
		gentity_t* other = &g_entities[pm->touchents[i]];

		if (ent->NPC != nullptr && ent->e_TouchFunc != touchF_NULL)
		{
			// last check unneccessary
			GEntity_TouchFunc(ent, other, &trace);
		}

		if (other->e_TouchFunc == touchF_NULL)
		{
			// not needed, but I'll leave it I guess (cache-hit issues)
			continue;
		}
		GEntity_TouchFunc(other, ent, &trace);
	}
}

/*
============
G_TouchTriggersLerped

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.

This version checks at 6 unit steps between last and current origins
============
*/
void G_TouchTriggersLerped(gentity_t* ent)
{
	int num;
	float dist;
	gentity_t* hit;
	trace_t trace;
	vec3_t diff;
	constexpr vec3_t range = { 40, 40, 52 };
	qboolean touched[MAX_GENTITIES];
	qboolean done = qfalse;

	if (!ent->client)
	{
		return;
	}

	// dead NPCs don't activate triggers!
	if (ent->client->ps.stats[STAT_HEALTH] <= 0)
	{
		if (ent->s.number >= MAX_CLIENTS)
		{
			return;
		}
	}

#ifdef _DEBUG
	for (int j = 0; j < 3; j++)
	{
		assert(!Q_isnan(ent->currentOrigin[j]));
		assert(!Q_isnan(ent->lastOrigin[j]));
	}
#endif// _DEBUG
	VectorSubtract(ent->currentOrigin, ent->lastOrigin, diff);
	dist = VectorNormalize(diff);
#ifdef _DEBUG
	assert(dist < 1024 && "insane distance in G_TouchTriggersLerped!");
#endif// _DEBUG

	if (dist > 1024)
	{
		return;
	}
	memset(touched, qfalse, sizeof touched);

	for (float curDist = 0; !done && ent->maxs[1] > 0; curDist += ent->maxs[1] / 2.0f)
	{
		vec3_t maxs;
		vec3_t mins;
		vec3_t end;
		gentity_t* touch[MAX_GENTITIES];
		if (curDist >= dist)
		{
			VectorCopy(ent->currentOrigin, end);
			done = qtrue;
		}
		else
		{
			VectorMA(ent->lastOrigin, curDist, diff, end);
		}
		VectorSubtract(end, range, mins);
		VectorAdd(end, range, maxs);

		num = gi.EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

		// can't use ent->absmin, because that has a one unit pad
		VectorAdd(end, ent->mins, mins);
		VectorAdd(end, ent->maxs, maxs);

		for (int i = 0; i < num; i++)
		{
			hit = touch[i];

			if (hit->e_TouchFunc == touchF_NULL && ent->e_TouchFunc == touchF_NULL)
			{
				continue;
			}
			if (!(hit->contents & CONTENTS_TRIGGER))
			{
				continue;
			}

			if (touched[i] == qtrue)
			{
				continue; //already touched this move
			}
			if (ent->client->ps.stats[STAT_HEALTH] <= 0)
			{
				if (Q_stricmp("trigger_teleport", hit->classname) || !(hit->spawnflags & 16/*TTSF_DEAD_OK*/))
				{
					//dead clients can only touch tiogger_teleports that are marked as touchable
					continue;
				}
			}
			// use seperate code for determining if an item is picked up
			// so you don't have to actually contact its bounding box
			/*
			if ( hit->s.eType == ET_ITEM ) {
				if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
					continue;
				}
			} else */
			{
				if (!gi.EntityContact(mins, maxs, hit))
				{
					continue;
				}
			}

			touched[i] = qtrue;

			memset(&trace, 0, sizeof trace);

			if (hit->e_TouchFunc != touchF_NULL)
			{
				GEntity_TouchFunc(hit, ent, &trace);
			}

			//WTF?  Why would a trigger ever fire off the NPC's touch func??!!!
			/*
			if ( ( ent->NPC != NULL ) && ( ent->e_TouchFunc != touchF_NULL ) ) {
				GEntity_TouchFunc( ent, hit, &trace );
			}
			*/
		}
	}
}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void G_TouchTriggers(gentity_t* ent)
{
	gentity_t* touch[MAX_GENTITIES];
	trace_t trace;
	vec3_t mins, maxs;
	constexpr vec3_t range = { 40, 40, 52 };

	if (!ent->client)
	{
		return;
	}

	// dead clients don't activate triggers!
	if (ent->client->ps.stats[STAT_HEALTH] <= 0)
	{
		return;
	}

	VectorSubtract(ent->client->ps.origin, range, mins);
	VectorAdd(ent->client->ps.origin, range, maxs);

	const int num = gi.EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd(ent->client->ps.origin, ent->mins, mins);
	VectorAdd(ent->client->ps.origin, ent->maxs, maxs);

	for (int i = 0; i < num; i++)
	{
		gentity_t* hit = touch[i];

		if (hit->e_TouchFunc == touchF_NULL && ent->e_TouchFunc == touchF_NULL)
		{
			continue;
		}
		if (!(hit->contents & CONTENTS_TRIGGER))
		{
			continue;
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		/*
		if ( hit->s.eType == ET_ITEM ) {
			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
				continue;
			}
		} else */
		{
			if (!gi.EntityContact(mins, maxs, hit))
			{
				continue;
			}
		}

		memset(&trace, 0, sizeof trace);

		if (hit->e_TouchFunc != touchF_NULL)
		{
			GEntity_TouchFunc(hit, ent, &trace);
		}

		if (ent->NPC != nullptr && ent->e_TouchFunc != touchF_NULL)
		{
			GEntity_TouchFunc(ent, hit, &trace);
		}
	}
}

/*
============
G_MoverTouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void G_MoverTouchPushTriggers(gentity_t* ent, vec3_t oldOrg)
{
	trace_t trace;
	vec3_t dir;
	vec3_t size;
	constexpr vec3_t range = { 40, 40, 52 };

	// non-moving movers don't hit triggers!
	if (!VectorLengthSquared(ent->s.pos.trDelta))
	{
		return;
	}

	VectorSubtract(ent->mins, ent->maxs, size);
	float stepSize = VectorLength(size);
	if (stepSize < 1)
	{
		stepSize = 1;
	}

	VectorSubtract(ent->currentOrigin, oldOrg, dir);
	const float dist = VectorNormalize(dir);
	for (float step = 0; step <= dist; step += stepSize)
	{
		vec3_t checkSpot;
		vec3_t maxs;
		vec3_t mins;
		gentity_t* touch[MAX_GENTITIES];
		VectorMA(ent->currentOrigin, step, dir, checkSpot);
		VectorSubtract(checkSpot, range, mins);
		VectorAdd(checkSpot, range, maxs);

		const int num = gi.EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

		// can't use ent->absmin, because that has a one unit pad
		VectorAdd(checkSpot, ent->mins, mins);
		VectorAdd(checkSpot, ent->maxs, maxs);

		for (int i = 0; i < num; i++)
		{
			gentity_t* hit = touch[i];

			if (hit->s.eType != ET_PUSH_TRIGGER)
			{
				continue;
			}

			if (hit->e_TouchFunc == touchF_NULL)
			{
				continue;
			}

			if (!(hit->contents & CONTENTS_TRIGGER))
			{
				continue;
			}

			if (!gi.EntityContact(mins, maxs, hit))
			{
				continue;
			}

			memset(&trace, 0, sizeof trace);

			if (hit->e_TouchFunc != touchF_NULL)
			{
				GEntity_TouchFunc(hit, ent, &trace);
			}
		}
	}
}

void G_MatchPlayerWeapon(gentity_t* ent)
{
	if (g_entities[0].inuse && g_entities[0].client)
	{
		//player is around
		int new_weap;
		if (g_entities[0].client->ps.weapon > WP_CONCUSSION)
		{
			new_weap = WP_BLASTER_PISTOL;
		}
		else
		{
			new_weap = g_entities[0].client->ps.weapon;
		}
		if (new_weap != WP_NONE && ent->client->ps.weapon != new_weap)
		{
			G_RemoveWeaponModels(ent);
			for (char& weapon : ent->client->ps.weapons)
			{
				weapon = 0;
			}
			ent->client->ps.weapons[new_weap] = 1;
			ent->client->ps.ammo[weaponData[new_weap].ammoIndex] = 999;
			ChangeWeapon(ent, new_weap);
			ent->client->ps.weapon = new_weap;
			ent->client->ps.weaponstate = WEAPON_READY;
			if (new_weap == WP_SABER)
			{
				//FIXME: AddSound/Sight Event
				const int num_sabers = WP_SaberInitBladeData(ent);
				WP_SaberAddG2SaberModels(ent);
				G_RemoveHolsterModels(ent);
				for (int saber_num = 0; saber_num < num_sabers; saber_num++)
				{
					ent->client->ps.saber[saber_num].type = g_entities[0].client->ps.saber[saber_num].type;
					for (int blade_num = 0; blade_num < ent->client->ps.saber[saber_num].numBlades; blade_num++)
					{
						ent->client->ps.saber[saber_num].blade[0].active = g_entities[0].client->ps.saber[saber_num].blade
							[blade_num].active;
						ent->client->ps.saber[saber_num].blade[0].length = g_entities[0].client->ps.saber[saber_num].blade
							[blade_num].length;
					}
				}
				ent->client->ps.saberAnimLevel = g_entities[0].client->ps.saberAnimLevel;
				ent->client->ps.saberStylesKnown = g_entities[0].client->ps.saberStylesKnown;
			}
			else
			{
				G_CreateG2AttachedWeaponModel(ent, weaponData[new_weap].weaponMdl, ent->handRBolt, 0);
				//holster sabers
				WP_SaberAddHolsteredG2SaberModels(ent);
			}
		}
	}
}

void G_NPCMunroMatchPlayerWeapon(gentity_t* ent)
{
	//special uber hack for cinematic players to match player's weapon
	if (!in_camera)
	{
		if (ent && ent->client && ent->NPC && ent->NPC->aiFlags & NPCAI_MATCHPLAYERWEAPON)
		{
			//we're a Player NPC
			G_MatchPlayerWeapon(ent);
		}
	}
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
extern void BG_ReduceBlasterMishapLevel(playerState_t* ps);
extern void BG_ReduceBlasterMishapLevelAdvanced(playerState_t* ps);
extern void WP_SaberFatigueRegenerate(int overrideAmt);
extern void WP_BlasterFatigueRegenerate(int overrideAmt);

void ClientTimerActions(gentity_t* ent, int msec)
{
	gclient_t* client = ent->client;
	client->timeResidual += msec;

	while (client->timeResidual >= 1000)
	{
		client->timeResidual -= 1000;

		if (ent->s.weapon != WP_NONE)
		{
			ent->client->sess.missionStats.weaponUsed[ent->s.weapon]++;
		}

		if (g_SerenityJediEngineMode->integer && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
		{
			if (ent->client->NPC_class != CLASS_GALAKMECH
				&& ent->client->ps.forceRageRecoveryTime < level.time
				&& !(ent->client->ps.forcePowersActive & 1 << FP_RAGE))
			{
				if (ent->client->ps.stats[STAT_ARMOR] < ent->client->ps.stats[STAT_MAX_HEALTH])
				{
					ent->client->ps.stats[STAT_ARMOR]++;
				}
			}
			// count down armor when over max
			if (ent->client->ps.stats[STAT_ARMOR] > ent->client->ps.stats[STAT_MAX_HEALTH])
			{
				ent->client->ps.stats[STAT_ARMOR]--;
			}
			// count down force when over max
			if (ent->client->ps.forcePower > ent->client->ps.forcePowerMax)
			{
				ent->client->ps.forcePower--;
			}
		}

		if (ent->flags & FL_OVERCHARGED_HEALTH)
		{
			//need to gradually reduce health back to max
			if (ent->client->ps.stats[STAT_HEALTH] > ent->client->ps.stats[STAT_MAX_HEALTH])
			{
				//decrement it
				ent->health--;
				ent->client->ps.stats[STAT_HEALTH] = ent->health;
			}
			else
			{
				//done
				ent->flags &= ~FL_OVERCHARGED_HEALTH;
			}
		}

		if (!(ent->client->ps.pm_flags & PMF_ATTACK_HELD) && !(ent->client->ps.pm_flags & PMF_ALT_ATTACK_HELD))
		{
			if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_MIN && ent->client->ps.weaponTime < 1)
			{
				WP_BlasterFatigueRegenerate(1);
			}
		}

		if (ent->client->ps.forcePowersActive & 1 << FP_SEE)
		{
			BG_ReduceBlasterMishapLevelAdvanced(&ent->client->ps);
		}

		if (g_SerenityJediEngineMode->integer)
		{
			if (ent->client->ps.saberFatigueChainCount > MISHAPLEVEL_NONE
				&& (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)) // player
				&& !BG_InSlowBounce(&ent->client->ps)
				&& !PM_SaberInBrokenParry(ent->client->ps.saberMove)
				&& !PM_SaberInAttackPure(ent->client->ps.saberMove)
				&& !PM_SaberInAttack(ent->client->ps.saberMove)
				&& !PM_SaberInTransitionAny(ent->client->ps.saberMove)
				&& !PM_InKnockDown(&ent->client->ps)
				&& ent->client->ps.saberLockTime < level.time
				&& ent->client->ps.saberBlockingTime < level.time
				&& ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				if (!(ent->client->ps.ManualBlockingFlags & 1 << MBF_BLOCKING))
				{
					if (ent->client->ps.saberFatigueChainCount > MISHAPLEVEL_HEAVY)
					{
						WP_SaberFatigueRegenerate(1);
					}
					else
					{
						WP_SaberFatigueRegenerate(2);
					}
				}
			}
			else if (ent->client->ps.saberFatigueChainCount > MISHAPLEVEL_NONE
				&& (ent->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(ent)) //npc
				&& !BG_InSlowBounce(&ent->client->ps)
				&& !PM_SaberInBrokenParry(ent->client->ps.saberMove)
				&& !PM_SaberInAttackPure(ent->client->ps.saberMove)
				&& !PM_SaberInAttack(ent->client->ps.saberMove)
				&& !PM_SaberInTransitionAny(ent->client->ps.saberMove)
				&& !PM_InKnockDown(&ent->client->ps)
				&& ent->client->ps.saberLockTime < level.time
				&& ent->client->ps.saberBlockingTime < level.time
				&& ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				WP_SaberFatigueRegenerate(1);
			}

			if (g_SerenityJediEngineMode->integer == 2
				&& (!PM_InAmputateMove(ent->client->ps.legsAnim)
					&& !ent->client->poisonTime
					&& !ent->client->stunTime
					&& !ent->client->AmputateTime
					&& !(ent->client->ps.forcePowersActive & 1 << FP_RAGE))
				&& ent->client->ps.forceRageRecoveryTime < level.time
				&& !PM_SaberInAttack(ent->client->ps.saberMove)
				&& !(ent->client->ps.ManualBlockingFlags & 1 << MBF_BLOCKING)
				&& ent->client->NPC_class != CLASS_PROJECTION)
			{
				//you heal 1 hp every 1 second.
				if (ent->health < client->ps.stats[STAT_MAX_HEALTH])
				{
					ent->health++;
				}
			}

			if (ent->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(ent) //this is for NPC,s
				&& g_SerenityJediEngineMode->integer == 2 && ent->client->ps.blockPoints < BLOCK_POINTS_MAX)
			{
				if (ent->flags & FL_BLOCKPOINTMODE)
				{
					ent->client->ps.blockPoints = BLOCK_POINTS_MAX;
				}
				else
				{
					if (!PM_SaberInMassiveBounce(ent->client->ps.torsoAnim)
						&& !BG_InSlowBounce(&ent->client->ps)
						&& !PM_InKnockDown(&ent->client->ps)
						&& !PM_SaberInBrokenParry(ent->client->ps.saberMove)
						&& ent->client->ps.saberBlockingTime < level.time) // npc, dont auto regen bp if doing this
					{
						ent->client->ps.blockPoints++;
					}
				}
			}

			if (ent->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(ent) //this is for NPC,s
				&& g_SerenityJediEngineMode->integer == 1
				&& ent->client->ps.forcePower < ent->client->ps.forcePowerMax
				&& ent->client->ps.weapon == WP_SABER
				&& ent->client->ps.SaberActive())
			{
				if (ent->flags & FL_FORCEMODE)
				{
					ent->client->ps.forcePower = BLOCK_POINTS_MAX;
				}
				else
				{
					if (!PM_SaberInMassiveBounce(ent->client->ps.torsoAnim)
						&& !BG_InSlowBounce(&ent->client->ps)
						&& !PM_InKnockDown(&ent->client->ps)
						&& !PM_SaberInBrokenParry(ent->client->ps.saberMove)
						&& ent->client->ps.saberBlockingTime < level.time) // npc, dont auto regen bp if doing this
					{
						ent->client->ps.forcePower++;
					}
				}
			}
		}

		Player_CheckBurn(ent);
		Player_CheckFreeze(ent);
	}
}

void ClientTimerProjectionLifeDrain(gentity_t* ent, int msec)
{
	const qboolean DoingProjection = ent->client->ps.communicatingflags & 1 << PROJECTING ? qtrue : qfalse;

	gclient_t* client = ent->client;
	client->timeResidual += msec;

	while (client->timeResidual >= 250)
	{
		client->timeResidual -= 250;

		if (ent->health > 1)
		{
			ent->health--;
			ent->client->ps.stats[STAT_HEALTH] = ent->health;
		}
		else if (!ent->client->Isprojecting)
		{
			G_Damage(ent, nullptr, nullptr, nullptr, ent->currentOrigin, 100000, DAMAGE_NO_PROTECTION,
				MOD_PROJECTION_END);
		}
		else if (!DoingProjection)
		{
			G_Damage(ent, nullptr, nullptr, nullptr, ent->currentOrigin, 100000, DAMAGE_NO_PROTECTION,
				MOD_PROJECTION_END);
		}
		else if (ent->health == 1)
		{
			G_Damage(ent, nullptr, nullptr, nullptr, ent->currentOrigin, 100000, DAMAGE_NO_PROTECTION,
				MOD_PROJECTION_END);
		}
	}
}

/*
====================
ClientIntermissionThink
====================
*/
static qboolean ClientCinematicThink(gclient_t* client)
{
	client->ps.eFlags &= ~EF_FIRING;

	// swap button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->usercmd.buttons;
	if (client->buttons & BUTTON_USE & (client->oldbuttons ^ client->buttons))
	{
		return qtrue;
	}
	return qfalse;
}

/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
extern void WP_SabersDamageTrace(gentity_t* ent, qboolean no_effects = qfalse);
extern void WP_SaberUpdateOldBladeData(gentity_t* ent);

void ClientEvents(gentity_t* ent, int oldEventSequence)
{
	int event;
	gclient_t* client;
#ifndef FINAL_BUILD
	qboolean	fired = qfalse;
#endif

	client = ent->client;

	for (int i = oldEventSequence; i < client->ps.eventSequence; i++)
	{
		event = client->ps.events[i & MAX_PS_EVENTS - 1];

		switch (event)
		{
		case EV_FALL_MEDIUM:
		case EV_FALL_FAR:
			if (ent->s.eType != ET_PLAYER)
			{
				break; // not in the player model
			}
			break;

		case EV_FIRE_WEAPON:
#ifndef FINAL_BUILD
			if (fired) {
				gi.Printf("DOUBLE EV_FIRE_WEAPON AND-OR EV_ALT_FIRE!!\n");
			}
			fired = qtrue;
#endif
			FireWeapon(ent, qfalse);
			break;

		case EV_ALT_FIRE:
#ifndef FINAL_BUILD
			if (fired) {
				gi.Printf("DOUBLE EV_FIRE_WEAPON AND-OR EV_ALT_FIRE!!\n");
			}
			fired = qtrue;
#endif
			FireWeapon(ent, qtrue);
			break;

		default:
			break;
		}
	}
	//by the way, if you have your saber in hand and it's on, do the damage trace
	if (client->ps.weapon == WP_SABER)
	{
		if (g_timescale->value >= 1.0f || !(client->ps.forcePowersActive & 1 << FP_SPEED))
		{
			constexpr int wait = FRAMETIME / 2;
			//sanity check
			if (client->ps.saberDamageDebounceTime - level.time > wait)
			{
				//when you unpause the game with force speed on, the time gets *really* wiggy...
				client->ps.saberDamageDebounceTime = level.time + wait;
			}
			if (client->ps.saberDamageDebounceTime <= level.time)
			{
				WP_SabersDamageTrace(ent);
				WP_SaberUpdateOldBladeData(ent);
				client->ps.saberDamageDebounceTime = level.time + wait;
			}
		}
	}
}

void G_ThrownDeathAnimForDeathAnim(gentity_t* hitEnt, vec3_t impactPoint)
{
	int anim = -1;
	if (!hitEnt || !hitEnt->client)
	{
		return;
	}
	switch (hitEnt->client->ps.legsAnim)
	{
	case BOTH_DEATH9: //fall to knees, fall over
	case BOTH_DEATH10: //fall to knees, fall over
	case BOTH_DEATH11: //fall to knees, fall over
	case BOTH_DEATH13: //stumble back, fall over
	case BOTH_DEATH17: //jerky fall to knees, fall over
	case BOTH_DEATH18: //grab gut, fall to knees, fall over
	case BOTH_DEATH19: //grab gut, fall to knees, fall over
	case BOTH_DEATH20: //grab shoulder, fall forward
	case BOTH_DEATH21: //grab shoulder, fall forward
	case BOTH_DEATH3: //knee collapse, twist & fall forward
	case BOTH_DEATH7: //knee collapse, twist & fall forward
	{
		vec3_t dir2Impact, fwdAngles, facing;
		VectorSubtract(impactPoint, hitEnt->currentOrigin, dir2Impact);
		dir2Impact[2] = 0;
		VectorNormalize(dir2Impact);
		VectorSet(fwdAngles, 0, hitEnt->client->ps.viewangles[YAW], 0);
		AngleVectors(fwdAngles, facing, nullptr, nullptr);
		const float dot = DotProduct(facing, dir2Impact); //-1 = hit in front, 0 = hit on side, 1 = hit in back
		if (dot > 0.5f)
		{
			//kicked in chest, fly backward
			switch (Q_irand(0, 4))
			{
				//FIXME: don't start at beginning of anim?
			case 0:
				anim = BOTH_DEATH1; //thrown backwards
				break;
			case 1:
				anim = BOTH_DEATH2; //fall backwards
				break;
			case 2:
				anim = BOTH_DEATH15; //roll over backwards
				break;
			case 3:
				anim = BOTH_DEATH22; //fast fall back
				break;
			case 4:
				anim = BOTH_DEATH23; //fast fall back
				break;
			default:;
			}
		}
		else if (dot < -0.5f)
		{
			//kicked in back, fly forward
			switch (Q_irand(0, 5))
			{
				//FIXME: don't start at beginning of anim?
			case 0:
				anim = BOTH_DEATH14;
				break;
			case 1:
				anim = BOTH_DEATH24;
				break;
			case 2:
				anim = BOTH_DEATH25;
				break;
			case 3:
				anim = BOTH_DEATH4; //thrown forwards
				break;
			case 4:
				anim = BOTH_DEATH5; //thrown forwards
				break;
			case 5:
				anim = BOTH_DEATH16; //thrown forwards
				break;
			default:;
			}
		}
		else
		{
			//hit on side, spin
			switch (Q_irand(0, 2))
			{
				//FIXME: don't start at beginning of anim?
			case 0:
				anim = BOTH_DEATH12;
				break;
			case 1:
				anim = BOTH_DEATH14;
				break;
			case 2:
				anim = BOTH_DEATH15;
				break;
			case 3:
				anim = BOTH_DEATH6;
				break;
			case 4:
				anim = BOTH_DEATH8;
				break;
			default:;
			}
		}
	}
	break;
	default:;
	}
	if (anim != -1)
	{
		NPC_SetAnim(hitEnt, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
}

//general check for performing certain attacks against others
qboolean G_CanBeEnemy(const gentity_t* self, const gentity_t* enemy)
{
	if (!self->inuse || !enemy->inuse || !self->client || !enemy->client)
	{
		return qfalse;
	}

	if (OnSameTeam(self, enemy))
	{
		//ff not on, don't hurt teammates
		return qfalse;
	}

	return qtrue;
}

qboolean WP_AbsorbKick(gentity_t* hitEnt, const gentity_t* pusher, const vec3_t pushDir)
{
	const qboolean NPCBlocking = hitEnt->client->ps.ManualBlockingFlags & 1 << MBF_NPCKICKBLOCK ? qtrue : qfalse;
	//NPC Blocking

	if (in_camera)
	{
		return qfalse;
	}

	if (G_InScriptedCinematicSaberAnim(hitEnt))
	{
		return qfalse;
	}

	if (hitEnt->client->NPC_class == CLASS_ROCKETTROOPER)
	{
		return qfalse;
	}

	if (hitEnt->client->moveType == MT_FLYSWIM)
	{
		//can't knock me down when I'm flying
		return qfalse;
	}

	if (PlayerCanAbsorbKick(hitEnt, pushDir) && (hitEnt->s.number < MAX_CLIENTS || G_ControlledByPlayer(hitEnt)))
		//player only in MD/AMD MODE
	{
		if (g_SerenityJediEngineMode->integer)
		{
			//MD/AMD MODE
			if (g_SerenityJediEngineMode->integer == 2)
			{
				//AMD Mode
				if (hitEnt->client->ps.blockPoints > 50)
				{
					G_Stagger(hitEnt);
					WP_BlockPointsDrain(hitEnt, FATIGUE_BP_ABSORB);
				}
				else
				{
					G_Stumble(hitEnt);
					WP_BlockPointsDrain(hitEnt, FATIGUE_BP_ABSORB);
				}
			}
			else
			{
				// MD mode
				if (hitEnt->client->ps.forcePower > 50)
				{
					G_Stagger(hitEnt);
					WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
				}
				else
				{
					G_Stumble(hitEnt);
					WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
				}
			}
		}
		else
		{
			//jka mode
			if (hitEnt->client->ps.forcePower > 75)
			{
				G_Stagger(hitEnt);
				WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
			}
		}
		G_Sound(hitEnt, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
	}
	else if (BotCanAbsorbKick(hitEnt, pushDir) && NPCBlocking
		&& hitEnt->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(hitEnt)) //NPC only any mode
	{
		if (g_SerenityJediEngineMode->integer)
		{
			if (g_SerenityJediEngineMode->integer == 2)
			{
				//Advanced MD Mode
				if (hitEnt->client->ps.blockPoints > 50 || jedi_is_kick_resistant(hitEnt))
				{
					G_Stagger(hitEnt);
					WP_BlockPointsDrain(hitEnt, FATIGUE_BP_ABSORB);
					if (!Q_irand(0, 4))
					{//20% chance
						WP_DeactivateSaber(hitEnt, qtrue);
					}
					if (jedi_is_kick_resistant(hitEnt))
					{
						jedi_play_blocked_push_sound(hitEnt);
					}
				}
				else
				{
					NPC_SetAnim(hitEnt, SETANIM_BOTH, Q_irand(BOTH_FLIP_BACK1, BOTH_FLIP_BACK2), SETANIM_AFLAG_PACE);
					WP_BlockPointsDrain(hitEnt, FATIGUE_BP_ABSORB);
				}
			}
			else
			{
				//MD MODE
				if (hitEnt->client->ps.forcePower > 50 || jedi_is_kick_resistant(hitEnt))
				{
					G_Stagger(hitEnt);
					WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
					if (!Q_irand(0, 4))
					{//20% chance
						WP_DeactivateSaber(hitEnt, qtrue);
					}
					if (jedi_is_kick_resistant(hitEnt))
					{
						jedi_play_blocked_push_sound(hitEnt);
					}
				}
				else
				{
					NPC_SetAnim(hitEnt, SETANIM_BOTH, Q_irand(BOTH_FLIP_BACK1, BOTH_FLIP_BACK2), SETANIM_AFLAG_PACE);
					WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
				}
			}
		}
		else
		{
			//JKA Mode
			if (hitEnt->client->ps.forcePower > 50 || jedi_is_kick_resistant(hitEnt))
			{
				G_Stagger(hitEnt);
				WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
				if (!Q_irand(0, 4))
				{//20% chance
					WP_DeactivateSaber(hitEnt, qtrue);
				}
				if (jedi_is_kick_resistant(hitEnt))
				{
					jedi_play_blocked_push_sound(hitEnt);
				}
			}
			else
			{
				NPC_SetAnim(hitEnt, SETANIM_BOTH, Q_irand(BOTH_FLIP_BACK1, BOTH_FLIP_BACK2), SETANIM_AFLAG_PACE);
				WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
			}
		}
		G_Sound(hitEnt, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
	}
	else
	{
		if (pusher->client->ps.legsAnim == BOTH_A7_KICK_F
			|| pusher->client->ps.legsAnim == BOTH_KICK_F_MD
			|| pusher->client->ps.torsoAnim == BOTH_TUSKENATTACK1
			|| pusher->client->ps.torsoAnim == BOTH_TUSKENATTACK2
			|| pusher->client->ps.torsoAnim == BOTH_TUSKENATTACK3)
		{
			if (jedi_is_kick_resistant(hitEnt))
			{
				G_MawStagger(hitEnt);
				if (!Q_irand(0, 4))
				{//20% chance
					WP_DeactivateSaber(hitEnt, qtrue);
				}
				else
				{
					jedi_play_blocked_push_sound(hitEnt);
				}
			}
			else
			{
				NPC_SetAnim(hitEnt, SETANIM_BOTH, Q_irand(BOTH_KNOCKDOWN1, BOTH_KNOCKDOWN2), SETANIM_AFLAG_PACE);
			}

			if (g_SerenityJediEngineMode->integer == 2)
			{
				WP_BlockPointsDrain(hitEnt, FATIGUE_BLOCKPOINTDRAIN);

				if (d_slowmoaction->integer && hitEnt->health >= 10 && hitEnt->client->ps.blockPoints < BLOCKPOINTS_HALF
					&& (pusher->s.number < MAX_CLIENTS || G_ControlledByPlayer(pusher)))
				{
					G_StartStasisEffect_FORCE_LEVEL_1(pusher, MEF_NO_SPIN, 300, 0.3f, 0);
				}
			}
			else
			{
				WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
			}
			if (pusher->client->ps.torsoAnim == BOTH_TUSKENATTACK1
				|| pusher->client->ps.torsoAnim == BOTH_TUSKENATTACK2
				|| pusher->client->ps.torsoAnim == BOTH_TUSKENATTACK3)
			{
				G_Sound(hitEnt, G_SoundIndex("sound/movers/objects/saber_slam"));
			}
			else
			{
				G_Sound(hitEnt, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
			}
			hitEnt->client->ps.saberMove = LS_NONE;
			AddFatigueMeleeBonus(pusher, hitEnt);
		}
		else if (pusher->client->ps.legsAnim == BOTH_A7_KICK_B)
		{
			if (jedi_is_kick_resistant(hitEnt))
			{
				G_MawStagger(hitEnt);
				if (!Q_irand(0, 4))
				{//20% chance
					WP_DeactivateSaber(hitEnt, qtrue);
				}
				if (jedi_is_kick_resistant(hitEnt))
				{
					jedi_play_blocked_push_sound(hitEnt);
				}
			}
			else
			{
				NPC_SetAnim(hitEnt, SETANIM_BOTH, Q_irand(BOTH_KNOCKDOWN1, BOTH_KNOCKDOWN5), SETANIM_AFLAG_PACE);
			}

			if (g_SerenityJediEngineMode->integer == 2)
			{
				WP_BlockPointsDrain(hitEnt, FATIGUE_BLOCKPOINTDRAIN);
			}
			else
			{
				WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
			}

			G_Sound(hitEnt, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
			hitEnt->client->ps.saberMove = LS_NONE;
			AddFatigueMeleeBonus(pusher, hitEnt);
		}
		else if (pusher->client->ps.legsAnim == BOTH_SWEEP_KICK)
		{
			if (jedi_is_kick_resistant(hitEnt))
			{
				G_MawStagger(hitEnt);
				WP_DeactivateSaber(hitEnt, qtrue);
				if (jedi_is_kick_resistant(hitEnt))
				{
					jedi_play_blocked_push_sound(hitEnt);
				}
			}
			else
			{
				NPC_SetAnim(hitEnt, SETANIM_BOTH, BOTH_KNOCKDOWN3, SETANIM_AFLAG_PACE);
			}

			if (g_SerenityJediEngineMode->integer == 2)
			{
				WP_BlockPointsDrain(hitEnt, FATIGUE_BLOCKPOINTDRAIN);

				if (d_slowmoaction->integer && hitEnt->health >= 10 && (pusher->s.number < MAX_CLIENTS ||
					G_ControlledByPlayer(pusher)))
				{
					G_StartStasisEffect_FORCE_LEVEL_1(pusher, MEF_NO_SPIN, 300, 0.3f, 0);
				}
			}
			else
			{
				WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
			}
			AddFatigueMeleeBonus(pusher, hitEnt);
			G_Sound(hitEnt, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
			G_Sound(pusher, G_SoundIndex(va("sound/weapons/melee/swing%d", Q_irand(1, 4))));
			hitEnt->client->ps.saberMove = LS_NONE;
		}
		else if (pusher->client->ps.legsAnim == BOTH_A7_KICK_R
			|| pusher->client->ps.torsoAnim == BOTH_A7_SLAP_R
			|| pusher->client->ps.torsoAnim == BOTH_SLAP_R)
		{
			if (jedi_is_kick_resistant(hitEnt))
			{
				G_MawStagger(hitEnt);
				if (!Q_irand(0, 4))
				{//20% chance
					WP_DeactivateSaber(hitEnt, qtrue);
				}
				if (jedi_is_kick_resistant(hitEnt))
				{
					jedi_play_blocked_push_sound(hitEnt);
				}
			}
			else
			{
				NPC_SetAnim(hitEnt, SETANIM_BOTH, BOTH_SLAPDOWNRIGHT, SETANIM_AFLAG_PACE);
			}

			if (g_SerenityJediEngineMode->integer == 2)
			{
				WP_BlockPointsDrain(hitEnt, FATIGUE_BLOCKPOINTDRAIN);

				if (d_slowmoaction->integer && hitEnt->health >= 10 && (pusher->s.number < MAX_CLIENTS ||
					G_ControlledByPlayer(pusher)))
				{
					G_StartStasisEffect_FORCE_LEVEL_1(pusher, MEF_NO_SPIN, 300, 0.3f, 0);
				}
			}
			else
			{
				WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
			}
			G_Sound(hitEnt, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
			G_Sound(pusher, G_SoundIndex(va("sound/weapons/melee/swing%d", Q_irand(1, 4))));
			hitEnt->client->ps.saberMove = LS_NONE;
			AddFatigueMeleeBonus(pusher, hitEnt);
		}
		else if (pusher->client->ps.legsAnim == BOTH_A7_KICK_L
			|| pusher->client->ps.torsoAnim == BOTH_A7_SLAP_L
			|| pusher->client->ps.torsoAnim == BOTH_SLAP_L)
		{
			if (jedi_is_kick_resistant(hitEnt))
			{
				G_MawStagger(hitEnt);
				if (!Q_irand(0, 4))
				{//20% chance
					WP_DeactivateSaber(hitEnt, qtrue);
				}
				if (jedi_is_kick_resistant(hitEnt))
				{
					jedi_play_blocked_push_sound(hitEnt);
				}
			}
			else
			{
				NPC_SetAnim(hitEnt, SETANIM_BOTH, BOTH_SLAPDOWNLEFT, SETANIM_AFLAG_PACE);
			}

			if (g_SerenityJediEngineMode->integer == 2)
			{
				WP_BlockPointsDrain(hitEnt, FATIGUE_BLOCKPOINTDRAIN);

				if (d_slowmoaction->integer && hitEnt->health >= 10 && (pusher->s.number < MAX_CLIENTS ||
					G_ControlledByPlayer(pusher)))
				{
					G_StartStasisEffect_FORCE_LEVEL_1(pusher, MEF_NO_SPIN, 300, 0.3f, 0);
				}
			}
			else
			{
				WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
			}
			G_Sound(hitEnt, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
			G_Sound(pusher, G_SoundIndex(va("sound/weapons/melee/swing%d", Q_irand(1, 4))));
			hitEnt->client->ps.saberMove = LS_NONE;
			AddFatigueMeleeBonus(pusher, hitEnt);
		}
		else if (pusher->client->ps.torsoAnim == BOTH_TUSKENLUNGE1)
		{
			if (jedi_is_kick_resistant(hitEnt))
			{
				G_MawStagger(hitEnt);
				WP_DeactivateSaber(hitEnt, qtrue);
				if (jedi_is_kick_resistant(hitEnt))
				{
					jedi_play_blocked_push_sound(hitEnt);
				}
			}
			else
			{
				NPC_SetAnim(hitEnt, SETANIM_BOTH, Q_irand(BOTH_SLAPDOWNRIGHT, BOTH_SLAPDOWNLEFT), SETANIM_AFLAG_PACE);
			}

			if (g_SerenityJediEngineMode->integer == 2)
			{
				WP_BlockPointsDrain(hitEnt, FATIGUE_BLOCKPOINTDRAIN);
			}
			else
			{
				WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
			}
			G_Sound(hitEnt, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
			G_Sound(pusher, G_SoundIndex(va("sound/weapons/melee/swing%d", Q_irand(1, 4))));
			hitEnt->client->ps.saberMove = LS_NONE;
			AddFatigueMeleeBonus(pusher, hitEnt);
		}
		else if (pusher->client->ps.torsoAnim == BOTH_A7_HILT) //for the hiltbash
		{
			NPC_SetAnim(hitEnt, SETANIM_BOTH, BOTH_BASHED1, SETANIM_AFLAG_PACE);

			if (g_SerenityJediEngineMode->integer == 2)
			{
				WP_BlockPointsDrain(hitEnt, FATIGUE_BLOCKPOINTDRAIN);
			}
			else
			{
				WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
			}
			G_Sound(hitEnt, G_SoundIndex("sound/movers/objects/saber_slam"));
			G_Sound(pusher, G_SoundIndex(va("sound/weapons/melee/swing%d", Q_irand(1, 4))));
			hitEnt->client->ps.saberMove = LS_NONE;
			AddFatigueMeleeBonus(pusher, hitEnt);
		}
		else
		{
			if (jedi_is_kick_resistant(hitEnt))
			{
				G_MawStagger(hitEnt);
				if (!Q_irand(0, 4))
				{//20% chance
					WP_DeactivateSaber(hitEnt, qtrue);
				}
				if (jedi_is_kick_resistant(hitEnt))
				{
					jedi_play_blocked_push_sound(hitEnt);
				}
			}
			else
			{
				NPC_SetAnim(hitEnt, SETANIM_BOTH, Q_irand(BOTH_KNOCKDOWN1, BOTH_KNOCKDOWN2), SETANIM_AFLAG_PACE);
			}

			if (g_SerenityJediEngineMode->integer == 2)
			{
				WP_BlockPointsDrain(hitEnt, FATIGUE_BLOCKPOINTDRAIN);
			}
			else
			{
				WP_ForcePowerDrain(hitEnt, FP_SABER_DEFENSE, FATIGUE_KICKHIT);
			}
			G_Sound(hitEnt, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
			G_Sound(pusher, G_SoundIndex(va("sound/weapons/melee/swing%d", Q_irand(1, 4))));
			hitEnt->client->ps.saberMove = LS_NONE;
			AddFatigueMeleeBonus(pusher, hitEnt);
		}
	}

	return qtrue;
}

extern void G_Kick_Throw(gentity_t* targ, const vec3_t new_dir, float push);

gentity_t* G_KickTrace(gentity_t* ent, vec3_t kickDir, float kickDist, vec3_t kickEnd, int kickDamage, float kickPush,
	qboolean doSoundOnWalls)
{
	vec3_t traceOrg, traceEnd;
	constexpr vec3_t kickMaxs = { 4, 4, 4 };
	constexpr vec3_t kickMins = { -2, -2, -2 };
	trace_t trace;
	gentity_t* hitEnt = nullptr;

	if (kickEnd && !VectorCompare(kickEnd, vec3_origin))
	{
		//they passed us the end point of the trace, just use that
		//this makes the trace flat
		VectorSet(traceOrg, ent->currentOrigin[0], ent->currentOrigin[1], kickEnd[2]);
		VectorCopy(kickEnd, traceEnd);
	}
	else
	{
		//extrude
		VectorSet(traceOrg, ent->currentOrigin[0], ent->currentOrigin[1], ent->currentOrigin[2] + ent->maxs[2] * 0.5f);
		VectorMA(traceOrg, kickDist, kickDir, traceEnd);
	}

	gi.trace(&trace, traceOrg, kickMins, kickMaxs, traceEnd, ent->s.number, MASK_SHOT, static_cast<EG2_Collision>(0),
		0);

	if (trace.fraction < 1.0f || trace.startsolid && trace.entityNum < ENTITYNUM_NONE)
	{
		hitEnt = &g_entities[trace.entityNum];

		if (ent->client->ps.lastKickedEntNum != trace.entityNum)
		{
			TIMER_Remove(ent, "kickSoundDebounce");
			ent->client->ps.lastKickedEntNum = trace.entityNum;
		}

		if (hitEnt)
		{
			//we hit an entity
			if (hitEnt->client)
			{
				if (!(hitEnt->client->ps.pm_flags & PMF_TIME_KNOCKBACK)
					&& TIMER_Done(hitEnt, "kickedDebounce") && G_CanBeEnemy(ent, hitEnt))
				{
					if (PM_InKnockDown(&hitEnt->client->ps) && !PM_InGetUp(&hitEnt->client->ps))
					{
						//don't hit people who are knocked down or being knocked down (okay to hit people getting up, though)
						return nullptr;
					}
					if (PM_Dyinganim(&hitEnt->client->ps))
					{
						//don't hit people who are in a death anim
						return nullptr;
					}
					if (PM_InRoll(&hitEnt->client->ps))
					{
						//can't hit people who are rolling
						return nullptr;
					}
					//don't hit same ent more than once per kick
					if (hitEnt->takedamage)
					{
						//hurt it
						if (!in_camera)
						{
							if (WP_AbsorbKick(hitEnt, ent, kickDir))
							{
								//but the lucky devil absorbed it by backflipping
								//toss them back a bit and make sure that the kicker gets the kill if the
								//player falls off a cliff or something.
								if (hitEnt->client)
								{
									hitEnt->client->ps.otherKiller = ent->s.number;
									hitEnt->client->ps.otherKillerDebounceTime = level.time + 10000;
									hitEnt->client->ps.otherKillerTime = level.time + 10000;
									hitEnt->client->otherKillerMOD = MOD_MELEE;
									hitEnt->client->otherKillerVehWeapon = 0;
									hitEnt->client->otherKillerWeaponType = WP_NONE;
									hitEnt->client->ps.saberMove = LS_READY;
								}
								G_Kick_Throw(hitEnt, kickDir, 75);
								return hitEnt;
							}
						}
						G_Damage(hitEnt, ent, ent, kickDir, trace.endpos, kickDamage * 0.2f, DAMAGE_NO_KNOCKBACK,
							MOD_MELEE);
					}
					//do kick hit sound and impact effect
					if (TIMER_Done(ent, "kickSoundDebounce"))
					{
						if (ent->client->ps.torsoAnim == BOTH_A7_HILT)
						{
							G_Sound(ent, G_SoundIndex("sound/movers/objects/saber_slam"));
						}
						else
						{
							vec3_t fxOrg, fxDir;
							VectorCopy(kickDir, fxDir);
							VectorMA(trace.endpos, Q_flrand(5.0f, 10.0f), fxDir, fxOrg);
							VectorScale(fxDir, -1, fxDir);
							G_PlayEffect(G_EffectIndex("melee/kick_impact.efx"), fxOrg, fxDir);
							G_Sound(ent, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
						}
						TIMER_Set(ent, "kickSoundDebounce", 1000);
					}
					TIMER_Set(hitEnt, "kickedDebounce", 500);

					if (ent->client->ps.torsoAnim == BOTH_A7_HILT ||
						ent->client->ps.torsoAnim == BOTH_SLAP_L ||
						ent->client->ps.torsoAnim == BOTH_SLAP_R ||
						ent->client->ps.torsoAnim == BOTH_A7_SLAP_R ||
						ent->client->ps.torsoAnim == BOTH_A7_SLAP_L)
					{
						//hit in head
						if (hitEnt->health > 0)
						{
							//knock down
							if (kickPush >= 150.0f && !Q_irand(0, 1))
							{
								//knock them down
								if (!(hitEnt->flags & FL_NO_KNOCKBACK))
								{
									G_Kick_Throw(hitEnt, kickDir, 80);
								}
								G_Slapdown(hitEnt, ent, kickDir, 80, qtrue);
							}
							else
							{
								//force them to play a pain anim
								if (hitEnt->s.number < MAX_CLIENTS)
								{
									NPC_SetPainEvent(hitEnt);
								}
								else
								{
									GEntity_PainFunc(hitEnt, ent, ent, hitEnt->currentOrigin, 0, MOD_MELEE);
								}
							}
							//just so we don't hit him again...
							hitEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
							hitEnt->client->ps.pm_time = 100;
						}
						else
						{
							if (!(hitEnt->flags & FL_NO_KNOCKBACK))
							{
								G_Kick_Throw(hitEnt, kickDir, 80);
							}
							//see if we should play a better looking death on them
							G_ThrownDeathAnimForDeathAnim(hitEnt, trace.endpos);
						}
					}
					else if (ent->client->ps.legsAnim == BOTH_GETUP_BROLL_B
						|| ent->client->ps.legsAnim == BOTH_GETUP_BROLL_F
						|| ent->client->ps.legsAnim == BOTH_GETUP_FROLL_B
						|| ent->client->ps.legsAnim == BOTH_GETUP_FROLL_F)
					{
						if (hitEnt->health > 0)
						{
							//knock down
							if (hitEnt->client->ps.groundEntityNum == ENTITYNUM_NONE)
							{
								//he's in the air?  Send him flying back
								if (!(hitEnt->flags & FL_NO_KNOCKBACK))
								{
									G_Kick_Throw(hitEnt, kickDir, 80);
								}
							}
							else
							{
								//just so we don't hit him again...
								hitEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
								hitEnt->client->ps.pm_time = 100;
							}
							//knock them down
							G_Slapdown(hitEnt, ent, kickDir, 80, qtrue);
						}
						else
						{
							if (!(hitEnt->flags & FL_NO_KNOCKBACK))
							{
								G_Kick_Throw(hitEnt, kickDir, 80);
							}
							//see if we should play a better looking death on them
							G_ThrownDeathAnimForDeathAnim(hitEnt, trace.endpos);
						}
					}
					else if (hitEnt->health <= 0)
					{
						//we kicked a dead guy
						if (!(hitEnt->flags & FL_NO_KNOCKBACK))
						{
							G_Kick_Throw(hitEnt, kickDir, kickPush * 2);
						}
						G_ThrownDeathAnimForDeathAnim(hitEnt, trace.endpos);
					}
					else
					{
						if (!(hitEnt->flags & FL_NO_KNOCKBACK))
						{
							G_Kick_Throw(hitEnt, kickDir, 80);
						}
						if ((hitEnt->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HEAVY ||
							hitEnt->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
							&& !(hitEnt->client->ps.ManualBlockingFlags & 1 << MBF_BLOCKING))
						{
							//knockdown
							if (hitEnt->client->ps.saberAnimLevel == SS_STAFF)
							{
								SabBeh_AnimateSlowBounce(hitEnt);
							}
							else
							{
								if (kickPush >= 150.0f && !Q_irand(0, 2))
								{
									G_Slapdown(hitEnt, ent, kickDir, 90, qtrue);
								}
								else
								{
									G_Slapdown(hitEnt, ent, kickDir, 80, qtrue);
								}
							}
						}
						else if (ent->client->ps.saberAnimLevel == SS_DESANN
							&& (hitEnt->client->ps.saberFatigueChainCount >= MISHAPLEVEL_HEAVY ||
								hitEnt->client->ps.BlasterAttackChainCount >= BLASTERMISHAPLEVEL_HEAVY)
							&& !(hitEnt->client->ps.ManualBlockingFlags & 1 << MBF_BLOCKING))
						{
							//knockdown
							if (kickPush >= 150.0f && !Q_irand(0, 2))
							{
								G_Slapdown(hitEnt, ent, kickDir, 90, qtrue);
							}
							else
							{
								G_Slapdown(hitEnt, ent, kickDir, 80, qtrue);
							}
						}
						else if (kickPush >= 150.0f && !Q_irand(0, 2))
						{
							G_Knockdown(hitEnt, ent, kickDir, 80, qtrue);
						}
						else
						{
							if (g_SerenityJediEngineMode->integer)
							{
								AnimateStun(hitEnt, ent);
							}
							else
							{
								G_Knockdown(hitEnt, ent, kickDir, 80, qtrue);
							}
						}
					}
				}
			}
			else
			{
				if (doSoundOnWalls)
				{
					//do kick hit sound and impact effect
					if (TIMER_Done(ent, "kickSoundDebounce"))
					{
						if (ent->client->ps.torsoAnim == BOTH_A7_HILT)
						{
							G_Sound(ent, G_SoundIndex("sound/movers/objects/saber_slam"));
						}
						else if (ent->client->ps.torsoAnim == BOTH_SLAP_L ||
							ent->client->ps.torsoAnim == BOTH_SLAP_R ||
							ent->client->ps.torsoAnim == BOTH_A7_SLAP_R ||
							ent->client->ps.torsoAnim == BOTH_A7_SLAP_L)
						{
							G_Sound(ent, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
							G_PlayEffect(G_EffectIndex("melee/kick_impact.efx"), trace.endpos, trace.plane.normal);
						}
						else
						{
							G_Sound(ent, G_SoundIndex(va("sound/weapons/melee/punch%d", Q_irand(1, 4))));
							G_PlayEffect(G_EffectIndex("melee/kick_impact.efx"), trace.endpos, trace.plane.normal);
						}
						TIMER_Set(ent, "kickSoundDebounce", 1000);
					}
				}
			}
		}
	}
	return hitEnt;
}

qboolean G_CheckRollSafety(const gentity_t* self, int anim, float testDist)
{
	vec3_t forward, right, testPos, angles;
	trace_t trace;
	int contents = CONTENTS_SOLID | CONTENTS_BOTCLIP;

	if (!self || !self->client)
	{
		return qfalse;
	}

	if (self->s.number < MAX_CLIENTS)
	{
		//player
		contents |= CONTENTS_PLAYERCLIP;
	}
	else
	{
		//NPC
		contents |= CONTENTS_MONSTERCLIP;
	}
	if (PM_InAttackRoll(self->client->ps.legsAnim))
	{
		//we don't care if people are in the way, we'll knock them down!
		contents &= ~CONTENTS_BODY;
	}
	angles[PITCH] = angles[ROLL] = 0;
	angles[YAW] = self->client->ps.viewangles[YAW]; //Add ucmd.angles[YAW]?
	AngleVectors(angles, forward, right, nullptr);

	switch (anim)
	{
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_R:
		VectorMA(self->currentOrigin, testDist, right, testPos);
		break;
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_FROLL_L:
		VectorMA(self->currentOrigin, -testDist, right, testPos);
		break;
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_FROLL_F:
		VectorMA(self->currentOrigin, testDist, forward, testPos);
		break;
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_FROLL_B:
		VectorMA(self->currentOrigin, -testDist, forward, testPos);
		break;
	default: //FIXME: add normal rolls?  Make generic for any forced-movement anim?
		return qtrue;
	}

	gi.trace(&trace, self->currentOrigin, self->mins, self->maxs, testPos, self->s.number, contents,
		static_cast<EG2_Collision>(0), 0);
	if (trace.fraction < 1.0f
		|| trace.allsolid
		|| trace.startsolid)
	{
		//inside something or will hit something
		return qfalse;
	}
	return qtrue;
}

void G_CamPullBackForLegsAnim(const gentity_t* ent, qboolean useTorso = qfalse)
{
	if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
	{
		const float animLength = PM_AnimLength(ent->client->clientInfo.animFileIndex,
			useTorso
			? static_cast<animNumber_t>(ent->client->ps.torsoAnim)
			: static_cast<animNumber_t>(ent->client->ps.legsAnim));
		const float elapsedTime = animLength - (useTorso
			? ent->client->ps.torsoAnimTimer
			: ent->client->ps.legsAnimTimer);
		float backDist;
		if (elapsedTime < animLength / 2.0f)
		{
			//starting anim
			backDist = elapsedTime / animLength * 120.0f;
		}
		else
		{
			//ending anim
			backDist = (animLength - elapsedTime) / animLength * 120.0f;
		}
		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_RNG;
		cg.overrides.thirdPersonRange = cg_thirdPersonRange.value + backDist;
	}
}

void G_CamCircleForLegsAnim(const gentity_t* ent)
{
	if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
	{
		const float animLength = PM_AnimLength(ent->client->clientInfo.animFileIndex,
			static_cast<animNumber_t>(ent->client->ps.legsAnim));
		const float elapsedTime = animLength - ent->client->ps.legsAnimTimer;
		const float angle = elapsedTime / animLength * 360.0f;
		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_ANG;
		cg.overrides.thirdPersonAngle = cg_thirdPersonAngle.value + angle;
	}
}

qboolean G_GrabClient(gentity_t* ent, const usercmd_t* ucmd)
{
	gentity_t* bestEnt = nullptr, * radiusEnts[128];
	constexpr float radius = 100.0f;
	constexpr float radiusSquared = radius * radius;
	float bestDistSq = radiusSquared + 1.0f;
	vec3_t boltOrg;

	const int numEnts = G_GetEntsNearBolt(ent, radiusEnts, radius, ent->handRBolt, boltOrg);

	for (int i = 0; i < numEnts; i++)
	{
		if (!radiusEnts[i]->inuse)
		{
			continue;
		}

		if (radiusEnts[i] == ent)
		{
			//Skip the rancor ent
			continue;
		}

		if (!radiusEnts[i]->inuse || radiusEnts[i]->health <= 0)
		{
			//must be alive
			continue;
		}

		if (radiusEnts[i]->client == nullptr)
		{
			//must be a client
			continue;
		}

		if (radiusEnts[i]->client->ps.eFlags & EF_HELD_BY_RANCOR
			|| radiusEnts[i]->client->ps.eFlags & EF_HELD_BY_WAMPA
			|| radiusEnts[i]->client->ps.eFlags & EF_HELD_BY_SAND_CREATURE)
		{
			//can't be one being held
			continue;
		}

		if (PM_LockedAnim(radiusEnts[i]->client->ps.torsoAnim)
			|| PM_LockedAnim(radiusEnts[i]->client->ps.legsAnim))
		{
			//don't interrupt
			continue;
		}
		if (radiusEnts[i]->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			//must be on ground
			continue;
		}

		if (PM_InOnGroundAnim(&radiusEnts[i]->client->ps))
		{
			//must be standing up
			continue;
		}

		if (fabs(radiusEnts[i]->currentOrigin[2] - ent->currentOrigin[2]) > 8.0f)
		{
			//have to be close in Z
			continue;
		}

		if (!PM_HasAnimation(radiusEnts[i], BOTH_PLAYER_PA_1))
		{
			//doesn't have matching anims
			continue;
		}

		const float distSq = DistanceSquared(radiusEnts[i]->currentOrigin, boltOrg);
		if (distSq < bestDistSq)
		{
			bestDistSq = distSq;
			bestEnt = radiusEnts[i];
		}
	}

	if (bestEnt != nullptr)
	{
		int lockType = LOCK_KYLE_GRAB1;
		if (ucmd->forwardmove > 0)
		{
			lockType = LOCK_KYLE_GRAB3;
		}
		else if (ucmd->forwardmove < 0)
		{
			lockType = LOCK_KYLE_GRAB2;
		}
		WP_SabersCheckLock2(ent, bestEnt, static_cast<sabersLockMode_t>(lockType));
		return qtrue;
	}
	return qfalse;
}

qboolean G_PullAttack(gentity_t* ent, usercmd_t* ucmd)
{
	qboolean overridAngles = qfalse;
	if (ent->client->ps.torsoAnim == BOTH_PULL_IMPALE_STAB
		|| ent->client->ps.torsoAnim == BOTH_PULL_IMPALE_SWING)
	{
		//pulling
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
	}
	else if (ent->client->ps.torsoAnim == BOTH_PULLED_INAIR_B
		|| ent->client->ps.torsoAnim == BOTH_PULLED_INAIR_F)
	{
		//being pulled
		const gentity_t* puller = &g_entities[ent->client->ps.pullAttackEntNum];
		if (puller
			&& puller->inuse
			&& puller->client
			&& (puller->client->ps.torsoAnim == BOTH_PULL_IMPALE_STAB
				|| puller->client->ps.torsoAnim == BOTH_PULL_IMPALE_SWING))
		{
			vec3_t pullDir;
			vec3_t pullPos;
			//calc where to pull me to
			/*
			VectorCopy( puller->client->ps.saber[0].blade[0].muzzlePoint, pullPos );
			VectorMA( pullPos, puller->client->ps.saber[0].blade[0].length*0.5f, puller->client->ps.saber[0].blade[0].muzzleDir, pullPos );
			*/
			vec3_t pullerFwd;
			AngleVectors(puller->client->ps.viewangles, pullerFwd, nullptr, nullptr);
			VectorMA(puller->currentOrigin, puller->maxs[0] * 1.5f + 16.0f, pullerFwd, pullPos);
			//pull me towards that pos
			VectorSubtract(pullPos, ent->currentOrigin, pullDir);
			const float pullDist = VectorNormalize(pullDir);
			const int sweetSpotTime = puller->client->ps.torsoAnim == BOTH_PULL_IMPALE_STAB ? 1250 : 1350;
			float pullLength = PM_AnimLength(puller->client->clientInfo.animFileIndex,
				static_cast<animNumber_t>(puller->client->ps.torsoAnim)) - sweetSpotTime;
			if (pullLength <= 0.25f)
			{
				pullLength = 0.25f;
			}
			//float pullTimeRemaining = ent->client->ps.pullAttackTime - level.time;

			//float pullSpeed = pullDist * (pullTimeRemaining/pullLength);
			const float pullSpeed = pullDist * 1000.0f / pullLength;

			VectorScale(pullDir, pullSpeed, ent->client->ps.velocity);
			//slide, if necessary
			ent->client->ps.pm_flags |= PMF_TIME_NOFRICTION;
			ent->client->ps.pm_time = 100;
			//make it so I don't actually hurt them when pulled at them...
			ent->forcePuller = puller->s.number;
			ent->forcePushTime = level.time + 100; // let the push effect last for 100 more ms
			//look at him
			PM_AdjustAnglesToPuller(ent, puller, ucmd,
				static_cast<qboolean>(ent->client->ps.legsAnim == BOTH_PULLED_INAIR_B));
			overridAngles = qtrue;
			//don't move
			if (ent->NPC)
			{
				VectorClear(ent->client->ps.moveDir);
			}
			ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		}
	}
	return overridAngles;
}

void G_FixMins(gentity_t* ent)
{
	//do a trace to make sure it's okay
	const float downdist = DEFAULT_MINS_2 - ent->mins[2];
	vec3_t end = { ent->currentOrigin[0], ent->currentOrigin[1], ent->currentOrigin[2] + downdist };
	trace_t trace;
	gi.trace(&trace, ent->currentOrigin, ent->mins, ent->maxs, end, ent->s.number, ent->clipmask,
		static_cast<EG2_Collision>(0), 0);
	if (!trace.allsolid && !trace.startsolid)
	{
		if (trace.fraction >= 1.0f)
		{
			//all clear
			//drop the bottom of my bbox back down
			ent->mins[2] = DEFAULT_MINS_2;
			if (ent->client)
			{
				ent->client->ps.pm_flags &= ~PMF_FIX_MINS;
			}
		}
		else
		{
			//move me up so the bottom of my bbox will be where the trace ended, at least
			//need to trace up, too
			const float updist = (1.0f - trace.fraction) * -downdist;
			end[2] = ent->currentOrigin[2] + updist;
			gi.trace(&trace, ent->currentOrigin, ent->mins, ent->maxs, end, ent->s.number, ent->clipmask,
				static_cast<EG2_Collision>(0), 0);
			if (!trace.allsolid && !trace.startsolid)
			{
				if (trace.fraction >= 1.0f)
				{
					//all clear
					//move me up
					ent->currentOrigin[2] += updist;
					//drop the bottom of my bbox back down
					ent->mins[2] = DEFAULT_MINS_2;
					G_SetOrigin(ent, ent->currentOrigin);
					gi.linkentity(ent);
					if (ent->client)
					{
						ent->client->ps.pm_flags &= ~PMF_FIX_MINS;
					}
				}
				else
				{
					//crap, no room to expand!
					if (ent->client->ps.legsAnimTimer <= 200)
					{
						//at the end of the anim, and we can't leave ourselves like this
						//so drop the maxs, put the mins back and move us up
						ent->maxs[2] += downdist;
						ent->currentOrigin[2] -= downdist;
						ent->mins[2] = DEFAULT_MINS_2;
						G_SetOrigin(ent, ent->currentOrigin);
						gi.linkentity(ent);
						//this way we'll be in a crouch when we're done
						ent->client->ps.legsAnimTimer = ent->client->ps.torsoAnimTimer = 0;
						ent->client->ps.pm_flags |= PMF_DUCKED;
						//FIXME: do we need to set a crouch anim here?
						if (ent->client)
						{
							ent->client->ps.pm_flags &= ~PMF_FIX_MINS;
						}
					}
				}
			} //crap, stuck
		}
	} //crap, stuck!
}

extern qboolean G_TuskenAttackAnimDamage(gentity_t* self);
extern void Tusken_StaffTracenew(gentity_t* self);
extern void Noghri_StickTracennew(gentity_t* self);

qboolean G_CheckClampUcmd(gentity_t* ent, usercmd_t* ucmd)
{
	qboolean overridAngles = qfalse;

	if (ent->client->NPC_class == CLASS_PROTOCOL
		|| ent->client->NPC_class == CLASS_R2D2
		|| ent->client->NPC_class == CLASS_R5D2
		|| ent->client->NPC_class == CLASS_GONK
		|| ent->client->NPC_class == CLASS_MOUSE)
	{
		//these droids *cannot* strafe (looks bad)
		if (ucmd->rightmove)
		{
			//clear the strafe
			ucmd->rightmove = 0;
			//movedir is invalid now, but PM_WalkMove will rebuild it from the ucmds, with the appropriate speed
			VectorClear(ent->client->ps.moveDir);
		}
	}

	if (ent->client->ps.pullAttackEntNum < ENTITYNUM_WORLD
		&& ent->client->ps.pullAttackTime > level.time)
	{
		return G_PullAttack(ent, ucmd);
	}

	if ((ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
		&& ent->s.weapon == WP_MELEE
		&& ent->client->ps.torsoAnim == BOTH_KYLE_GRAB)
	{
		//see if we grabbed enemy
		if (ent->client->ps.torsoAnimTimer <= 200)
		{
			//close to end of anim
			if (G_GrabClient(ent, ucmd))
			{
				//grabbed someone!
			}
			else
			{
				//missed
				NPC_SetAnim(ent, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				ent->client->ps.weaponTime = ent->client->ps.torsoAnimTimer;
			}
		}
		ucmd->rightmove = ucmd->forwardmove = ucmd->upmove = 0;
	}

	if (!ent->s.number && ent->aimDebounceTime > level.time
		|| ent->client->ps.pm_time && ent->client->ps.pm_flags & PMF_TIME_KNOCKBACK
		|| ent->forcePushTime > level.time)
	{
		//being knocked back, can't do anything!
		ucmd->buttons = 0;
		ucmd->forwardmove = 0;
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
		}
	}

	overridAngles = PM_AdjustAnglesForKnockdown(ent, ucmd, qfalse) ? qtrue : overridAngles;
	if (PM_GetupAnimNoMove(ent->client->ps.legsAnim))
	{
		ucmd->forwardmove = ucmd->rightmove = 0; //ucmd->upmove = ?
	}
	//check saberlock
	if (ent->client->ps.saberLockTime > level.time)
	{
		//in saberlock
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		if (ent->client->ps.saberLockTime - level.time > SABER_LOCK_DELAYED_TIME)
		{
			//2 second delay before either can push
			ucmd->buttons = 0;
		}
		else
		{
			ucmd->buttons &= ~(ucmd->buttons & ~BUTTON_ATTACK);
		}
		overridAngles = PM_AdjustAnglesForSaberLock(ent, ucmd) ? qtrue : overridAngles;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
		}
	}
	//check force drain
	if (ent->client->ps.forcePowersActive & 1 << FP_DRAIN)
	{
		//draining
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		ucmd->buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_FORCE_FOCUS);
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
		}
	}

	if (ent->client->ps.saberMove == LS_A_LUNGE)
	{
		//can't move during lunge
		ucmd->rightmove = ucmd->upmove = 0;
		if (ent->client->ps.legsAnimTimer > 500 && (ent->s.number || !player_locked && !PlayerAffectedByStasis()))
		{
			ucmd->forwardmove = 127;
		}
		else
		{
			ucmd->forwardmove = 0;
		}
		if (ent->NPC)
		{
			//invalid now
			VectorClear(ent->client->ps.moveDir);
		}
	}

	if (ent->client->ps.legsAnim == BOTH_FORCEWALLRUNFLIP_ALT
		&& ent->client->ps.legsAnimTimer)
	{
		vec3_t vFwd, fwdAng = { 0, ent->currentAngles[YAW], 0 };
		AngleVectors(fwdAng, vFwd, nullptr, nullptr);
		if (ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			float savZ = ent->client->ps.velocity[2];
			VectorScale(vFwd, 100, ent->client->ps.velocity);
			ent->client->ps.velocity[2] = savZ;
		}
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		overridAngles = PM_AdjustAnglesForWallRunUpFlipAlt(ent, ucmd) ? qtrue : overridAngles;
	}

	if (ent->client->ps.saberMove == LS_A_JUMP_T__B_)
	{
		//can't move during leap
		if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE || !ent->s.number && (player_locked ||
			PlayerAffectedByStasis()))
		{
			//hit the ground
			ucmd->forwardmove = 0;
		}
		ucmd->rightmove = ucmd->upmove = 0;
		if (ent->NPC)
		{
			//invalid now
			VectorClear(ent->client->ps.moveDir);
		}
	}

	if (ent->client->ps.saberMove == LS_A_BACKFLIP_ATK
		&& ent->client->ps.legsAnim == BOTH_JUMPATTACK7)
	{
		//backflip attack
		if (ent->client->ps.legsAnimTimer > 800 //not at end
			&& PM_AnimLength(ent->client->clientInfo.animFileIndex, BOTH_JUMPATTACK7) - ent->client->ps.legsAnimTimer >=
			400) //not in beginning
		{
			//middle of anim
			if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				//still on ground?
				vec3_t yawAngles, backDir;

				//push backwards some?
				VectorSet(yawAngles, 0, ent->client->ps.viewangles[YAW] + 180, 0);
				AngleVectors(yawAngles, backDir, nullptr, nullptr);
				VectorScale(backDir, 100, ent->client->ps.velocity);

				//jump!
				ent->client->ps.velocity[2] = 180;
				ent->client->ps.forceJumpZStart = ent->client->ps.origin[2];
				//so we don't take damage if we land at same height
				ent->client->ps.pm_flags |= PMF_JUMPING | PMF_SLOW_MO_FALL;

				//FIXME: NPCs yell?
				G_AddEvent(ent, EV_JUMP, 0);
				if (g_SerenityJediEngineMode->integer == 2 && ent->client->ps.forcePowerLevel[FP_LEVITATION] <
					FORCE_LEVEL_3)
				{
					//short burst
					G_SoundOnEnt(ent, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
				}
				else
				{
					//holding it
					G_SoundOnEnt(ent, CHAN_BODY, "sound/weapons/force/jump.mp3");
				}
				ucmd->upmove = 0; //clear any actual jump command
			}
		}
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		if (ent->NPC)
		{
			//invalid now
			VectorClear(ent->client->ps.moveDir);
		}
	}

	if (ent->client->ps.legsAnim == BOTH_JUMPATTACK6
		&& ent->client->ps.legsAnimTimer > 0)
	{
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		//FIXME: don't slide off people/obstacles?
		if (ent->client->ps.legsAnimTimer >= 100 //not at end
			&& PM_AnimLength(ent->client->clientInfo.animFileIndex, BOTH_JUMPATTACK6) - ent->client->ps.legsAnimTimer >=
			250) //not in beginning
		{
			//middle of anim
			//push forward
			ucmd->forwardmove = 127;
		}

		if (ent->client->ps.legsAnimTimer >= 900 //not at end
			&& PM_AnimLength(ent->client->clientInfo.animFileIndex, BOTH_JUMPATTACK6) - ent->client->ps.
			legsAnimTimer >= 950 //not in beginning
			|| ent->client->ps.legsAnimTimer >= 1600
			&& PM_AnimLength(ent->client->clientInfo.animFileIndex, BOTH_JUMPATTACK6) - ent->client->ps.
			legsAnimTimer >= 400) //not in beginning
		{
			//one of the two jumps
			if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				//still on ground?
				//jump!
				ent->client->ps.velocity[2] = 250;
				ent->client->ps.forceJumpZStart = ent->client->ps.origin[2];
				//so we don't take damage if we land at same height
				ent->client->ps.pm_flags |= PMF_JUMPING; //|PMF_SLOW_MO_FALL;
				//FIXME: NPCs yell?
				G_AddEvent(ent, EV_JUMP, 0);
				if (g_SerenityJediEngineMode->integer == 2 && ent->client->ps.forcePowerLevel[FP_LEVITATION] <
					FORCE_LEVEL_3)
				{
					//short burst
					G_SoundOnEnt(ent, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
				}
				else
				{
					//holding it
					G_SoundOnEnt(ent, CHAN_BODY, "sound/weapons/force/jump.mp3");
				}
			}
			else
			{
				//FIXME: if this is the second jump, maybe we should just stop the anim?
			}
		}

		//dynamically reduce bounding box to let character sail over heads of enemies
		if (ent->client->ps.legsAnimTimer >= 1450
			&& PM_AnimLength(ent->client->clientInfo.animFileIndex, BOTH_JUMPATTACK6) - ent->client->ps.
			legsAnimTimer >= 400
			|| ent->client->ps.legsAnimTimer >= 400
			&& PM_AnimLength(ent->client->clientInfo.animFileIndex, BOTH_JUMPATTACK6) - ent->client->ps.
			legsAnimTimer >= 1100)
		{
			//in a part of the anim that we're pretty much sideways in, raise up the mins
			ent->mins[2] = 0;
			ent->client->ps.pm_flags |= PMF_FIX_MINS;
		}
		else if (ent->client->ps.pm_flags & PMF_FIX_MINS)
		{
			//drop the mins back down
			G_FixMins(ent);
		}
		if (ent->NPC)
		{
			//invalid now
			VectorClear(ent->client->ps.moveDir);
		}
	}
	else if (ent->client->ps.pm_flags & PMF_FIX_MINS)
	{
		G_FixMins(ent);
	}

	if (ent->client->ps.legsAnim == BOTH_BUTTERFLY_FL1
		&& ent->client->ps.saberMove == LS_JUMPATTACK_STAFF_LEFT
		|| ent->client->ps.legsAnim == BOTH_BUTTERFLY_FR1
		&& ent->client->ps.saberMove == LS_JUMPATTACK_STAFF_RIGHT
		|| ent->client->ps.legsAnim == BOTH_BUTTERFLY_RIGHT
		&& ent->client->ps.saberMove == LS_BUTTERFLY_RIGHT
		|| ent->client->ps.legsAnim == BOTH_BUTTERFLY_LEFT
		&& ent->client->ps.saberMove == LS_BUTTERFLY_LEFT)
	{
		//forward flip/spin attack
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;

		/*if ( ent->client->ps.legsAnim == BOTH_BUTTERFLY_FL1
			|| ent->client->ps.legsAnim == BOTH_BUTTERFLY_FR1
			|| ent->client->ps.legsAnim == BOTH_BUTTERFLY_LEFT
			|| ent->client->ps.legsAnim == BOTH_BUTTERFLY RIGHT )*/
		{
			//FIXME: don't slide off people/obstacles?
			if (ent->client->ps.legsAnim != BOTH_BUTTERFLY_LEFT
				&& ent->client->ps.legsAnim != BOTH_BUTTERFLY_RIGHT)
			{
				if (ent->client->ps.legsAnimTimer >= 100 //not at end
					&& PM_AnimLength(ent->client->clientInfo.animFileIndex,
						static_cast<animNumber_t>(ent->client->ps.legsAnim)) - ent->client->ps.
					legsAnimTimer >= 250) //not in beginning
				{
					//middle of anim
					//push forward
					ucmd->forwardmove = 127;
				}
			}

			if (ent->client->ps.legsAnimTimer >= 1700 && ent->client->ps.legsAnimTimer < 1800)
			{
				//one of the two jumps
				if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
				{
					//still on ground?
					//jump!
					ent->client->ps.velocity[2] = 250;
					ent->client->ps.forceJumpZStart = ent->client->ps.origin[2];
					//so we don't take damage if we land at same height
					ent->client->ps.pm_flags |= PMF_JUMPING; //|PMF_SLOW_MO_FALL;
					//FIXME: NPCs yell?
					G_AddEvent(ent, EV_JUMP, 0);
					if (g_SerenityJediEngineMode->integer == 2 && ent->client->ps.forcePowerLevel[FP_LEVITATION] <
						FORCE_LEVEL_3)
					{
						//short burst
						G_SoundOnEnt(ent, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
					}
					else
					{
						//holding it
						G_SoundOnEnt(ent, CHAN_BODY, "sound/weapons/force/jump.mp3");
					}
				}
				else
				{
					//FIXME: if this is the second jump, maybe we should just stop the anim?
				}
			}
		}

		if (ent->NPC)
		{
			//invalid now
			VectorClear(ent->client->ps.moveDir);
		}
	}

	if (ent->client->ps.legsAnim == BOTH_A7_SOULCAL
		&& ent->client->ps.saberMove == LS_STAFF_SOULCAL)
	{
		//forward spinning staff attack
		ucmd->upmove = 0;

		if (PM_CanRollFromSoulCal(&ent->client->ps))
		{
			ucmd->upmove = -127;
		}
		else
		{
			ucmd->rightmove = 0;
			//FIXME: don't slide off people/obstacles?
			if (ent->client->ps.legsAnimTimer >= 2750)
			{
				//not at end
				//push forward
				ucmd->forwardmove = 64;
			}
			else
			{
				ucmd->forwardmove = 0;
			}
		}
		if (ent->client->ps.legsAnimTimer >= 2650
			&& ent->client->ps.legsAnimTimer < 2850)
		{
			//the jump
			if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				//still on ground?
				//jump!
				ent->client->ps.velocity[2] = 250;
				ent->client->ps.forceJumpZStart = ent->client->ps.origin[2];
				//so we don't take damage if we land at same height
				ent->client->ps.pm_flags |= PMF_JUMPING; //|PMF_SLOW_MO_FALL;
				//FIXME: NPCs yell?
				G_AddEvent(ent, EV_JUMP, 0);
				if (g_SerenityJediEngineMode->integer == 2 && ent->client->ps.forcePowerLevel[FP_LEVITATION] <
					FORCE_LEVEL_3)
				{
					//short burst
					G_SoundOnEnt(ent, CHAN_BODY, "sound/weapons/force/jumpsmall.mp3");
				}
				else
				{
					//holding it
					G_SoundOnEnt(ent, CHAN_BODY, "sound/weapons/force/jump.mp3");
				}
			}
		}

		if (ent->NPC)
		{
			//invalid now
			VectorClear(ent->client->ps.moveDir);
		}
	}

	if (ent->client->ps.torsoAnim == BOTH_LK_DL_S_T_SB_1_W)
	{
		G_CamPullBackForLegsAnim(ent, qtrue);
	}

	if (ent->client->ps.torsoAnim == BOTH_A6_FB
		|| ent->client->ps.torsoAnim == BOTH_A6_LR)
	{
		//can't turn or move during dual attack
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
	}
	else if (ent->client->ps.legsAnim == BOTH_ROLL_STAB)
	{
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		if (ent->client->ps.legsAnimTimer)
		{
			ucmd->upmove = -127;
		}
		if (ent->client->ps.dualSabers && ent->client->ps.saber[1].Active())
		{
			G_CamPullBackForLegsAnim(ent);
		}
	}
	else if (PM_SuperBreakLoseAnim(ent->client->ps.torsoAnim))
	{
		//can't turn during Kyle's grappling
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		if (ent->client->ps.legsAnim == BOTH_LK_DL_ST_T_SB_1_L)
		{
			PM_CmdForRoll(&ent->client->ps, ucmd);
			if (ent->NPC)
			{
				//invalid now
				VectorClear(ent->client->ps.moveDir);
			}
			ent->client->ps.speed = 400;
		}
	}
	else if (ent->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START
		|| ent->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD
		|| ent->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_END
		|| ent->client->ps.legsAnim == BOTH_FORCE_DRAIN_GRABBED)
	{
		//can't turn or move
		if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
		{
			//player
			float forceDrainAngle = 90.0f;
			if (ent->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START)
			{
				//starting drain
				float animLength = PM_AnimLength(ent->client->clientInfo.animFileIndex,
					static_cast<animNumber_t>(ent->client->ps.torsoAnim));
				float elapsedTime = animLength - ent->client->ps.legsAnimTimer;
				float angle = elapsedTime / animLength * forceDrainAngle;
				cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_ANG;
				cg.overrides.thirdPersonAngle = cg_thirdPersonAngle.value + angle;
			}
			else if (ent->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD)
			{
				//draining
				cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_ANG;
				cg.overrides.thirdPersonAngle = cg_thirdPersonAngle.value + forceDrainAngle;
			}
			else if (ent->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_END)
			{
				//ending drain
				float animLength = PM_AnimLength(ent->client->clientInfo.animFileIndex,
					static_cast<animNumber_t>(ent->client->ps.torsoAnim));
				float elapsedTime = animLength - ent->client->ps.legsAnimTimer;
				float angle = forceDrainAngle - elapsedTime / animLength * forceDrainAngle;
				cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_ANG;
				cg.overrides.thirdPersonAngle = cg_thirdPersonAngle.value + angle;
			}
		}

		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
		}
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
	}
	else if (ent->client->ps.legsAnim == BOTH_PLAYER_PA_1
		|| ent->client->ps.legsAnim == BOTH_PLAYER_PA_2
		|| ent->client->ps.legsAnim == BOTH_PLAYER_PA_3
		|| ent->client->ps.legsAnim == BOTH_PLAYER_PA_3_FLY
		|| ent->client->ps.legsAnim == BOTH_KYLE_PA_1
		|| ent->client->ps.legsAnim == BOTH_KYLE_PA_2
		|| ent->client->ps.legsAnim == BOTH_KYLE_PA_3)
	{
		//can't turn during Kyle's grappling
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
		}
		overridAngles = PM_AdjustAnglesForGrapple(ent, ucmd) ? qtrue : overridAngles;
		{
			//actually do some damage during sequence
			int dflags = DAMAGE_NO_KNOCKBACK | DAMAGE_NO_ARMOR | DAMAGE_NO_KILL;
			if (TIMER_Done(ent, "grappleDamageDebounce"))
			{
				int damage = 0;
				switch (ent->client->ps.legsAnim)
				{
				case BOTH_PLAYER_PA_1:
					if (ent->client->ps.legsAnimTimer <= 3150
						&& ent->client->ps.legsAnimTimer > 3050)
					{
						TIMER_Set(ent, "grappleDamageDebounce", 150);
						if (ent->s.number < MAX_CLIENTS)
						{
							//special case
							damage = Q_irand(1, 3);
						}
						else
						{
							damage = Q_irand(3, 5);
						}
					}
					if (ent->client->ps.legsAnimTimer <= 1150
						&& ent->client->ps.legsAnimTimer > 1050)
					{
						TIMER_Set(ent, "grappleDamageDebounce", 150);
						if (ent->s.number < MAX_CLIENTS)
						{
							//special case
							damage = Q_irand(3, 5);
						}
						else
						{
							damage = Q_irand(5, 8);
						}
					}
					if (ent->client->ps.legsAnimTimer <= 100)
					{
						TIMER_Set(ent, "grappleDamageDebounce", 150);
						if (ent->s.number < MAX_CLIENTS)
						{
							//special case
							damage = Q_irand(5, 8);
						}
						else
						{
							damage = Q_irand(10, 20);
						}
						dflags &= ~DAMAGE_NO_KILL;
					}
					break;
				case BOTH_PLAYER_PA_2:
					if (ent->client->ps.legsAnimTimer <= 5700
						&& ent->client->ps.legsAnimTimer > 5600)
					{
						TIMER_Set(ent, "grappleDamageDebounce", 150);
						if (ent->s.number < MAX_CLIENTS)
						{
							//special case
							damage = Q_irand(3, 6);
						}
						else
						{
							damage = Q_irand(7, 10);
						}
					}
					if (ent->client->ps.legsAnimTimer <= 5200
						&& ent->client->ps.legsAnimTimer > 5100)
					{
						TIMER_Set(ent, "grappleDamageDebounce", 150);
						if (ent->s.number < MAX_CLIENTS)
						{
							//special case
							damage = Q_irand(1, 3);
						}
						else
						{
							damage = Q_irand(3, 5);
						}
					}
					if (ent->client->ps.legsAnimTimer <= 4550
						&& ent->client->ps.legsAnimTimer > 4450)
					{
						TIMER_Set(ent, "grappleDamageDebounce", 150);
						if (ent->s.number < MAX_CLIENTS)
						{
							//special case
							damage = Q_irand(3, 6);
						}
						else
						{
							damage = Q_irand(10, 15);
						}
					}
					if (ent->client->ps.legsAnimTimer <= 3550
						&& ent->client->ps.legsAnimTimer > 3450)
					{
						TIMER_Set(ent, "grappleDamageDebounce", 150);
						if (ent->s.number < MAX_CLIENTS)
						{
							//special case
							damage = Q_irand(3, 6);
						}
						else
						{
							damage = Q_irand(10, 20);
						}
						dflags &= ~DAMAGE_NO_KILL;
					}
					break;
				case BOTH_PLAYER_PA_3:
					if (ent->client->ps.legsAnimTimer <= 3800
						&& ent->client->ps.legsAnimTimer > 3700)
					{
						TIMER_Set(ent, "grappleDamageDebounce", 150);
						if (ent->s.number < MAX_CLIENTS)
						{
							//special case
							damage = Q_irand(2, 5);
						}
						else
						{
							damage = Q_irand(5, 8);
						}
					}
					if (ent->client->ps.legsAnimTimer <= 3100
						&& ent->client->ps.legsAnimTimer > 3000)
					{
						TIMER_Set(ent, "grappleDamageDebounce", 150);
						if (ent->s.number < MAX_CLIENTS)
						{
							//special case
							damage = Q_irand(2, 5);
						}
						else
						{
							damage = Q_irand(5, 8);
						}
					}
					if (ent->client->ps.legsAnimTimer <= 1650
						&& ent->client->ps.legsAnimTimer > 1550)
					{
						TIMER_Set(ent, "grappleDamageDebounce", 150);
						if (ent->s.number < MAX_CLIENTS)
						{
							//special case
							damage = Q_irand(3, 6);
						}
						else
						{
							damage = Q_irand(7, 12);
						}
					}
					break;
				case BOTH_PLAYER_PA_3_FLY:
					if (ent->client->ps.legsAnimTimer <= 100)
					{
						TIMER_Set(ent, "grappleDamageDebounce", 150);
						damage = Q_irand(10, 20);
						dflags &= ~DAMAGE_NO_KILL;
					}
					break;
				default:;
				}
				if (damage)
				{
					G_Damage(ent, ent->enemy, ent->enemy, nullptr, ent->currentOrigin, damage, dflags, MOD_MELEE);
				}
			}
		}
		qboolean clearMove = qtrue;
		int endOfAnimTime = 100;
		if (ent->s.number < MAX_CLIENTS
			&& ent->client->ps.legsAnim == BOTH_PLAYER_PA_3_FLY)
		{
			//player holds extra long so you have more time to decide to do the quick getup
			//endOfAnimTime += PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME;
			if (ent->client->ps.legsAnimTimer <= endOfAnimTime) //PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME )
			{
				clearMove = qfalse;
			}
		}
		if (clearMove)
		{
			ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		}

		if (ent->client->ps.legsAnimTimer <= endOfAnimTime)
		{
			//pretty much done with the anim, so get up now
			if (ent->client->ps.legsAnim == BOTH_PLAYER_PA_3)
			{
				vec3_t ang = { 10, ent->currentAngles[YAW], 0 };
				vec3_t throwDir;
				AngleVectors(ang, throwDir, nullptr, nullptr);
				if (!(ent->flags & FL_NO_KNOCKBACK))
				{
					G_Throw(ent, throwDir, -100);
				}
				ent->client->ps.legsAnimTimer = ent->client->ps.torsoAnimTimer = 0;
				NPC_SetAnim(ent, SETANIM_BOTH, BOTH_PLAYER_PA_3_FLY, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				ent->client->ps.weaponTime = ent->client->ps.legsAnimTimer;
				ent->forcePushTime = level.time + 500;
			}
			else if (ent->client->ps.legsAnim == BOTH_PLAYER_PA_1)
			{
				vec3_t ang = { 10, ent->currentAngles[YAW], 0 };
				vec3_t throwDir;
				AngleVectors(ang, throwDir, nullptr, nullptr);
				if (!(ent->flags & FL_NO_KNOCKBACK))
				{
					G_Throw(ent, throwDir, -100);
				}
				ent->client->ps.legsAnimTimer = ent->client->ps.torsoAnimTimer = 0;
				NPC_SetAnim(ent, SETANIM_BOTH, BOTH_KNOCKDOWN2, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				ent->client->ps.weaponTime = ent->client->ps.legsAnimTimer;
				if (ent->s.number < MAX_CLIENTS)
				{
					//player holds extra long so you have more time to decide to do the quick getup
					ent->client->ps.legsAnimTimer += PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME;
					ent->client->ps.torsoAnimTimer += PLAYER_KNOCKDOWN_HOLD_EXTRA_TIME;
				}
			}
		}
		if (d_slowmodeath->integer <= 3
			&& ent->s.number < MAX_CLIENTS)
		{
			//no matrix effect, so slide the camera back and to the side
			G_CamPullBackForLegsAnim(ent);
			G_CamCircleForLegsAnim(ent);
		}
	}
	else if (ent->client->ps.legsAnim == BOTH_FORCELONGLEAP_START
		|| ent->client->ps.legsAnim == BOTH_FORCELONGLEAP_ATTACK
		|| ent->client->ps.legsAnim == BOTH_FORCELONGLEAP_LAND)
	{
		//can't turn during force leap
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
		}
		overridAngles = PM_AdjustAnglesForLongJump(ent, ucmd) ? qtrue : overridAngles;
	}
	else if (ent->s.number < MAX_CLIENTS && G_TuskenAttackAnimDamage(ent) &&
		(ent->s.weapon == WP_NOGHRI_STICK || ent->s.weapon == WP_TUSKEN_RIFLE || ent->s.weapon == WP_TUSKEN_STAFF))
	{
		//reuse saber trace timer for this. these traces are only meant to be called every 50ms
		//really should rework to be more like other traces.

		int wait = FRAMETIME / 2;
		//sanity check

		if (ent->client->ps.saberDamageDebounceTime - level.time > wait)
		{
			//when you unpause the game with force speed on, the time gets *really* wiggy...
			ent->client->ps.saberDamageDebounceTime = level.time + wait;
		}
		if (ent->client->ps.saberDamageDebounceTime <= level.time)
		{
			if (ent->s.weapon == WP_NOGHRI_STICK)
				Noghri_StickTracennew(ent);
			else if (ent->s.weapon == WP_TUSKEN_RIFLE || ent->s.weapon == WP_TUSKEN_STAFF)
				Tusken_StaffTracenew(ent);

			ent->client->ps.saberDamageDebounceTime = level.time + wait;
		}
	}
	else if (PM_KickingAnim(ent->client->ps.legsAnim)
		|| ent->client->ps.legsAnim == BOTH_ARIAL_F1
		|| ent->client->ps.legsAnim == BOTH_ARIAL_LEFT
		|| ent->client->ps.legsAnim == BOTH_ARIAL_RIGHT
		|| ent->client->ps.legsAnim == BOTH_CARTWHEEL_LEFT
		|| ent->client->ps.legsAnim == BOTH_CARTWHEEL_RIGHT
		|| ent->client->ps.legsAnim == BOTH_JUMPATTACK7
		|| ent->client->ps.legsAnim == BOTH_BUTTERFLY_FL1
		|| ent->client->ps.legsAnim == BOTH_BUTTERFLY_FR1
		|| ent->client->ps.legsAnim == BOTH_BUTTERFLY_RIGHT
		|| ent->client->ps.legsAnim == BOTH_BUTTERFLY_LEFT
		|| ent->client->ps.legsAnim == BOTH_A7_SOULCAL)
	{
		//can't move, check for damage frame
		if (PM_KickingAnim(ent->client->ps.legsAnim))
		{
			ucmd->forwardmove = ucmd->rightmove = 0;
			if (ent->NPC)
			{
				VectorClear(ent->client->ps.moveDir);
			}
		}
		vec3_t kickDir = { 0, 0, 0 }, kickDir2 = { 0, 0, 0 }, kickEnd = { 0, 0, 0 }, kickEnd2 = { 0, 0, 0 }, fwdAngs = {
				   0, ent->client->ps.viewangles[YAW], 0
		};
		float animLength = PM_AnimLength(ent->client->clientInfo.animFileIndex,
			static_cast<animNumber_t>(ent->client->ps.legsAnim));
		float elapsedTime = animLength - ent->client->ps.legsAnimTimer;
		float remainingTime = animLength - elapsedTime;
		float kickDist = ent->maxs[0] * 1.5f + STAFF_KICK_RANGE + 8.0f; //fudge factor of 8
		float kickDist2 = kickDist;
		int kickDamage = Q_irand(3, 8);
		int kickDamage2 = Q_irand(3, 8);
		int kickPush = Q_flrand(50.0f, 100.0f);
		int kickPush2 = Q_flrand(50.0f, 100.0f);
		qboolean doKick = qfalse;
		qboolean doKick2 = qfalse;
		qboolean kickSoundOnWalls = qfalse;

		if (ent->client->ps.torsoAnim == BOTH_A7_HILT)
		{
			if (elapsedTime >= 250 && remainingTime >= 250)
			{
				//front
				doKick = qtrue;
				if (ent->handRBolt != -1)
				{
					//actually trace to a bolt
					G_GetBoltPosition(ent, ent->handRBolt, kickEnd);
					VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
					kickDir[2] = 0; //ah, flatten it, I guess...
					VectorNormalize(kickDir);
					//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
					VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
				}
				else
				{
					//guess
					AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
				}
			}
		}
		else
		{
			switch (ent->client->ps.legsAnim)
			{
			case BOTH_A7_SOULCAL:
				kickPush = Q_flrand(150.0f, 250.0f);
				if (elapsedTime >= 1400 && elapsedTime <= 1500)
				{
					//right leg
					doKick = qtrue;
					if (ent->footRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
				}
				break;
			case BOTH_ARIAL_F1:
				if (elapsedTime >= 550 && elapsedTime <= 1000)
				{
					//right leg
					doKick = qtrue;
					if (ent->footRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
				}
				if (elapsedTime >= 800 && elapsedTime <= 1200)
				{
					//left leg
					doKick2 = qtrue;
					if (ent->footLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footLBolt, kickEnd2);
						VectorSubtract(kickEnd2, ent->currentOrigin, kickDir2);
						kickDir2[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir2);
						VectorMA(kickEnd2, 8.0f, kickDir2, kickEnd2);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir2, nullptr, nullptr);
					}
				}
				break;
			case BOTH_ARIAL_LEFT:
			case BOTH_ARIAL_RIGHT:
				if (elapsedTime >= 200 && elapsedTime <= 600)
				{
					//lead leg
					int footBolt = ent->client->ps.legsAnim == BOTH_ARIAL_LEFT ? ent->footRBolt : ent->footLBolt;
					//mirrored anims
					doKick = qtrue;
					if (footBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, footBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
				}
				if (elapsedTime >= 400 && elapsedTime <= 850)
				{
					//trailing leg
					int footBolt = ent->client->ps.legsAnim == BOTH_ARIAL_LEFT ? ent->footLBolt : ent->footRBolt;
					//mirrored anims
					doKick2 = qtrue;
					if (footBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, footBolt, kickEnd2);
						VectorSubtract(kickEnd2, ent->currentOrigin, kickDir2);
						kickDir2[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir2);
						VectorMA(kickEnd2, 8.0f, kickDir2, kickEnd2);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir2, nullptr, nullptr);
					}
				}
				break;
			case BOTH_CARTWHEEL_LEFT:
			case BOTH_CARTWHEEL_RIGHT:
				if (elapsedTime >= 200 && elapsedTime <= 600)
				{
					//lead leg
					int footBolt = ent->client->ps.legsAnim == BOTH_CARTWHEEL_LEFT ? ent->footRBolt : ent->footLBolt;
					//mirrored anims
					doKick = qtrue;
					if (footBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, footBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
				}
				if (elapsedTime >= 350 && elapsedTime <= 650)
				{
					//trailing leg
					int footBolt = ent->client->ps.legsAnim == BOTH_CARTWHEEL_LEFT ? ent->footLBolt : ent->footRBolt;
					//mirrored anims
					doKick2 = qtrue;
					if (footBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, footBolt, kickEnd2);
						VectorSubtract(kickEnd2, ent->currentOrigin, kickDir2);
						kickDir2[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir2);
						VectorMA(kickEnd2, 8.0f, kickDir2, kickEnd2);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir2, nullptr, nullptr);
					}
				}
				break;
			case BOTH_JUMPATTACK7:
				if (elapsedTime >= 300 && elapsedTime <= 900)
				{
					//right knee
					doKick = qtrue;
					if (ent->kneeRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->kneeRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
				}
				if (elapsedTime >= 600 && elapsedTime <= 900)
				{
					//left leg
					doKick2 = qtrue;
					if (ent->footLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footLBolt, kickEnd2);
						VectorSubtract(kickEnd2, ent->currentOrigin, kickDir2);
						kickDir2[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir2);
						VectorMA(kickEnd2, 8.0f, kickDir2, kickEnd2);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir2, nullptr, nullptr);
					}
				}
				break;
			case BOTH_BUTTERFLY_FL1:
			case BOTH_BUTTERFLY_FR1:
				if (elapsedTime >= 950 && elapsedTime <= 1300)
				{
					//lead leg
					int footBolt = ent->client->ps.legsAnim == BOTH_BUTTERFLY_FL1 ? ent->footRBolt : ent->footLBolt;
					//mirrored anims
					doKick = qtrue;
					if (footBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, footBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
				}
				if (elapsedTime >= 1150 && elapsedTime <= 1600)
				{
					//trailing leg
					int footBolt = ent->client->ps.legsAnim == BOTH_BUTTERFLY_FL1 ? ent->footLBolt : ent->footRBolt;
					//mirrored anims
					doKick2 = qtrue;
					if (footBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, footBolt, kickEnd2);
						VectorSubtract(kickEnd2, ent->currentOrigin, kickDir2);
						kickDir2[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir2);
						VectorMA(kickEnd2, 8.0f, kickDir2, kickEnd2);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir2, nullptr, nullptr);
					}
				}
				break;
			case BOTH_BUTTERFLY_LEFT:
			case BOTH_BUTTERFLY_RIGHT:
				if (elapsedTime >= 100 && elapsedTime <= 450
					|| elapsedTime >= 1100 && elapsedTime <= 1350)
				{
					//lead leg
					int footBolt = ent->client->ps.legsAnim == BOTH_BUTTERFLY_LEFT ? ent->footLBolt : ent->footRBolt;
					//mirrored anims
					doKick = qtrue;
					if (footBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, footBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
				}
				if (elapsedTime >= 900 && elapsedTime <= 1600)
				{
					//trailing leg
					int footBolt = ent->client->ps.legsAnim == BOTH_BUTTERFLY_LEFT ? ent->footRBolt : ent->footLBolt;
					//mirrored anims
					doKick2 = qtrue;
					if (footBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, footBolt, kickEnd2);
						VectorSubtract(kickEnd2, ent->currentOrigin, kickDir2);
						kickDir2[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir2);
						VectorMA(kickEnd2, 8.0f, kickDir2, kickEnd2);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir2, nullptr, nullptr);
					}
				}
				break;
			case BOTH_GETUP_BROLL_B:
			case BOTH_GETUP_BROLL_F:
			case BOTH_GETUP_FROLL_B:
			case BOTH_GETUP_FROLL_F:
				kickPush = Q_flrand(150.0f, 250.0f); //Q_flrand( 75.0f, 125.0f );
				if (elapsedTime >= 250 && remainingTime >= 250)
				{
					//front
					doKick = qtrue;
					if (ent->footRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
				}
				if (ent->client->ps.legsAnim == BOTH_GETUP_BROLL_B
					|| ent->client->ps.legsAnim == BOTH_GETUP_FROLL_B)
				{
					//rolling back, pull back the view
					G_CamPullBackForLegsAnim(ent);
				}
				break;
			case BOTH_FLYING_KICK:
			case BOTH_A7_KICK_F_AIR:
				kickPush = Q_flrand(150.0f, 250.0f);
				kickSoundOnWalls = qtrue;
				if (elapsedTime >= 100 && remainingTime >= 500)
				{
					//front
					doKick = qtrue;
					if (ent->footRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 16.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
				}
				break;
			case BOTH_A7_KICK_F:
				kickSoundOnWalls = qtrue;
				//FIXME: push forward?
				if (elapsedTime >= 250 && remainingTime >= 250)
				{
					//front
					doKick = qtrue;
					if (ent->footRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
				}
				break;
			case BOTH_KICK_F_MD:
				kickSoundOnWalls = qtrue;
				//FIXME: push forward?
				if (elapsedTime >= 250 && remainingTime >= 250)
				{
					//front
					doKick = qtrue;
					if (ent->footRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 16.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
				}
				break;
			case BOTH_A7_KICK_B_AIR:
				kickPush = Q_flrand(150.0f, 250.0f); //Q_flrand( 75.0f, 125.0f );
				kickSoundOnWalls = qtrue;
				if (elapsedTime >= 100 && remainingTime >= 400)
				{
					//back
					doKick = qtrue;
					if (ent->footRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
				}
				break;
			case BOTH_A7_KICK_B:
				kickSoundOnWalls = qtrue;
				if (elapsedTime >= 250 && remainingTime >= 250)
				{
					//back
					doKick = qtrue;
					if (ent->footRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
				}
				break;
			case BOTH_SWEEP_KICK:
				kickSoundOnWalls = qtrue;
				if (elapsedTime >= 250 && remainingTime >= 250)
				{
					//back
					doKick = qtrue;
					if (ent->footRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 16.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
				}
				break;
			case BOTH_A7_KICK_R_AIR:
				kickPush = Q_flrand(150.0f, 250.0f); //Q_flrand( 75.0f, 125.0f );
				kickSoundOnWalls = qtrue;
				if (elapsedTime >= 150 && remainingTime >= 300)
				{
					//left
					doKick = qtrue;
					if (ent->footLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
				}
				break;
			case BOTH_A7_KICK_R:
				kickSoundOnWalls = qtrue;
				//FIXME: push right?
				if (elapsedTime >= 250 && remainingTime >= 250)
				{
					//right
					doKick = qtrue;
					if (ent->footRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
					}
				}
				break;
				//case BOTH_A7_SLAP_R:
				//	if (level.framenum & 1)
				//	{
				//		//back
				//		int handBolt = ent->client->ps.torsoAnim == BOTH_A7_SLAP_R ? ent->handRBolt : ent->handLBolt;
				//		//mirrored anims
				//		doKick = qtrue;
				//		kickDist = 80;
				//		if (handBolt != -1) //changed to accomadate teh new hand hand anim
				//		{
				//			//actually trace to a bolt
				//			G_GetBoltPosition(ent, handBolt, kickEnd);
				//			VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
				//			kickDir[2] = 0; //ah, flatten it, I guess...
				//			VectorNormalize(kickDir);
				//			VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
				//		}
				//	}
				//	else if (ent->handLBolt != -1) //for hand slap
				//	{
				//		//actually trace to a bolt
				//		if (elapsedTime >= 250 && remainingTime >= 250)
				//		{
				//			//right, though either would do
				//			doKick = qtrue;
				//			kickDist = 80;
				//			G_GetBoltPosition(ent, ent->handLBolt, kickEnd);
				//			VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
				//			kickDir[2] = 0;
				//			VectorNormalize(kickDir);
				//			VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
				//		}
				//	}
				//	else if (ent->elbowLBolt != -1) //for hand slap
				//	{
				//		//actually trace to a bolt
				//		if (elapsedTime >= 250 && remainingTime >= 250)
				//		{
				//			//right, though either would do
				//			doKick = qtrue;
				//			kickDist = 80;
				//			G_GetBoltPosition(ent, ent->elbowLBolt, kickEnd);
				//			VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
				//			kickDir[2] = 0;
				//			VectorNormalize(kickDir);
				//			VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
				//		}
				//	}
				//	break;
			case BOTH_SLAP_R:
			case BOTH_A7_SLAP_R:
				kickSoundOnWalls = qtrue;

				if (level.framenum & 1)
				{
					//right
					doKick = qtrue;
					if (ent->handRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->handRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else if (ent->elbowRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->elbowRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
					}
				}
				break;
				//case MELEE_STANCE_HOLD_T://right slap
				//case MELEE_STANCE_HOLD_RT://right slap
				//case MELEE_STANCE_HOLD_BR:
				//case BOTH_MELEE_R://right slap
			case BOTH_MELEE2: //right slap
				//case BOTH_MELEE4://right slap
				//case BOTH_MELEEUP://right slap
				//case BOTH_WOOKIE_SLAP:
			case BOTH_FJSS_TL_BR:
				kickSoundOnWalls = qtrue;
				//FIXME: push right?
				if (level.framenum & 1)
				{
					//right
					doKick = qtrue;
					if (ent->handRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->handRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else if (ent->handLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->handLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else if (ent->elbowRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->elbowRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
					}
				}
				break;
			case BOTH_TUSKENLUNGE1:
			case BOTH_TUSKENATTACK2:
				kickSoundOnWalls = qtrue;
				if (level.framenum & 1)
				{
					//right
					doKick = qtrue;
					if (ent->handRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->handRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else if (ent->handLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->handLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else if (ent->elbowRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->elbowRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else if (ent->elbowLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->elbowLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
					}
				}
				break;
			case BOTH_A7_KICK_L_AIR:
				kickPush = Q_flrand(150.0f, 250.0f); //Q_flrand( 75.0f, 125.0f );
				kickSoundOnWalls = qtrue;
				if (elapsedTime >= 150 && remainingTime >= 300)
				{
					//left
					doKick = qtrue;
					if (ent->footLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
				}
				break;
			case BOTH_A7_KICK_L:
				kickSoundOnWalls = qtrue;
				//FIXME: push left?
				if (elapsedTime >= 250 && remainingTime >= 250)
				{
					//left
					doKick = qtrue;
					if (ent->footLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
				}
				break;
			case BOTH_A7_SLAP_L:
				if (level.framenum & 1)
				{
					//back
					int handBolt = ent->client->ps.torsoAnim == BOTH_A7_SLAP_L ? ent->handRBolt : ent->handLBolt;
					//mirrored anims
					doKick = qtrue;
					kickDist = 80;
					if (handBolt != -1) //changed to accommodate teh new hand hand anim
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, handBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
				}
				else if (ent->handLBolt != -1) //for hand slap
				{
					//actually trace to a bolt
					if (elapsedTime >= 250 && remainingTime >= 250)
					{
						//right, though either would do
						doKick = qtrue;
						kickDist = 80;
						G_GetBoltPosition(ent, ent->handLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0;
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
				}
				else if (ent->elbowLBolt != -1) //for hand slap
				{
					//actually trace to a bolt
					if (elapsedTime >= 250 && remainingTime >= 250)
					{
						//right, though either would do
						doKick = qtrue;
						kickDist = 80;
						G_GetBoltPosition(ent, ent->elbowLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0;
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
				}
				break;
			case BOTH_SLAP_L:
				kickSoundOnWalls = qtrue;
				if (level.framenum & 1)
				{
					//right
					doKick = qtrue;
					if (ent->handLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->handLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else if (ent->elbowLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->elbowLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
				}
				break;
				//case MELEE_STANCE_HOLD_LT://left slap
				//case MELEE_STANCE_HOLD_BL://left slap
				//case MELEE_STANCE_HOLD_B://left slap
				//case BOTH_MELEE_L://left slap
			case BOTH_MELEE1: //left slap
				//case BOTH_MELEE3://left slap
			case BOTH_FJSS_TR_BL:
				kickSoundOnWalls = qtrue;
				//FIXME: push left?
				if (level.framenum & 1)
				{
					//right
					doKick = qtrue;
					if (ent->handLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->handLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else if (ent->handRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->handRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
				}
				break;
			case BOTH_TUSKENATTACK1:
			case BOTH_TUSKENATTACK3:
				kickSoundOnWalls = qtrue;
				//FIXME: push left?
				if (level.framenum & 1)
				{
					//right
					doKick = qtrue;
					if (ent->handLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->handLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else if (ent->handRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->handRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else if (ent->elbowLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->elbowLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else if (ent->elbowRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->elbowRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						VectorMA(kickEnd, 20.0f, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
				}
				break;
			case BOTH_A7_KICK_S:
				kickPush = Q_flrand(150.0f, 250.0f);
				if (ent->footRBolt != -1)
				{
					//actually trace to a bolt
					if (elapsedTime >= 550
						&& elapsedTime <= 1050)
					{
						doKick = qtrue;
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
						VectorMA(kickEnd, 8.0f, kickDir, kickEnd);
					}
				}
				else
				{
					//guess
					if (elapsedTime >= 400 && elapsedTime < 500)
					{
						//front
						doKick = qtrue;
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
					else if (elapsedTime >= 500 && elapsedTime < 600)
					{
						//front-right?
						doKick = qtrue;
						fwdAngs[YAW] += 45;
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
					else if (elapsedTime >= 600 && elapsedTime < 700)
					{
						//right
						doKick = qtrue;
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
					}
					else if (elapsedTime >= 700 && elapsedTime < 800)
					{
						//back-right?
						doKick = qtrue;
						fwdAngs[YAW] += 45;
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
					}
					else if (elapsedTime >= 800 && elapsedTime < 900)
					{
						//back
						doKick = qtrue;
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
					else if (elapsedTime >= 900 && elapsedTime < 1000)
					{
						//back-left?
						doKick = qtrue;
						fwdAngs[YAW] += 45;
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
					else if (elapsedTime >= 1000 && elapsedTime < 1100)
					{
						//left
						doKick = qtrue;
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
					else if (elapsedTime >= 1100 && elapsedTime < 1200)
					{
						//front-left?
						doKick = qtrue;
						fwdAngs[YAW] += 45;
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
				}
				break;
			case BOTH_A7_KICK_BF:
				kickPush = Q_flrand(150.0f, 250.0f); //Q_flrand( 75.0f, 125.0f );
				if (elapsedTime < 1500)
				{
					//auto-aim!
					overridAngles = PM_AdjustAnglesForBFKick(ent, ucmd, fwdAngs,
						static_cast<qboolean>(elapsedTime < 850))
						? qtrue
						: overridAngles;
				}
				if (ent->footRBolt != -1)
				{
					//actually trace to a bolt
					if (elapsedTime >= 750 && elapsedTime < 850
						|| elapsedTime >= 1400 && elapsedTime < 1500)
					{
						//right, though either would do
						doKick = qtrue;
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
						VectorMA(kickEnd, 8, kickDir, kickEnd);
					}
				}
				else
				{
					//guess
					if (elapsedTime >= 250 && elapsedTime < 350)
					{
						//front
						doKick = qtrue;
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
					}
					else if (elapsedTime >= 350 && elapsedTime < 450)
					{
						//back
						doKick = qtrue;
						AngleVectors(fwdAngs, kickDir, nullptr, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
				}
				break;
			case BOTH_A7_KICK_RL:
				kickSoundOnWalls = qtrue;
				kickPush = Q_flrand(150.0f, 250.0f);
				if (elapsedTime >= 250 && elapsedTime < 350)
				{
					//right
					doKick = qtrue;
					if (ent->footRBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footRBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
						VectorMA(kickEnd, 8, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
					}
				}
				else if (elapsedTime >= 350 && elapsedTime < 450)
				{
					//left
					doKick = qtrue;
					if (ent->footLBolt != -1)
					{
						//actually trace to a bolt
						G_GetBoltPosition(ent, ent->footLBolt, kickEnd);
						VectorSubtract(kickEnd, ent->currentOrigin, kickDir);
						kickDir[2] = 0; //ah, flatten it, I guess...
						VectorNormalize(kickDir);
						//NOTE: have to fudge this a little because it's not getting enough range with the anim as-is
						VectorMA(kickEnd, 8, kickDir, kickEnd);
					}
					else
					{
						//guess
						AngleVectors(fwdAngs, nullptr, kickDir, nullptr);
						VectorScale(kickDir, -1, kickDir);
					}
				}
				break;
			default:;
			}
		}
		if (doKick)
		{
			G_KickTrace(ent, kickDir, kickDist, kickEnd, kickDamage, kickPush, kickSoundOnWalls);
		}
		if (doKick2)
		{
			G_KickTrace(ent, kickDir2, kickDist2, kickEnd2, kickDamage2, kickPush2, kickSoundOnWalls);
		}
	}
	else if (ent->client->ps.saberMove == LS_DUAL_FB)
	{
		//pull back the view
		G_CamPullBackForLegsAnim(ent);
	}
	else if (ent->client->ps.saberMove == LS_A_BACK || ent->client->ps.saberMove == LS_A_BACK_CR
		|| ent->client->ps.saberMove == LS_A_BACKSTAB)
	{
		//can't move or turn during back attacks
		ucmd->forwardmove = ucmd->rightmove = 0;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
		}
		if ((overridAngles = PM_AdjustAnglesForBackAttack(ent, ucmd) ? qtrue : overridAngles) == qtrue)
		{
			//pull back the view
			G_CamPullBackForLegsAnim(ent);
		}
	}
	else if (ent->client->ps.torsoAnim == BOTH_WALL_FLIP_BACK1
		|| ent->client->ps.torsoAnim == BOTH_WALL_FLIP_BACK2
		|| ent->client->ps.legsAnim == BOTH_FORCEWALLRUNFLIP_END
		|| ent->client->ps.legsAnim == BOTH_FORCEWALLREBOUND_BACK)
	{
		//pull back the view
		G_CamPullBackForLegsAnim(ent);
	}
	else if (ent->client->ps.torsoAnim == BOTH_A6_SABERPROTECT)
	{
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
			ent->client->ps.forceJumpCharge = 0;
		}
		if (!ent->s.number)
		{
			float animLength = PM_AnimLength(ent->client->clientInfo.animFileIndex,
				static_cast<animNumber_t>(ent->client->ps.torsoAnim));
			float elapsedTime = animLength - ent->client->ps.torsoAnimTimer;
			float backDist = 0;
			if (elapsedTime <= 300.0f)
			{
				//starting anim
				backDist = elapsedTime / 300.0f * 90.0f;
			}
			else if (ent->client->ps.torsoAnimTimer <= 300.0f)
			{
				//ending anim
				backDist = ent->client->ps.torsoAnimTimer / 300.0f * 90.0f;
			}
			else
			{
				//in middle of anim
				backDist = 90.0f;
			}
			//back off and look down
			cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_RNG | CG_OVERRIDE_3RD_PERSON_POF;
			cg.overrides.thirdPersonRange = cg_thirdPersonRange.value + backDist;
			cg.overrides.thirdPersonPitchOffset = cg_thirdPersonPitchOffset.value + backDist / 2.0f;
		}
		overridAngles = PM_AdjustAnglesForSpinProtect(ent, ucmd) ? qtrue : overridAngles;
	}
	else if (ent->client->ps.legsAnim == BOTH_A3_SPECIAL)
	{
		//push forward
		float animLength = PM_AnimLength(ent->client->clientInfo.animFileIndex,
			static_cast<animNumber_t>(ent->client->ps.torsoAnim));
		float elapsedTime = animLength - ent->client->ps.torsoAnimTimer;
		ucmd->upmove = ucmd->rightmove = 0;
		ucmd->forwardmove = 0;
		if (elapsedTime >= 350 && elapsedTime < 1500)
		{
			//push forward
			ucmd->forwardmove = 64;
			ent->client->ps.speed = 200.0f;
		}
	}
	else if (ent->client->ps.legsAnim == BOTH_A2_SPECIAL)
	{
		//push forward
		ucmd->upmove = ucmd->rightmove = 0;
		ucmd->forwardmove = 0;
		if (ent->client->ps.legsAnimTimer > 200.0f)
		{
			float animLength = PM_AnimLength(ent->client->clientInfo.animFileIndex,
				static_cast<animNumber_t>(ent->client->ps.torsoAnim));
			float elapsedTime = animLength - ent->client->ps.torsoAnimTimer;
			if (elapsedTime < 750
				|| elapsedTime >= 1650 && elapsedTime < 2400)
			{
				//push forward
				ucmd->forwardmove = 64;
				ent->client->ps.speed = 200.0f;
			}
		}
	} //FIXME: fast special?
	else if (ent->client->ps.legsAnim == BOTH_A1_SPECIAL
		&& (ucmd->forwardmove || ucmd->rightmove || VectorCompare(ent->client->ps.moveDir, vec3_origin) && ent->client
			->ps.speed > 0))
	{
		//moving during full-body fast special
		ent->client->ps.legsAnimTimer = 0; //don't hold this legsAnim, allow them to run
		//FIXME: just add this to the list of overridable special moves in PM_Footsteps?
	}
	else if (ent->client->ps.legsAnim == BOTH_FLIP_LAND)
	{
		//moving during full-body fast special
		float animLength = PM_AnimLength(ent->client->clientInfo.animFileIndex,
			static_cast<animNumber_t>(ent->client->ps.torsoAnim));
		float elapsedTime = animLength - ent->client->ps.torsoAnimTimer;
		ucmd->upmove = ucmd->rightmove = ucmd->forwardmove = 0;
		if (elapsedTime > 600 && elapsedTime < 800
			&& ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			//jump - FIXME: how do we stop double-jumps?
			ent->client->ps.pm_flags |= PMF_JUMP_HELD;
			ent->client->ps.groundEntityNum = ENTITYNUM_NONE;
			ent->client->ps.jumpZStart = ent->currentOrigin[2];
			ent->client->ps.velocity[2] = JUMP_VELOCITY;
			G_AddEvent(ent, EV_JUMP, 0);
		}
	}
	else if ((PM_SuperBreakWinAnim(ent->client->ps.torsoAnim)
		|| PM_SuperBreakLoseAnim(ent->client->ps.torsoAnim))
		&& ent->client->ps.torsoAnimTimer)
	{
		//can't move or turn
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
			ent->client->ps.forceJumpCharge = 0;
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
	}
	else if (ent->client->ps.legsAnim == BOTH_DODGE_HOLD_BL ||
		ent->client->ps.legsAnim == BOTH_DODGE_HOLD_BR ||
		ent->client->ps.legsAnim == BOTH_DODGE_HOLD_FL ||
		ent->client->ps.legsAnim == BOTH_DODGE_HOLD_L ||
		ent->client->ps.legsAnim == BOTH_DODGE_HOLD_R ||
		ent->client->ps.legsAnim == BOTH_DODGE_HOLD_FR)
	{
		///can't move
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->forwardmove = 0;
	}
	else if (ent->client->ps.legsAnim == BOTH_DODGE_BL ||
		ent->client->ps.legsAnim == BOTH_DODGE_BR ||
		ent->client->ps.legsAnim == BOTH_DODGE_FL ||
		ent->client->ps.legsAnim == BOTH_DODGE_L ||
		ent->client->ps.legsAnim == BOTH_DODGE_R ||
		ent->client->ps.legsAnim == BOTH_DODGE_FR)
	{
		//can't move
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->forwardmove = 0;
	}
	else if (ent->client->ps.legsAnim == BOTH_PULL_IMPALE_STAB || ent->client->ps.legsAnim == BOTH_GRAPPLE_FIRE)
	{
		//can't move during a pull stab
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->forwardmove = 0;
	}
	else if (PM_InAmputateMove(ent->client->ps.legsAnim))
	{
		//can't move during a pull stab
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->forwardmove = 0;
	}
	/////////////////////////////////////////////////////
	else if (BG_FullBodyTauntAnim(ent->client->ps.legsAnim))
	{
		if (ucmd->buttons & BUTTON_ATTACK
			|| ucmd->buttons & BUTTON_ALT_ATTACK
			|| ucmd->buttons & BUTTON_USE_FORCE
			|| ucmd->buttons & BUTTON_FORCEGRIP
			|| ucmd->buttons & BUTTON_FORCE_LIGHTNING
			|| ucmd->buttons & BUTTON_LIGHTNING_STRIKE
			|| ucmd->buttons & BUTTON_PROJECTION
			|| ucmd->buttons & BUTTON_FORCE_DRAIN
			|| ucmd->buttons & BUTTON_BLOCK
			|| ucmd->buttons & BUTTON_KICK
			|| ucmd->buttons & BUTTON_USE
			|| ucmd->buttons & BUTTON_DASH
			|| ucmd->buttons & BUTTON_FORCEGRASP
			|| ucmd->buttons & BUTTON_REPULSE
			|| ucmd->rightmove
			|| ucmd->forwardmove
			|| ucmd->upmove)
		{
			//stop the anim
			if (ent->client->ps.legsAnim == BOTH_MEDITATE)
			{
				NPC_SetAnim(ent, SETANIM_BOTH, BOTH_MEDITATE_END, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			}
			else if (ent->client->ps.legsAnim == BOTH_MEDITATE_SABER)
			{
				NPC_SetAnim(ent, SETANIM_BOTH, BOTH_MEDITATE_SABER_END, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			}
			else
			{
				ent->client->ps.legsAnimTimer = ent->client->ps.torsoAnimTimer = 0;
			}
		}
		else
		{
			if (ent->client->ps.legsAnim == BOTH_MEDITATE)
			{
				if (ent->client->ps.legsAnimTimer < 100)
				{
					ent->client->ps.legsAnimTimer = 100;
				}
			}
			if (ent->client->ps.legsAnim == BOTH_MEDITATE_SABER)
			{
				if (ent->client->ps.legsAnimTimer < 100)
				{
					ent->client->ps.legsAnimTimer = 100;
				}
			}
			if (ent->client->ps.legsAnimTimer > 0 || ent->client->ps.torsoAnimTimer > 0)
			{
				ucmd->rightmove = 0;
				ucmd->upmove = 0;
				ucmd->forwardmove = 0;
				ucmd->buttons = 0;
				if (ent->NPC)
				{
					VectorClear(ent->client->ps.moveDir);
					ent->client->ps.forceJumpCharge = 0;
				}
				overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
			}
		}
	}
	else if (ent->client->ps.legsAnim == BOTH_MEDITATE_END && ent->client->ps.legsAnimTimer > 0)
	{
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->forwardmove = 0;
		ucmd->buttons = 0;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
			ent->client->ps.forceJumpCharge = 0;
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
	}
	else if (ent->client->ps.legsAnim == BOTH_MEDITATE_SABER_END && ent->client->ps.legsAnimTimer > 0)
	{
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->forwardmove = 0;
		ucmd->buttons = 0;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
			ent->client->ps.forceJumpCharge = 0;
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
	}
	//EMOTE SYSTEM
	else if (BG_FullBodyEmoteAnim(ent->client->ps.torsoAnim))
	{
		if (ucmd->buttons & BUTTON_ATTACK
			|| ucmd->buttons & BUTTON_ALT_ATTACK
			|| ucmd->buttons & BUTTON_USE_FORCE
			|| ucmd->buttons & BUTTON_FORCEGRIP
			|| ucmd->buttons & BUTTON_FORCE_LIGHTNING
			|| ucmd->buttons & BUTTON_LIGHTNING_STRIKE
			|| ucmd->buttons & BUTTON_PROJECTION
			|| ucmd->buttons & BUTTON_FORCE_DRAIN
			|| ucmd->buttons & BUTTON_BLOCK
			|| ucmd->buttons & BUTTON_KICK
			|| ucmd->buttons & BUTTON_USE
			|| ucmd->buttons & BUTTON_DASH
			|| ucmd->rightmove
			|| ucmd->forwardmove
			|| ucmd->upmove)
		{
			//stop the anim
			if (ent->client->ps.torsoAnim == PLAYER_SURRENDER_START)
			{
				NPC_SetAnim(ent, SETANIM_BOTH, PLAYER_SURRENDER_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			}
			else
			{
				ent->client->ps.legsAnimTimer = ent->client->ps.torsoAnimTimer = 0;
			}
		}
		else
		{
			if (ent->client->ps.torsoAnim == PLAYER_SURRENDER_START)
			{
				if (ent->client->ps.torsoAnimTimer < 100)
				{
					ent->client->ps.legsAnimTimer = 100;
				}
			}
			if (ent->client->ps.legsAnimTimer > 0 || ent->client->ps.torsoAnimTimer > 0)
			{
				ucmd->rightmove = 0;
				ucmd->upmove = 0;
				ucmd->forwardmove = 0;
				ucmd->buttons = 0;
				if (ent->NPC)
				{
					VectorClear(ent->client->ps.moveDir);
					ent->client->ps.forceJumpCharge = 0;
				}
				overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
			}
		}
	}
	else if (ent->client->ps.torsoAnim == PLAYER_SURRENDER_STOP && ent->client->ps.torsoAnimTimer > 0)
	{
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->forwardmove = 0;
		ucmd->buttons = 0;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
			ent->client->ps.forceJumpCharge = 0;
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
	}
	else if (BG_FullBodyCowerAnim(ent->client->ps.torsoAnim))
	{
		if (ucmd->buttons & BUTTON_ATTACK
			|| ucmd->buttons & BUTTON_ALT_ATTACK
			|| ucmd->buttons & BUTTON_USE_FORCE
			|| ucmd->buttons & BUTTON_FORCEGRIP
			|| ucmd->buttons & BUTTON_FORCE_LIGHTNING
			|| ucmd->buttons & BUTTON_LIGHTNING_STRIKE
			|| ucmd->buttons & BUTTON_PROJECTION
			|| ucmd->buttons & BUTTON_FORCE_DRAIN
			|| ucmd->buttons & BUTTON_BLOCK
			|| ucmd->buttons & BUTTON_KICK
			|| ucmd->buttons & BUTTON_USE
			|| ucmd->buttons & BUTTON_DASH
			|| ucmd->rightmove
			|| ucmd->forwardmove
			|| ucmd->upmove)
		{
			//stop the anim
			if (ent->client->ps.torsoAnim == BOTH_COWER1)
			{
				NPC_SetAnim(ent, SETANIM_BOTH, BOTH_COWER1_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
			}
			else
			{
				ent->client->ps.legsAnimTimer = ent->client->ps.torsoAnimTimer = 0;
			}
		}
		else
		{
			if (ent->client->ps.torsoAnim == BOTH_COWER1)
			{
				if (ent->client->ps.torsoAnimTimer < 100)
				{
					ent->client->ps.legsAnimTimer = 100;
				}
			}
			if (ent->client->ps.legsAnimTimer > 0 || ent->client->ps.torsoAnimTimer > 0)
			{
				ucmd->rightmove = 0;
				ucmd->upmove = 0;
				ucmd->forwardmove = 0;
				ucmd->buttons = 0;
				if (ent->NPC)
				{
					VectorClear(ent->client->ps.moveDir);
					ent->client->ps.forceJumpCharge = 0;
				}
				overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
			}
		}
	}
	else if (ent->client->ps.legsAnim == BOTH_COWER1_STOP && ent->client->ps.legsAnimTimer > 0)
	{
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->forwardmove = 0;
		ucmd->buttons = 0;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
			ent->client->ps.forceJumpCharge = 0;
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
	} //stiffened up
	else if (g_SerenityJediEngineMode->integer == 2
		&& !in_camera
		&& (ent->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(ent))
		&& PM_SaberInMassiveBounce(ent->client->ps.torsoAnim)
		&& ent->client->ps.torsoAnimTimer)
	{
		//npc can't move or turn
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
			ent->client->ps.forceJumpCharge = 0;
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
	}
	else if ((ent->client->ps.legsAnim == BOTH_HOP_R ||
		ent->client->ps.legsAnim == BOTH_HOP_L ||
		ent->client->ps.legsAnim == BOTH_HOP_B ||
		ent->client->ps.legsAnim == BOTH_HOP_F ||
		ent->client->ps.legsAnim == BOTH_DASH_R ||
		ent->client->ps.legsAnim == BOTH_DASH_L ||
		ent->client->ps.legsAnim == BOTH_DASH_B ||
		ent->client->ps.legsAnim == BOTH_DASH_F) && ent->client->ps.legsAnimTimer > 0)
	{
		ucmd->upmove = 0;
	}
	else if (In_LedgeIdle(ent->client->ps.torsoAnim))
	{
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
		//ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
	}
	else if (ent->client->ps.forcePowersActive & 1 << FP_REPULSE && (ent->client->ps.torsoAnim ==
		BOTH_FORCE_PROTECT_FAST || ent->client->ps.torsoAnim == BOTH_FORCE_REPULSE))
	{
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->forwardmove = 0;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
			ent->client->ps.forceJumpCharge = 0;
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
	}
	else if (ent->client->ps.repulseChargeStart)
	{
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->forwardmove = 0;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
			ent->client->ps.forceJumpCharge = 0;
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
	}
	else if (ent->client->ps.torsoAnim == BOTH_FORCE_REPULSE)
	{
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->forwardmove = 0;
		if (ent->NPC)
		{
			VectorClear(ent->client->ps.moveDir);
			ent->client->ps.forceJumpCharge = 0;
		}
		overridAngles = PM_LockAngles(ent, ucmd) ? qtrue : overridAngles;
	}
	//END EMOTE
	//////////////////////////////////////////////////////////////////////
	else if (!ent->s.number)
	{
		if (ent->client->NPC_class != CLASS_ATST)
		{
			// Not in a vehicle.
			if (ent->s.m_iVehicleNum == 0)
			{
				if (!MatrixMode)
				{
					cg.overrides.active &= ~(CG_OVERRIDE_3RD_PERSON_RNG | CG_OVERRIDE_3RD_PERSON_POF |
						CG_OVERRIDE_3RD_PERSON_ANG);
					cg.overrides.thirdPersonRange = 0;
				}
			}
		}
	}

	if (ent->client->ps.userInt3 == qtrue)
	{
		ucmd->buttons &= ~BUTTON_ATTACK;
		ucmd->buttons &= ~BUTTON_ALT_ATTACK;
	}

	if (PM_InRoll(&ent->client->ps))
	{
		if (ent->s.number >= MAX_CLIENTS || !player_locked && !PlayerAffectedByStasis())
		{
			//FIXME: NPCs should try to face us during this roll, so they roll around us...?
			PM_CmdForRoll(&ent->client->ps, ucmd);
			if (ent->s.number >= MAX_CLIENTS)
			{
				//make sure it doesn't roll me off a ledge
				if (!G_CheckRollSafety(ent, ent->client->ps.legsAnim, 24))
				{
					//crap!  I guess all we can do is stop...  UGH
					ucmd->rightmove = ucmd->forwardmove = 0;
				}
			}
		}
		if (ent->NPC)
		{
			//invalid now
			VectorClear(ent->client->ps.moveDir);
		}
		ent->client->ps.speed = 400;
	}

	if (PM_InCartwheel(ent->client->ps.legsAnim))
	{
		//can't keep moving in cartwheel
		if (ent->client->ps.legsAnimTimer > 100)
		{
			//still have time left in the anim
			ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
			if (ent->NPC)
			{
				//invalid now
				VectorClear(ent->client->ps.moveDir);
			}
			if (ent->s.number || !player_locked && !PlayerAffectedByStasis())
			{
				switch (ent->client->ps.legsAnim)
				{
				case BOTH_ARIAL_LEFT:
				case BOTH_CARTWHEEL_LEFT:
					ucmd->rightmove = -127;
					break;
				case BOTH_ARIAL_RIGHT:
				case BOTH_CARTWHEEL_RIGHT:
					ucmd->rightmove = 127;
					break;
				case BOTH_ARIAL_F1:
					ucmd->forwardmove = 127;
					break;
				default:
					break;
				}
			}
		}
	}

	if (ent->client->ps.legsAnim == BOTH_BUTTERFLY_LEFT
		|| ent->client->ps.legsAnim == BOTH_BUTTERFLY_RIGHT)
	{
		if (ent->client->ps.legsAnimTimer > 100)
		{
			//still have time left in the anim
			ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
			if (ent->NPC)
			{
				//invalid now
				VectorClear(ent->client->ps.moveDir);
			}
			if (ent->s.number || !player_locked && !PlayerAffectedByStasis())
			{
				if (ent->client->ps.legsAnimTimer > 450)
				{
					switch (ent->client->ps.legsAnim)
					{
					case BOTH_BUTTERFLY_LEFT:
						ucmd->rightmove = -127;
						break;
					case BOTH_BUTTERFLY_RIGHT:
						ucmd->rightmove = 127;
						break;
					default:
						break;
					}
				}
			}
		}
	}

	overridAngles = PM_AdjustAnglesForStabDown(ent, ucmd) ? qtrue : overridAngles;
	overridAngles = PM_AdjustAngleForWallJump(ent, ucmd, qtrue) ? qtrue : overridAngles;
	overridAngles = PM_AdjustAngleForWallRunUp(ent, ucmd, qtrue) ? qtrue : overridAngles;
	overridAngles = PM_AdjustAngleForWallRun(ent, ucmd, qtrue) ? qtrue : overridAngles;

	return overridAngles;
}

void BG_AddPushVecToUcmd(const gentity_t* self, usercmd_t* ucmd)
{
	vec3_t forward, right, moveDir;

	if (!self->client)
	{
		return;
	}
	const float pushSpeed = VectorLengthSquared(self->client->pushVec);
	if (!pushSpeed)
	{
		//not being pushed
		return;
	}

	AngleVectors(self->client->ps.viewangles, forward, right, nullptr);
	VectorScale(forward, ucmd->forwardmove / 127.0f * self->client->ps.speed, moveDir);
	VectorMA(moveDir, ucmd->rightmove / 127.0f * self->client->ps.speed, right, moveDir);
	//moveDir is now our intended move velocity

	VectorAdd(moveDir, self->client->pushVec, moveDir);
	self->client->ps.speed = VectorNormalize(moveDir);
	//moveDir is now our intended move velocity plus our push Vector

	const float fMove = 127.0 * DotProduct(forward, moveDir);
	const float rMove = 127.0 * DotProduct(right, moveDir);
	ucmd->forwardmove = floor(fMove); //If in the same dir , will be positive
	ucmd->rightmove = floor(rMove); //If in the same dir , will be positive

	if (self->client->pushVecTime < level.time)
	{
		VectorClear(self->client->pushVec);
	}
}

void NPC_Accelerate(const gentity_t* ent, qboolean fullWalkAcc, qboolean fullRunAcc)
{
	if (!ent->client || !ent->NPC)
	{
		return;
	}

	if (!ent->NPC->stats.acceleration)
	{
		//No acceleration means just start and stop
		ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
	}
	//FIXME:  in cinematics always accel/decel?
	else if (ent->NPC->desiredSpeed <= ent->NPC->stats.walkSpeed)
	{
		//Only accelerate if at walkSpeeds
		if (ent->NPC->desiredSpeed > ent->NPC->currentSpeed + ent->NPC->stats.acceleration)
		{
			//ent->client->ps.friction = 0;
			ent->NPC->currentSpeed += ent->NPC->stats.acceleration;
		}
		else if (ent->NPC->desiredSpeed > ent->NPC->currentSpeed)
		{
			//ent->client->ps.friction = 0;
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
		else if (fullWalkAcc && ent->NPC->desiredSpeed < ent->NPC->currentSpeed - ent->NPC->stats.acceleration)
		{
			//decelerate even when walking
			ent->NPC->currentSpeed -= ent->NPC->stats.acceleration;
		}
		else if (ent->NPC->desiredSpeed < ent->NPC->currentSpeed)
		{
			//stop on a dime
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
	}
	else //  if ( ent->NPC->desiredSpeed > ent->NPC->stats.walkSpeed )
	{
		//Only decelerate if at runSpeeds
		if (fullRunAcc && ent->NPC->desiredSpeed > ent->NPC->currentSpeed + ent->NPC->stats.acceleration)
		{
			//Accelerate to runspeed
			//ent->client->ps.friction = 0;
			ent->NPC->currentSpeed += ent->NPC->stats.acceleration;
		}
		else if (ent->NPC->desiredSpeed > ent->NPC->currentSpeed)
		{
			//accelerate instantly
			//ent->client->ps.friction = 0;
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
		else if (fullRunAcc && ent->NPC->desiredSpeed < ent->NPC->currentSpeed - ent->NPC->stats.acceleration)
		{
			ent->NPC->currentSpeed -= ent->NPC->stats.acceleration;
		}
		else if (ent->NPC->desiredSpeed < ent->NPC->currentSpeed)
		{
			ent->NPC->currentSpeed = ent->NPC->desiredSpeed;
		}
	}
}

/*
-------------------------
NPC_GetWalkSpeed
-------------------------
*/

static int NPC_GetWalkSpeed(const gentity_t* ent)
{
	int walkSpeed;

	if (ent->client == nullptr || ent->NPC == nullptr)
		return 0;

	switch (ent->client->playerTeam)
	{
	case TEAM_PLAYER: //To shutup compiler, will add entries later (this is stub code)
	default:
		walkSpeed = ent->NPC->stats.walkSpeed;
		break;
	}

	return walkSpeed;
}

/*
-------------------------
NPC_GetRunSpeed
-------------------------
*/

static int NPC_GetRunSpeed(const gentity_t* ent)
{
	int runSpeed;

	if (ent->client == nullptr || ent->NPC == nullptr)
		return 0;

	// team no longer indicates species/race.  Use NPC_class to adjust speed for specific npc types
	switch (ent->client->NPC_class)
	{
	case CLASS_PROBE: // droid cases here to shut-up compiler
	case CLASS_GONK:
	case CLASS_R2D2:
	case CLASS_R5D2:
	case CLASS_MARK1:
	case CLASS_MARK2:
	case CLASS_PROTOCOL:
	case CLASS_ATST: // hmm, not really your average droid
	case CLASS_MOUSE:
	case CLASS_SEEKER:
	case CLASS_REMOTE:
		runSpeed = ent->NPC->stats.runSpeed;
		break;

	default:
		runSpeed = ent->NPC->stats.runSpeed;
		break;
	}

	return runSpeed;
}

void G_HeldByMonster(gentity_t* ent, usercmd_t** ucmd)
{
	if (ent && ent->activator && ent->activator->inuse && ent->activator->health > 0)
	{
		gentity_t* monster = ent->activator;
		//take the monster's waypoint as your own
		ent->waypoint = monster->waypoint;

		//update the actual origin of the victim
		mdxaBone_t boltMatrix;

		// Getting the bolt here
		int boltIndex = monster->gutBolt; //default to being held in his mouth
		if (monster->count == 1)
		{
			//being held in hand rather than the mouth, so use *that* bolt
			boltIndex = monster->handRBolt;
		}
		vec3_t monAngles = { 0 };
		monAngles[YAW] = monster->currentAngles[YAW]; //only use YAW when passing angles to G2
		gi.G2API_GetBoltMatrix(monster->ghoul2, monster->playerModel, boltIndex,
			&boltMatrix, monAngles, monster->currentOrigin, cg.time ? cg.time : level.time,
			nullptr, monster->s.modelScale);
		// Storing ent position, bolt position, and bolt axis
		gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, ent->client->ps.origin);
		gi.linkentity(ent);
		//lock view angles
		PM_AdjustAnglesForHeldByMonster(ent, monster, *ucmd);
		if (monster->client && monster->client->NPC_class == CLASS_WAMPA)
		{
			//can only hit attack button
			(*ucmd)->buttons &= ~((*ucmd)->buttons & ~BUTTON_ATTACK);
		}
	}
	else if (ent)
	{
		//doh, my captor died!
		ent->activator = nullptr;
		if (ent->client)
		{
			ent->client->ps.eFlags &= ~(EF_HELD_BY_WAMPA | EF_HELD_BY_RANCOR);
		}
	}
	// don't allow movement, weapon switching, and most kinds of button presses
	(*ucmd)->forwardmove = 0;
	(*ucmd)->rightmove = 0;
	(*ucmd)->upmove = 0;
}

// yes...   so stop skipping...
void G_StopCinematicSkip(void)
{
	gi.cvar_set("skippingCinematic", "0");
	gi.cvar_set("timescale", "1");
}

void G_StartCinematicSkip(void)
{
	if (cinematicSkipScript[0])
	{
		Quake3Game()->RunScript(&g_entities[0], cinematicSkipScript);

		cinematicSkipScript[0] = 0;
		gi.cvar_set("skippingCinematic", "1");
		gi.cvar_set("timescale", "100");
	}
	else
	{
		// no... so start skipping...
		gi.cvar_set("skippingCinematic", "1");
		gi.cvar_set("timescale", "100");
	}
}

void G_CheckClientIdleSabers(gentity_t* ent, const usercmd_t* ucmd)
{
	if (!ent || !ent->client || ent->health <= 0)
	{
		return;
	}

	if (ent && ent->client->ps.torsoAnim == BOTH_WIND)
	{
		return;
	}

	if (!ent->s.number && (!cg.renderingThirdPerson || cg.zoomMode))
	{
		if (ent->client->idleTime < level.time)
		{
			ent->client->idleTime = level.time;
		}
		return;
	}
	if (!VectorCompare(vec3_origin, ent->client->ps.velocity)
		|| ucmd->buttons || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove
		|| !PM_StandingAnim(ent->client->ps.legsAnim)
		|| ent->enemy
		|| ent->client->ps.legsAnimTimer
		|| ent->client->ps.torsoAnimTimer)
	{
		//FIXME: also check for turning?
		if (!VectorCompare(vec3_origin, ent->client->ps.velocity)
			|| ucmd->buttons || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove
			|| ent->enemy)
		{
			//if in an idle, break out
			switch (ent->client->ps.legsAnim)
			{
			case BOTH_STAND1IDLE1:
			case BOTH_STAND9IDLE1:
			case BOTH_STAND_SABER_ON_IDLE:
			case BOTH_STAND_SABER_ON_IDLE_DUELS:
			case BOTH_STAND_SABER_ON_IDLE_STAFF:
			case BOTH_STAND2IDLE1:
			case BOTH_STAND2IDLE2:
			case BOTH_STAND3IDLE1:
			case BOTH_STAND5IDLE1:
			case BOTH_STANDYODAIDLE_STICK:
			case BOTH_MENUIDLE1:
				ent->client->ps.legsAnimTimer = 0;
				break;
			default:;
			}
			switch (ent->client->ps.torsoAnim)
			{
			case BOTH_STAND1IDLE1:
			case BOTH_STAND9IDLE1:
			case BOTH_STAND_SABER_ON_IDLE:
			case BOTH_STAND_SABER_ON_IDLE_DUELS:
			case BOTH_STAND_SABER_ON_IDLE_STAFF:
			case BOTH_STAND2IDLE1:
			case BOTH_STAND2IDLE2:
			case BOTH_STAND3IDLE1:
			case BOTH_STAND5IDLE1:
			case BOTH_STANDYODAIDLE_STICK:
			case BOTH_MENUIDLE1:
				ent->client->ps.torsoAnimTimer = 0;
				break;
			default:;
			}
		}
		//
		if (ent->client->idleTime < level.time)
		{
			ent->client->idleTime = level.time;
		}
	}
	else if (g_SerenityJediEngineMode->integer == 2 && level.time - ent->client->idleTime > 2000)
	{
		//been idle for 2 seconds
		int idleAnim = -1;

		switch (ent->client->ps.legsAnim)
		{
		case BOTH_STAND1:
		case BOTH_STAND6:
			idleAnim = BOTH_STAND1IDLE1;
			break;
		case BOTH_STAND9:
			idleAnim = BOTH_STAND9IDLE1;
			break;
		case BOTH_SABERTAVION_STANCE:
		case BOTH_SABERTAVION_STANCE_JKA:
		case BOTH_SABERDESANN_STANCE:
		case BOTH_SABERDESANN_STANCE_JKA:
		case BOTH_SABERFAST_STANCE:
		case BOTH_SABERFAST_STANCE_JKA:
		case BOTH_SABERSLOW_STANCE:
		case BOTH_SABERSLOW_STANCE_JKA:
		case BOTH_SABERSINGLECROUCH:
		case BOTH_STAND_SABER_ON:
			idleAnim = BOTH_STAND_SABER_ON_IDLE;
			break;
		case BOTH_SABERDUAL_STANCE:
		case BOTH_SABERDUAL_STANCE_JKA:
		case BOTH_SABERDUALCROUCH:
		case BOTH_STAND_SABER_ON_DUELS:
			idleAnim = BOTH_STAND_SABER_ON_IDLE_DUELS;
			break;
		case BOTH_SABERSTAFF_STANCE:
		case BOTH_SABERSTAFF_STANCE_JKA:
		case BOTH_SABERSTAFFCROUCH:
		case BOTH_STAND_SABER_ON_STAFF:
			idleAnim = BOTH_STAND_SABER_ON_IDLE_STAFF;
			break;
		case BOTH_STAND2:
		case BOTH_STAND2_JKA:
			idleAnim = BOTH_STAND2IDLE2;
			break;
		case BOTH_STAND3:
			idleAnim = BOTH_STAND3IDLE1;
			break;
		case BOTH_STAND5:
			idleAnim = BOTH_STAND5IDLE1;
			break;
		case BOTH_STANDYODA_STICK:
			idleAnim = BOTH_STANDYODAIDLE_STICK;
			break;
		case BOTH_MENUIDLE1:
			idleAnim = BOTH_MENUIDLE1;
			break;
		default:;
		}
		if (idleAnim != -1 && PM_HasAnimation(ent, idleAnim))
		{
			NPC_SetAnim(ent, SETANIM_BOTH, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			//don't idle again after this anim for a while
			ent->client->idleTime = level.time + PM_AnimLength(ent->client->clientInfo.animFileIndex, static_cast<animNumber_t>(idleAnim)) + Q_irand(0, 4000);
		}
	}
	else if (g_SerenityJediEngineMode->integer < 2 && level.time - ent->client->idleTime > 4000)
	{
		//been idle for 5 seconds
		int idleAnim = -1;

		switch (ent->client->ps.legsAnim)
		{
		case BOTH_STAND1:
		case BOTH_STAND6:
			idleAnim = BOTH_STAND1IDLE1;
			break;
		case BOTH_STAND9:
			idleAnim = BOTH_STAND9IDLE1;
			break;
		case BOTH_STAND_SABER_ON:
			idleAnim = BOTH_STAND_SABER_ON_IDLE;
			break;
		case BOTH_STAND_SABER_ON_DUELS:
			idleAnim = BOTH_STAND_SABER_ON_IDLE_DUELS;
			break;
		case BOTH_STAND_SABER_ON_STAFF:
			idleAnim = BOTH_STAND_SABER_ON_IDLE_STAFF;
			break;
		case BOTH_STAND2:
		case BOTH_STAND2_JKA:
			idleAnim = BOTH_STAND2IDLE2;
			break;
		case BOTH_STAND3:
			idleAnim = BOTH_STAND3IDLE1;
			break;
		case BOTH_STAND5:
			idleAnim = BOTH_STAND5IDLE1;
			break;
		case BOTH_STANDYODA_STICK:
			idleAnim = BOTH_STANDYODAIDLE_STICK;
			break;
		case BOTH_MENUIDLE1:
			idleAnim = BOTH_MENUIDLE1;
			break;
			//
		case BOTH_SABERTAVION_STANCE:
			idleAnim = BOTH_SABERTAVION_STANCE;
			break;
		case BOTH_SABERDESANN_STANCE:
			idleAnim = BOTH_SABERDESANN_STANCE;
			break;
		case BOTH_SABERSTAFF_STANCE:
			idleAnim = BOTH_SABERSTAFF_STANCE;
			break;
		case BOTH_SABERDUAL_STANCE:
			idleAnim = BOTH_SABERDUAL_STANCE;
			break;
		case BOTH_SABERFAST_STANCE:
			idleAnim = BOTH_SABERFAST_STANCE;
			break;
		case BOTH_SABERSLOW_STANCE:
			idleAnim = BOTH_SABERSLOW_STANCE;
			break;
			//
		case BOTH_SABERTAVION_STANCE_JKA:
			idleAnim = BOTH_SABERTAVION_STANCE_JKA;
			break;
		case BOTH_SABERDESANN_STANCE_JKA:
			idleAnim = BOTH_SABERDESANN_STANCE_JKA;
			break;
		case BOTH_SABERSTAFF_STANCE_JKA:
			idleAnim = BOTH_SABERSTAFF_STANCE_JKA;
			break;
		case BOTH_SABERDUAL_STANCE_JKA:
			idleAnim = BOTH_SABERDUAL_STANCE_JKA;
			break;
		case BOTH_SABERFAST_STANCE_JKA:
			idleAnim = BOTH_SABERFAST_STANCE_JKA;
			break;
		case BOTH_SABERSLOW_STANCE_JKA:
			idleAnim = BOTH_SABERSLOW_STANCE_JKA;
			break;
		default:;
		}
		if (idleAnim != -1 && PM_HasAnimation(ent, idleAnim))
		{
			NPC_SetAnim(ent, SETANIM_BOTH, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			//don't idle again after this anim for a while
			ent->client->idleTime = level.time + PM_AnimLength(ent->client->clientInfo.animFileIndex, static_cast<animNumber_t>(idleAnim)) + Q_irand(0, 2000);
		}
	}
}

qboolean GunisMelee(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_MELEE:
	case WP_STUN_BATON:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean GunisSbd(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_SBD_BLASTER:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean GunisShort(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_BLASTER_PISTOL:
	case WP_BRYAR_PISTOL:
	case WP_DUAL_PISTOL:
	case WP_JAWA:
	case WP_REBELBLASTER:
	case WP_REY:
	case WP_JANGO:
	case WP_CLONEPISTOL:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean GunisLong(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_NOGHRI_STICK:
	case WP_BLASTER:
	case WP_BOWCASTER:
	case WP_BATTLEDROID:
	case WP_THEFIRSTORDER:
	case WP_CLONECARBINE:
	case WP_CONCUSSION:
	case WP_CLONECOMMANDO:
	case WP_REBELRIFLE:
	case WP_BOBA:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean GunisMassive(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_DISRUPTOR:
	case WP_TUSKEN_RIFLE:
	case WP_TUSKEN_STAFF:
	case WP_CLONERIFLE:
	case WP_FLECHETTE:
	case WP_ROCKET_LAUNCHER:
	case WP_REPEATER:
	case WP_DEMP2:
		return qtrue;
	default:;
	}
	return qfalse;
}

qboolean GunisBomb(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_THERMAL:
	case WP_TRIP_MINE:
	case WP_DET_PACK:
		return qtrue;
	default:;
	}
	return qfalse;
}

void G_CheckClientIdleGuns(gentity_t* ent, const usercmd_t* ucmd)
{
	if (!ent || !ent->client || ent->health <= 0)
	{
		return;
	}

	if (ent && ent->client->ps.torsoAnim == BOTH_WIND)
	{
		return;
	}

	if (!ent->s.number && /*!cg_trueguns.integer ||*/ cg.zoomMode)
	{
		if (ent->client->idleTime < level.time)
		{
			ent->client->idleTime = level.time;
		}
		return;
	}
	if (!VectorCompare(vec3_origin, ent->client->ps.velocity)
		|| ucmd->buttons || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove
		|| !PM_StandingAnim(ent->client->ps.legsAnim)
		|| ent->enemy
		|| ent->client->ps.legsAnimTimer
		|| ent->client->ps.torsoAnimTimer)
	{
		if (!VectorCompare(vec3_origin, ent->client->ps.velocity)
			|| ucmd->buttons || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove
			|| ent->enemy)
		{
			//if in an idle, break out
			switch (ent->client->ps.legsAnim)
			{
			case BOTH_STAND1IDLE1:
			case BOTH_STAND2IDLE1:
			case BOTH_STAND2IDLE2:
			case BOTH_STAND3IDLE1:
			case BOTH_STAND5IDLE1:
			case BOTH_STAND9IDLE1:
			case BOTH_STANDYODAIDLE_STICK:
			case BOTH_MENUIDLE1:
			case TORSO_WEAPONREST2:
			case TORSO_WEAPONREST3:
			case TORSO_WEAPONREST4:
			case TORSO_WEAPONIDLE2:
			case BOTH_WEAPONREST2P:
				ent->client->ps.legsAnimTimer = 0;
				break;
			default:;
			}
			switch (ent->client->ps.torsoAnim)
			{
			case BOTH_STAND1IDLE1:
			case BOTH_STAND2IDLE1:
			case BOTH_STAND2IDLE2:
			case BOTH_STAND3IDLE1:
			case BOTH_STAND5IDLE1:
			case BOTH_STAND9IDLE1:
			case BOTH_STANDYODAIDLE_STICK:
			case BOTH_MENUIDLE1:
			case TORSO_WEAPONREST2:
			case TORSO_WEAPONREST3:
			case TORSO_WEAPONREST4:
			case TORSO_WEAPONIDLE2:
			case BOTH_WEAPONREST2P:
				ent->client->ps.torsoAnimTimer = 0;
				break;
			default:;
			}
		}
		//
		if (ent->client->idleTime < level.time)
		{
			ent->client->idleTime = level.time;
		}
	}
	else if (GunisMelee(ent) && level.time - ent->client->idleTime > 5000)
	{
		//been idle for 2 seconds

		constexpr int idleAnim = BOTH_STAND1IDLE1;

		if (PM_HasAnimation(ent, idleAnim))
		{
			NPC_SetAnim(ent, SETANIM_BOTH, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
	else if (GunisSbd(ent) && level.time - ent->client->idleTime > 5000)
	{
		//been idle for 2 seconds

		constexpr int idleAnim = TORSO_WEAPONIDLE2;

		if (PM_HasAnimation(ent, idleAnim))
		{
			NPC_SetAnim(ent, SETANIM_TORSO, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
	else if (GunisShort(ent) && level.time - ent->client->idleTime > 3000)
	{
		//been idle for 2 seconds
		int idleAnim;

		if (ent->weaponModel[1] > 0)
		{
			//dual pistols
			idleAnim = BOTH_WEAPONREST2P;
		}
		else
		{
			//single pistol
			idleAnim = TORSO_WEAPONREST2;
		}

		if (idleAnim != -1 && PM_HasAnimation(ent, idleAnim))
		{
			NPC_SetAnim(ent, SETANIM_TORSO, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
	else if (GunisLong(ent) && level.time - ent->client->idleTime > 5000)
	{
		//been idle for 2 seconds

		constexpr int idleAnim = TORSO_WEAPONREST3;

		if (PM_HasAnimation(ent, idleAnim))
		{
			NPC_SetAnim(ent, SETANIM_TORSO, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
	else if (GunisMassive(ent) && level.time - ent->client->idleTime > 5000)
	{
		//been idle for 2 seconds

		constexpr int idleAnim = TORSO_WEAPONREST4;

		if (PM_HasAnimation(ent, idleAnim))
		{
			NPC_SetAnim(ent, SETANIM_TORSO, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
	else if (GunisBomb(ent) && level.time - ent->client->idleTime > 5000)
	{
		//been idle for 2 seconds

		constexpr int idleAnim = BOTH_STAND9IDLE1;

		if (PM_HasAnimation(ent, idleAnim))
		{
			NPC_SetAnim(ent, SETANIM_TORSO, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
	else
	{
		if (level.time - ent->client->idleTime > 5000)
		{
			//been idle for 2 seconds

			constexpr int idleAnim = BOTH_STAND1IDLE1;

			if (PM_HasAnimation(ent, idleAnim))
			{
				NPC_SetAnim(ent, SETANIM_BOTH, idleAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
		}
	}
}

void G_CheckMovingLoopingSounds(gentity_t* ent, const usercmd_t* ucmd)
{
	if (ent->client)
	{
		if (ent->NPC && !VectorCompare(vec3_origin, ent->client->ps.moveDir) //moving using moveDir
			|| ucmd->forwardmove || ucmd->rightmove //moving using ucmds
			|| ucmd->upmove && FlyingCreature(ent) //flier using ucmds to move
			|| FlyingCreature(ent) && !VectorCompare(vec3_origin, ent->client->ps.velocity) && ent->health > 0)
			//flier using velocity to move
		{
			switch (ent->client->NPC_class)
			{
			case CLASS_R2D2:
				ent->s.loopSound = G_SoundIndex("sound/chars/r2d2/misc/r2_move_lp.wav");
				break;
			case CLASS_R5D2:
				ent->s.loopSound = G_SoundIndex("sound/chars/r2d2/misc/r2_move_lp2.wav");
				break;
			case CLASS_MARK2:
				ent->s.loopSound = G_SoundIndex("sound/chars/mark2/misc/mark2_move_lp");
				break;
			case CLASS_MOUSE:
				ent->s.loopSound = G_SoundIndex("sound/chars/mouse/misc/mouse_lp");
				break;
			case CLASS_PROBE:
				ent->s.loopSound = G_SoundIndex("sound/chars/probe/misc/probedroidloop");
				break;
			default:
				break;
			}
		}
		else
		{
			//not moving under your own control, stop loopSound
			if (ent->client->NPC_class == CLASS_R2D2 || ent->client->NPC_class == CLASS_R5D2
				|| ent->client->NPC_class == CLASS_MARK2 || ent->client->NPC_class == CLASS_MOUSE
				|| ent->client->NPC_class == CLASS_PROBE)
			{
				ent->s.loopSound = 0;
			}
		}
	}
}

/*
==============
DoCallout

This will mark an enemy. If our friend is an NPC, order them to attack.
==============
*/

void DoCallout(gentity_t* caller, gentity_t* ourFriend)
{
	trace_t tr;
	vec3_t endTrace, forward;
	vec3_t eyes;
	gentity_t* player = &g_entities[0];

	VectorCopy(caller->currentOrigin, eyes);
	eyes[2] += 20.0f;
	AngleVectors(caller->client->ps.viewangles, forward, nullptr, nullptr);
	VectorMA(eyes, 16384, forward, endTrace);
	gi.trace(&tr, eyes, vec3_origin, vec3_origin, endTrace, caller->s.number, MASK_SHOT, G2_NOCOLLIDE, 0);

	if (tr.fraction >= 1.0f)
	{
		// Didn't hit anything.
		return;
	}

	if (tr.entityNum == ENTITYNUM_WORLD)
	{
		// Didn't hit an entity
		return;
	}

	if (g_allowcallout->integer == 0)
	{
		return;
	}

	gentity_t* tracedEnemy = &g_entities[tr.entityNum];

	if (tracedEnemy->s.eType != ET_PLAYER)
	{
		// We hit an entity, but it wasn't an NPC.
		return;
	}

	if (tracedEnemy->health <= 0)
	{
		// We hit an entity, but it wasn't an NPC.
		return;
	}

	if (tracedEnemy->client->playerTeam == ourFriend->client->playerTeam)
	{
		// They're on the same team. Whoops.
		return;
	}

	if (caller->s.number == 0)
	{
		// Sic 'em!
		G_SetEnemy(ourFriend, tracedEnemy);
	}

	NPC_SetAnim(player, SETANIM_TORSO, BOTH_ATTACK_COMMAND, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

	if (Distance(ourFriend->currentOrigin, tracedEnemy->currentOrigin) >= 512)
	{
		G_AddEvent(caller, Q_irand(EV_ESCAPING1, EV_ESCAPING3), 0);
		NPC_SetAnim(ourFriend, SETANIM_TORSO, BOTH_ORDER_RECIVED, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	}
	else
	{
		vec3_t dir;
		VectorSubtract(ourFriend->currentOrigin, tracedEnemy->currentOrigin, dir);
		const vec_t angle = DotProduct(ourFriend->currentAngles, dir);

		if (angle >= 0.2 && angle <= 1.0)
		{
			G_AddEvent(caller, Q_irand(EV_CONFUSE1, EV_CONFUSE3), 0);
			NPC_SetAnim(ourFriend, SETANIM_TORSO, BOTH_ORDER_RECIVED, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
		else
		{
			G_AddEvent(caller, Q_irand(EV_SIGHT1, EV_SIGHT3), 0);
			NPC_SetAnim(ourFriend, SETANIM_TORSO, BOTH_ORDER_RECIVED, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
	tracedEnemy->markTime = level.time + 5000;
	caller->calloutTime = level.time + 5000;
}

/*
==============
ClientAlterSpeed

This function is called ONLY from ClientThinkReal, and is responsible for setting client ps.speed
==============
*/
extern qboolean g_standard_humanoid(gentity_t* self);

void ClientAlterSpeed(gentity_t* ent, usercmd_t* ucmd, qboolean controlledByPlayer, float vehicleFrameTimeModifier)
{
	gclient_t* client = ent->client;
	const Vehicle_t* pVeh = nullptr;

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		pVeh = ent->m_pVehicle;
	}

	// set speed
	// This may be wrong: If we're an npc and we are in a vehicle???
	if (ent->NPC != nullptr && ent->client && ent->s.m_iVehicleNum != 0/*&& ent->client->NPC_class == CLASS_VEHICLE*/)
	{
		//we don't actually scale the ucmd, we use actual speeds
		//FIXME: swoop should keep turning (and moving forward?) for a little bit?
		if (ent->NPC->combatMove == qfalse)
		{
			if (!(ucmd->buttons & BUTTON_USE))
			{
				//Not leaning
				const auto Flying = static_cast<qboolean>(ucmd->upmove && ent->client->moveType == MT_FLYSWIM);
				const auto Climbing = static_cast<qboolean>(ucmd->upmove && ent->watertype & CONTENTS_LADDER);

				client->ps.friction = 6;

				if (ucmd->forwardmove || ucmd->rightmove || Flying)
				{
					//if ( ent->NPC->behaviorState != BS_FORMATION )
					{
						//In - Formation NPCs set thier desiredSpeed themselves
						if (ucmd->buttons & BUTTON_WALKING)
						{
							ent->NPC->desiredSpeed = NPC_GetWalkSpeed(ent); //ent->NPC->stats.walkSpeed;
						}
						else //running
						{
							ent->NPC->desiredSpeed = NPC_GetRunSpeed(ent); //ent->NPC->stats.runSpeed;
						}

						if (ent->NPC->currentSpeed >= 80 && !controlledByPlayer)
						{
							//At higher speeds, need to slow down close to stuff
							//Slow down as you approach your goal
							if (ent->NPC->distToGoal < SLOWDOWN_DIST && !(ent->NPC->aiFlags & NPCAI_NO_SLOWDOWN)) //128
							{
								if (ent->NPC->desiredSpeed > MIN_NPC_SPEED)
								{
									const float slowdownSpeed = static_cast<float>(ent->NPC->desiredSpeed) * ent->NPC->
										distToGoal / SLOWDOWN_DIST;

									ent->NPC->desiredSpeed = ceil(slowdownSpeed);
									if (ent->NPC->desiredSpeed < MIN_NPC_SPEED)
									{
										//don't slow down too much
										ent->NPC->desiredSpeed = MIN_NPC_SPEED;
									}
								}
							}
						}
					}
				}
				else if (Climbing)
				{
					ent->NPC->desiredSpeed = ent->NPC->stats.walkSpeed;
				}
				else
				{
					//We want to stop
					ent->NPC->desiredSpeed = 0;
				}

				NPC_Accelerate(ent, qfalse, qfalse);

				if (ent->NPC->currentSpeed <= 24 && ent->NPC->desiredSpeed < ent->NPC->currentSpeed)
				{
					//No-one walks this slow
					client->ps.speed = ent->NPC->currentSpeed = 0; //Full stop
					ucmd->forwardmove = 0;
					ucmd->rightmove = 0;
				}
				else
				{
					if (ent->NPC->currentSpeed <= ent->NPC->stats.walkSpeed)
					{
						//Play the walkanim
						ucmd->buttons |= BUTTON_WALKING;
					}
					else
					{
						ucmd->buttons &= ~BUTTON_WALKING;
					}

					if (ent->NPC->currentSpeed > 0)
					{
						//We should be moving
						if (Climbing || Flying)
						{
							if (!ucmd->upmove)
							{
								//We need to force them to take a couple more steps until stopped
								ucmd->upmove = ent->NPC->last_ucmd.upmove; //was last_upmove;
							}
						}
						else if (!ucmd->forwardmove && !ucmd->rightmove)
						{
							//We need to force them to take a couple more steps until stopped
							ucmd->forwardmove = ent->NPC->last_ucmd.forwardmove; //was last_forwardmove;
							ucmd->rightmove = ent->NPC->last_ucmd.rightmove; //was last_rightmove;
						}
					}

					client->ps.speed = ent->NPC->currentSpeed;
					if (player && player->client && player->client->ps.viewEntity == ent->s.number)
					{
					}
					else
					{
						//Slow down on turns - don't orbit!!!
						float turndelta;
						// if the NPC is locked into a Yaw, we want to check the lockedDesiredYaw...otherwise the NPC can't walk backwards, because it always thinks it trying to turn according to desiredYaw
						if (client->renderInfo.renderFlags & RF_LOCKEDANGLE)
							// yeah I know the RF_ flag is a pretty ugly hack...
						{
							turndelta = (180 - fabs(AngleDelta(ent->currentAngles[YAW], ent->NPC->lockedDesiredYaw))) /
								180;
						}
						else
						{
							turndelta = (180 - fabs(AngleDelta(ent->currentAngles[YAW], ent->NPC->desiredYaw))) / 180;
						}

						if (turndelta < 0.75f)
						{
							client->ps.speed = 0;
						}
						else if (ent->NPC->distToGoal < 100 && turndelta < 1.0)
						{
							//Turn is greater than 45 degrees or closer than 100 to goal
							client->ps.speed = floor(static_cast<float>(client->ps.speed) * turndelta);
						}
					}
				}
			}
		}
		else
		{
			ent->NPC->desiredSpeed = ucmd->buttons & BUTTON_WALKING ? NPC_GetWalkSpeed(ent) : NPC_GetRunSpeed(ent);

			client->ps.speed = ent->NPC->desiredSpeed;
		}
	}
	else
	{
		//Client sets ucmds and such for speed alterations
		client->ps.speed = g_speed->value;

		if (client->ps.heldClient < ENTITYNUM_WORLD)
		{
			client->ps.speed *= 0.3f;
		}
		else if (BG_SprintAnim(client->ps.legsAnim))
		{
			if (g_standard_humanoid(ent))
			{
				if (client->NPC_class == CLASS_VADER)
				{
					client->ps.speed *= 1.15f;
				}
				else
				{
					client->ps.speed *= 1.40f;
				}
			}
		}
		else if (BG_SaberSprintAnim(client->ps.legsAnim))
		{
			if (g_standard_humanoid(ent))
			{
				if (client->NPC_class == CLASS_VADER)
				{
					client->ps.speed *= 1.15f;
				}
				else
				{
					client->ps.speed *= 1.30f;
				}
			}
		}
		else if (BG_WeaponSprintAnim(client->ps.legsAnim))
		{
			if (g_standard_humanoid(ent))
			{
				if (client->NPC_class == CLASS_VADER)
				{
					client->ps.speed *= 1.10f;
				}
				else
				{
					client->ps.speed *= 1.20f;
				}
			}
		}
		else if (PM_SaberInAttack(client->ps.saberMove) && ucmd->forwardmove < 0)
		{
			//if running backwards while attacking, don't run as fast.
			switch (client->ps.saberAnimLevel)
			{
			case SS_FAST:
				client->ps.speed *= 0.85f;
				break;
			case SS_MEDIUM:
			case SS_DUAL:
			case SS_STAFF:
				client->ps.speed *= 0.70f;
				break;
			case SS_STRONG:
				client->ps.speed *= 0.55f;
				break;
			default:;
			}
			if (g_saberMoveSpeed->value != 1.0f)
			{
				client->ps.speed *= g_saberMoveSpeed->value;
			}
		}
		else if (PM_LeapingSaberAnim(client->ps.legsAnim))
		{
			//no mod on speed when leaping
			//FIXME: maybe jump?
		}
		else if (PM_SpinningSaberAnim(client->ps.legsAnim))
		{
			client->ps.speed *= 0.5f;
			if (g_saberMoveSpeed->value != 1.0f)
			{
				client->ps.speed *= g_saberMoveSpeed->value;
			}
		}
		else if (client->ps.weapon == WP_SABER && ucmd->buttons & BUTTON_ATTACK)
		{
			//if attacking with saber while running, drop your speed
			//FIXME: should be weaponTime?  Or in certain anims?
			switch (client->ps.saberAnimLevel)
			{
			case SS_TAVION:
			case SS_FAST:
				client->ps.speed *= 0.80f;
				break;
			case SS_MEDIUM:
			case SS_DUAL:
			case SS_STAFF:
				client->ps.speed *= 0.85f;
				break;
			case SS_STRONG:
				client->ps.speed *= 0.70f;
				break;
			default:;
			}
			if (g_saberMoveSpeed->value != 1.0f)
			{
				client->ps.speed *= g_saberMoveSpeed->value;
			}
		}
		else if (client->ps.weapon == WP_SABER && client->ps.saberAnimLevel == FORCE_LEVEL_3 &&
			PM_SaberInTransition(client->ps.saberMove))
		{
			//Now, we want to even slow down in transitions for level 3 (since it has chains and stuff)
			if (ucmd->forwardmove < 0)
			{
				client->ps.speed *= 0.4f;
			}
			else
			{
				client->ps.speed *= 0.6f;
			}
		}
	}
	if (client->NPC_class == CLASS_ATST && client->ps.legsAnim == BOTH_RUN1START)
	{
		//HACK: when starting to move as atst, ramp up speed
		//float animLength = PM_AnimLength( client->clientInfo.animFileIndex, (animNumber_t)client->ps.legsAnim);
		//client->ps.speed *= ( animLength - client->ps.legsAnimTimer)/animLength;
		if (client->ps.legsAnimTimer > 100)
		{
			client->ps.speed = 0;
		}
	}

	//Apply forced movement
	if (client->forced_forwardmove)
	{
		ucmd->forwardmove = client->forced_forwardmove;
		if (!client->ps.speed)
		{
			if (ent->NPC != nullptr)
			{
				client->ps.speed = ent->NPC->stats.runSpeed;
			}
			else
			{
				client->ps.speed = g_speed->value; //default is 320
			}
		}
	}

	if (client->forced_rightmove)
	{
		ucmd->rightmove = client->forced_rightmove;
		if (!client->ps.speed)
		{
			if (ent->NPC != nullptr)
			{
				client->ps.speed = ent->NPC->stats.runSpeed;
			}
			else
			{
				client->ps.speed = g_speed->value; //default is 320
			}
		}
	}

	if (!pVeh)
	{
		if (ucmd->forwardmove < 0 && !(ucmd->buttons & BUTTON_WALKING) && client->ps.groundEntityNum != ENTITYNUM_NONE)
		{
			//running backwards is slower than running forwards
			client->ps.speed *= 0.75;
		}

		if (client->ps.forceRageRecoveryTime > level.time)
		{
			client->ps.speed *= 0.75;
		}

		if (client->ps.weapon == WP_SABER)
		{
			if (client->ps.saber[0].moveSpeedScale != 1.0f)
			{
				client->ps.speed *= client->ps.saber[0].moveSpeedScale;
			}
			if (client->ps.dualSabers
				&& client->ps.saber[1].moveSpeedScale != 1.0f)
			{
				client->ps.speed *= client->ps.saber[1].moveSpeedScale;
			}
		}
	}
}

extern qboolean ForceDrain2(gentity_t* ent);
extern void ForceGrip(gentity_t* ent);
extern void ForceLightning(gentity_t* ent);
extern void ForceProtect(gentity_t* ent);
extern void ForceRage(gentity_t* ent);
extern void ForceSeeing(gentity_t* ent);
extern void ForceTelepathy(gentity_t* ent);
extern void ForceAbsorb(gentity_t* ent);
extern void ForceHeal(gentity_t* ent);
extern void ForceDestruction(gentity_t* ent);
extern void ForceStasis(gentity_t* ent);
extern void ForceJediRepulse(gentity_t* ent);
extern void ForceGrasp(gentity_t* ent);
extern void ForceLightningStrike(gentity_t* ent);
extern void ForceFear(gentity_t* ent);
extern void ForceDeadlySight(gentity_t* ent);
extern void ForceBlast(gentity_t* self);
extern void ForceProjection(gentity_t* self);

static void ProcessGenericCmd(gentity_t* ent, byte cmd)
{
	switch (cmd)
	{
	default:
		break;
	case GENCMD_FORCE_HEAL:
		ForceHeal(ent);
		break;
	case GENCMD_FORCE_SPEED:
		if (ent->client->ps.communicatingflags & 1 << DASHING)
		{
			ForceSpeedDash(ent);
		}
		else
		{
			ForceSpeed(ent);
		}
		break;
	case GENCMD_FORCE_THROW:
		if (g_SerenityJediEngineMode->integer)
		{
			if (ent->client->NPC_class == CLASS_GALEN
				&& (ent->s.weapon == WP_MELEE || ent->s.weapon == WP_NONE || ent->s.weapon == WP_SABER && !ent->
					client->ps.SaberActive())
				&& ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				ForceRepulse(ent, qfalse);
			}
			else
			{
				ForceThrow_MD(ent, qfalse);
			}
		}
		else
		{
			ForceThrow_JKA(ent, qfalse);
		}
		break;
	case GENCMD_FORCE_PULL:
		if (g_SerenityJediEngineMode->integer)
		{
			if (ent->client->NPC_class == CLASS_GALEN
				&& (ent->s.weapon == WP_MELEE || ent->s.weapon == WP_NONE || ent->s.weapon == WP_SABER && !ent->
					client->ps.SaberActive())
				&& ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
			{
				ForceRepulse(ent, qtrue);
			}
			else
			{
				ForceThrow_MD(ent, qtrue);
			}
		}
		else
		{
			ForceThrow_JKA(ent, qtrue);
		}
		break;
	case GENCMD_FORCE_DISTRACT:
		ForceTelepathy(ent);
		break;
	case GENCMD_FORCE_GRIP:
		ForceGrip(ent);
		break;
	case GENCMD_FORCE_LIGHTNING:
		ForceLightning(ent);
		break;
	case GENCMD_FORCE_RAGE:
		ForceRage(ent);
		break;
	case GENCMD_FORCE_PROTECT:
		ForceProtect(ent);
		break;
	case GENCMD_FORCE_ABSORB:
		ForceAbsorb(ent);
		break;
	case GENCMD_FORCE_DRAIN:
		ForceDrain2(ent);
		break;
	case GENCMD_FORCE_SEEING:
		ForceSeeing(ent);
		break;
	case GENCMD_FORCE_DESTRUCTION:
		ForceDestruction(ent);
		break;
	case GENCMD_FORCE_STASIS:
		ForceStasis(ent);
		break;
	case GENCMD_FORCE_GRASP:
		ForceGrasp(ent);
		break;
	case GENCMD_FORCE_REPULSE:
		ForceJediRepulse(ent);
		ent->client->ps.powerups[PW_INVINCIBLE] = level.time + ent->client->ps.torsoAnimTimer + 2000;
		break;
	case GENCMD_FORCE_LIGHTNING_STRIKE:
		ForceLightningStrike(ent);
		break;
	case GENCMD_FORCE_FEAR:
		ForceFear(ent);
		break;
	case GENCMD_FORCE_DEADLYSIGHT:
		ForceDeadlySight(ent);
		break;
	case GENCMD_FORCE_PROJECTION:
		ForceProjection(ent);
		break;
	case GENCMD_FORCE_BLAST:
		ForceBlast(ent);
		break;
	}
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

==============
*/

void CG_BreathPuffsVader(const gentity_t* ent)
{
	gclient_t* client = ent->client;

	if (ent->health < 1 ||
		client->VaderBreathTime > cg.time ||
		in_camera || g_VaderBreath->integer == 0 ||
		cg_setVaderBreath.integer == 1 ||
		cg_setVaderBreathdamaged.integer == 1)
	{
		return;
	}

	if (ent->s.eFlags & EF_NODRAW)
	{
		return;
	}

	if (PM_CrouchAnim(client->ps.legsAnim) ||
		PM_CrouchAnim(client->ps.torsoAnim) ||
		PM_LungeAnim(client->ps.torsoAnim) ||
		PM_RollingAnim(client->ps.legsAnim))
	{
		return;
	}

	if (ent->client->NPC_class == CLASS_VADER)
	{
		if (ent && !Q_stricmp("md_vader_ep3", ent->NPC_type) ||
			ent && !Q_stricmp("md_vader_anh", ent->NPC_type) ||
			ent && !Q_stricmp("md_vader_tv", ent->NPC_type) ||
			ent && !Q_stricmp("md_vad_tfu", ent->NPC_type) ||
			ent && !Q_stricmp("md_vad_vr", ent->NPC_type) ||
			ent && !Q_stricmp("md_vader_ds", ent->NPC_type) ||
			ent && !Q_stricmp("md_vader", ent->NPC_type))
		{
			if (ent->health <= client->ps.stats[STAT_MAX_HEALTH] / 3)
			{
				G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath3.mp3");
			}
			else
			{
				if (client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
				{
					G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/vader_fast_breath.mp3");
				}
				else
				{
					G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath1.mp3");
				}
			}
		}
		else if (ent && !Q_stricmp("md_vader_bw", ent->NPC_type) ||
			ent && !Q_stricmp("md_vad2_tfu", ent->NPC_type))
		{
			if (ent->health <= client->ps.stats[STAT_MAX_HEALTH] / 3)
			{
				G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath4.mp3");
			}
			else
			{
				if (client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
				{
					G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/vader_fast_breath.mp3");
				}
				else
				{
					G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath3.mp3");
				}
			}
		}
	}
	else if (client->NPC_class == CLASS_DESANN)
	{
		if (ent && !Q_stricmp("md_vader_ep3", ent->NPC_type) ||
			ent && !Q_stricmp("md_vader_tv", ent->NPC_type) ||
			ent && !Q_stricmp("md_vader_anh", ent->NPC_type) ||
			ent && !Q_stricmp("md_vad_tfu", ent->NPC_type) ||
			ent && !Q_stricmp("md_vad_vr", ent->NPC_type) ||
			ent && !Q_stricmp("md_vader_ds", ent->NPC_type) ||
			ent && !Q_stricmp("md_vader", ent->NPC_type))
		{
			if (ent->health <= client->ps.stats[STAT_MAX_HEALTH] / 3)
			{
				G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath3.mp3");
			}
			else
			{
				if (client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
				{
					G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/vader_fast_breath.mp3");
				}
				else
				{
					G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath1.mp3");
				}
			}
		}
		else if (ent && !Q_stricmp("md_vader_bw", ent->NPC_type) ||
			ent && !Q_stricmp("md_vad2_tfu", ent->NPC_type))
		{
			if (ent->health <= client->ps.stats[STAT_MAX_HEALTH] / 3)
			{
				G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath4.mp3");
			}
			else
			{
				if (client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
				{
					G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/vader_fast_breath.mp3");
				}
				else
				{
					G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath3.mp3");
				}
			}
		}
	}

	if (PM_SaberInAttack(client->ps.saberMove))
	{
		client->VaderBreathTime = cg.time + 4000; // every 4 seconds.
	}
	else if (in_camera)
	{
		client->VaderBreathTime = cg.time + 4000; // every 4 seconds.
	}
	else if (gi.VoiceVolume[ent->s.number] > 0)
	{
		client->VaderBreathTime = cg.time + 10000; // every 10 seconds.
	}
	else
	{
		if (PM_RunningAnim(client->ps.legsAnim))
		{
			client->VaderBreathTime = cg.time + 3000; // every 2 seconds.
		}
		else
		{
			client->VaderBreathTime = cg.time + 6000; // every 6 seconds.
		}
	}
}

extern vmCvar_t cg_setVaderBreath;
extern vmCvar_t cg_setVaderBreathdamaged;

void CG_SetVaderBreath(const gentity_t* ent)
{
	gclient_t* client = ent->client;

	if (ent->health < 1 || client->VaderBreathTime > cg.time)
	{
		return;
	}

	if (ent->s.eFlags & EF_NODRAW)
	{
		return;
	}

	if (!in_camera)
	{
		return;
	}

	if (PM_CrouchAnim(client->ps.legsAnim) ||
		PM_CrouchAnim(client->ps.torsoAnim) ||
		PM_LungeAnim(client->ps.torsoAnim) ||
		PM_RollingAnim(client->ps.legsAnim))
	{
		return;
	}

	if (ent && !Q_stricmp("md_vader_ep3", ent->NPC_type) ||
		ent && !Q_stricmp("md_vader_tv", ent->NPC_type) ||
		ent && !Q_stricmp("md_vader_anh", ent->NPC_type) ||
		ent && !Q_stricmp("md_vad_vr", ent->NPC_type) ||
		ent && !Q_stricmp("md_vader_ds", ent->NPC_type) ||
		ent && !Q_stricmp("md_vader", ent->NPC_type))
	{
		if (ent->health <= client->ps.stats[STAT_MAX_HEALTH] / 3)
		{
			G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath3.mp3");
		}
		else
		{
			if (client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
			{
				G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/vader_fast_breath.mp3");
			}
			else
			{
				G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath1.mp3");
			}
		}
	}
	else if (ent && !Q_stricmp("md_vad_tfu", ent->NPC_type) ||
		ent && !Q_stricmp("md_vad_vr", ent->NPC_type))
	{
		if (client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
		{
			G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/vader_fast_breath.mp3");
		}
		else
		{
			G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath2.mp3");
		}
	}
	else if (ent && !Q_stricmp("md_vader_bw", ent->NPC_type) ||
		ent && !Q_stricmp("md_vad2_tfu", ent->NPC_type))
	{
		if (client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
		{
			G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/vader_fast_breath.mp3");
		}
		else
		{
			G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath3.mp3");
		}
	}

	if (PM_SaberInAttack(client->ps.saberMove))
	{
		client->VaderBreathTime = cg.time + 4000; // every 4 seconds.
	}
	else if (in_camera)
	{
		client->VaderBreathTime = cg.time + 4000; // every 4 seconds.
	}
	else if (gi.VoiceVolume[ent->s.number] > 0)
	{
		client->VaderBreathTime = cg.time + 10000; // every 10 seconds.
	}
	else
	{
		if (PM_RunningAnim(client->ps.legsAnim))
		{
			client->VaderBreathTime = cg.time + 3000; // every 2 seconds.
		}
		else
		{
			client->VaderBreathTime = cg.time + 6000; // every 6 seconds.
		}
	}
}

void CG_SetVaderBreathDamaged(const gentity_t* ent)
{
	gclient_t* client = ent->client;

	if (ent->health < 1 || client->VaderBreathTime > cg.time)
	{
		return;
	}

	if (ent->s.eFlags & EF_NODRAW)
	{
		return;
	}

	if (!in_camera)
	{
		return;
	}

	if (PM_CrouchAnim(client->ps.legsAnim) ||
		PM_CrouchAnim(client->ps.torsoAnim) ||
		PM_LungeAnim(client->ps.torsoAnim) ||
		PM_RollingAnim(client->ps.legsAnim))
	{
		return;
	}

	if (ent && !Q_stricmp("md_vader_ep3", ent->NPC_type) ||
		ent && !Q_stricmp("md_vader_tv", ent->NPC_type) ||
		ent && !Q_stricmp("md_vader_anh", ent->NPC_type) ||
		ent && !Q_stricmp("md_vad_tfu", ent->NPC_type) ||
		ent && !Q_stricmp("md_vad_vr", ent->NPC_type) ||
		ent && !Q_stricmp("md_vader_ds", ent->NPC_type) ||
		ent && !Q_stricmp("md_vader", ent->NPC_type))
	{
		if (client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
		{
			G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/vader_fast_breath.mp3");
		}
		else
		{
			G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath3.mp3");
		}
	}
	else if (ent && !Q_stricmp("md_vader_bw", ent->NPC_type) ||
		ent && !Q_stricmp("md_vad2_tfu", ent->NPC_type))
	{
		if (client->ps.PlayerEffectFlags & 1 << PEF_SPRINTING)
		{
			G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/vader_fast_breath.mp3");
		}
		else
		{
			G_SoundOnEnt(ent, CHAN_VOICE, "sound/chars/darthvader/breath4.mp3");
		}
	}

	if (PM_SaberInAttack(client->ps.saberMove))
	{
		client->VaderBreathTime = cg.time + 4000; // every 4 seconds.
	}
	else if (in_camera)
	{
		client->VaderBreathTime = cg.time + 4000; // every 4 seconds.
	}
	else if (gi.VoiceVolume[ent->s.number] > 0)
	{
		client->VaderBreathTime = cg.time + 10000; // every 10 seconds.
	}
	else
	{
		if (PM_RunningAnim(ent->client->ps.legsAnim))
		{
			client->VaderBreathTime = cg.time + 3000; // every 2 seconds.
		}
		else
		{
			client->VaderBreathTime = cg.time + 6000; // every 6 seconds.
		}
	}
}

//// reload ////

int ReloadTime(const gentity_t* ent)
{
	if (ent->client->ps.weapon == WP_FLECHETTE)
	{
		return 1000;
	}
	if (ent->client->ps.weapon == WP_ROCKET_LAUNCHER)
	{
		return 3000;
	}
	return 300;
}

int PainTime(const gentity_t* ent)
{
	if (ent->client->ps.weapon == WP_FLECHETTE)
	{
		return 250;
	}
	if (ent->client->ps.weapon == WP_ROCKET_LAUNCHER)
	{
		return 500;
	}
	return 150;
}

qboolean IsHoldingGun(const gentity_t* ent)
{
	switch (ent->s.weapon)
	{
	case WP_BLASTER_PISTOL:
	case WP_BLASTER:
	case WP_DISRUPTOR:
	case WP_BOWCASTER:
	case WP_REPEATER:
	case WP_DEMP2:
	case WP_FLECHETTE:
	case WP_ROCKET_LAUNCHER:
	case WP_CONCUSSION:
	case WP_BRYAR_PISTOL:
	case WP_DUAL_PISTOL:
		//
	case WP_TUSKEN_RIFLE:
	case WP_BATTLEDROID:
	case WP_THEFIRSTORDER:
	case WP_CLONECARBINE:
	case WP_REBELBLASTER:
	case WP_CLONERIFLE:
	case WP_CLONECOMMANDO:
	case WP_REBELRIFLE:
	case WP_REY:
	case WP_JANGO:
	case WP_BOBA:
	case WP_CLONEPISTOL:
		return qtrue;
	default:;
	}
	return qfalse;
}

int ClipSize(int ammo, gentity_t* ent)
{
	switch (ammo)
	{
	case AMMO_BLASTER:
		return 20;
	case AMMO_POWERCELL:
		return 30;
	case AMMO_METAL_BOLTS:
		return 50;
	case AMMO_ROCKETS:
		return 2;
	default:;
	}
	return -1;
}

int MagazineSize(int ammo, gentity_t* ent)
{
	switch (ammo)
	{
	case AMMO_BLASTER:
		return 50;
	case AMMO_POWERCELL:
		return 50;
	case AMMO_METAL_BOLTS:
		return 75;
	case AMMO_ROCKETS:
		return 3;
	default:;
	}
	return -1;
}

void ReloadGun(gentity_t* ent)
{
	if (ent->reloadCooldown > level.time)
	{
		return;
	}

	if (ent->client->ps.weaponTime > 0 && ent->client->ps.weaponstate != WEAPON_READY)
	{
		// Weapon is busy, cannot reload at this moment
		return;
	}
	if (ent->client->ps.weaponstate == WEAPON_CHARGING)
	{
		return;
	}

	if (IsHoldingGun(ent))
	{
		if (ent->client->ps.BlasterAttackChainCount > BLASTERMISHAPLEVEL_TWENTYSIX)
		{
			if (ent->s.weapon == WP_BRYAR_PISTOL ||
				ent->s.weapon == WP_BLASTER_PISTOL ||
				ent->s.weapon == WP_DUAL_PISTOL ||
				ent->s.weapon == WP_REY ||
				ent->s.weapon == WP_JANGO ||
				ent->s.weapon == WP_CLONEPISTOL ||
				ent->s.weapon == WP_REBELBLASTER)
			{
				if (ent->weaponModel[1] > 0)
				{
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_2PISTOLFAIL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
				else
				{
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_PISTOLFAIL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				}
			}
			else if (ent->s.weapon == WP_ROCKET_LAUNCHER)
			{
				NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ROCKETFAIL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}
			else
			{
				NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RIFLEFAIL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
			}

			G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/reloadfail.mp3");
			G_SoundOnEnt(ent, CHAN_VOICE_ATTEN, "*pain25.wav");
			G_Damage(ent, nullptr, nullptr, nullptr, ent->currentOrigin, 2, DAMAGE_NO_ARMOR, MOD_LAVA);
			ent->reloadTime = level.time + PainTime(ent);
		}
		else
		{
			if (ent->s.weapon == WP_BRYAR_PISTOL ||
				ent->s.weapon == WP_BLASTER ||
				ent->s.weapon == WP_BLASTER_PISTOL ||
				ent->s.weapon == WP_BATTLEDROID ||
				ent->s.weapon == WP_THEFIRSTORDER ||
				ent->s.weapon == WP_REY ||
				ent->s.weapon == WP_JANGO ||
				ent->s.weapon == WP_DUAL_PISTOL ||
				ent->s.weapon == WP_REBELBLASTER)
			{
				if (ent->client->ps.ammo[AMMO_BLASTER] < ClipSize(AMMO_BLASTER, ent))
				{
					ent->client->ps.ammo[AMMO_BLASTER] += MagazineSize(AMMO_BLASTER, ent);

					if (ent->s.weapon == WP_BRYAR_PISTOL ||
						ent->s.weapon == WP_BLASTER_PISTOL ||
						ent->s.weapon == WP_DUAL_PISTOL ||
						ent->s.weapon == WP_REY ||
						ent->s.weapon == WP_JANGO ||
						ent->s.weapon == WP_REBELBLASTER)
					{
						if (ent->weaponModel[1] > 0)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_2PISTOLRELOAD,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_PISTOLRELOAD,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RIFLERELOAD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}

					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/reload.mp3");
					ent->reloadTime = level.time + ReloadTime(ent);
				}
				else
				{
					if (ent->s.weapon == WP_BRYAR_PISTOL ||
						ent->s.weapon == WP_BLASTER_PISTOL ||
						ent->s.weapon == WP_DUAL_PISTOL ||
						ent->s.weapon == WP_REY ||
						ent->s.weapon == WP_JANGO ||
						ent->s.weapon == WP_REBELBLASTER)
					{
						if (ent->weaponModel[1] > 0)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_2PISTOLCHARGE,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_PISTOLCHARGE,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RIFLERECHARGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/recharge.mp3");
				}
			}
			else if (ent->s.weapon == WP_DISRUPTOR ||
				ent->s.weapon == WP_BOWCASTER ||
				ent->s.weapon == WP_DEMP2 ||
				ent->s.weapon == WP_TUSKEN_RIFLE ||
				ent->s.weapon == WP_REBELRIFLE ||
				ent->s.weapon == WP_BOBA)
			{
				if (ent->client->ps.ammo[AMMO_POWERCELL] < ClipSize(AMMO_POWERCELL, ent))
				{
					ent->client->ps.ammo[AMMO_POWERCELL] += MagazineSize(AMMO_POWERCELL, ent);

					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RIFLERELOAD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/reload.mp3");
					ent->reloadTime = level.time + ReloadTime(ent);
				}
				else
				{
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RIFLERECHARGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/recharge.mp3");
				}
			}
			else if (ent->s.weapon == WP_REPEATER ||
				ent->s.weapon == WP_FLECHETTE ||
				ent->s.weapon == WP_CONCUSSION ||
				ent->s.weapon == WP_CLONECARBINE ||
				ent->s.weapon == WP_CLONERIFLE ||
				ent->s.weapon == WP_CLONECOMMANDO ||
				ent->s.weapon == WP_CLONEPISTOL)
			{
				if (ent->client->ps.ammo[AMMO_METAL_BOLTS] < ClipSize(AMMO_METAL_BOLTS, ent))
				{
					ent->client->ps.ammo[AMMO_METAL_BOLTS] += MagazineSize(AMMO_METAL_BOLTS, ent);

					if (ent->s.weapon == WP_REPEATER ||
						ent->s.weapon == WP_FLECHETTE ||
						ent->s.weapon == WP_CONCUSSION ||
						ent->s.weapon == WP_CLONECARBINE ||
						ent->s.weapon == WP_CLONERIFLE ||
						ent->s.weapon == WP_CLONECOMMANDO)
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RIFLERELOAD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					else if (ent->s.weapon == WP_CLONEPISTOL)
					{
						if (ent->weaponModel[1] > 0)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_2PISTOLRELOAD,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_PISTOLRELOAD,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/reload.mp3");
					ent->reloadTime = level.time + ReloadTime(ent);
				}
				else
				{
					if (ent->s.weapon == WP_CLONEPISTOL)
					{
						if (ent->weaponModel[1] > 0)
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_2PISTOLCHARGE,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
						else
						{
							NPC_SetAnim(ent, SETANIM_TORSO, BOTH_PISTOLCHARGE,
								SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
						}
					}
					else
					{
						NPC_SetAnim(ent, SETANIM_TORSO, BOTH_RIFLERECHARGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/recharge.mp3");
				}
			}
			else if (ent->s.weapon == WP_ROCKET_LAUNCHER)
			{
				if (ent->client->ps.ammo[AMMO_ROCKETS] < ClipSize(AMMO_ROCKETS, ent))
				{
					ent->client->ps.ammo[AMMO_ROCKETS] += MagazineSize(AMMO_ROCKETS, ent);
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ROCKETRELOAD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/reload.mp3");
					ent->reloadTime = level.time + ReloadTime(ent);
				}
				else
				{
					NPC_SetAnim(ent, SETANIM_TORSO, BOTH_ROCKETCHARGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					G_SoundOnEnt(ent, CHAN_WEAPON, "sound/weapons/recharge.mp3");
				}
			}
			else
			{
				ent->reloadTime = 0;
			}
			ent->client->ps.weaponTime = ent->client->ps.torsoAnimTimer;
			BG_ReduceBlasterMishapLevelAdvanced(&ent->client->ps);
		}

		if (ent->s.number == 0 && cg.zoomMode)
		{
			// yeah, it should really toggle them so it plays the end sound....
			cg.zoomMode = 0;
		}

		ent->client->ps.weaponstate = WEAPON_RELOADING;
	}
}

void CancelReload(gentity_t* ent)
{
	ent->reloadTime = 0;
	ent->reloadCooldown = level.time + 500;
}

////////////////////// reload

extern void Weapon_AltStun_Fire(gentity_t* ent);
extern void Ent_CheckBarrierIsAllowed(gentity_t* ent);
extern void Ent_CheckBarrierIsAllowed_WithSaber(gentity_t* ent);

void ClientThink_real(gentity_t* ent, usercmd_t* ucmd)
{
	gclient_t* client;
	pmove_t pm;
	vec3_t oldOrigin;
	int oldEventSequence;
	int msec;
	qboolean inSpinFlipAttack = PM_AdjustAnglesForSpinningFlip(ent, ucmd, qfalse);
	qboolean controlledByPlayer = qfalse;
	Vehicle_t* pVeh = nullptr;

	qboolean HoldingStun = ent->client->ps.communicatingflags & 1 << STUNNING ? qtrue : qfalse;

	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		pVeh = ent->m_pVehicle;
	}

	//Don't let the player do anything if in a camera
	if (ent->s.eFlags & EF_HELD_BY_RANCOR
		|| ent->s.eFlags & EF_HELD_BY_WAMPA)
	{
		G_HeldByMonster(ent, &ucmd);
	}

	if (ent->client && ent->client->ps.powerups[PW_GALAK_SHIELD])
	{
		if (ent->client->ps.weapon == WP_SABER)
		{
			Ent_CheckBarrierIsAllowed_WithSaber(ent);
		}
		else
		{
			Ent_CheckBarrierIsAllowed(ent);
		}
	}

	if (ent->s.number == 0)
	{
		extern cvar_t* g_skippingcin;

		if (ent->s.eFlags & EF_LOCKED_TO_WEAPON)
		{
			G_UpdateEmplacedWeaponData(ent);
			RunEmplacedWeapon(ent, &ucmd);
		}
		if (ent->client->ps.saberLockTime > level.time && ent->client->ps.saberLockEnemy != ENTITYNUM_NONE)
		{
			NPC_SetLookTarget(ent, ent->client->ps.saberLockEnemy, level.time + 1000);
		}
		if (ent->client->renderInfo.lookTargetClearTime < level.time
			//NOTE: here this is used as a debounce, not an actual timer
			&& ent->health > 0 //must be alive
			&& (!ent->enemy || ent->client->ps.saberMove != LS_A_BACKSTAB))
			//don't update if in backstab unless don't currently have an enemy
		{
			//NOTE: doesn't keep updating to nearest enemy once you're dead
			int newLookTarget;
			if (!G_ValidateLookEnemy(ent, ent->enemy))
			{
				ent->enemy = nullptr;
			}
			//FIXME: make this a little prescient?
			G_ChooseLookEnemy(ent, ucmd);
			if (ent->enemy)
			{
				//target
				newLookTarget = ent->enemy->s.number;
				//so we don't change our minds in the next 1 second
				ent->client->renderInfo.lookTargetClearTime = level.time + 1000;
				ent->client->renderInfo.lookMode = LM_ENT;
			}
			else
			{
				//no target
				//FIXME: what about sightalerts and missiles?
				newLookTarget = ENTITYNUM_NONE;
				newLookTarget = G_FindLocalInterestPoint(ent);
				if (newLookTarget != ENTITYNUM_NONE)
				{
					//found something of interest
					ent->client->renderInfo.lookMode = LM_INTEREST;
				}
				else
				{
					//okay, no interesting things and no enemies, so look for items
					newLookTarget = G_FindLookItem(ent);
					ent->client->renderInfo.lookMode = LM_ENT;
				}
			}
			if (ent->client->renderInfo.lookTarget != newLookTarget)
			{
				//transitioning
				NPC_SetLookTarget(ent, newLookTarget, level.time + 1000);
			}
		}
		if (in_camera)
		{
			// watch the code here, you MUST "return" within this IF(), *unless* you're stopping the cinematic skip.
			//
			if (ClientCinematicThink(ent->client))
			{
				if (g_skippingcin->integer) // already doing cinematic skip?
				{
					// yes...   so stop skipping...
					G_StopCinematicSkip();
				}
				else
				{
					// no... so start skipping...
					G_StartCinematicSkip();
					return;
				}
			}
			else
			{
				return;
			}
		}
		// If he's riding the vehicle...
		else if (ent->s.m_iVehicleNum != 0 && ent->health > 0)
		{
		}
		else
		{
			if (g_skippingcin->integer)
			{
				//We're skipping the cinematic and it's over now
				gi.cvar_set("timescale", "1");
				gi.cvar_set("skippingCinematic", "0");
			}
			if (ent->client->ps.pm_type == PM_DEAD && cg.missionStatusDeadTime < level.time)
			{
				//mission status screen is up because player is dead, stop all scripts
				stop_icarus = qtrue;
			}
		}

		if (!in_camera &&
			g_entities[0].nearAllies != ENTITYNUM_NONE &&
			ucmd->buttons & BUTTON_USE &&
			g_entities[0].calloutTime <= level.time && g_allowcallout->integer == 1)
		{
			DoCallout(&g_entities[0], &g_entities[g_entities[0].nearAllies]);
		}

		if (cg.zoomMode == 2)
		{
			// Any kind of movement when the player is NOT ducked when the disruptor gun is zoomed will cause us to auto-magically un-zoom
			if ((ucmd->forwardmove || ucmd->rightmove)
				&& ucmd->upmove >= 0 //crouching-moving is ok
				&& !(ucmd->buttons & BUTTON_USE) /*leaning is ok*/
				|| ucmd->upmove > 0 //jumping not allowed
				)
			{
				// already zooming, so must be wanting to turn it off
				G_Sound(ent, G_SoundIndex("sound/weapons/disruptor/zoomend.wav"));
				cg.zoomMode = 0;
				cg.zoomTime = cg.time;
				cg.zoomLocked = qfalse;
			}
		}

		if ((player_locked
			|| PlayerAffectedByStasis()
			|| ent->client->ps.eFlags & EF_FORCE_GRIPPED
			|| ent->client->ps.eFlags & EF_FORCE_DRAINED
			|| ent->client->ps.eFlags & EF_FORCE_GRASPED
			|| ent->client->ps.legsAnim == BOTH_PLAYER_PA_1
			|| ent->client->ps.legsAnim == BOTH_PLAYER_PA_2
			|| ent->client->ps.legsAnim == BOTH_PLAYER_PA_3)
			&& ent->client->ps.pm_type < PM_DEAD) // unless dead
		{
			//lock out player control
			if (!player_locked && !PlayerAffectedByStasis())
			{
				VectorClearM(ucmd->angles);
			}
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->buttons = 0;
			ucmd->upmove = 0;
			PM_AdjustAnglesToGripper(ent, ucmd);
		}
		if (ent->client->ps.leanofs)
		{
			//no shooting while leaning
			ucmd->buttons &= ~BUTTON_ATTACK;
			if (ent->client->ps.weapon != WP_DISRUPTOR)
			{
				//can still zoom around corners
				ucmd->buttons &= ~BUTTON_ALT_ATTACK;
			}
		}
	}
	else
	{
		if (ent->s.eFlags & EF_LOCKED_TO_WEAPON)
		{
			G_UpdateEmplacedWeaponData(ent);
		}
		if (player && player->client && player->client->ps.viewEntity == ent->s.number)
		{
			controlledByPlayer = qtrue;
			int sav_weapon = ucmd->weapon;
			memcpy(ucmd, &player->client->usercmd, sizeof(usercmd_t));
			ucmd->weapon = sav_weapon;
			ent->client->usercmd = *ucmd;
		}

		// Transfer over our driver's commands to us (the vehicle).
		if (ent->owner && ent->client && ent->client->NPC_class == CLASS_VEHICLE)
		{
			memcpy(ucmd, &ent->owner->client->usercmd, sizeof(usercmd_t));
			ucmd->buttons &= ~BUTTON_USE; //Vehicles NEVER try to use ANYTHING!!!
			//ucmd->weapon = ent->client->ps.weapon;	// but keep our weapon.
			ent->client->usercmd = *ucmd;
		}

		G_NPCMunroMatchPlayerWeapon(ent);
	}

	if (g_VaderBreath->integer == 1)
	{
		CG_BreathPuffsVader(ent);
	}
	if (cg_setVaderBreath.integer == 1)
	{
		CG_SetVaderBreath(ent);
	}
	if (cg_setVaderBreathdamaged.integer == 1)
	{
		CG_SetVaderBreathDamaged(ent);
	}

	// If we are a vehicle, update ourself.
	if (pVeh
		&& (pVeh->m_pVehicleInfo->Inhabited(pVeh)
			|| pVeh->m_iBoarding != 0
			|| pVeh->m_pVehicleInfo->type != VH_ANIMAL))
	{
		pVeh->m_pVehicleInfo->Update(pVeh, ucmd);
	}
	else if (ent->client)
	{
		//this is any client that is not a vehicle (OR: is a vehicle and it not being ridden, is not being boarded, or is a TaunTaun...!
		if (ent->client->NPC_class == CLASS_GONK ||
			ent->client->NPC_class == CLASS_MOUSE ||
			ent->client->NPC_class == CLASS_R2D2 ||
			ent->client->NPC_class == CLASS_R5D2)
		{
			//no jumping or strafing in these guys
			ucmd->upmove = ucmd->rightmove = 0;
		}
		else if (ent->client->NPC_class == CLASS_ATST || ent->client->NPC_class == CLASS_RANCOR)
		{
			//no jumping in atst
			if (ent->client->ps.pm_type != PM_NOCLIP)
			{
				ucmd->upmove = 0;
			}
			if (ent->client->ps.groundEntityNum != ENTITYNUM_NONE)
			{
				//ATST crushes anything underneath it
				gentity_t* under = &g_entities[ent->client->ps.groundEntityNum];
				if (under && under->health && under->takedamage)
				{
					vec3_t down = { 0, 0, -1 };
					//FIXME: we'll be doing traces down from each foot, so we'll have a real impact origin
					G_Damage(under, ent, ent, down, under->currentOrigin, 100, 0, MOD_CRUSH);
				}
				//so they know to run like hell when I get close
				//FIXME: project this in the direction I'm moving?
				if (ent->client->NPC_class != CLASS_RANCOR && !Q_irand(0, 10))
				{
					//not so often...
					AddSoundEvent(ent, ent->currentOrigin, ent->maxs[1] * 5, AEL_DANGER, qfalse, qtrue);
					AddSightEvent(ent, ent->currentOrigin, ent->maxs[1] * 5, AEL_DANGER, 100);
				}
			}
		}
		else if (ent->client->ps.groundEntityNum < ENTITYNUM_WORLD && !ent->client->ps.forceJumpCharge)
		{
			//standing on an entity and not currently force jumping
			gentity_t* groundEnt = &g_entities[ent->client->ps.groundEntityNum];
			if (groundEnt && groundEnt->client)
			{
				// If you landed on a speeder or animal vehicle...
				if (groundEnt->client && groundEnt->client->NPC_class == CLASS_VEHICLE)
				{
					if (ent->client->NPC_class != CLASS_VEHICLE)
					{
						//um... vehicles shouldn't ride other vehicles, mmkay?
						if (groundEnt->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL && PM_HasAnimation(
							ent, BOTH_VT_IDLE)
							|| groundEnt->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER && PM_HasAnimation(
								ent, BOTH_VS_IDLE))
						{
							groundEnt->m_pVehicle->m_pVehicleInfo->Board(groundEnt->m_pVehicle, ent);
						}
					}
				}
				else if (groundEnt->client && groundEnt->client->NPC_class == CLASS_SAND_CREATURE
					&& G_HasKnockdownAnims(ent))
				{
					//step on a sand creature = *you* fall down
					G_Knockdown(ent, groundEnt, vec3_origin, 300, qtrue);
				}
				else if (groundEnt->client && groundEnt->client->NPC_class == CLASS_RANCOR)
				{
					//step on a Rancor, it bucks & throws you off
					if (groundEnt->client->ps.legsAnim != BOTH_ATTACK3
						&& groundEnt->client->ps.legsAnim != BOTH_ATTACK4
						&& groundEnt->client->ps.legsAnim != BOTH_BUCK_RIDER)
					{
						//don't interrupt special anims
						vec3_t throwDir, right;
						AngleVectors(groundEnt->currentAngles, throwDir, right, nullptr);
						VectorScale(throwDir, -1, throwDir);
						VectorMA(throwDir, Q_flrand(-0.5f, 0.5f), right, throwDir);
						throwDir[2] = 0.2f;
						VectorNormalize(throwDir);
						if (!(ent->flags & FL_NO_KNOCKBACK))
						{
							G_Throw(ent, throwDir, Q_flrand(50, 200));
						}
						if (G_HasKnockdownAnims(ent))
						{
							G_Knockdown(ent, groundEnt, vec3_origin, 300, qtrue);
						}
						NPC_SetAnim(groundEnt, SETANIM_BOTH, BOTH_BUCK_RIDER,
							SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
				else if (groundEnt->client->ps.groundEntityNum != ENTITYNUM_NONE &&
					groundEnt->health > 0 && !PM_InRoll(&groundEnt->client->ps)
					&& !(groundEnt->client->ps.eFlags & EF_LOCKED_TO_WEAPON)
					&& !(groundEnt->client->ps.eFlags & EF_HELD_BY_RANCOR)
					&& !(groundEnt->client->ps.eFlags & EF_HELD_BY_WAMPA)
					&& !(groundEnt->client->ps.eFlags & EF_HELD_BY_SAND_CREATURE)
					&& !inSpinFlipAttack)

				{
					//landed on a live client who is on the ground, jump off them and knock them down
					qboolean forceKnockdown = qfalse;

					// If in a vehicle when land on someone, always knockdown.
					if (pVeh)
					{
						forceKnockdown = qtrue;
					}
					else if (ent->s.number
						&& ent->NPC
						&& ent->client->ps.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0) //ent->s.weapon == WP_SABER )
					{
						//force-jumper landed on someone
						//don't jump off, too many ledges, plus looks weird
						if (groundEnt->client->playerTeam != ent->client->playerTeam)
						{
							//don't knock down own guys
							forceKnockdown = static_cast<qboolean>(Q_irand(0, RANK_CAPTAIN + 4) < ent->NPC->rank);
						}
						//now what... push the groundEnt out of the way?
						if (!ent->client->ps.velocity[0]
							&& !ent->client->ps.velocity[1])
						{
							//not moving, shove us a little
							vec3_t slideFwd;
							AngleVectors(ent->client->ps.viewangles, slideFwd, nullptr, nullptr);
							slideFwd[2] = 0.0f;
							VectorNormalize(slideFwd);
							ent->client->ps.velocity[0] = slideFwd[0] * 10.0f;
							ent->client->ps.velocity[1] = slideFwd[1] * 10.0f;
						}
						//slide for a little
						ent->client->ps.pm_flags |= PMF_TIME_NOFRICTION;
						ent->client->ps.pm_time = 100;
					}
					else if (ent->health > 0)
					{
						if (!PM_InRoll(&ent->client->ps)
							&& !PM_FlippingAnim(ent->client->ps.legsAnim))
						{
							if (ent->s.number && ent->s.weapon == WP_SABER)
							{
								ent->client->ps.forceJumpCharge = 320; //FIXME: calc this intelligently?
							}
							else if (!ucmd->upmove)
							{
								//if not ducking (which should cause a roll), then jump
								ucmd->upmove = 127;
							}
							if (!ucmd->forwardmove && !ucmd->rightmove)
							{
								//  If not moving, don't want to jump straight up
								//FIXME: trace for clear di?
								if (!Q_irand(0, 3))
								{
									ucmd->forwardmove = 127;
								}
								else if (!Q_irand(0, 3))
								{
									ucmd->forwardmove = -127;
								}
								else if (!Q_irand(0, 1))
								{
									ucmd->rightmove = 127;
								}
								else
								{
									ucmd->rightmove = -127;
								}
							}
							if (!ent->s.number && ucmd->upmove < 0)
							{
								//player who should roll- force it
								int rollAnim = BOTH_ROLL_F;
								if (ucmd->forwardmove >= 0)
								{
									rollAnim = BOTH_ROLL_F;
								}
								else if (ucmd->forwardmove < 0)
								{
									rollAnim = BOTH_ROLL_B;
								}
								else if (ucmd->rightmove > 0)
								{
									rollAnim = BOTH_ROLL_R;
								}
								else if (ucmd->rightmove < 0)
								{
									rollAnim = BOTH_ROLL_L;
								}
								NPC_SetAnim(ent, SETANIM_BOTH, rollAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
								G_AddEvent(ent, EV_ROLL, 0);
								ent->client->ps.saberMove = LS_NONE;
							}
						}
					}
					else
					{
						//a corpse?  Shit
						//Hmm, corpses should probably *always* knockdown...
						forceKnockdown = qtrue;
						ent->clipmask &= ~CONTENTS_BODY;
					}
					//FIXME: need impact sound event
					GEntity_PainFunc(groundEnt, ent, ent, groundEnt->currentOrigin, 0, MOD_CRUSH);
					if (!forceKnockdown
						&& (groundEnt->client->NPC_class == CLASS_DESANN
							|| groundEnt->client->NPC_class == CLASS_SITHLORD
							|| groundEnt->client->NPC_class == CLASS_VADER)
						&& ent->client->NPC_class != CLASS_LUKE)
					{
						//can't knock down desann unless you're luke
						//FIXME: should he smack you away like Galak Mech?
					}
					else if (forceKnockdown //forced
						|| ent->client->NPC_class == CLASS_DESANN
						|| ent->client->NPC_class == CLASS_SITHLORD
						|| ent->client->NPC_class == CLASS_GROGU
						|| ent->client->NPC_class == CLASS_VADER //desann always knocks people down
						|| (groundEnt->s.number && (groundEnt->s.weapon != WP_SABER || !groundEnt->NPC || groundEnt->
							NPC->rank < Q_irand(RANK_CIVILIAN, RANK_CAPTAIN + 1))
							//an NPC who is either not a saber user or passed the rank-based probability test
							|| (!ent->s.number || G_ControlledByPlayer(groundEnt)) && !Q_irand(0, 3) && cg.
							renderingThirdPerson && !cg.zoomMode)
						//or a player in third person, 25% of the time
						&& groundEnt->client->playerTeam != ent->client->playerTeam //and not on the same team
						&& ent->client->ps.legsAnim != BOTH_JUMPATTACK6) //not in the sideways-spinning jump attack
					{
						int knockAnim = BOTH_KNOCKDOWN1;
						if (PM_CrouchAnim(groundEnt->client->ps.legsAnim))
						{
							//knockdown from crouch
							knockAnim = BOTH_KNOCKDOWN4;
						}
						else
						{
							vec3_t gEFwd, gEAngles = { 0, groundEnt->client->ps.viewangles[YAW], 0 };
							AngleVectors(gEAngles, gEFwd, nullptr, nullptr);
							if (DotProduct(ent->client->ps.velocity, gEFwd) > 50)
							{
								//pushing him forward
								knockAnim = BOTH_KNOCKDOWN3;
							}
						}
						NPC_SetAnim(groundEnt, SETANIM_BOTH, knockAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
					}
				}
			}
		}
	}

	client = ent->client;

	// mark the time, so the connection sprite can be removed
	client->lastCmdTime = level.time;
	client->pers.lastCommand = *ucmd;

	// sanity check the command time to prevent speedup cheating
	if (ucmd->serverTime > level.time + 200)
	{
		ucmd->serverTime = level.time + 200;
	}
	if (ucmd->serverTime < level.time - 1000)
	{
		ucmd->serverTime = level.time - 1000;
	}

	msec = ucmd->serverTime - client->ps.commandTime;
	if (msec < 1)
	{
		msec = 1;
	}
	if (msec > 200)
	{
		msec = 200;
	}

	if (client->noclip)
	{
		client->ps.pm_type = PM_NOCLIP;
	}
	else if (client->ps.stats[STAT_HEALTH] <= 0)
	{
		client->ps.pm_type = PM_DEAD;
	}
	else
	{
		client->ps.pm_type = PM_NORMAL;
	}

	//FIXME: if global gravity changes this should update everyone's personal gravity...
	if (!(ent->svFlags & SVF_CUSTOM_GRAVITY))
	{
		if (client->inSpaceIndex && client->inSpaceIndex != ENTITYNUM_NONE)
		{
			//in space, so no gravity...
			client->ps.gravity = 0.0f;
			if (ent->s.number < MAX_CLIENTS)
			{
				VectorScale(client->ps.velocity, 0.8f, client->ps.velocity);
			}
		}
		else
		{
			if (client->ps.eFlags2 & EF2_SHIP_DEATH)
			{
				//float there
				VectorClear(client->ps.velocity);
				client->ps.gravity = 1.0f;
			}
			else
			{
				client->ps.gravity = g_gravity->value;
			}
		}
	}

	// Activate the Blocking flags
	if (g_SerenityJediEngineMode->integer
		&& !ent->NPC &&
		(ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)) &&
		client->ps.weapon == WP_SABER &&
		client->ps.SaberActive() &&
		!PM_SaberInMassiveBounce(client->ps.torsoAnim) &&
		!PM_SaberInBashedAnim(client->ps.torsoAnim) &&
		!PM_Saberinstab(client->ps.saberMove))
	{
		if (Manual_RunningAndSaberblocking(ent))
		{
			if (!(client->ps.ManualBlockingFlags & 1 << MBF_BLOCKING))
			{
				client->ps.ManualBlockingFlags |= 1 << MBF_BLOCKING;
				client->ps.userInt3 |= 1 << FLAG_BLOCKING;
				client->ps.ManualBlockingTime = level.time; //Blocking time 1 on
			}
		}
		else if (manual_saberblocking(ent))
		{
			if (!(client->ps.ManualBlockingFlags & 1 << MBF_BLOCKING))
			{
				client->ps.ManualBlockingFlags |= 1 << MBF_BLOCKING;
				client->ps.userInt3 |= 1 << FLAG_BLOCKING;
				client->ps.ManualBlockingTime = level.time; //Blocking time 1 on
			}

			if (client->usercmd.buttons & BUTTON_WALKING && g_SerenityJediEngineMode->integer == 2)
			{
				if (!(client->ps.ManualBlockingFlags & 1 << MBF_BLOCKWALKING) && (ucmd->forwardmove || ucmd->rightmove))
				{
					client->ps.ManualBlockingFlags |= 1 << MBF_BLOCKWALKING;
				}
				client->usercmd.buttons &= ~BUTTON_WALKING;
			}

			if (client->usercmd.buttons & BUTTON_ATTACK)
			{
				if (!(client->ps.ManualBlockingFlags & 1 << MBF_PROJBLOCKING))
				{
					client->ps.ManualBlockingFlags |= 1 << MBF_PROJBLOCKING;
					client->ps.ManualMBlockingTime = level.time;
				}
				client->usercmd.buttons &= ~BUTTON_ATTACK;

				if (client->ps.ManualblockStartTime <= 0)//fresh start
				{// They just pressed block. Mark the time...
					client->ps.ManualblockStartTime = level.time;

					if (!(client->ps.ManualBlockingFlags & 1 << MBF_MBLOCKING))
					{
						client->ps.ManualBlockingFlags |= 1 << MBF_MBLOCKING;// activate the function
					}
					if (d_combatinfo->integer || g_DebugSaberCombat->integer)
					{
						Com_Printf(S_COLOR_ORANGE"MBlock was pressed, MBlocking is activated at time % i.\n", client->ps.ManualblockStartTime);
					}
				}
				else
				{
					if (g_spskill->integer == 0) //easy blocking
					{
						if (client->ps.ManualBlockingFlags & 1 << MBF_MBLOCKING && level.time - client->ps.ManualblockStartTime >= g_SaberPerfectBlockingTimerEasy->integer)
						{// Been holding block for too long....Turn off
							client->ps.ManualBlockingFlags &= ~(1 << MBF_MBLOCKING);
							if (d_combatinfo->integer)
							{
								Com_Printf(S_COLOR_ORANGE"MBlocking has Deactivated because you hit the timer limit\n");
							}
						}
					}
					else if (g_spskill->integer == 1) //medium blocking
					{
						if (client->ps.ManualBlockingFlags & 1 << MBF_MBLOCKING && level.time - client->ps.ManualblockStartTime >= g_SaberPerfectBlockingTimerNormal->integer)
						{// Been holding block for too long....Turn off
							client->ps.ManualBlockingFlags &= ~(1 << MBF_MBLOCKING);
							if (d_combatinfo->integer)
							{
								Com_Printf(S_COLOR_ORANGE"MBlocking has Deactivated because you hit the timer limit\n");
							}
						}
					}
					else
					{
						if (client->ps.ManualBlockingFlags & 1 << MBF_MBLOCKING && level.time - client->ps.ManualblockStartTime >= g_SaberPerfectBlockingTimerHard->integer)
						{// Been holding block for too long....Turn off
							client->ps.ManualBlockingFlags &= ~(1 << MBF_MBLOCKING);
							if (d_combatinfo->integer)
							{
								Com_Printf(S_COLOR_ORANGE"MBlocking has Deactivated because you hit the timer limit\n");
							}
						}
					}
				}

				if (client->ps.pm_flags & PMF_ACCURATE_MISSILE_BLOCK_HELD)
				{// started function
					if (client->ps.BoltblockStartTime <= 0) //fresh start
					{// They just pressed block. Mark the time...
						client->ps.BoltblockStartTime = level.time; //start the timer

						if (!(client->ps.ManualBlockingFlags & 1 << MBF_ACCURATEMISSILEBLOCKING))
						{
							client->ps.ManualBlockingFlags |= 1 << MBF_ACCURATEMISSILEBLOCKING; // activate the function
						}
					}
					else if (client->ps.ManualBlockingFlags & 1 << MBF_ACCURATEMISSILEBLOCKING && level.time - client->ps.BoltblockStartTime >= 3000)
					{// Been holding block for too long....let go.
						client->ps.ManualBlockingFlags &= ~(1 << MBF_ACCURATEMISSILEBLOCKING);
					}
				}
				else
				{// not doing it so reset
					client->ps.BoltblockStartTime = 0;
					client->ps.ManualBlockingFlags &= ~(1 << MBF_ACCURATEMISSILEBLOCKING);
				}
			}
			else
			{
				// No longer pressed, but we still need to make sure they are not spamming.
				client->ps.ManualblockStartTime = 0;
				client->ps.ManualBlockingFlags &= ~(1 << MBF_PROJBLOCKING);
				client->ps.ManualBlockingFlags &= ~(1 << MBF_MBLOCKING);
				client->ps.ManualBlockingFlags &= ~(1 << MBF_ACCURATEMISSILEBLOCKING);
				client->IsBlockingLightning = qfalse;
			}
		}
		else
		{
			client->ps.ManualBlockingFlags &= ~(1 << MBF_BLOCKING);
			client->ps.ManualBlockingFlags &= ~(1 << MBF_PROJBLOCKING);
			client->ps.ManualBlockingFlags &= ~(1 << MBF_MBLOCKING);
			client->ps.ManualBlockingFlags &= ~(1 << MBF_ACCURATEMISSILEBLOCKING);
			client->ps.userInt3 &= ~(1 << FLAG_BLOCKING);
			client->ps.ManualBlockingTime = 0; //Blocking time 1 on
			client->ps.ManualMBlockingTime = 0;
			client->IsBlockingLightning = qfalse;
			client->ps.ManualBlockingFlags &= ~(1 << MBF_BLOCKWALKING);
		}
	}

	if (client->ps.ManualBlockingFlags & 1 << MBF_BLOCKING && !client->ps.SaberActive())
	{
		client->ps.ManualBlockingFlags &= ~(1 << MBF_BLOCKING);
		client->ps.ManualBlockingFlags &= ~(1 << MBF_PROJBLOCKING);
		client->ps.ManualBlockingFlags &= ~(1 << MBF_MBLOCKING);
	}

	if (Manual_MeleeDodging(ent))
	{
		if (client->ps.DodgeStartTime <= 0 && level.time - client->ps.DodgeLastStartTime >= 1300)
		{
			// They just pressed block. Mark the time... 3000 wait between allowed presses.
			client->ps.DodgeStartTime = level.time; //Blocking 2
			client->ps.DodgeLastStartTime = level.time; //Blocking 3

			if (!(client->ps.ManualBlockingFlags & 1 << MBF_MELEEDODGE))
			{
				client->ps.ManualBlockingFlags |= 1 << MBF_MELEEDODGE;
			}
		}
		else
		{
			if (level.time - client->ps.DodgeStartTime >= 800) //Blocking 3
			{
				// When block was pressed, wait 200 before letting go of block.
				client->ps.DodgeStartTime = 0; //Blocking 2
				client->ps.ManualBlockingFlags &= ~(1 << MBF_MELEEDODGE);
			}
		}
	}
	else
	{
		client->ps.ManualBlockingFlags &= ~(1 << MBF_MELEEDODGE);
	}

	if (/*g_SerenityJediEngineMode->integer
		&&*/ ent->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(ent))
	{
		if (NPC_Can_Do_Blocking_stances_In_SJE_Mode(ent))
		{
			if (client->ps.ManualblockStartTime <= 0)//fresh start
			{// They just pressed block. Mark the time...
				if (!(client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKSTANCE))
				{
					client->ps.ManualBlockingFlags |= 1 << MBF_NPCBLOCKSTANCE;// activate the function
					client->ps.ManualblockStartTime = level.time;
				}
				if (d_combatinfo->integer || g_DebugSaberCombat->integer)
				{
					Com_Printf(S_COLOR_ORANGE"NPC MANUAL BLOCK was pressed, MBlocking is activated at time % i.\n", client->ps.ManualblockStartTime);
				}
			}
			else
			{
				if (client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKSTANCE && level.time - client->ps.ManualblockStartTime >= 5000) // NPC 5 SECONDS LIMIT
				{// Been holding block for too long....Turn off
					client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKSTANCE);
					if (d_combatinfo->integer)
					{
						Com_Printf(S_COLOR_ORANGE"NPC MANUAL BLOCK has Deactivated because you hit the timer limit\n");
					}
				}
				else if (!(client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKSTANCE) && level.time - client->ps.ManualblockStartTime >= 10000) // NPC 5 SECONDS LIMIT
				{// Been holding block for too long....Turn off
					client->ps.ManualBlockingFlags |= 1 << MBF_NPCBLOCKSTANCE;// activate the function
					client->ps.ManualblockStartTime = level.time;
					if (d_combatinfo->integer)
					{
						Com_Printf(S_COLOR_ORANGE"NPC MANUAL BLOCK has activated because you hit the timer limit\n");
					}
				}
			}
		}
		else
		{
			client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKSTANCE);
		}

		if (Manual_NPCSaberblocking(ent))
		{
			if (!(client->ps.ManualBlockingFlags & 1 << MBF_NPCBLOCKING))
			{
				client->ps.ManualBlockingFlags |= 1 << MBF_NPCBLOCKING;
				client->ps.userInt3 |= 1 << FLAG_NPCBLOCKING;
			}
		}
		else
		{
			client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCBLOCKING);
			client->ps.userInt3 &= ~(1 << FLAG_NPCBLOCKING);
		}
	}

	if (ent->NPC || ent->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(ent) &&
		client->ps.weapon == WP_SABER &&
		client->ps.SaberActive()
		&& !PM_SaberInMassiveBounce(client->ps.torsoAnim)
		&& !PM_SaberInBashedAnim(client->ps.torsoAnim)
		&& !PM_Saberinstab(client->ps.saberMove))
	{
		if (Manual_NPCKickAbsorbing(ent))
		{
			if (!(client->ps.ManualBlockingFlags & 1 << MBF_NPCKICKBLOCK))
			{
				client->ps.ManualBlockingFlags |= 1 << MBF_NPCKICKBLOCK;
			}
		}
		else
		{
			client->ps.ManualBlockingFlags &= ~(1 << MBF_NPCKICKBLOCK);
		}
	}

	if (g_SerenityJediEngineMode->integer <= 1
		&& (ent->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(ent))
		&& !PM_SaberInMassiveBounce(client->ps.torsoAnim)
		&& !PM_SaberInBashedAnim(client->ps.torsoAnim)
		&& !PM_Saberinstab(client->ps.saberMove))
	{
		if (Manual_JKAMode_NPCSaberblocking(ent))
		{
			if (!(client->ps.ManualBlockingFlags & 1 << MBF_JKAMODENPCBLOCKING))
			{
				client->ps.ManualBlockingFlags |= 1 << MBF_JKAMODENPCBLOCKING;
			}
		}
		else
		{
			client->ps.ManualBlockingFlags &= ~(1 << MBF_JKAMODENPCBLOCKING);
		}
	}

	// Activate the timing flags
	if (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))
	{
		if (IsRespecting(ent))
		{
			client->ps.respectingtime = level.time;

			if (client->ps.respectingstartTime <= 0 && level.time - client->ps.respectinglaststartTime >= 8000)
			{
				// They just pressed respect. Mark the time... 8000 wait between allowed Respecting.
				client->ps.respectingstartTime = level.time;
				client->ps.respectinglaststartTime = level.time;

				if (!(client->ps.communicatingflags & 1 << RESPECTING))
				{
					client->ps.communicatingflags |= 1 << RESPECTING;
				}
			}
			else
			{
				if (level.time - client->ps.respectingstartTime >= 3000)
				{
					// When respect was pressed, wait 3000 before letting go of respect.
					client->ps.respectingstartTime = 0;
					client->ps.communicatingflags &= ~(1 << RESPECTING);
				}
			}
		}
		else if (is_anim_requires_responce(ent))
		{
			client->ps.gesturingtime = level.time;

			if (client->ps.gesturingstartTime <= 0 && level.time - client->ps.gesturinglaststartTime >= 8000)
			{
				// They just pressed respect. Mark the time... 8000 wait between allowed Respecting.
				client->ps.gesturingstartTime = level.time;
				client->ps.gesturinglaststartTime = level.time;

				if (!(client->ps.communicatingflags & 1 << GESTURING))
				{
					client->ps.communicatingflags |= 1 << GESTURING;
				}
			}
			else
			{
				if (level.time - client->ps.gesturingstartTime >= 3000)
				{
					// When respect was pressed, wait 3000 before letting go of respect.
					client->ps.gesturingstartTime = 0;
					client->ps.communicatingflags &= ~(1 << GESTURING);
				}
			}
		}
		else if (IsSurrenderingAnimRequiresResponce(ent))
		{
			client->ps.surrendertimeplayer = level.time;

			if (client->ps.surrenderstartTime <= 0 && level.time - client->ps.surrenderlaststartTime >= 8000)
			{
				// They just pressed surrender. Mark the time... 8000 wait between allowed surrender.
				client->ps.surrenderstartTime = level.time;
				client->ps.surrenderlaststartTime = level.time;

				if (!(client->ps.communicatingflags & 1 << SURRENDERING))
				{
					client->ps.communicatingflags |= 1 << SURRENDERING;
				}
			}
			else
			{
				if (level.time - client->ps.surrenderstartTime >= 3000)
				{
					// When respect was pressed, wait 3000 before letting go of respect.
					client->ps.surrenderstartTime = 0;
					client->ps.communicatingflags &= ~(1 << SURRENDERING);
				}
			}
		}
		else if (IsPressingDashButton(ent)
			&& !PM_KickMove(ent->client->ps.saberMove)
			&& !PM_SaberInAttack(ent->client->ps.saberMove))
		{
			client->ps.dashtimeplayer = level.time;

			if (client->ps.dashstartTime <= 0 && level.time - client->ps.dashlaststartTime >= 5000)
			{
				// They just pressed dash. Mark the time... 8000 wait between allowed dash.
				client->ps.dashstartTime = level.time;
				client->ps.dashlaststartTime = level.time;

				if (!(client->ps.communicatingflags & 1 << DASHING))
				{
					client->ps.communicatingflags |= 1 << DASHING;
				}
			}
			else
			{
				if (level.time - client->ps.dashstartTime >= 1500)
				{
					// When respect was pressed, wait 3000 before letting go of respect.
					client->ps.dashstartTime = 0;
					client->ps.communicatingflags &= ~(1 << DASHING);
				}
			}
		}
		else if (IsPressingDestructButton(ent))
		{
			client->ps.destructtimeplayer = level.time;

			if (client->ps.destructstartTime <= 0 && level.time - client->ps.destructlaststartTime >= 4000)
			{
				// They just pressed dash. Mark the time... 8000 wait between allowed dash.
				client->ps.destructstartTime = level.time;
				client->ps.destructlaststartTime = level.time;

				if (!(client->ps.communicatingflags & 1 << DESTRUCTING))
				{
					client->ps.communicatingflags |= 1 << DESTRUCTING;
				}
			}
			else
			{
				if (level.time - client->ps.destructstartTime >= 3500)
				{
					// When respect was pressed, wait 3000 before letting go of respect.
					client->ps.destructstartTime = 0;
					client->ps.communicatingflags &= ~(1 << DESTRUCTING);
				}
			}
		}
		else if (client->Isprojecting)
		{
			client->ps.projecttimeplayer = level.time;

			if (client->ps.projectstartTime <= 0 && level.time - client->ps.projectlaststartTime >= 8000)
			{
				// They just pressed dash. Mark the time... 8000 wait between allowed dash.
				client->ps.projectstartTime = level.time;
				client->ps.projectlaststartTime = level.time;

				if (!(client->ps.communicatingflags & 1 << PROJECTING))
				{
					client->ps.communicatingflags |= 1 << PROJECTING;
				}
			}
			else
			{
				if (level.time - client->ps.projectstartTime >= 3500)
				{
					// When respect was pressed, wait 3000 before letting go of respect.
					client->ps.projectstartTime = 0;
					client->ps.communicatingflags &= ~(1 << PROJECTING);
				}
			}
		}
		else if (client->ps.weapon == WP_STUN_BATON && client->usercmd.buttons & BUTTON_ALT_ATTACK &&
			g_SerenityJediEngineMode->integer == 2)
		{
			client->ps.grappletimeplayer = level.time;

			if (client->ps.grapplestartTime <= 0 && level.time - client->ps.grapplelaststartTime >= 3000)
			{
				// They just pressed grapple. Mark the time... 3000 wait between allowed grapple.
				client->ps.grapplestartTime = level.time;
				client->ps.grapplelaststartTime = level.time;

				if (!(client->ps.communicatingflags & 1 << STUNNING))
				{
					client->ps.communicatingflags |= 1 << STUNNING;
				}
			}
			else
			{
				if (level.time - client->ps.grapplestartTime >= 3000)
				{
					// When grapple was pressed, wait 1500 before letting go of grapple.
					client->ps.grapplestartTime = 0;
					client->ps.communicatingflags &= ~(1 << STUNNING);
				}
			}
		}
		else if (IsPressingKickButton(ent))
		{
			if (client->ps.kickstartTime <= 0 && level.time - client->ps.kicklaststartTime >= 2000)
			{
				// They just pressed kick. Mark the time... 5000 wait between allowed kicks.
				client->ps.kickstartTime = level.time;
				client->ps.kicklaststartTime = level.time;

				if (!(client->ps.communicatingflags & 1 << KICKING))
				{
					client->ps.communicatingflags |= 1 << KICKING;
				}
			}
			else
			{
				if (level.time - client->ps.kicklaststartTime >= 500)
				{
					// When kick was pressed, wait 3000 before letting go.
					client->ps.kickstartTime = 0;
					client->ps.communicatingflags &= ~(1 << KICKING);
				}
			}
		}
		else
		{
			client->ps.respectingtime = 0;
			client->ps.gesturingtime = 0;
			client->ps.surrendertimeplayer = 0;
			client->ps.communicatingflags &= ~(1 << SURRENDERING);
			client->ps.communicatingflags &= ~(1 << RESPECTING);
			client->ps.communicatingflags &= ~(1 << GESTURING);
			client->ps.communicatingflags &= ~(1 << DASHING);
			client->ps.communicatingflags &= ~(1 << DESTRUCTING);
			client->ps.communicatingflags &= ~(1 << PROJECTING);
			client->ps.communicatingflags &= ~(1 << KICKING);
			if (client->ps.weapon != WP_STUN_BATON || (client->ps.communicatingflags |= client->ps.grapplestartTime >=
				3000))
			{
				client->ps.communicatingflags &= ~(1 << STUNNING);
			}
		}
	}

	if (ent->s.number == 0)
	{
		ClientAlterSpeed(ent, ucmd, controlledByPlayer, 0);
	}

	//FIXME: need to do this before check to avoid walls and cliffs (or just cliffs?)
	BG_AddPushVecToUcmd(ent, ucmd);

	G_CheckClampUcmd(ent, ucmd);

	WP_ForcePowersUpdate(ent, ucmd);

	//////////////BLOCKPOINTS REGEN ////////////////////////////////////////////////////////

	if (ent->s.number < MAX_CLIENTS && client->ps.blockPoints < BLOCK_POINTS_MAX)
	{
		WP_BlockPointsUpdate(ent);
	}

	//if we have the saber in hand, check for starting a block to reflect shots
	if (ent->s.number < MAX_CLIENTS //player
		|| ent->NPC && G_JediInNormalAI(ent)) //NPC jedi not in a special AI mode
	{
		if (g_SerenityJediEngineMode->integer && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent)))
		{
			wp_saber_start_missile_block_check_md(ent, ucmd);
		}
		else
		{
			wp_saber_start_missile_block_check(ent, ucmd);
		}
	}

	// Update the position of the saber, and check to see if we're throwing it
	if (client->ps.saberEntityNum != ENTITYNUM_NONE)
	{
		int updates = 1;
		if (ent->NPC)
		{
			updates = 3; //simulate player update rate?
		}
		for (int update = 0; update < updates; update++)
		{
			if (g_SerenityJediEngineMode->integer == 2 && (ent->s.number < MAX_CLIENTS || G_ControlledByPlayer(ent))) //AMD Mode player only
			{
				WP_SaberUpdateMD(ent, ucmd);
			}
			else
			{
				WP_SaberUpdateJKA(ent, ucmd);
			}
		}
	}

	//NEED to do this every frame, since these overrides do not go into the save/load data
	if (client && ent->s.m_iVehicleNum != 0 && !ent->s.number && !MatrixMode)
	{
		//FIXME: extern and read from g_vehicleInfo?
		Vehicle_t* pPlayerVeh = ent->owner->m_pVehicle;
		if (pPlayerVeh && pPlayerVeh->m_pVehicleInfo->cameraOverride)
		{
			// Vehicle Camera Overrides
			//--------------------------
			cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_RNG | CG_OVERRIDE_FOV | CG_OVERRIDE_3RD_PERSON_VOF |
				CG_OVERRIDE_3RD_PERSON_POF;
			cg.overrides.thirdPersonRange = pPlayerVeh->m_pVehicleInfo->cameraRange;
			cg.overrides.fov = pPlayerVeh->m_pVehicleInfo->cameraFOV;
			cg.overrides.thirdPersonVertOffset = pPlayerVeh->m_pVehicleInfo->cameraVertOffset;
			cg.overrides.thirdPersonPitchOffset = pPlayerVeh->m_pVehicleInfo->cameraPitchOffset;

			if (pPlayerVeh->m_pVehicleInfo->cameraAlpha)
			{
				cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_APH;
			}

			// If In A Speeder (NOT DURING TURBO)
			//------------------------------------
			if (level.time > pPlayerVeh->m_iTurboTime && pPlayerVeh->m_pVehicleInfo->type == VH_SPEEDER)
			{
				// If Using Strafe And Use Keys
				//------------------------------
				if (pPlayerVeh->m_ucmd.rightmove != 0 &&
					//					(pPlayerVeh->m_pParentEntity->client->ps.speed>=0) &&
					pPlayerVeh->m_ucmd.buttons & BUTTON_USE
					)
				{
					cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_ANG; // Turn On Angle Offset
					cg.overrides.thirdPersonRange *= -2; // Camera In Front Of Player
					cg.overrides.thirdPersonAngle = pPlayerVeh->m_ucmd.rightmove > 0 ? 20 : -20;
				}

				// Auto Pullback Of Camera To Show Enemy
				//---------------------------------------
				else
				{
					cg.overrides.active &= ~CG_OVERRIDE_3RD_PERSON_ANG; // Turn Off Angle Offset
					if (ent->enemy)
					{
						vec3_t actorDirection;
						vec3_t enemyDirection;

						AngleVectors(ent->currentAngles, actorDirection, nullptr, nullptr);
						VectorSubtract(ent->enemy->currentOrigin, ent->currentOrigin, enemyDirection);
						float enemyDistance = VectorNormalize(enemyDirection);

						if (enemyDistance > cg.overrides.thirdPersonRange && enemyDistance < 400 && DotProduct(
							actorDirection, enemyDirection) < -0.5f)
						{
							cg.overrides.thirdPersonRange = enemyDistance;
						}
					}
				}
			}
		}
	}
	else if (client->ps.eFlags & EF_IN_ATST)
	{
		cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_RNG | CG_OVERRIDE_3RD_PERSON_POF | CG_OVERRIDE_3RD_PERSON_VOF;
		cg.overrides.thirdPersonRange = 240;
		if (cg_thirdPersonAutoAlpha.integer)
		{
			if (ent->health > 0 && ent->client->ps.viewangles[PITCH] < 15 && ent->client->ps.viewangles[PITCH] > 0)
			{
				cg.overrides.active |= CG_OVERRIDE_3RD_PERSON_APH;
				if (cg.overrides.thirdPersonAlpha > 0.525f)
				{
					cg.overrides.thirdPersonAlpha -= 0.025f;
				}
				else if (cg.overrides.thirdPersonAlpha > 0.5f)
				{
					cg.overrides.thirdPersonAlpha = 0.5f;
				}
			}
			else if (cg.overrides.active & CG_OVERRIDE_3RD_PERSON_APH)
			{
				if (cg.overrides.thirdPersonAlpha > cg_thirdPersonAlpha.value)
				{
					cg.overrides.active &= ~CG_OVERRIDE_3RD_PERSON_APH;
				}
				else if (cg.overrides.thirdPersonAlpha < cg_thirdPersonAlpha.value - 0.1f)
				{
					cg.overrides.thirdPersonAlpha += 0.1f;
				}
				else if (cg.overrides.thirdPersonAlpha < cg_thirdPersonAlpha.value)
				{
					cg.overrides.thirdPersonAlpha = cg_thirdPersonAlpha.value;
					cg.overrides.active &= ~CG_OVERRIDE_3RD_PERSON_APH;
				}
			}
		}
		if (client->ps.viewangles[PITCH] > 0)
		{
			cg.overrides.thirdPersonPitchOffset = ent->client->ps.viewangles[PITCH] * -0.75;
			cg.overrides.thirdPersonVertOffset = 300 + ent->client->ps.viewangles[PITCH] * -10;
			if (cg.overrides.thirdPersonVertOffset < 0)
			{
				cg.overrides.thirdPersonVertOffset = 0;
			}
		}
		else if (ent->client->ps.viewangles[PITCH] < 0)
		{
			cg.overrides.thirdPersonPitchOffset = ent->client->ps.viewangles[PITCH] * -0.75;
			cg.overrides.thirdPersonVertOffset = 300 + ent->client->ps.viewangles[PITCH] * -5;
			if (cg.overrides.thirdPersonVertOffset > 300)
			{
				cg.overrides.thirdPersonVertOffset = 300;
			}
		}
		else
		{
			cg.overrides.thirdPersonPitchOffset = 0;
			cg.overrides.thirdPersonVertOffset = 200;
		}
	}

	//play/stop any looping sounds tied to controlled movement
	G_CheckMovingLoopingSounds(ent, ucmd);

	//remember your last angles
	VectorCopy(client->ps.viewangles, ent->lastAngles);
	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset(&pm, 0, sizeof pm);

	pm.gent = ent;
	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	//	pm.tracemask = MASK_PLAYERSOLID;	// used differently for navgen
	pm.tracemask = ent->clipmask;
	pm.trace = gi.trace;
	pm.pointcontents = gi.pointcontents;
	pm.debugLevel = g_debugMove->integer;
	pm.noFootsteps = qfalse; //( g_dmflags->integer & DF_NO_FOOTSTEPS ) > 0;

	if (ent->client && ent->NPC)
	{
		pm.cmd.weapon = ent->client->ps.weapon;
	}

	VectorCopy(client->ps.origin, oldOrigin);

	// perform a pmove
	Pmove(&pm);
	pm.gent = nullptr;

	ProcessGenericCmd(ent, pm.cmd.generic_cmd);

	// save results of pmove
	if (client->ps.eventSequence != oldEventSequence)
	{
		ent->eventTime = level.time;
		{
			int seq;

			seq = client->ps.eventSequence - 1 & MAX_PS_EVENTS - 1;
			ent->s.event = client->ps.events[seq] | (ent->client->ps.eventSequence & 3) << 8;
			ent->s.eventParm = ent->client->ps.eventParms[seq];
		}
	}
	PlayerStateToEntityState(&client->ps, &ent->s);

	VectorCopy(ent->currentOrigin, ent->lastOrigin);
#if 1
	// use the precise origin for linking
	VectorCopy(client->ps.origin, ent->currentOrigin);
#else
	//We don't use prediction anymore, so screw this
	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy(ent->s.pos.trBase, ent->currentOrigin);
#endif

	//Had to leave this in, some legacy code must still be using s.angles
	//Shouldn't interfere with interpolation of angles, should it?
	VectorCopy(ent->client->ps.viewangles, ent->currentAngles);
	//	if (pVeh)
	//	{
	//		gi.Printf("%d\n", ucmd->angles[2]);
	//	}

	VectorCopy(pm.mins, ent->mins);
	VectorCopy(pm.maxs, ent->maxs);

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	VectorCopyM(ucmd->angles, client->pers.cmd_angles);

	// execute client events
	ClientEvents(ent, oldEventSequence);

	if (pm.useEvent)
	{
		//TODO: Use
		TryUse(ent);
	}

	// link entity now, after any personal teleporters have been used
	gi.linkentity(ent);
	ent->client->hiddenDist = 0;
	if (!ent->client->noclip)
	{
		G_TouchTriggersLerped(ent);
	}

	// touch other objects
	ClientImpacts(ent, &pm);

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// check for respawning
	if (client->ps.stats[STAT_HEALTH] <= 0)
	{
		// wait for the attack button to be pressed
		if (ent->NPC == nullptr && level.time > client->respawnTime)
		{
			// pressing attack or use is the normal respawn method
			if (ucmd->buttons & BUTTON_ATTACK)
			{
				respawn(ent);
			}
		}
		if (ent
			&& !ent->s.number
			&& ent->enemy
			&& ent->enemy != ent
			&& ent->enemy->s.number < ENTITYNUM_WORLD
			&& ent->enemy->inuse
			&& !(cg.overrides.active & CG_OVERRIDE_3RD_PERSON_ANG))
		{
			//keep facing enemy
			vec3_t deadDir;
			float deadYaw;
			VectorSubtract(ent->enemy->currentOrigin, ent->currentOrigin, deadDir);
			deadYaw = AngleNormalize180(vectoyaw(deadDir));
			if (deadYaw > client->ps.stats[STAT_DEAD_YAW] + 1)
			{
				client->ps.stats[STAT_DEAD_YAW]++;
			}
			else if (deadYaw < client->ps.stats[STAT_DEAD_YAW] - 1)
			{
				client->ps.stats[STAT_DEAD_YAW]--;
			}
			else
			{
				client->ps.stats[STAT_DEAD_YAW] = deadYaw;
			}
		}
		return;
	}

	// perform once-a-second actions
	ClientTimerActions(ent, msec);

	if (ent->s.clientNum >= MAX_CLIENTS && !G_ControlledByPlayer(ent) && npc_is_projected(ent))
	{
		//decrement it
		ClientTimerProjectionLifeDrain(ent, msec);
	}

	ClientEndPowerUps(ent);
	//try some idle anims on ent if getting no input and not moving for some time
	if (!npc_is_projected(ent))
	{
		if (client->ps.weapon == WP_SABER)
		{
			G_CheckClientIdleSabers(ent, ucmd);
		}
		else
		{
			G_CheckClientIdleGuns(ent, ucmd);
		}
	}

	if (client->ps.weapon == WP_STUN_BATON && HoldingStun && g_SerenityJediEngineMode->integer == 2)
	{
		if (client->ps.pm_type != PM_DEAD && !ent->client->stunhasbeenfired)
		{
			if (client && client->stunDebounceTime > level.time)
			{
				if (client->stun)
				{
					Weapon_StunFree(client->stun);
				}
			}
			else
			{
				Weapon_AltStun_Fire(ent);
				ent->client->stunhasbeenfired = qtrue;
				G_SoundOnEnt(ent, CHAN_ITEM, "sound/weapons/noghri/fire.mp3");
				ent->client->stunDebounceTime = level.time + 0;
			}
		}
		else if (client->stun && (client->stunHeld == qfalse ||
			!HoldingStun ||
			ucmd->buttons & BUTTON_USE ||
			ucmd->buttons & BUTTON_BLOCK ||
			ent->client->ps.pm_type == PM_DEAD))
		{
			Weapon_StunFree(client->stun);
		}
	}
	else
	{
		if (client->stun && (client->stunHeld == qfalse ||
			!HoldingStun ||
			ucmd->buttons & BUTTON_USE ||
			ucmd->buttons & BUTTON_BLOCK ||
			ent->client->ps.pm_type == PM_DEAD))
		{
			Weapon_StunFree(client->stun);
		}

		if (!(ent->client->ps.eFlags & EF_ALT_FIRING))
		{
			client->stunHeld = qfalse;
		}
	}

	if (ent->client->ps.weapon == WP_MELEE && ent->client->ps.inventory[INV_GRAPPLEHOOK])
	{
		if (ent->client->usercmd.buttons & BUTTON_BLOCK && !(ent->client->usercmd.buttons & BUTTON_WALKING) && !
			PM_RunningAnim(ent->client->ps.legsAnim) && !PM_WalkingAnim(ent->client->ps.legsAnim)
			&& ent->client->ps.pm_type != PM_DEAD
			&& !ent->client->hookhasbeenfired)
		{
			if (ent->client && ent->client->hookDebounceTime > level.time)
			{
				if (ent->client->hook)
				{
					Weapon_HookFree(ent->client->hook);
				}
			}
			else
			{
				if (ent->client->moveType == MT_FLYSWIM || ent->s.groundEntityNum == ENTITYNUM_NONE || JET_Flying(ent)
					|| PM_CrouchAnim(ent->client->ps.legsAnim))
				{
					Boba_FireWristMissile(ent, BOBA_MISSILE_VIBROBLADE);
				}
				else
				{
					Weapon_GrapplingHook_Fire(ent);
					ent->client->hookhasbeenfired = qtrue;

					if (ent->client->usercmd.buttons & BUTTON_BLOCK && ent->s.groundEntityNum != ENTITYNUM_NONE)
					{
						//holding attack
						NPC_SetAnim(ent, SETANIM_BOTH, BOTH_GRAPPLE_FIRE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
					}

					G_SoundOnEnt(ent, CHAN_ITEM, "sound/weapons/grapple/hookfire.wav");
					ent->client->hookDebounceTime = level.time + 0;
				}
			}
		}
		else if (ent->client->hook && (ent->client->fireHeld == qfalse ||
			!(ent->client->usercmd.buttons & BUTTON_BLOCK) ||
			ent->client->usercmd.buttons & BUTTON_WALKING ||
			PM_RunningAnim(ent->client->ps.legsAnim) ||
			PM_WalkingAnim(ent->client->ps.legsAnim) ||
			ent->client->ps.pm_type == PM_DEAD))
		{
			Weapon_HookFree(ent->client->hook);
		}
	}
	else
	{
		if (ent->client->hook && (ent->client->fireHeld == qfalse ||
			!(ent->client->usercmd.buttons & BUTTON_BLOCK) ||
			ent->client->usercmd.buttons & BUTTON_WALKING ||
			PM_RunningAnim(ent->client->ps.legsAnim) ||
			PM_WalkingAnim(ent->client->ps.legsAnim) ||
			ent->client->ps.pm_type == PM_DEAD))
		{
			Weapon_HookFree(ent->client->hook);
		}

		if (!(ent->client->ps.eFlags & EF_FIRING))
		{
			ent->client->fireHeld = qfalse;
		}
	}
}

void MandoThink_real(gentity_t* ent, const usercmd_t* ucmd)
{
	constexpr int anim = BOTH_GRAPPLE_FIRE;

	if (!ent->client->ps.inventory[INV_GRAPPLEHOOK])
	{
		return;
	}

	if (ent->client->ps.weapon == WP_MELEE)
	{
		if (ucmd->buttons & BUTTON_BLOCK && !(ucmd->buttons & BUTTON_WALKING) && !
			PM_RunningAnim(ent->client->ps.legsAnim) && !PM_WalkingAnim(ent->client->ps.legsAnim)
			&& ent->client->ps.pm_type != PM_DEAD
			&& !ent->client->hookhasbeenfired)
		{
			if (ent->client && ent->client->hookDebounceTime > level.time)
			{
				if (ent->client->hook)
				{
					Weapon_HookFree(ent->client->hook);
				}
			}
			else
			{
				if (ent->client->moveType == MT_FLYSWIM || ent->s.groundEntityNum == ENTITYNUM_NONE || JET_Flying(ent)
					|| PM_CrouchAnim(ent->client->ps.legsAnim))
				{
					Boba_FireWristMissile(ent, BOBA_MISSILE_VIBROBLADE);
				}
				else
				{
					Weapon_GrapplingHook_Fire(ent);
					ent->client->hookhasbeenfired = qtrue;

					if (ucmd->buttons & BUTTON_BLOCK && ent->s.groundEntityNum != ENTITYNUM_NONE)
					{
						//holding attack
						NPC_SetAnim(ent, SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0);
					}

					G_SoundOnEnt(ent, CHAN_ITEM, "sound/weapons/grapple/hookfire.wav");
					ent->client->hookDebounceTime = level.time + 0;
				}
			}
		}
		else if (ent->client->hook && (ent->client->fireHeld == qfalse ||
			!(ucmd->buttons & BUTTON_BLOCK) ||
			ucmd->buttons & BUTTON_WALKING ||
			PM_RunningAnim(ent->client->ps.legsAnim) ||
			PM_WalkingAnim(ent->client->ps.legsAnim) ||
			ent->client->ps.pm_type == PM_DEAD))
		{
			Weapon_HookFree(ent->client->hook);
		}
	}
	else
	{
		if (ent->client->hook && (ent->client->fireHeld == qfalse ||
			!(ucmd->buttons & BUTTON_BLOCK) ||
			ucmd->buttons & BUTTON_WALKING ||
			PM_RunningAnim(ent->client->ps.legsAnim) ||
			PM_WalkingAnim(ent->client->ps.legsAnim) ||
			ent->client->ps.pm_type == PM_DEAD))
		{
			Weapon_HookFree(ent->client->hook);
		}

		if (!(ent->client->ps.eFlags & EF_FIRING))
		{
			ent->client->fireHeld = qfalse;
		}
	}
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
extern void PM_CheckForceUseButton(gentity_t* ent, usercmd_t* ucmd);
extern qboolean PM_GentCantJump(const gentity_t* gent);

void ClientThink(int clientNum, usercmd_t* ucmd)
{
	qboolean restore_ucmd = qfalse;
	usercmd_t sav_ucmd = { 0 };

	gentity_t* ent = g_entities + clientNum;

	if (ent->s.number < MAX_CLIENTS)
	{
		if (ent->client->ps.viewEntity > 0 && ent->client->ps.viewEntity < ENTITYNUM_WORLD)
		{
			//you're controlling another NPC
			const gentity_t* controlled = &g_entities[ent->client->ps.viewEntity];
			qboolean freed = qfalse;
			if (controlled->NPC
				&& controlled->NPC->controlledTime
				&& ent->client->ps.forcePowerLevel[FP_TELEPATHY] > FORCE_LEVEL_3)
			{
				//An NPC I'm controlling with mind trick
				if (controlled->NPC->controlledTime < level.time)
				{
					//time's up!
					G_ClearViewEntity(ent);
					freed = qtrue;
				}
				else if (ucmd->upmove > 0 || ucmd->buttons & BUTTON_BLOCK)
				{
					//jumping gets you out of it FIXME: check some other button instead... like ESCAPE... so you could even have total control over an NPC?
					G_ClearViewEntity(ent);
					ucmd->upmove = 0; //ucmd->buttons = 0;
					//stop player from doing anything for a half second after
					ent->aimDebounceTime = level.time + 500;
					freed = qtrue;
				}
			}
			else if (controlled->client //an NPC
				&& PM_GentCantJump(controlled) //that cannot jump
				&& controlled->client->moveType != MT_FLYSWIM) //and does not use upmove to fly
			{
				//these types use jump to get out
				if (ucmd->upmove > 0 || ucmd->buttons & BUTTON_BLOCK)
				{
					//jumping gets you out of it FIXME: check some other button instead... like ESCAPE... so you could even have total control over an NPC?
					G_ClearViewEntity(ent);
					ucmd->upmove = 0; //ucmd->buttons = 0;
					//stop player from doing anything for a half second after
					ent->aimDebounceTime = level.time + 500;
					freed = qtrue;
				}
			}
			else
			{
				//others use the blocking key
				if (ucmd->buttons & BUTTON_BLOCK)
				{
					//jumping gets you out of it FIXME: check some other button instead... like ESCAPE... so you could even have total control over an NPC?
					G_ClearViewEntity(ent);
					ucmd->buttons = 0;
					freed = qtrue;
				}
			}

			if (!freed)
			{
				//still controlling, save off my ucmd and clear it for my actual run through pmove
				restore_ucmd = qtrue;
				memcpy(&sav_ucmd, ucmd, sizeof(usercmd_t));
				memset(ucmd, 0, sizeof(usercmd_t));
				//to keep pointing in same dir, need to set ucmd->angles
				ucmd->angles[PITCH] = ANGLE2SHORT(ent->client->ps.viewangles[PITCH]) - ent->client->ps.delta_angles[
					PITCH];
				ucmd->angles[YAW] = ANGLE2SHORT(ent->client->ps.viewangles[YAW]) - ent->client->ps.delta_angles[YAW];
				ucmd->angles[ROLL] = 0;
				if (controlled->NPC)
				{
					VectorClear(controlled->client->ps.moveDir);
					controlled->client->ps.speed = sav_ucmd.buttons & BUTTON_WALKING
						? controlled->NPC->stats.walkSpeed
						: controlled->NPC->stats.runSpeed;
				}
			}
			else
			{
				ucmd->angles[PITCH] = ANGLE2SHORT(ent->client->ps.viewangles[PITCH]) - ent->client->ps.delta_angles[
					PITCH];
				ucmd->angles[YAW] = ANGLE2SHORT(ent->client->ps.viewangles[YAW]) - ent->client->ps.delta_angles[YAW];
				ucmd->angles[ROLL] = 0;
			}
		}
		else if (ent->client->NPC_class == CLASS_ATST)
		{
			if (ucmd->upmove > 0)
			{
				//get out of ATST
				GEntity_UseFunc(ent->activator, ent, ent);
				ucmd->upmove = 0; //ucmd->buttons = 0;
			}
		}

		PM_CheckForceUseButton(ent, ucmd);
	}

	Vehicle_t* p_vehicle;

	// Rider logic.
	// NOTE: Maybe this should be extracted into a RiderUpdate() within the vehicle.
	if ((p_vehicle = G_IsRidingVehicle(ent)) != nullptr)
	{
		if (p_vehicle->m_pVehicleInfo->UpdateRider(p_vehicle, ent, ucmd))
		{
			restore_ucmd = qtrue;
			memcpy(&sav_ucmd, ucmd, sizeof(usercmd_t));
			memset(ucmd, 0, sizeof(usercmd_t));
			ucmd->angles[PITCH] = sav_ucmd.angles[PITCH];
			ucmd->angles[YAW] = sav_ucmd.angles[YAW];
			ucmd->angles[ROLL] = sav_ucmd.angles[ROLL];
			{
				//trying to change weapons to a valid weapon for this vehicle, to preserve this weapon change command
				ucmd->weapon = sav_ucmd.weapon;
			}
			{
				//keep our current weapon
				{
					//not changing weapons and we are using one of our weapons, not using vehicle weapon
					//so we actually want to do our fire weapon on us, not the vehicle
					ucmd->buttons = sav_ucmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK);
				}
			}
		}
	}

	ent->client->usercmd = *ucmd;

	//	if ( !g_syncronousClients->integer )
	{
		ClientThink_real(ent, ucmd);
	}

	// If a vehicle, make sure to attach our driver and passengers here (after we pmove, which is done in Think_Real))
	if (ent->client && ent->client->NPC_class == CLASS_VEHICLE)
	{
		p_vehicle = ent->m_pVehicle;
		p_vehicle->m_pVehicleInfo->AttachRiders(p_vehicle);
	}

	// ClientThink_real can end up freeing this ent, need to check
	if (restore_ucmd && ent->client)
	{
		//restore ucmd for later so NPC you're controlling can refer to them
		memcpy(&ent->client->usercmd, &sav_ucmd, sizeof(usercmd_t));
	}

	if (ent->s.number)
	{
		//NPCs drown, burn from lava, etc, also
		P_WorldEffects(ent);
	}
}

void ClientEndPowerUps(const gentity_t* ent)
{
	if (ent == nullptr || ent->client == nullptr)
	{
		return;
	}
	// turn off any expired powerups
	for (int i = 0; i < MAX_POWERUPS; i++)
	{
		if (ent->client->ps.powerups[i] < level.time)
		{
			ent->client->ps.powerups[i] = 0;
		}
	}
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame(gentity_t* ent)
{
	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//

	// burn from lava, etc
	P_WorldEffects(ent);

	// apply all the damage taken this frame
	P_DamageFeedback(ent);

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	/*
	if ( level.time - ent->client->lastCmdTime > 1000 ) {
		ent->s.eFlags |= EF_CONNECTION;
	} else {
		ent->s.eFlags &= ~EF_CONNECTION;
	}
	*/

	ent->client->ps.stats[STAT_HEALTH] = ent->health; // FIXME: get rid of ent->health...

	//	G_SetClientSound (ent);
}