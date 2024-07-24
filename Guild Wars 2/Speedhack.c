#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <inttypes.h>

void PrintHotkey()
{
	while(1)
	{
		for(int i=0; i<256; i++)
		{
			if (GetAsyncKeyState(i) == 0xffff8000)
			{
				printf("Hotkey: %c [ 0x%x ]\n", i, i );
			}
		}
	}
}

uintptr_t GetGw2ModuleBaseAddr(unsigned pid, char *GuildWarsExePath)
{
	bool ModuleExists = false;
	HANDLE snap;
	MODULEENTRY32  lpme;
	lpme.dwSize = sizeof(MODULEENTRY32);

	// TH32CS_SNAPMODULE support also 32 bit modules
	HANDLE  snapshot =  CreateToolhelp32Snapshot(TH32CS_SNAPMODULE32 | TH32CS_SNAPMODULE, pid);

	if (snapshot == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	Module32First(snapshot, &lpme);
	do
	{
		printf("lpme.szExePath: %s\n", lpme.szExePath );

		if (strncmp(lpme.szExePath, GuildWarsExePath, strlen(GuildWarsExePath)) == 0)
		{
			CloseHandle(snapshot);
			return (uintptr_t)lpme.modBaseAddr;
		}

	}
	while (Module32Next (snapshot , &lpme ) == true) ;

	CloseHandle(snapshot);

	return 0;
}

uintptr_t GetSpeedhackAddress(HANDLE ProcessHandle, uintptr_t GW2BaseAddr, uintptr_t PlayerBaseAddr)
{
	uintptr_t addr= 0;

	if (ReadProcessMemory(ProcessHandle, (void*)GW2BaseAddr + PlayerBaseAddr, &addr, sizeof(addr), 0)== false)
	{
		printf("[ReadProcessMemory]: Unable to read data from memory address. Error code: %lu\n", GetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}

	printf("addr 1: %llx\n", addr );

	if (ReadProcessMemory(ProcessHandle, (void*)addr + 0x50, &addr, sizeof(addr), 0)== false)
	{
		printf("[ReadProcessMemory]: Unable to read data from memory address. Error code: %lu\n", GetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}

	printf("addr 2: %llx\n", addr );

	return addr + 0x2b8;
}

float GetCurrentSpeed(HANDLE ProcessHandle, uintptr_t SpeedhackAddr)
{
	float speed = 0.0;
	if (ReadProcessMemory(ProcessHandle, (void*)SpeedhackAddr, &speed, sizeof(speed), 0)== false)
	{
		printf("[ReadProcessMemory]: Unable to read data from memory address. Error code: %lu\n", GetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}

	return speed;
}

void ChangeSpeed(HANDLE ProcessHandle, uintptr_t SpeedhackAddr, float speed)
{
	if (WriteProcessMemory (ProcessHandle, (void*)SpeedhackAddr, &speed, sizeof(float), 0 ) == false)
	{
		printf("[WriteProcessMemory]: Unable to read data from memory address. Error code: %lu\n", GetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}
}

int main()
{
	//PrintHotkey();

	// This needs to be compiled as a 64 bit application. Guild Wars 2 is a 64 Bit game.

	char GuildWarsExePath [] = "D:\\Steam\\steamapps\\common\\Guild Wars 2\\Gw2-64.exe";

    uintptr_t PlayerBaseAddr = 0x027A2D00;

	HWND hwnd = FindWindow(0, "Guild Wars 2");

	if (hwnd == 0)
	{
		printf("[FindWindow] error code: %lu\n", GetLastError());
		getchar();
		return 0;
	}

	unsigned long pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);

	if (pid == 0)
	{
		printf("[GetWindowThreadProcessId] error code: %lu\n", GetLastError());
    	getchar();
		return 0;
	}

    printf("PID: %lu\n", pid);

	HANDLE ProcessHandle = OpenProcess( PROCESS_ALL_ACCESS, false, pid );

	if ( ProcessHandle == NULL)
	{
		printf("The process could not be found. Error code: %lu\n", GetLastError());
		getchar();
		return 0;
	}

	printf("ProcessHandle: %p\n", ProcessHandle);


	uintptr_t GW2BaseAddr = GetGw2ModuleBaseAddr(pid, GuildWarsExePath);

	if (GW2BaseAddr == 0)
	{
		printf("[GetGw2ModuleBaseAddr]: Unable to get the Gw2-64.exe Base Address}\n");
		getchar();
		return 0;
	}

	printf("Guild Wars 2 Base Address: %llx\n", GW2BaseAddr);
	uintptr_t SpeedhackAddr = GetSpeedhackAddress(ProcessHandle, GW2BaseAddr, PlayerBaseAddr);

	if (SpeedhackAddr == 0)
	{
		printf("[ReadProcessMemory]: Unable to read data from memory address. Error code: %lu\n", GetLastError());
		getchar();
		return 0;
	}

	printf("Speedhack addr: %llx\n", SpeedhackAddr);
	float speed = GetCurrentSpeed(ProcessHandle, SpeedhackAddr);

	while(1)
	{
		for(int i=0; i<256; i++)
		{
			if (GetAsyncKeyState(0x58) == 0xffff8000)   // 0x58 = X
			{
				speed++;
				ChangeSpeed(ProcessHandle, SpeedhackAddr, speed);
			}

			else if (GetAsyncKeyState(0x43) == 0xffff8000)   // 0x43 = C
			{
				speed--;
				ChangeSpeed(ProcessHandle, SpeedhackAddr, speed);
			}

			else if (GetAsyncKeyState(0x59) == 0xffff8000)   // 0x59 = Y
			{
				speed = 9.1875;
                ChangeSpeed(ProcessHandle, SpeedhackAddr, speed);
			}

			printf("Current speed: %f\n", speed);
			Sleep(1);
		}
	}
	return 0;
}
