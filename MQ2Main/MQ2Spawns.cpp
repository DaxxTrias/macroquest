/*****************************************************************************
    MQ2Main.dll: MacroQuest2's extension DLL for EverQuest
    Copyright (C) 2002-2003 Plazmic, 2003-2004 Lax

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2, as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
******************************************************************************/
#if !defined(CINTERFACE)
#error /DCINTERFACE
#endif

#define DBG_SPEW

//#define DEBUG_TRY 1
#include "MQ2Main.h"



PMQGROUNDPENDING pPendingGrounds=0;
CRITICAL_SECTION csPendingGrounds;
BOOL ProcessPending=false;

void AddGroundItem()
{
	PGROUNDITEM pGroundItem=(PGROUNDITEM)pItemList;
	EnterCriticalSection(&csPendingGrounds);
	PMQGROUNDPENDING pPending=new MQGROUNDPENDING;
	pPending->pGroundItem=pGroundItem;
	pPending->pNext=pPendingGrounds;
	pPending->pLast=0;
	if (pPendingGrounds)
		pPendingGrounds->pLast=pPending;
	pPendingGrounds=pPending;
	LeaveCriticalSection(&csPendingGrounds);
}

void RemoveGroundItem(PGROUNDITEM pGroundItem)
{
	if (pPendingGrounds)
	{
		EnterCriticalSection(&csPendingGrounds);
		PMQGROUNDPENDING pPending=pPendingGrounds;
		while(pPending)
		{
			if (pGroundItem==pPending->pGroundItem)
			{
				if (pPending->pNext)
					pPending->pNext->pLast=pPending->pLast;
				if (pPending->pLast)
					pPending->pLast->pNext=pPending->pNext;
				else
					pPendingGrounds=pPending->pNext;
				delete pPending;
				break;
			}
			pPending=pPending->pNext;
		}
		LeaveCriticalSection(&csPendingGrounds);
	}
	PluginsRemoveGroundItem((PGROUNDITEM)pGroundItem);
}

class EQItemListHook
{
public:
	VOID EQItemList_Trampoline();
	VOID EQItemList_Detour()
	{
		VOID (EQItemListHook::*tmp)(void) = EQItemList_Trampoline; 
		__asm {
			call [tmp];
			push eax;
			push ebx;
			push ecx;
			push edx;
			push esi;
			push edi;
			call AddGroundItem;
			pop edi;
			pop esi;
			pop edx;
			pop ecx;
			pop ebx;
			pop eax;
		};
	}

	void dEQItemList_Trampoline();
	void dEQItemList_Detour()
	{
		void (EQItemListHook::*tmp)(void) = dEQItemList_Trampoline;
		__asm {
			push ecx;
			push ecx;
			call RemoveGroundItem;
			pop ecx;
			pop ecx;
			call [tmp];
		};
	}
};

DETOUR_TRAMPOLINE_EMPTY(VOID EQItemListHook::EQItemList_Trampoline(VOID)); 
DETOUR_TRAMPOLINE_EMPTY(VOID EQItemListHook::dEQItemList_Trampoline(VOID)); 

class EQPlayerHook
{
public:

	
	void EQPlayer_ExtraDetour(PSPAWNINFO pSpawn)
	{// note: we need to keep the original registers.
		__asm {push eax};
		__asm {push ebx};
		__asm {push ecx};
		__asm {push edx};
		__asm {push esi};
		__asm {push edi};
		SetNameSpriteState(1);
		PluginsAddSpawn(pSpawn);
		__asm {pop edi};
		__asm {pop esi};
		__asm {pop edx};
		__asm {pop ecx};
		__asm {pop ebx};
		__asm {pop eax};
	}
	/**/

	void EQPlayer_Trampoline(DWORD,DWORD,DWORD,DWORD,DWORD);
	void EQPlayer_Detour(DWORD a,DWORD b,DWORD c,DWORD d,DWORD e)
	{
		PSPAWNINFO pSpawn;
		__asm {mov [pSpawn], ecx};

		EQPlayer_Trampoline(a,b,c,d,e);
		EQPlayer_ExtraDetour(pSpawn);
		/**/
	}

	void dEQPlayer_Trampoline(void);
	void dEQPlayer_Detour(void)
	{
		void (EQPlayerHook::*tmp)(void) = dEQPlayer_Trampoline; 
		__asm {
			push ecx;
			push ecx;
			call PluginsRemoveSpawn;
			pop ecx;
			pop ecx;
			call [tmp];
		};
	}

	int SetNameSpriteState_Trampoline(bool Show);
	int SetNameSpriteState_Detour(bool Show)
	{
		if (gGameState!=GAMESTATE_INGAME || !Show)
			return SetNameSpriteState_Trampoline(Show);
		return 1;
	}

	int SetNameSpriteState(bool Show)
	{
		PSPAWNINFO pSpawn;
		__asm{mov [pSpawn], ecx};
#define SetCaption(CaptionString) \
		{\
			if (CaptionString[0])\
			{\
				strcpy(NewCaption,CaptionString);\
				pNamingSpawn=pSpawn;\
				ParseMacroParameter(GetCharInfo()->pSpawn,NewCaption);\
				pNamingSpawn=0;\
				((EQPlayer*)pSpawn)->ChangeBoneStringSprite(0,NewCaption);\
				return 1;\
			}\
		}
		CHAR NewCaption[MAX_STRING]={0};
		switch(GetSpawnType((PSPAWNINFO)pSpawn))
		{
		case NPC:
			SetCaption(gszSpawnNPCName);
			break;
		case PC:
			if (!gPCNames) 
				return 0;
			SetCaption(gszSpawnPlayerName[gShowNames]);
			break;
		case CORPSE:
			SetCaption(gszSpawnCorpseName);
			break;
		case MOUNT://mount names make it crash!
			return 0;
		case PET:
			SetCaption(gszSpawnPetName);
			break;
		}
		return SetNameSpriteState_Trampoline(Show);
#undef SetCaption
	}

};

