#include "common.h"


#define KILL_ADDR 0x48310038UL			// Don't change this boosheet either. Address for the kill switch.


//Any one who changes these macros will be fucking eviscerated
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
__LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)
//end death threat

int fd;
off_t target;
void *map_base;
int kill_value;


void kill_switch::operator()() {
	
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	target = KILL_ADDR;
	


	
	//printf("/dev/mem opened.\n"); 
	
	while (true) {
		kill_value = ((read_mem()) & 0x80);	//if 0 then killed, if 1 then not killed
		
		if (!kill_value) {
			isRunning = false;
			hasBeenKilled = true;
		}
		else{
			isRunning = true;
		}
		system("sleep 0.25");			//DO NOT MAKE THIS sleep(0.25); The sleep command doesn't like decimal seconds
										//making the program take about 90% CPU usage.
	}
}


// Rehashed version of devmem2 (command line tool for reading memory)
// It's a quick & dirty way to read the kill switch without recompiling the kernel.
unsigned long read_mem () {
    void *virt_addr;
	unsigned long read_result;

    //fflush(stdout);
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) FATAL;

    //printf("Memory mapped at address %p.\n", map_base); 
    //fflush(stdout);
    virt_addr = (void*)((unsigned long)map_base + (target & MAP_MASK));  //compiler doesnt like void* arithmetic so map_base is typecasted into a ul which is adjusted and made back into a void*
	read_result = *((unsigned long *) virt_addr);
	
    //printf("Value at address 0x%X (%p): 0x%X\n", target, virt_addr, read_result);
	
    //fflush(stdout);
	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
    //close(fd);
	return read_result;
	
}
