#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <inttypes.h>

int main()
{
	char AssaultCubeExePath [] = "D:\\Spiele\\AssaultCube 1.3.0.2\\bin_win32\\ac_client.exe";
  
	uintptr_t recoil_addr = 0x004C2EC3;
	char opcodes[] = "\x90\x90\x90\x90\x90";

	printf("opcode size: %zu\n" , sizeof(opcodes));

	HWND hwnd = FindWindow(0, "AssaultCube");

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

	if (WriteProcessMemory (ProcessHandle, (void*)recoil_addr, &opcodes, sizeof(opcodes)-1, 0 ) == false)
	{
		printf("[WriteProcessMemory]: Unable to read data from memory address. Error code: %lu\n", GetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}

	else
	{
    printf("Opcodes injected\n");
	}

	CloseHandle(ProcessHandle);
	getchar();
	return 0;
}
