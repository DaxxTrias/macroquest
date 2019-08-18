/*
 * MacroQuest2: The extension platform for EverQuest
 * Copyright (C) 2002-2019 MacroQuest Authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <sstream>

#include "../MQ2Plugin.h"
#include "MQ2Map.h"
PreSetup("MQ2Map");

long repeatLast = 0;
long repeatInterval = 10;
clock_t highPulseRepeatLast = clock();
long highPulseRepeatIntervalMillis = 50;
unsigned long bmMapRefresh = 0;

int activeLayer = 2;

WORD currentZoneId = 0;

BOOL repeatMaphide = FALSE;
BOOL repeatMapshow = FALSE;

DWORD HighlightColor = 0xFF700070;
DWORD HighlightSIDELEN = 100;
BOOL HighlightPulse = FALSE;
BOOL HighlightPulseIncreasing = TRUE;
int HighlightPulseIndex = 0;
DWORD HighlightPulseDiff = HighlightSIDELEN / 10;

CHAR MapSpecialClickString[16][MAX_STRING] =
{
	"",// unused, will always target
	"",//SHIFT 
	"/maphide id %i",//CTRL
	"",//CTRL|SHIFT
	"/highlight id %i",//LALT 
	"",//LALT|SHIFT
	"",//LALT|CTRL
	"",//LALT|SHIFT|CTRL
	"",//RALT
	"",//RALT|SHIFT
	"",//RALT|CTRL
	"",//RALT|SHIFT|CTRL
	"",//RALT|LALT
	"",//RALT|LALT|SHIFT
	"",//RALT|LALT|CTRL
	"" //RALT|LALT|SHIFT|CTRL
};

CHAR MapLeftClickString[16][MAX_STRING] =
{
	"",// unused, will always target
	"",//SHIFT 
	"",//CTRL
	"",//CTRL|SHIFT
	"/nav locxyz %x %y %z",//LALT 
	"/nav locxyz %x %y %u",//LALT|SHIFT
	"/nav locxyz %x %y %d",//LALT|CTRL
	"",//LALT|SHIFT|CTRL
	"",//RALT
	"",//RALT|SHIFT
	"",//RALT|CTRL
	"",//RALT|SHIFT|CTRL
	"",//RALT|LALT
	"",//RALT|LALT|SHIFT
	"",//RALT|LALT|CTRL
	"" //RALT|LALT|SHIFT|CTRL
};

CHAR MapNameString[MAX_STRING] = { "%N" };
CHAR MapTargetNameString[MAX_STRING] = { "%N" };
CHAR mapshowStr[MAX_STRING] = { "" };
CHAR maphideStr[MAX_STRING] = { "" };
SEARCHSPAWN MapFilterCustom = { 0 };
SEARCHSPAWN MapFilterNamed = { 0 };
MAPFILTER MapFilterOptions[] = {
	{ "All",          TRUE,(DWORD)-1,          TRUE,(DWORD)MAPFILTER_Invalid,TRUE,  "Enables/disables map functions" },
	{ "PC",           FALSE,(DWORD)0xFF00FF,   TRUE,MAPFILTER_All,TRUE,      "Displays PCs" },
	{ "PCConColor",   FALSE,(DWORD)-1,         TRUE,MAPFILTER_PC,FALSE,      "Displays PCs in consider colors" },
	{ "Group",        FALSE,(DWORD)0x0080C0,   TRUE,MAPFILTER_PC,FALSE,      "Displays group members in a specific color" },
	{ "Mount",        FALSE,(DWORD)0x707070,   TRUE,MAPFILTER_All,TRUE,      "Displays mounts" },
	{ "NPC",          FALSE,(DWORD)0x404040,   TRUE,MAPFILTER_All,TRUE,      "Displays NPCs" },
	{ "NPCConColor",  FALSE,(DWORD)-1,         TRUE,MAPFILTER_NPC,FALSE,     "Displays NPCs in consider colors" },
	{ "Untargetable", FALSE,(DWORD)0x404040,   TRUE,MAPFILTER_All,TRUE,      "Displays Untargetable NPCs" },
	{ "Pet",          FALSE,(DWORD)0x707070,   TRUE,MAPFILTER_All,TRUE,      "Displays pets" },
	{ "Corpse",       FALSE,(DWORD)0x00C000,   TRUE,MAPFILTER_All,TRUE,      "Displays corpses" },
	{ "Chest",        FALSE,(DWORD)0xC08000,   TRUE,MAPFILTER_All,TRUE,      "Displays chestesses" },
	{ "Trigger",      FALSE,(DWORD)0xC08000,   TRUE,MAPFILTER_All,TRUE,      "Displays hidden triggers" },
	{ "Trap",         FALSE,(DWORD)0xC08000,   TRUE,MAPFILTER_All,TRUE,      "Displays hidden traps" },
	{ "Timer",        FALSE,(DWORD)0xC08000,   TRUE,MAPFILTER_All,TRUE,      "Displays hidden timers" },
	{ "Ground",       FALSE,(DWORD)0xC0C0C0,   TRUE,MAPFILTER_All,TRUE,      "Displays ground items" },
	{ "Target",       FALSE,(DWORD)0xC00000,   TRUE,MAPFILTER_All,FALSE,     "Displays your target" },
	{ "TargetLine",   FALSE,(DWORD)0x808080,   TRUE,MAPFILTER_Target,FALSE,  "Displays a line to your target" },
	{ "TargetRadius", FALSE,(DWORD)0x808080,   FALSE,MAPFILTER_Target,FALSE, "Sets radius of a circle around your target to # (omit or set to 0 to disable)" },
	{ "TargetMelee",  FALSE,(DWORD)0xFF8080,   FALSE,MAPFILTER_Target,FALSE, "Draws a melee-range circle around your target" },
	{ "Vector",       FALSE,(DWORD)-1,         TRUE,MAPFILTER_All,TRUE,      "Displays heading vectors" },
	{ "Custom",       FALSE,(DWORD)-1,         FALSE,MAPFILTER_All,TRUE,     "Sets custom filter (omit to disable)" },
	{ "CastRadius",   FALSE,(DWORD)0x808080,   FALSE,MAPFILTER_All,FALSE,    "Sets radius of casting circle to # (omit or set to 0 to disable)" },
	{ "NormalLabels", FALSE,(DWORD)-1,         TRUE,(DWORD)MAPFILTER_Invalid,FALSE, "Toggles non-MQ2 label display" },
	{ "Menu",         FALSE,(DWORD)-1,         TRUE,(DWORD)MAPFILTER_Invalid,FALSE, "Allows display of right-click context menu" },
	{ "SpellRadius",  FALSE,(DWORD)0x00C000,   FALSE,MAPFILTER_All,FALSE,    "Sets radius of 2nd casting circle to # (omit or set to 0 to disable)" },
	{ "Aura",         FALSE,(DWORD)0x404040,   TRUE,MAPFILTER_All,TRUE,      "Displays Auras" },
	{ "Object",       FALSE,(DWORD)0x404040,   TRUE,MAPFILTER_All,TRUE,      "Displays inanimate objects" },
	{ "Banner",       FALSE,(DWORD)0x404040,   TRUE,MAPFILTER_All,TRUE,      "Displays banners" },
	{ "Campfire",     FALSE,(DWORD)0x404040,   TRUE,MAPFILTER_All,TRUE,      "Displays campfires" },
	{ "PCCorpse",     FALSE,(DWORD)0x00C000,   TRUE,MAPFILTER_All,TRUE,	     "Displays PC corpses, when corpse setting is on" },
	{ "NPCCorpse",    FALSE,(DWORD)0x00C000,   TRUE,MAPFILTER_All,TRUE,      "Displays NPC corpses, when corpse setting is on" },
	{ "Mercenary",    FALSE,(DWORD)0x404040,   TRUE,MAPFILTER_All,TRUE,      "Displays mercenaries" },
	{ "Named",        FALSE,(DWORD)0x404040,   TRUE,MAPFILTER_All,TRUE,      "Displays named NPCs" },
	{ "Marker",       FALSE,(DWORD)-1,         FALSE,MAPFILTER_All,TRUE,  "Displays marker (mobtype triangle/square/diamond size)" },
	{ "TargetPath",   FALSE,(DWORD)-1,         TRUE,MAPFILTER_Target,FALSE,  "Draws EQ Path to selected target" },
	{ NULL,           FALSE,(DWORD)-1,         FALSE,(DWORD)MAPFILTER_Invalid,FALSE,  NULL }
};

CSidlScreenWnd::VirtualFunctionTable* CMyMapViewWnd__OldvfTable = nullptr;
CSidlScreenWnd::VirtualFunctionTable* MapViewMap_OldvfTable = nullptr;
void * CMyMapViewWnd__OldDestructor = nullptr;
void * CMyMapViewWnd__OldHandleRButtonDown = nullptr;
void * CMyMapViewWnd__OldPostDraw = nullptr;
void * MapViewMap__OldHandleRButtonDown = nullptr;

DWORD __declspec(naked) CMyMapViewWnd__Destructor(const BOOL Deallocate)
{
	__asm {
		push ecx;
		push edx;//maybe a compiler issue, but if we dont push edx as well here, we will crash on /loadskin... -eqmule Oct 23 2015
		push eax;//it doesnt really matter what the reason is, because it wont hurt pushing it in older compilers
	}			 //cause we pop it anyway at the bottom of this func...
				 //the important thing is that esp is the same both on entering this func and on exit...
				 //(and it is now)

	if (CMyMapViewWnd__OldvfTable && MapViewMap_OldvfTable) {
		// make our own little stack frame here
		// operator delete assumes that it is there
		// it uses (unnecessarily) ebp-4
		__asm {
			push    ebp
				push    eax
				push    eax
				mov		ebp, esp
		}
		delete *reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(pMapViewWnd);
		*reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(pMapViewWnd) = CMyMapViewWnd__OldvfTable;
		delete *reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(&pMapViewWnd->MapView);
		*reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(&pMapViewWnd->MapView) = MapViewMap_OldvfTable;
		CMyMapViewWnd__OldvfTable = nullptr;
		MapViewMap_OldvfTable = nullptr;
		__asm {
			pop     eax
				pop     eax
				pop     ebp
		}
	}

	__asm {
		pop eax;
		pop edx;
		pop ecx;
		jmp[CMyMapViewWnd__OldDestructor];
	}
}

bool RButtonDown()
{
	if (pCurrentMapLabel)
	{
		return MapSelectTarget();
	}
	if (!IsOptionEnabled(MAPFILTER_ContextMenu))
		return false;
	return true;
}
//int MapViewMap::HandleRButtonDown(const CXPoint& point, UINT Flags) - should probably just change to this
int __declspec(naked) MapViewMap__HandleRButtonDown(const CXPoint& point, uint32_t flags)
{
	__asm {
		push ecx;
		push eax;
	}
	if (RButtonDown())
	{
		__asm {
			pop eax;
			pop ecx;
			jmp[MapViewMap__OldHandleRButtonDown];
		};
	}
	else
	{
		__asm {
			pop eax;
			pop ecx;
			xor eax, eax;
			retn 8;
		}
	}
}

VOID __declspec(naked) CMyMapViewWnd__PostDraw()
{
	__asm {

		push esi;
		mov esi, ecx;

		call[MapUpdate];
		call[MapAttach];

		mov ecx, esi;
		call[CMyMapViewWnd__OldPostDraw];
		push eax;

		call[MapDetach];
		pop eax;
		mov ecx, esi;
		pop esi;
		ret;
	};
}
#define INVALID_FLOOR ((float)-1.0e27)
class CMyMapViewWnd
{
public:
    int HandleLButtonDown_Trampoline(const CXPoint&, uint32_t);
    int HandleLButtonDown_Detour(const CXPoint& point, uint32_t flags)
	{
        CMapViewWnd *pWnd = reinterpret_cast<CMapViewWnd*>(this);
		float points[3] = { 0 ,0,0};
        points[0] = (float)point.x;
        points[1] = (float)point.y;
        points[2] = 0;
        pWnd->GetWorldCoordinates(points); // this writes the world X & Y coords into points

        PZONEINFO pZone = reinterpret_cast<PZONEINFO>(pZoneInfo);
        CDisplay *pDsp = reinterpret_cast<CDisplay*>(pDisplay);
        std::vector<float> z_hits;
        if (pZone && pDsp) {
			FLOAT curr_z = 0.0f;
			for(float f = pZone->Ceiling;f>pZone->Floor;f-=2.0f)
			{
				curr_z = pDsp->GetFloorHeight(points[0], points[1], f);
				if (curr_z != INVALID_FLOOR)
				{
					//Sleep(0);
					break;
				}
			}
			do {
				z_hits.push_back(curr_z);
				curr_z = pDsp->GetFloorHeight(points[0], points[1], curr_z - 2.f);
			} while (curr_z >= pZone->Floor - 1.f);
        } else if (auto pSpawn = reinterpret_cast<PSPAWNINFO>(pLocalPlayer)) {
            // if no zone or pDsp, let's just put our own Z here. Shouldn't actually happen...
            z_hits.push_back(pSpawn->Z);
        }

        MapClickLocation(points, z_hits);

        return HandleLButtonDown_Trampoline(point, flags);
    }

	DWORD Constructor_Trampoline(class CXWnd *);
	DWORD Constructor_Detour(class CXWnd *wnd)
	{
		DWORD Ret = Constructor_Trampoline(wnd);
		StealVFTable(reinterpret_cast<CMapViewWnd*>(this));
		return Ret;
	}

	static void StealVFTable(CMapViewWnd * pWnd)
	{
		// Copy existing vftables
		auto pvfTable = new CSidlScreenWnd::VirtualFunctionTable(**reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(pWnd));
		auto pMapViewMapVfTable = new CSidlScreenWnd::VirtualFunctionTable(**reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(&pWnd->MapView));

		// Replace functions with our detoured ones
		pvfTable->Destructor = CMyMapViewWnd__Destructor;
		pMapViewMapVfTable->PostDraw = CMyMapViewWnd__PostDraw;
		pMapViewMapVfTable->HandleRButtonDown = MapViewMap__HandleRButtonDown;

		// Preserve pointers to existing vftables
		CMyMapViewWnd__OldvfTable = *reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(pWnd);
		MapViewMap_OldvfTable = *reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(&pWnd->MapView);

		// Preserve 
		CMyMapViewWnd__OldPostDraw = MapViewMap_OldvfTable->PostDraw;
		CMyMapViewWnd__OldDestructor = CMyMapViewWnd__OldvfTable->Destructor;
		MapViewMap__OldHandleRButtonDown = pMapViewMapVfTable->HandleRButtonDown;

		// Replace vftables with our ones
		*reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(pWnd) = pvfTable;
		*reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(&pWnd->MapView) = pMapViewMapVfTable;
	}

	static void RestoreVFTable(CMapViewWnd *pWnd)
	{
			if (CMyMapViewWnd__OldvfTable && MapViewMap_OldvfTable) {
				delete *reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(pWnd);
				*reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(pWnd) = CMyMapViewWnd__OldvfTable;
				delete *reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(&pWnd->MapView);
				*reinterpret_cast<CSidlScreenWnd::VirtualFunctionTable**>(&pWnd->MapView) = MapViewMap_OldvfTable;
			}
	}
};

DETOUR_TRAMPOLINE_EMPTY(DWORD CMyMapViewWnd::Constructor_Trampoline(class CXWnd *));
DETOUR_TRAMPOLINE_EMPTY(int CMyMapViewWnd::HandleLButtonDown_Trampoline(const CXPoint&, unsigned __int32));

bool Update = true;

#ifndef ISXEQ
// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID)
{
	DebugSpewAlways("Initializing MQ2Map");

	bmMapRefresh = AddMQ2Benchmark("Map Refresh");
	unsigned long i;
	CHAR szBuffer[MAX_STRING] = { 0 };
	CHAR tmp_1[MAX_STRING] = { 0 };
	CHAR tmp_2[MAX_STRING] = { 0 };
	for (i = 0; MapFilterOptions[i].szName; i++) {
		sprintf_s(szBuffer, "%s-Color", MapFilterOptions[i].szName);
		MapFilterOptions[i].Enabled = GetPrivateProfileInt("Map Filters", MapFilterOptions[i].szName, MapFilterOptions[i].Default, INIFileName);
		MapFilterOptions[i].Color = GetPrivateProfileInt("Map Filters", szBuffer, MapFilterOptions[i].DefaultColor, INIFileName) | 0xFF000000;
		sprintf_s(tmp_1, "%s-Size", MapFilterOptions[i].szName);
		GetPrivateProfileString("Marker Filters", MapFilterOptions[i].szName, "None", tmp_2, MAX_STRING, INIFileName);
		DWORD mark = FindMarker(tmp_2);
		if (mark == 99) mark = 0;

		MapFilterOptions[i].Marker = mark;
		MapFilterOptions[i].MarkerSize = GetPrivateProfileInt("Marker Filters", tmp_1, 0, INIFileName);
	}

	activeLayer = GetPrivateProfileInt("Map Filters", "ActiveLayer", activeLayer, INIFileName);

	UpdateDefaultMapLoc();

	repeatMapshow = GetPrivateProfileInt("Map Filters", "Mapshow-Repeat", FALSE, INIFileName);
	repeatMaphide = GetPrivateProfileInt("Map Filters", "Maphide-Repeat", FALSE, INIFileName);

	HighlightSIDELEN = GetPrivateProfileInt("Map Filters", "HighSize", HighlightSIDELEN, INIFileName);
	HighlightPulse = GetPrivateProfileInt("Map Filters", "HighPulse", HighlightPulse, INIFileName);
	HighlightPulseIncreasing = TRUE;
	HighlightPulseIndex = 0;
	HighlightPulseDiff = HighlightSIDELEN / 10;

	GetPrivateProfileString("Map Filters", "Mapshow", "", mapshowStr, MAX_STRING, INIFileName);
	GetPrivateProfileString("Map Filters", "Maphide", "", maphideStr, MAX_STRING, INIFileName);
	MapInit();
	GetPrivateProfileString("Naming Schemes", "Normal", "%N", MapNameString, MAX_STRING, INIFileName);
	GetPrivateProfileString("Naming Schemes", "Target", "%N", MapTargetNameString, MAX_STRING, INIFileName);

	for (i = 1; i<16; i++)
	{
		sprintf_s(szBuffer, "KeyCombo%d", i);
		GetPrivateProfileString("Right Click", szBuffer, MapSpecialClickString[i], MapSpecialClickString[i], MAX_STRING, INIFileName);
		GetPrivateProfileString("Left Click", szBuffer, MapLeftClickString[i], MapLeftClickString[i], MAX_STRING, INIFileName);
	}

	// Do not use Custom, since the string isn't stored
	MapFilterOptions[MAPFILTER_Custom].Enabled = 0;


	AddCommand("/mapfilter", MapFilters, 0, 1, 1);
	AddCommand("/maphide", MapHideCmd, 0, 1, 1);
	AddCommand("/mapshow", MapShowCmd, 0, 1, 1);
	AddCommand("/highlight", MapHighlightCmd, 0, 1, 1);
	AddCommand("/mapnames", MapNames, 0, 1, 1);
	AddCommand("/mapclick", MapClickCommand, 0, 1, 0);
	AddCommand("/mapactivelayer", MapActiveLayerCmd, 0, 1, 1);
	AddCommand("/maploc", MapSetLocationCmd, 0, 1, 1);
	AddCommand("/clearloc", MapClearLocationCmd, 0, 1, 1);

	EzDetourwName(CMapViewWnd__CMapViewWnd, &CMyMapViewWnd::Constructor_Detour, &CMyMapViewWnd::Constructor_Trampoline,"CMapViewWnd__CMapViewWnd");
	if (pMapViewWnd)
		CMyMapViewWnd::StealVFTable(pMapViewWnd);

    EzDetourwName(CMapViewWnd__HandleLButtonDown, &CMyMapViewWnd::HandleLButtonDown_Detour, &CMyMapViewWnd::HandleLButtonDown_Trampoline, "CMapViewWnd__HandleLButton");

	AddMQ2Data("MapSpawn", dataMapSpawn);
	ClearSearchSpawn(&MapFilterNamed);
	ParseSearchSpawn("#", &MapFilterNamed);
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
	DebugSpewAlways("Shutting down MQ2Map");
	Update = false;
	RemoveMQ2Data("MapSpawn");

	RemoveDetour(CMapViewWnd__CMapViewWnd);
    RemoveDetour(CMapViewWnd__HandleLButtonDown);

	MapClear();
	if (pMapViewWnd)
		CMyMapViewWnd::RestoreVFTable(pMapViewWnd);

	RemoveMQ2Benchmark(bmMapRefresh);
	RemoveCommand("/maphide");
	RemoveCommand("/mapshow");
	RemoveCommand("/mapfilter");
	RemoveCommand("/highlight");
	RemoveCommand("/mapnames");
	RemoveCommand("/mapclick");
	RemoveCommand("/mapactivelayer");
	RemoveCommand("/maploc");
	RemoveCommand("/clearloc");
}

// This is called every time MQ pulses
PLUGIN_API VOID OnPulse(VOID)
{
	// DONT leave in this debugspew, even if you leave in all the others
	//   DebugSpewAlways("MQ2Mapshow::OnPulse()");

	long curTime = MakeTime();
	clock_t curClockTime = clock();
	bool cleared = false;

	// Clear MapLocs on zone
	if (PCHARINFO charInfo = GetCharInfo()) {
		if (currentZoneId != (charInfo->zoneId & 0x7FFF))
		{
			for (auto const& [tag, loc] : LocationMap)
			{
				ClearMapLocLines(loc);
				delete loc;
			}
			LocationMap.clear();
			currentZoneId = (charInfo->zoneId & 0x7FFF);
		}
	}
	CHAR szBuffer[MAX_STRING] = { 0 };

	if (curClockTime > highPulseRepeatLast + highPulseRepeatIntervalMillis && HighlightPulse)
	{
		if (HighlightPulseIndex == 5 || HighlightPulseIndex == -5)
			HighlightPulseIncreasing = !HighlightPulseIncreasing;

		if (HighlightPulseIncreasing)
			HighlightPulseIndex++;
		else
			HighlightPulseIndex--;

		highPulseRepeatLast = curClockTime;
	}

	if (curTime > repeatLast + repeatInterval) {

		if (repeatMapshow && strlen(mapshowStr) > 0) {
			if (!cleared) {
				MapClear();
				MapGenerate();
				cleared = true;
			}

			SEARCHSPAWN ss;
			ClearSearchSpawn(&ss);
			ParseSearchSpawn(mapshowStr, &ss);
			MapShow(ss);
		}

		if (repeatMaphide && strlen(maphideStr) > 0) {
			if (!cleared) {
				MapClear();
				MapGenerate();
				cleared = true;
			}

			SEARCHSPAWN ss;
			ClearSearchSpawn(&ss);
			ParseSearchSpawn(maphideStr, &ss);
			MapHide(ss);
		}

		repeatLast = curTime;
	}
}

// This is called each time a spawn is added to a zone (inserted into EQ's list of spawns),
// or for each existing spawn when a plugin first initializes
// NOTE: When you zone, these will come BEFORE OnZoned
PLUGIN_API VOID OnAddSpawn(PSPAWNINFO pNewSpawn)
{
	// your toon's spawn id changes and it's no longer zero to start
	// don't added it all 
	if (pNewSpawn) {
		if (PCHARINFO pMe = GetCharInfo()) {
			if (Update && pNewSpawn->SpawnID != 0 && pMe->pSpawn != pNewSpawn) {
				//DebugSpewAlways("MQ2Map::OnAddSpawn(%s) = %d", pNewSpawn->Name, pNewSpawn->SpawnID);
				AddSpawn(pNewSpawn);
			}
		}
	}
}

// This is called each time a spawn is removed from a zone (removed from EQ's list of spawns).
// It is NOT called for each existing spawn when a plugin shuts down.
PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pSpawn)
{
	//DebugSpewAlways("MQ2Map::OnRemoveSpawn(%s) = %d", pSpawn->Name, pSpawn->SpawnID);
	if (Update && pSpawn)
		RemoveSpawn(pSpawn);
}

PLUGIN_API VOID SetGameState(DWORD GameState)
{
	if (GameState == 3)
	{
		MapClear();
	}
}

// This is called each time a ground item is added to a zone
// or for each existing ground item when a plugin first initializes
// NOTE: When you zone, these will come BEFORE OnZoned
PLUGIN_API VOID OnAddGroundItem(PGROUNDITEM pNewGroundItem)
{
	DebugSpewAlways("MQ2Map::OnAddGroundItem(%d)", pNewGroundItem->DropID);
	if (Update)
		AddGroundItem(pNewGroundItem);
}

// This is called each time a ground item is removed from a zone
// It is NOT called for each existing ground item when a plugin shuts down.
PLUGIN_API VOID OnRemoveGroundItem(PGROUNDITEM pGroundItem)
{
	DebugSpewAlways("MQ2Map::OnRemoveGroundItem(%d)", pGroundItem->DropID);
	if (Update)
		RemoveGroundItem(pGroundItem);
}

PLUGIN_API  PMAPLINE MQ2MapAddLine() {
	return InitLine();
}

PLUGIN_API VOID MQ2MapDeleteLine(PMAPLINE pLine) {
	DeleteLine(pLine);
}

#endif
