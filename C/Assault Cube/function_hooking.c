#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>
#include <tlhelp32.h>
#include <inttypes.h>

uintptr_t GetModuleBaseaddr(unsigned pid, char *ExePath)
{
	bool ModuleExists = false;
	HANDLE snap;
	MODULEENTRY32  lpme;
	lpme.dwSize = sizeof(MODULEENTRY32);

	// TH32CS_SNAPMODULE supports also 32 bit modules
	HANDLE  snapshot =  CreateToolhelp32Snapshot(TH32CS_SNAPMODULE32 | TH32CS_SNAPMODULE, pid);

	if (snapshot == INVALID_HANDLE_VALUE){
		return 0;
	}

	Module32First(snapshot, &lpme);
	do{
		//printf("lpme.szExePath: %s\n", lpme.szExePath );
		if (strncmp(lpme.szExePath, ExePath, strlen(ExePath)) == 0){
			CloseHandle(snapshot);
			return (uintptr_t)lpme.modBaseAddr;
		}

	}
	while (Module32Next (snapshot , &lpme ) == true) ;

	CloseHandle(snapshot);

	return 0;
}

unsigned char *SaveOpcodes(HANDLE ProcessHandle, uintptr_t jmp_from_addr, int size)
{
    unsigned char *saved_opcodes = (unsigned char*)malloc(size);

    // Save lea eax, [esp+1c] opcodes
	if (ReadProcessMemory(ProcessHandle, (void*)jmp_from_addr + 2, saved_opcodes, size, 0)== false){
		printf("[ReadProcessMemory]: Unable to read data from memory address. Error code: %lu\n", GetLastError());
		getchar();
		exit(EXIT_FAILURE);
	}

    return saved_opcodes;
}

void hook_function(HANDLE ProcessHandle, uintptr_t jmp_to_addr, uintptr_t jmp_from_addr, char *info_text, bool print_debug_msg)
{
    // jmp_to_addr   = allocated memory
    // jmp_from_addr = address where we can find: dec [eax]
    jmp_to_addr = jmp_to_addr - jmp_from_addr - 5;

	printf("---------------------------------------------------------\n");
    printf("Hook_function: \n\n");
    printf("%s", info_text);
    printf("Jump to this address: %" PRIxPTR "\n", jmp_to_addr);

    unsigned char opcodes[] = {
    '\xE9',                                // JMP

    // Memory address (Jump to this memory address)
    (unsigned char) ( jmp_to_addr >> 0 ) , // Least significant byte
    (unsigned char) ( jmp_to_addr >> 8 ) ,
	(unsigned char) ( jmp_to_addr >> 16) ,
    (unsigned char) ( jmp_to_addr >> 24) , // Most significant byte

    '\x90'                                 // NOP
    };

    if (WriteProcessMemory (ProcessHandle, (void*)jmp_from_addr, &opcodes, sizeof(opcodes), 0 ) == false){
        printf("[WriteProcessMemory]: Unable to read data from memory address. Error code: %lu\n", GetLastError());
        getchar();
        exit(EXIT_FAILURE);
    }

   	printf("Jump from this address: %" PRIxPTR "\n", jmp_from_addr);

    if (print_debug_msg == 1) {
        printf("opcodes[0]: %2x\n", opcodes[0]);

        // This block prints the memory address as single opcodes
        printf("opcodes[1]: %2x\n", opcodes[1]); //Least significant byte
        printf("opcodes[2]: %2x\n", opcodes[2]);
        printf("opcodes[3]: %2x\n", opcodes[3]);
        printf("opcodes[4]: %2x\n", opcodes[4]); // Most significant bytte

        printf("opcodes[5]: %2x\n", opcodes[5]);
    }

    printf("---------------------------------------------------------\n");

}

void inject_opcodes(HANDLE ProcessHandle, uintptr_t allocated_memory, unsigned char new_opcodes[], unsigned new_opcodes_size){

        if (WriteProcessMemory (ProcessHandle, (void*)allocated_memory, new_opcodes, new_opcodes_size, 0 ) == false){
            printf("[WriteProcessMemory]: Unable to read data from memory address. Error code: %lu\n", GetLastError());
            getchar();
            exit(EXIT_FAILURE);
        }
}

