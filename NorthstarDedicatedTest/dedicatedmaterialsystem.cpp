#pragma once
#include "pch.h"
#include "dedicated.h"
#include "dedicatedmaterialsystem.h"
#include "hookutils.h"

void InitialiseDedicatedMaterialSystem(HMODULE baseAddress)
{
	if (!IsDedicated())
		return;

	//while (!IsDebuggerPresent())
	//	Sleep(100);

	// not using these for now since they're related to nopping renderthread/gamewindow i.e. very hard
	// we use -noshaderapi instead now
	//{
	//	// function that launches renderthread
	//	char* ptr = (char*)baseAddress + 0x87047;
	//	TempReadWrite rw(ptr);
	//
	//	// make it not launch renderthread
	//	*ptr = (char)0x90;
	//	*(ptr + 1) = (char)0x90;
	//	*(ptr + 2) = (char)0x90;
	//	*(ptr + 3) = (char)0x90;
	//	*(ptr + 4) = (char)0x90;
	//	*(ptr + 5) = (char)0x90;
	//}
	//
	//{
	//	// some function that waits on renderthread job
	//	char* ptr = (char*)baseAddress + 0x87d00;
	//	TempReadWrite rw(ptr);
	//
	//	// return immediately
	//	*ptr = (char)0xC3;
	//}

	{
		// CMaterialSystem::FindMaterial
		char* ptr = (char*)baseAddress + 0x5F0F1;
		TempReadWrite rw(ptr);

		// make the game always use the error material
		*ptr = 0xE9;
		*(ptr + 1) = (char)0x34;
		*(ptr + 2) = (char)0x03;
		*(ptr + 3) = (char)0x00;
	}

	if (DisableDedicatedWindowCreation())
	{
		{
			// materialsystem rpak type registrations
			char* ptr = (char*)baseAddress + 0x22B5;
			TempReadWrite rw(ptr);

			// nop a call that crashes, not needed on dedi
			*ptr = 0x90;
			*(ptr + 1) = (char)0x90;
			*(ptr + 2) = (char)0x90;
			*(ptr + 3) = (char)0x90;
			*(ptr + 4) = (char)0x90;
		}

		{
			// some renderthread stuff
			char* ptr = (char*)baseAddress + 0x8C10;
			TempReadWrite rw(ptr);

			// call => nop
			*ptr = (char)0x90;
			*(ptr + 1) = (char)0x90;
		}

		// rpak type callbacks
		// these need to be nopped for dedi
		{
			// materialsystem rpak type: shader
			char* ptr = (char*)baseAddress + 0x2850;
			TempReadWrite rw(ptr);

			// ret
			*ptr = (char)0xC3;
		}

		{
			// materialsystem rpak type: texture
			char* ptr = (char*)baseAddress + 0x2B00;
			TempReadWrite rw(ptr);

			// ret
			*ptr = (char)0xC3;
		}

		{
			// materialsystem rpak type: material
			char* ptr = (char*)baseAddress + 0x50AA0;
			TempReadWrite rw(ptr);

			// ret
			*ptr = (char)0xC3;
		}
	}
}

// rpak pain
struct RpakTypeDefinition
{
	int64_t magic;
	char* longName;

	// more fields but they don't really matter for what we use them for
};

typedef void*(*RegisterRpakTypeType)(RpakTypeDefinition* rpakStruct, unsigned int a1, unsigned int a2);
RegisterRpakTypeType RegisterRpakType;

typedef void(*RegisterMaterialSystemRpakTypes)();

void* RegisterRpakTypeHook(RpakTypeDefinition* rpakStruct, unsigned int a1, unsigned int a2)
{
	// make sure this prints right
	char magicName[5];
	memcpy(magicName, &rpakStruct->magic, 4);
	magicName[4] = 0; // null terminator

	spdlog::info("rpak type {} {} registered", magicName, rpakStruct->longName);

	// reregister rpak types that aren't registered on a windowless dedi
	if (rpakStruct->magic == 0x64636C72) // rlcd magic, this one is registered last
		((RegisterMaterialSystemRpakTypes)((char*)GetModuleHandleA("materialsystem_dx11.dll") + 0x22A0))(); // slightly hellish call, registers materialsystem rpak types

	return RegisterRpakType(rpakStruct, a1, a2);
}

typedef void*(*PakLoadAPI__LoadRpakType)(char* filename, void* unknown, int flags);
PakLoadAPI__LoadRpakType PakLoadAPI__LoadRpak;

void* PakLoadAPI__LoadRpakHook(char* filename, void* unknown, int flags)
{
	spdlog::info("PakLoadAPI__LoadRpakHook {}", filename);
	
	// on dedi, don't load any paks that aren't required
	if (strncmp(filename, "common", 6))
		return 0;
	
	return PakLoadAPI__LoadRpak(filename, unknown, flags);
}

typedef void* (*PakLoadAPI__LoadRpak2Type)(char* filename, void* unknown, int flags, void* callback, void* callback2);
PakLoadAPI__LoadRpak2Type PakLoadAPI__LoadRpak2;

void* PakLoadAPI__LoadRpak2Hook(char* filename, void* unknown, int flags, void* callback, void* callback2)
{
	spdlog::info("PakLoadAPI__LoadRpak2Hook {}", filename);

	// on dedi, don't load any paks that aren't required
	if (strncmp(filename, "common", 6))
		return 0;

	return PakLoadAPI__LoadRpak2(filename, unknown, flags, callback, callback2);
}

void InitialiseDedicatedRtechGame(HMODULE baseAddress)
{
	if (!IsDedicated())
		return;

	baseAddress = GetModuleHandleA("rtech_game.dll");

	HookEnabler hook;
	// unfortunately this is unstable, seems to freeze when changing maps
	//ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0xB0F0, &PakLoadAPI__LoadRpakHook, reinterpret_cast<LPVOID*>(&PakLoadAPI__LoadRpak));
	//ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0xB170, &PakLoadAPI__LoadRpak2Hook, reinterpret_cast<LPVOID*>(&PakLoadAPI__LoadRpak2));

	if (DisableDedicatedWindowCreation())
	{
		ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x7BE0, &RegisterRpakTypeHook, reinterpret_cast<LPVOID*>(&RegisterRpakType));
	}
}