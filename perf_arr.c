#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define N 2 // number of hw events to monitor
#define P 4 // number of processes to spin off
#define L 10000000000 // nuber of silly iterations

int global_sigchld_trip = 0; // catch end child
void sighandler(int);

// function to add perf event to event list
// based on example from:
//     http://web.eece.maine.edu/~vweaver/projects/perf_events/perf_event_open.html
static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
					int cpu, int group_fd, unsigned long flags)
{
	int ret;
	ret = syscall(__NR_perf_event_open, hw_event, 
                    pid, cpu, group_fd, flags);
	return ret;
}

// no function for seeing if we can monitor kernel events
int no_function(int seconds)
{
    sleep(seconds);    
    printf("\n\n Start stress \n\n");
    system("stress -c 4 -t 10");

    return 0;
}



int
main(int argc, char **argv){

    signal(SIGCHLD, sighandler);

	// ARRAY DECS
	int num_events = N;
	uint pe_arr[10] = {
                        PERF_COUNT_HW_CPU_CYCLES,
	                    PERF_COUNT_HW_INSTRUCTIONS,
	                    PERF_COUNT_HW_CACHE_REFERENCES,
	                    PERF_COUNT_HW_CACHE_MISSES,
                        PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
                        PERF_COUNT_HW_BRANCH_MISSES,
                        PERF_COUNT_HW_BUS_CYCLES,
                        //PERF_COUNT_HW_STALLED_CYCLES_FRONTEND,
                        //PERF_COUNT_HW_STALLED_CYCLES_BACKEND,
                        //PERF_COUNT_HW_REF_CPU_CYCLES
                    };
	
    char name_arr[10][50] = {
                            "PERF_COUNT_HW_CPU_CYCLES",
                            "PERF_COUNT_HW_INSTRUCTIONS",
                            "PERF_COUNT_HW_CACHE_REFERENCES",
                            "PERF_COUNT_HW_CACHE_MISSES",
                            "PERF_COUNT_HW_BRANCH_INSTRUCTIONS",
                            "PERF_COUNT_HW_BRANCH_MISSES",
                            "PERF_COUNT_HW_BUS_CYCLES",
                            "PERF_COUNT_HW_STALLED_CYCLES_FRONTEND",
                            "PERF_COUNT_HW_STALLED_CYCLES_BACKEND",
                            "PERF_COUNT_HW_REF_CPU_CYCLES"
                           };
    

	struct perf_event_attr pat_arr[N];
	long long counts[N];
	int fd_arr[N];

	int x; //dummy


	for(int i=0; i<num_events; i++)
	{
		memset(&pat_arr[i], 0, sizeof(struct perf_event_attr));
		pat_arr[i].type = PERF_TYPE_HARDWARE;
		pat_arr[i].size = sizeof(struct perf_event_attr);
		pat_arr[i].config = pe_arr[i];
		pat_arr[i].disabled = 1;
		pat_arr[i].exclude_kernel = 1;
		pat_arr[i].exclude_hv = 1;
		pat_arr[i].inherit = 1;

		if(i==0){fd_arr[i] = perf_event_open(&pat_arr[i],0,-1,-1,0);}
		else{fd_arr[i] = perf_event_open(&pat_arr[i],0,-1,fd_arr[0],0);}
		if (fd_arr[i] == -1){
			fprintf(stderr, "Error opening leader %llx\n", pat_arr[i].config);
			exit(EXIT_FAILURE);
		}
		printf("FD%d: %d \t ITEM: %s\n",i,fd_arr[i], name_arr[i]);
	}

	// reset and enable counters
	for(int i=0; i<num_events; i++){
		ioctl(fd_arr[i], PERF_EVENT_IOC_RESET,0);
		ioctl(fd_arr[i], PERF_EVENT_IOC_ENABLE,0);
	}

/////////////////// ACTION ///////////////////////////

    /*-------- CHILD PROCESS BEING REDORDED -------*/
    pid_t proc = fork();
    if(proc==0){
        printf("entered child process\n");
        int loop = L;
        int no_sleep = 10;
        x = no_function(no_sleep);
        //x = silly_events(loop);
        printf("exiting child process\n");
        return x;
    }
    /*-------- ACTION TAKEN DURRING REDORDING-------*/
    else{
        while(!global_sigchld_trip){
            sleep(1);
            for(int i=0;i<num_events;i++){read(fd_arr[i], &counts[i], sizeof(long long));}
            for(int i=0;i<num_events;i++){ioctl(fd_arr[i], PERF_EVENT_IOC_RESET,0);}
            for(int i=0;i<num_events;i++){printf("%lld %s\t", counts[i], name_arr[i]);}
            printf("\n");
            x = 1;
        }
    }


	for(int i=0;i<num_events;i++){ioctl(fd_arr[i], PERF_EVENT_IOC_DISABLE,0);}
	for(int i=0;i<num_events;i++){read(fd_arr[i], &counts[i], sizeof(long long));}
	for(int i=0;i<num_events;i++){printf("Used %lld %s\t", counts[i], name_arr[i]);}
	for(int i=0;i<num_events;i++){close(fd_arr[i]);}
	return x;
}

void sighandler(int signum) {
   printf("Caught signal %d, coming out...\n", signum);
   global_sigchld_trip = 1;
}
