/***
 *
 *	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 *   Use, distribution, and modification of this source code and/or resulting
 *   object code is restricted to non-commercial enhancements to products from
 *   Valve LLC.  All other use, distribution, or modification is prohibited
 *   without written permission from Valve LLC.
 *
 ****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(weapon_carbine, CCarbine);


//=========================================================
//=========================================================
void CCarbine::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmAR.mdl");
	m_iId = WEAPON_CARBINE;

	m_iDefaultAmmo = CARBINE_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.
}


void CCarbine::Precache()
{
	PRECACHE_MODEL("models/v_carbine.mdl");
	PRECACHE_MODEL("models/w_9mmAR.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shellTE_MODEL

	PRECACHE_MODEL("models/grenade.mdl"); // grenade

	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND("weapons/hks1.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks2.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks3.wav"); // H to the K

	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usCarbine = PRECACHE_EVENT(1, "events/carbine.sc");
}

bool CCarbine::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->iMaxClip = CARBINE_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_CARBINE;
	p->iWeight = CARBINE_WEIGHT;

	return true;
}

bool CCarbine::Deploy()
{
	return DefaultDeploy("models/v_carbine.mdl", "models/p_carbine.mdl", CARBINE_DEPLOY, "carbine");
}


void CCarbine::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;

#ifdef CLIENT_DLL
	if (bIsMultiplayer())
#else
	if (g_pGameRules->IsMultiplayer())
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usCarbine, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.1);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}



void CCarbine::SecondaryAttack()
{
}

void CCarbine::Reload()
{
	if (m_pPlayer->ammo_9mm <= 0)
		return;

	DefaultReload(CARBINE_MAX_CLIP, CARBINE_RELOAD, 2.0);
}


void CCarbine::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		iAnim = CARBINE_LONGIDLE;
		break;

	default:
	case 1:
		iAnim = CARBINE_IDLE1;
		break;
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}



class CCarbineAmmo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_9mmARclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_9mmARclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo(AMMO_MP5CLIP_GIVE, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_carbine, CCarbineAmmo);