int main()
{
    /*/
	
		>> 32 bit <<

		uintptr_t addr = 0xAABBCCDD;

		 unsigned char addr_char_ar [] = {
		(unsigned char) (addr >> 0 ), // Least significant byte
		(unsigned char) (addr >> 8 ),
		(unsigned char) (addr >> 16), 
		(unsigned char) (addr >> 24)  // Most significant byte
		};

		0xAABBCCDD = 10101010101110111100110011011101

		Most         |			            | Least 
		significant	 |           			| significant
					 |                  	| byte
		------------------------------------------------		 
				0xAA |     0xBB |     0xCC  |     0xDD |
			10101010 | 10111011 | 11001100  | 11011101 |
		-----------------------------------------------|

		printf("addr_char_ar[0]: 0x%2x\n", addr_char_ar[0]); // print: 0xdd
		printf("addr_char_ar[1]: 0x%2x\n", addr_char_ar[1]); // print: 0xcc
		printf("addr_char_ar[2]: 0x%2x\n", addr_char_ar[2]); // print: 0xbb
		printf("addr_char_ar[3]: 0x%2x\n", addr_char_ar[3]); // print: 0xaa


		:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

		>> 64 bit <<

		uintptr_t addr = 0xAABBCCDDEEFF1122;

		 unsigned char addr_char_ar [] = {
		(unsigned char) (addr >> 0 ), // Least significant byte
		(unsigned char) (addr >> 8 ),
		(unsigned char) (addr >> 16), 
		(unsigned char) (addr >> 24),  
		(unsigned char) (addr >> 32),
		(unsigned char) (addr >> 40),
		(unsigned char) (addr >> 48),
		(unsigned char) (addr >> 56)  // Most significant byte
		};

		0xAABBCC = 1010101010111011110011001101110111101110111111110001000100100010

		Most         |			           												| Least       
		significant	 |           														| significant 
					 |                  												| byte        
		--------------------------------------------------------------------------------------------	 
				0xAA |     0xBB |     0xCC  |     0xDD |     0xEE |     0xFF |     0x11 |     0x22 |
			10101010 | 10111011 | 11001100  | 11011101 | 11101110 | 11111111 | 00010001 | 00100010 |
		-----------------------------------------------|--------------------------------------------	

		printf("addr_char_ar[0]: 0x%2x\n", addr_char_ar[0]); // print: 0x22
		printf("addr_char_ar[1]: 0x%2x\n", addr_char_ar[1]); // print: 0x11
		printf("addr_char_ar[2]: 0x%2x\n", addr_char_ar[2]); // print: 0xff
		printf("addr_char_ar[3]: 0x%2x\n", addr_char_ar[3]); // print: 0xee
		printf("addr_char_ar[4]: 0x%2x\n", addr_char_ar[4]); // print: 0xdd
		printf("addr_char_ar[5]: 0x%2x\n", addr_char_ar[5]); // print: 0xcc
		printf("addr_char_ar[6]: 0x%2x\n", addr_char_ar[6]); // print: 0xbb
		printf("addr_char_ar[7]: 0x%2x\n", addr_char_ar[7]); // print: 0xaa

		:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

		>> How to calculate opcodes for the JMP command << 

		(AddressFromAllocatedMemory - AddressFromWhereWeJump) - 5 = Address in big endian

		Example:
		0x00980000 (AddrFromAllocatedMemory) - 0x004C73EF (AddrFromWhereWeJump) - 5 = 4B8C0C

		Now we need to convert 4B8C0C to a little endian address at that is: 0C 8C 4B 00
		So the correct opcodes are: E9 0C 8C 4B 00
		
		E9 is the opcode for the follwing Assembly command: E9

    /*/

    unsigned long pid = 0;
    char *AssaultCube_exe_path = "D:\\Spiele\\AssaultCube 1.3.0.2\\bin_win32\\ac_client.exe";

	HWND hwnd = FindWindow(0, "AssaultCube");

	if (hwnd == 0){
		printf("[FindWindow] error code: %lu\n", GetLastError());
		getchar();
		return 0;
	}

	GetWindowThreadProcessId(hwnd, &pid);

	if (pid == 0){
		printf("[GetWindowThreadProcessId] error code: %lu\n", GetLastError());
    	getchar();
		return 0;
	}

    printf("PID: %lu\n", pid);
	HANDLE ProcessHandle = OpenProcess( PROCESS_ALL_ACCESS, false, pid );

	if ( ProcessHandle == NULL){
		printf("The process could not be found. Error code: %lu\n", GetLastError());
		getchar();
		return 0;
	}

	printf("ProcessHandle: %p\n", ProcessHandle);

   	uintptr_t allocated_memory = (uintptr_t)VirtualAllocEx( ProcessHandle, NULL, 0x64, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    printf("Allocated memory: %p\n", (void*)allocated_memory );

   	uintptr_t module_base_addr = GetModuleBaseaddr(pid, AssaultCube_exe_path);
    uintptr_t jmp_from_addr = module_base_addr + 0xc73ef;

    printf("Address of dec [eax]: %p\n", (void*)jmp_from_addr);

	unsigned char *saved_opcodes = SaveOpcodes(ProcessHandle, jmp_from_addr, 4);

    printf("\n");
    // Saves the opcodes for the follwing command: lea eax, [esp+1c]
    printf("Saved opcode: %2x\n", saved_opcodes[0]);
    printf("Saved opcode: %2x\n", saved_opcodes[1]);
    printf("Saved opcode: %2x\n", saved_opcodes[2]);
    printf("Saved opcode: %2x\n", saved_opcodes[3]);
    printf("\n");

    unsigned char command_opcodes[] = "\xff\x00"           // inc [eax]
    							      "\x8D\x44\x24\x1C";  // lea eax,[esp+0x1c]

    // Jump from the memory address of the dec [eax] command to the allocated memory space.
	hook_function(ProcessHandle, allocated_memory, jmp_from_addr, "Hook: dec [eax]\n", 1);
    printf("\n\n");

    // Move value from [eax] to allocated_memory + 0x16
    unsigned char move_register_value_to_addr [] = {
    '\x56'		  ,  								   // push esi
    '\x8b','\x30' ,  								   // mov esi,[eax]

    '\x89', '\x35', 								   // mov [MemoryAddr],esi
    (unsigned char) ((allocated_memory + 0x16) >> 0 ), // Least significant byte from allocated_memory + 0x15
    (unsigned char) ((allocated_memory + 0x16) >> 8 ),
    (unsigned char) ((allocated_memory + 0x16) >> 16),
    (unsigned char) ((allocated_memory + 0x16) >> 24), // Most significant byte from allocated_memory + 0x15

    '\x5E'          								   // pop esi
    };

    inject_opcodes(ProcessHandle, allocated_memory, move_register_value_to_addr, sizeof(move_register_value_to_addr));

    // jump to allocated memory
    inject_opcodes(ProcessHandle, allocated_memory + 0xA, command_opcodes, sizeof(command_opcodes)-1);

    uintptr_t jmp_to_addr = jmp_from_addr;
    hook_function(ProcessHandle, jmp_to_addr + 5, allocated_memory + 0x10, "Hook: Leave allocated memory space\n", true);

    printf("\n");

    int current_ammo     = 0;
    int saved_ammo_value = -1;

    while(1){

        if (ReadProcessMemory(ProcessHandle, (void*)allocated_memory + 0x16, &current_ammo, sizeof(int), 0)== false){
            printf("[ReadProcessMemory]: Unable to read data from memory address. Error code: %lu\n", GetLastError());
            getchar();
            exit(EXIT_FAILURE);
        }

    	if (saved_ammo_value != current_ammo) {
        	printf("Current ammo: %d\n", current_ammo + 1);

            saved_ammo_value = current_ammo;
        }

        Sleep(1);
    }

  	printf("---------------------------------------------------------\n");
    printf("\n");

    printf("Press enter to terminate the program.\n");
    getchar();

}


