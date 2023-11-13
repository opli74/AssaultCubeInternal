#pragma once
#include "Hook.h"
#include <vector>
#include <iostream>

//bool Detour32(BYTE* src, BYTE* dst)
//{
//    if (!src || !dst)
//    {
//        std::cerr << "Detour32: Invalid source or destination pointers." << std::endl;
//        return false;
//    }
//
//    DWORD curProtection;
//    if (!VirtualProtect(src, Detour::DETOUR_SIZE, PAGE_EXECUTE_READWRITE, &curProtection))
//    {
//        std::cerr << "Detour32: VirtualProtect failed." << std::endl;
//        return false;
//    }
//
//    *src = 0xE9;
//    *reinterpret_cast<uintptr_t*>(src + 1) = reinterpret_cast<uintptr_t>(dst);
//
//    if (!VirtualProtect(src, Detour::DETOUR_SIZE, curProtection, &curProtection))
//    {
//        std::cerr << "Detour32: Restoring original protection failed." << std::endl;
//        return false;
//    }
//
//    return true;
//}
//
//BYTE* TrampHook32(BYTE* src, BYTE* dst)
//{
//    if (!src || !dst)
//    {
//        std::cerr << "TrampHook32: Invalid source or destination pointers." << std::endl;
//        return nullptr;
//    }
//
//    // Locate Gateway
//    BYTE* gateway = static_cast<BYTE*>(VirtualAlloc(0, Detour::JMP_SIZE + Detour::DETOUR_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
//    if (!gateway)
//    {
//        std::cerr << "TrampHook32: VirtualAlloc failed." << std::endl;
//        return nullptr;
//    }
//
//    // Write the stolen bytes to the gateway
//    memcpy(gateway, src, Detour::DETOUR_SIZE);
//
//    // Use the JMP instruction for an absolute jump
//    *(gateway + Detour::DETOUR_SIZE) = 0xE9;
//
//    // Calculate the absolute address and write it directly at the gateway
//    *reinterpret_cast<uintptr_t*>(gateway + Detour::DETOUR_SIZE + 1) = reinterpret_cast<uintptr_t>(dst);
//
//    // Perform detour
//    if (!Detour32(src, dst))
//    {
//        std::cerr << "TrampHook32: Detour32 failed." << std::endl;
//        VirtualFree(gateway, 0, MEM_RELEASE);
//        return nullptr;
//    }
//
//    return gateway;
//}

bool Detour32(BYTE* src, BYTE* dst, const uintptr_t len)
{
	if (len < 5) return false;

	DWORD curProtection;
	VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &curProtection);

	uintptr_t relativeAddress = dst - src - 5;

	*src = 0xE9;

	*(uintptr_t*)(src + 1) = relativeAddress;

	VirtualProtect(src, len, curProtection, &curProtection);
	return true;
}

BYTE* TrampHook32(BYTE* src, BYTE* dst, const uintptr_t len)
{
	if (len < 5) return 0;

	//Create Gateway
	BYTE* gateway = (BYTE*)VirtualAlloc(0, len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	//write the stolen bytes to the gateway
	memcpy_s(gateway, len, src, len);

	//Get the gateway to destination address
	uintptr_t gatewayRelativeAddr = src - gateway - 5;

	// add the jmp opcode to the end of the gateway
	*(gateway + len) = 0xE9;

	//Write the address of the gateway to the jmp
	*(uintptr_t*)((uintptr_t)gateway + len + 1) = gatewayRelativeAddr;

	//Perform the detour
	Detour32(src, dst, len);

	return gateway;
}

Hook::Hook(BYTE* src, BYTE* dst, BYTE* PtrToGatewayFnPtr) //Basic Constructor
{
	this->src = src;
	this->dst = dst;
	this->PtrToGatewayFnPtr = PtrToGatewayFnPtr;
}

Hook::Hook(const char* exportName, const char* modName, BYTE* dst, BYTE* PtrToGatewayFnPtr) //Wrapper Constructor
{
	HMODULE hMod = GetModuleHandleA(modName);

	this->src = (BYTE*)GetProcAddress(hMod, exportName);
	this->dst = dst;
	this->PtrToGatewayFnPtr = PtrToGatewayFnPtr;
}

void Hook::Enable()
{
	memcpy(originalBytes, src, 5);
	*(uintptr_t*)PtrToGatewayFnPtr = (uintptr_t)TrampHook32(src, dst, 5);		//Get gateway returned by TrampHook
	bStatus = true;
}

void Hook::Disable()
{
	mem::write((BYTE*)src, originalBytes, 5);
	bStatus = false;
}

void Hook::Toggle()
{
	if (bStatus) Enable();
	else Disable();
}
