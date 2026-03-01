#include "pad.h"
#include "types.h"

/* externs */
extern int32_t cellPadGetData(uint32_t port_no, cellPadData *data);
extern void sprintf(char *dst, char *fmt, ...);
void memcpy(void* dst, void* src, int size);
void syscall(int syscall, ...);
#define sys_usleep 0x8d
#define sys_sleep  0x08E
#define sys_time_get_current_time 0x91
#define sys_fs_open 0x321
#define sys_fs_read 0x322
#define sys_fs_write 0x323
#define sys_fs_close 0x324
#define sys_fs_opendir 0x325
#define sys_fs_readdir 0x326
#define sys_fs_rmdir 0x32D
#define sys_fs_unlink 0x32e
#define sys_fs_rename 0x32C

// world, state
extern int current_planet;
extern int should_load;
extern int planet_timer;
extern int player_state;
extern int death_count;
extern Vec4 player_coords;

/* variables */
int old_planet;
int old_frame_count;
int old_death_count;

// Copied from savefile helper source lmao im lazy as fuck man
#define api_mod (*(char*)0xD9FF00)
#define api_load (*(char*)0xD9FF01)
#define api_setaside (*(char*)0xD9FF02)
#define api_savefile (*(char*)0xD9FF03)
#define api_loadfile (*(char*)0xD9FF04)

#define save_save ((void*)0x1100000)

#define fastload1 (*(unsigned int*)0x134EBD4)
#define fastload2 (*(short*)0x134EE70)


// A pointer to the current save data info in memory.
#define savedata_info (*((void**)0xCB0A98))
// A pointer path to the current save data buffer.
#define savedata_buf (*(void**)((int)savedata_info + 4))
// Used for loading already-loaded save data.
// Arguments: unk, buf
#define perform_load ((void (*)(int, void*))0x1E1CF4)

int variable_to_set;
int i;
int tempfd;
uint64_t tempnread;

inline void read_temp_file(){
    syscall(sys_fs_open, "/dev_hdd0/game/NPEA00387/USRDIR/tempsave", 0, &tempfd, 0, 0, 0);
    syscall(sys_fs_read, tempfd, save_save, 0x200000, &tempnread);
    syscall(sys_fs_close, tempfd);
}

inline void write_temp_file(){
    syscall(sys_fs_open, "/dev_hdd0/game/NPEA00387/USRDIR/tempsave", 0x241, &tempfd, 0, 0, 0);
    syscall(sys_fs_write, tempfd, savedata_buf, 0x200000, &tempnread);
    syscall(sys_fs_close, tempfd);
}

inline void sfhelper() {
	api_mod = 1;
	// Set aside save file
	if(api_setaside) {
		api_setaside = 0;
		
		for(i = 0; i < 0x200000; i += 0x8000) {
			memcpy((void*)((int)save_save + i), (void*)((int)savedata_buf + i), 0x8000);
		}
	}
	if(api_loadfile){
		read_temp_file();
		api_loadfile = 0;
	}
	if(api_savefile){
		write_temp_file();
		api_savefile = 0;
	}
	if(api_load == 1) {
		// Check if set aside savadata buffer is empty and set aside file
		if(*(int*)save_save == 0) {
			for(i = 0; i < 0x200000; i += 0x8000) {
				memcpy((void*)((int)save_save + i), (void*)((int)savedata_buf + i), 0x8000);
			}
		}
		// Load set aside savedata buffer
		perform_load(0, save_save);
		api_load = 2;
		
		planet_timer = 0;
	}
}

inline void set_fast_loads() {
	if(api_load == 2)
	{
		if(planet_timer > 30)
		{
			fastload1 = 3;
			fastload2 = 0x0101;
			api_load = 0;
		}
	}
}

typedef struct TrophyEntry {
    int32_t id;
    int32_t unlocked;
} TrophyEntry;

typedef struct TrophyMgr {
    uint32_t unk_0;
    int32_t unk_4;
    TrophyEntry trophies[33];
    uint8_t unk_110[0x80];
    int32_t unk_190;
    uint8_t unk_194[0x18];
    int32_t context; // 0x1ac
    int32_t handle;  // 0x1b0
    int32_t trophyCount; 
} TrophyMgr;

extern TrophyMgr* mgr;
extern void NPTrophyRegister(TrophyMgr* mgr);
extern void NPTrophyShutdown(TrophyMgr* mgr);

void hook() {
	sfhelper();
	set_fast_loads();
	
	if(variable_to_set == 1) {
		variable_to_set = 0;
		NPTrophyShutdown(mgr);
        for(int i = 0; i < mgr->trophyCount; i++) {
            mgr->trophies[i].unlocked = 1;
        }
		mgr->trophies[0x20].unlocked = 0; // keep the temperature low
		mgr->trophies[0x19].unlocked = 0; // it was a very good year
		mgr->trophies[0x0A].unlocked = 0; // ry3no
		
		syscall(sys_sleep, 1);
		NPTrophyRegister(mgr);
		syscall(sys_sleep, 1);
		NPTrophyShutdown(mgr);
		syscall(sys_sleep, 1);
		NPTrophyRegister(mgr);
	}
}
