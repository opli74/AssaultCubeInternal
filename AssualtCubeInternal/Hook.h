#pragma once
#include <Windows.h>
#include "Mem.h"

bool Detour32(BYTE* src, BYTE* dst, const uintptr_t len);
BYTE* TrampHook32(BYTE* src, BYTE* dst, const uintptr_t len);

struct Hook
{
	bool bStatus{ false };

	BYTE* src{ nullptr };
	BYTE* dst{ nullptr };
	BYTE* PtrToGatewayFnPtr{ nullptr };

	BYTE originalBytes[10]{ 0 };

	//Constructors
	Hook(BYTE* src, BYTE* dst, BYTE* PtrToGatewayFnPtr);
	Hook(const char* exportName, const char* modName, BYTE* dst, BYTE* PtrtoGatewayFnPtr);

	//Function Def's
	void Enable();
	void Disable();
	void Toggle();
};

struct HookInfo_t
{
	uintptr_t source;
	uintptr_t destination;

	bool operator==(HookInfo_t& other) { return (source == other.source && destination == other.destination); }
};

namespace veh
{
	namespace
	{
		SYSTEM_INFO mSysInfo;
		PVOID mHandle;
		HookInfo_t vehHookInfo;
		DWORD mOldProtect;

		bool areInSamePage(uintptr_t* first, uintptr_t* second)
		{
			MEMORY_BASIC_INFORMATION source_info;
			if (!VirtualQuery(first, &source_info, sizeof(MEMORY_BASIC_INFORMATION)))
				return true;

			MEMORY_BASIC_INFORMATION destination_info;
			if (!VirtualQuery(second, &destination_info, sizeof(MEMORY_BASIC_INFORMATION)))
				return true;

			return (source_info.BaseAddress == destination_info.BaseAddress);
		}

		LONG __stdcall handler(EXCEPTION_POINTERS* pException)
		{
			if (pException->ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION)
			{
				if (pException->ContextRecord->Eip == vehHookInfo.source)
				{
					pException->ContextRecord->Eip = vehHookInfo.destination;
					pException->ContextRecord->EFlags |= PAGE_GUARD;
					return EXCEPTION_CONTINUE_EXECUTION;
				}
			}
			else if (pException->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
			{
				DWORD tmp;
				VirtualProtect(reinterpret_cast<LPVOID>(vehHookInfo.source), mSysInfo.dwPageSize, PAGE_EXECUTE_READ | PAGE_GUARD, &tmp);
				return EXCEPTION_CONTINUE_EXECUTION;
			}

			return EXCEPTION_CONTINUE_EXECUTION;
		}

		bool setup()
		{
			GetSystemInfo(&mSysInfo);
			mHandle = AddVectoredExceptionHandler(1, handler);

			return mHandle;
		}

		bool hook(uintptr_t ogFunction, uintptr_t hkFunction)
		{
			vehHookInfo.source = ogFunction;
			vehHookInfo.destination = hkFunction;

			if (areInSamePage((uintptr_t*)ogFunction, (uintptr_t*)hkFunction))
				return false;

			if (mHandle && VirtualProtect((LPVOID)ogFunction, 1, PAGE_EXECUTE_READ | PAGE_GUARD, &mOldProtect))
				return true;

			return false;
		}

		bool unhook()
		{
			DWORD old;
			if (mHandle && //Make sure we have a valid Handle to the registered VEH
				VirtualProtect((LPVOID)vehHookInfo.source, 1, mOldProtect, &old) && //Restore old Flags
				RemoveVectoredExceptionHandler(mHandle) //Remove the VEH
				)
				return true;

			return false;
		}
	}
}