VOID UpdateSpawnCaptions()
{
	DWORD N;
	DWORD Count=0;
	for (N = 0 ; N < 120 ; N++)
	{
		if (EQPlayerHook* pSpawn=(EQPlayerHook*)EQP_DistArray[N].VarPtr.Ptr)
		if (EQP_DistArray[N].Value.Float<=80.0f)
		{
			if (pSpawn->SetNameSpriteState(true))
			{
				((EQPlayer*)pSpawn)->SetNameSpriteTint();
				Count++;
				if (Count>=gMaxSpawnCaptions)
				{
					return;
				}
			}
		}
		else
		{
			return;
		}
	}
}

DETOUR_TRAMPOLINE_EMPTY(int EQPlayerHook::SetNameSpriteState_Trampoline(bool Show));
DETOUR_TRAMPOLINE_EMPTY(VOID EQPlayerHook::dEQPlayer_Trampoline(VOID)); 
DETOUR_TRAMPOLINE_EMPTY(VOID EQPlayerHook::EQPlayer_Trampoline(DWORD,DWORD,DWORD,DWORD,DWORD)); 

VOID InitializeMQ2Spawns()
{
	DebugSpew("Initializing Spawn-related Hooks");
	bmUpdateSpawnSort=AddMQ2Benchmark("UpdateSpawnSort");
	bmUpdateSpawnCaptions=AddMQ2Benchmark("UpdateSpawnCaptions");

	EzDetour(EQPlayer__EQPlayer,EQPlayerHook::EQPlayer_Detour,EQPlayerHook::EQPlayer_Trampoline);
	EzDetour(EQPlayer__dEQPlayer,EQPlayerHook::dEQPlayer_Detour,EQPlayerHook::dEQPlayer_Trampoline);
	EzDetour(EQItemList__EQItemList,EQItemListHook::EQItemList_Detour,EQItemListHook::EQItemList_Trampoline);
	EzDetour(EQItemList__dEQItemList,EQItemListHook::dEQItemList_Detour,EQItemListHook::dEQItemList_Trampoline);
	EzDetour(EQPlayer__SetNameSpriteState,EQPlayerHook::SetNameSpriteState_Detour,EQPlayerHook::SetNameSpriteState_Trampoline);

	InitializeCriticalSection(&csPendingGrounds);
	ProcessPending=true;
	ZeroMemory(&EQP_DistArray,sizeof(EQP_DistArray));
	gSpawnCount=0;
}

VOID ShutdownMQ2Spawns()
{
	DebugSpew("Shutting Down Spawn-related Hooks");
	RemoveDetour(EQPlayer__EQPlayer);
	RemoveDetour(EQPlayer__dEQPlayer);
	RemoveDetour(EQItemList__EQItemList);
	RemoveDetour(EQItemList__dEQItemList);
	RemoveDetour(EQPlayer__SetNameSpriteState);
	ProcessPending=false;
	EnterCriticalSection(&csPendingGrounds);
	DeleteCriticalSection(&csPendingGrounds);
	while(pPendingGrounds)
	{
		PMQGROUNDPENDING pNext=pPendingGrounds->pNext;
		delete pPendingGrounds;
		pPendingGrounds=pNext;
	}
	ZeroMemory(EQP_DistArray,sizeof(EQP_DistArray));
	gSpawnCount=0;
	RemoveMQ2Benchmark(bmUpdateSpawnSort);
	RemoveMQ2Benchmark(bmUpdateSpawnCaptions);
}

VOID ProcessPendingGroundItems()
{
	if (ProcessPending && pPendingGrounds)
	{
		EnterCriticalSection(&csPendingGrounds);
		while(pPendingGrounds)
		{
			PMQGROUNDPENDING pNext=pPendingGrounds->pNext;
			PluginsAddGroundItem(pPendingGrounds->pGroundItem);
			delete pPendingGrounds;
			pPendingGrounds=pNext;
		}
		LeaveCriticalSection(&csPendingGrounds);
	}
}

VOID UpdateMQ2SpawnSort()
{
	EnterMQ2Benchmark(bmUpdateSpawnSort);
	ZeroMemory(EQP_DistArray,sizeof(EQP_DistArray));
	gSpawnCount=0;
	PSPAWNINFO pSpawn=(PSPAWNINFO)pSpawnList;
	while(pSpawn)
	{
		EQP_DistArray[gSpawnCount].VarPtr.Ptr=pSpawn;
		EQP_DistArray[gSpawnCount].Value.Float=GetDistance(pSpawn->X,pSpawn->Y);
		gSpawnCount++;
		pSpawn=pSpawn->pNext;
	}
	// quicksort!
	qsort(&EQP_DistArray[0],gSpawnCount,sizeof(MQRANK),MQRankFloatCompare);
	ExitMQ2Benchmark(bmUpdateSpawnSort);
	static unsigned long nCaptions=100;
	++nCaptions;
	if (gGameState==GAMESTATE_INGAME && nCaptions>7)
	{
		nCaptions=0;
		Benchmark(bmUpdateSpawnCaptions,UpdateSpawnCaptions());
	}
}


