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

#define N 5 // number of hw events to monitor
#define M 2 // number of hw events to monitor

// setup to abort main process when child process returns
int global_sigchld_trip = 0; // catch end child
void sighandler(int signum) {
   printf("Caught signal %d, coming out...\n", signum);
   global_sigchld_trip = 1;
}

// function to add perf event to event list
// based on example from:
//     http://web.eece.maine.edu/~vweaver/projects/perf_events/perf_event_open.html
static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
					int cpu, int group_fd, unsigned long flags){
	int ret;
	ret = syscall(__NR_perf_event_open, hw_event, 
                    pid, cpu, group_fd, flags);
	return ret;
}

// no function for seeing if we can monitor kernel events
int no_function(int seconds){
    //sleep(seconds);    
    printf("\n\n Start stress \n\n");
    system("stress -d 20 -c 4 -t 30");
    return 0;
}



int main(int argc, char **argv){

    // setup bail on child return
    signal(SIGCHLD, sighandler);

    // Setup for reading system information
    FILE *temp, *freq; // linux virtual files
    char temp_buff[10]; // reading temp
    char freq_buff[20]; // reading proc freq

    // output csv file to store collected data
    FILE *output;

    // declare hardware events that can be monitored
	int num_hw_events = N;
	uint pe_hw[7] = {
                        PERF_COUNT_HW_CPU_CYCLES,
	                    PERF_COUNT_HW_INSTRUCTIONS,
	                    PERF_COUNT_HW_CACHE_MISSES,
                        PERF_COUNT_HW_BRANCH_MISSES,
                        PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
                        //----------------------------//
	                    PERF_COUNT_HW_CACHE_REFERENCES,
                        PERF_COUNT_HW_BUS_CYCLES,
                    };
	
    char hw_name_arr[7][50] = {
                            "PERF_COUNT_HW_CPU_CYCLES",
                            "PERF_COUNT_HW_INSTRUCTIONS",
                            "PERF_COUNT_HW_CACHE_MISSES",
                            "PERF_COUNT_HW_BRANCH_MISSES",
                            "PERF_COUNT_HW_BRANCH_INSTRUCTIONS",
                            //---------------------------------//
                            "PERF_COUNT_HW_CACHE_REFERENCES",
                            "PERF_COUNT_HW_BUS_CYCLES",
                           };
                           

    // declare hardware cache events that can be monitored
 	int num_hw_cache_events = M;
	uint pe_hw_cache[12] = {
                    (PERF_COUNT_HW_CACHE_LL) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
                    (PERF_COUNT_HW_CACHE_L1D) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16), 
                    //----------------------------------------------------//
                    (PERF_COUNT_HW_CACHE_LL) | (PERF_COUNT_HW_CACHE_OP_WRITE<< 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
                    (PERF_COUNT_HW_CACHE_L1I) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
                    (PERF_COUNT_HW_CACHE_BPU) | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
                    (PERF_COUNT_HW_CACHE_L1D) | (PERF_COUNT_HW_CACHE_OP_WRITE<< 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
                    (PERF_COUNT_HW_CACHE_BPU) | (PERF_COUNT_HW_CACHE_OP_WRITE<< 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16),
                    };
	
    char hw_cache_name_arr[12][50] = {
                    "PERF_COUNT_HW_CACHE_LL_read_miss",
                    "PERF_COUNT_HW_CACHE_L1D_read_miss",
                    //-------------------------------//
                    "PERF_COUNT_HW_CACHE_LL_write_miss",
                    "PERF_COUNT_HW_CACHE_L1I_read_miss",
                    "PERF_COUNT_HW_CACHE_BPU_read_miss",

                    "PERF_COUNT_HW_CACHE_L1D_write_miss",
                    "PERF_COUNT_HW_CACHE_BPU_write_miss",
                    };

    // hols all perf events to be added to the event list
	struct perf_event_attr pat_arr[num_hw_events+num_hw_cache_events];

    // holds return from counter values
	long long counts[num_hw_events+num_hw_cache_events];

    // file descriptors that point to the individual events
	int fd_arr[num_hw_events+num_hw_cache_events];


    // initialize hw events
	for(int i=0; i<num_hw_events; i++)
	{
		memset(&pat_arr[i], 0, sizeof(struct perf_event_attr));
		pat_arr[i].type = PERF_TYPE_HARDWARE;
		pat_arr[i].size = sizeof(struct perf_event_attr);
		pat_arr[i].config = pe_hw[i];
        if(i==0){pat_arr[i].disabled = 1;}
        else{pat_arr[i].disabled = 0;}
		pat_arr[i].exclude_kernel = 1;
		pat_arr[i].exclude_hv = 1;
		pat_arr[i].inherit = 1;

		if(i==0){fd_arr[i] = perf_event_open(&pat_arr[i],0,-1,-1,0);}
		else{fd_arr[i] = perf_event_open(&pat_arr[i],0,-1,fd_arr[0],0);}
		if (fd_arr[i] == -1){
			fprintf(stderr, "Error opening leader %llx\n", pat_arr[i].config);
			exit(EXIT_FAILURE);
		}
		printf("FD%d: %d \t ITEM: %s\n",i,fd_arr[i], hw_name_arr[i]);
	}

    // initialize hw cache events
	for(int i=0; i<num_hw_cache_events; i++)
	{
		memset(&pat_arr[i+num_hw_events], 0, sizeof(struct perf_event_attr));
		pat_arr[i+num_hw_events].type = PERF_TYPE_HW_CACHE;
		pat_arr[i+num_hw_events].size = sizeof(struct perf_event_attr);
		pat_arr[i+num_hw_events].config = pe_hw_cache[i];
        if(i+num_hw_events==0){printf("dis=1");pat_arr[i+num_hw_events].disabled = 1;}
        else{printf("dis=0");pat_arr[i+num_hw_events].disabled = 0;}
		pat_arr[i+num_hw_events].exclude_kernel = 1;
		pat_arr[i+num_hw_events].exclude_hv = 1;
		pat_arr[i+num_hw_events].inherit = 1;

		if(i+num_hw_events==0){fd_arr[i+num_hw_events] = perf_event_open(&pat_arr[i+num_hw_events],0,-1,-1,0);}
		else{fd_arr[i+num_hw_events] = perf_event_open(&pat_arr[i+num_hw_events],0,-1,fd_arr[0],0);}
		if (fd_arr[i+num_hw_events] == -1){
            printf("\ni: %d\nnhe:%d\n",i,num_hw_events);
			fprintf(stderr, "Error opening leader %llx\n", pat_arr[i+num_hw_events].config);
			exit(EXIT_FAILURE);
		}
		printf("FD%d: %d \t ITEM: %s\n",i+num_hw_events,fd_arr[i+num_hw_events], hw_cache_name_arr[i]);
	}


	// reset and enable counters
	for(int i=0; i<num_hw_events+num_hw_cache_events; i++){
		ioctl(fd_arr[i], PERF_EVENT_IOC_RESET,0);
		ioctl(fd_arr[i], PERF_EVENT_IOC_ENABLE,0);
	}

/////////////////// ACTION ///////////////////////////

    /*-------- CHILD PROCESS BEING REDORDED -------*/
    printf("\nSHOULD FORK RIGHTE HERE\n");
    pid_t proc = fork();
    if(proc==0){
        printf("entered child process\n");
        int no_sleep = 1;
        no_function(no_sleep);
        printf("exiting child process\n");
        return 0;
    }
    /*-------- ACTION TAKEN DURRING REDORDING-------*/
    else{

        // output file header
        printf("\n\n\nWRITING HEADER\n\n\n");
        output = fopen("output.txt","w+");
        fprintf(output, "temp,");
        for(int i=0;i<num_hw_events;i++){
            fprintf(output, hw_name_arr[i]);
            if(i+1==num_hw_events+num_hw_cache_events){break;}
            fprintf(output, ",");
        }
        for(int i=0;i<num_hw_cache_events;i++){
            fprintf(output,hw_cache_name_arr[i]);
            if(i+1==num_hw_cache_events){break;}
            fprintf(output, ",");
        }
        fprintf(output,"\n");
        fclose(output);
        

        int loop_counter = 0;
        while(!global_sigchld_trip){
            sleep(1);
            temp = fopen("/sys/class/thermal/thermal_zone0/temp","r");
            freq = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq","r");
            for(int i=0;i<num_hw_events+num_hw_cache_events;i++){read(fd_arr[i], &counts[i], sizeof(long long));}
            for(int i=0;i<num_hw_events+num_hw_cache_events;i++){ioctl(fd_arr[i], PERF_EVENT_IOC_RESET,0);}
            fscanf(temp, "%s", temp_buff);
            fscanf(freq, "%s", freq_buff);
            int cur_temp = atoi(temp_buff);
            if(cur_temp > 80000){
                printf("\n\nEXIT TEMP: %d\n", cur_temp);
                printf("\n\nRUNTIME: %d seconds\n\n", cur_temp);
                kill(0, SIGINT);
                exit(0);
            }
            fclose(temp);
            fclose(freq);
            //fgets(buff, 255, (FILE*)fp);
            for(int i=0;i<num_hw_events;i++){printf("%lld %s\t\n", counts[i], hw_name_arr[i]);}
            printf("--------------------------------------\n");
            for(int i=0;i<num_hw_cache_events;i++){printf("%lld %s\t\n", counts[i+num_hw_events], hw_cache_name_arr[i]);}
            printf("--------------------------------------\n");
            printf("temperature: %s\n", temp_buff);
            printf("frequency: %s\n", freq_buff);
            printf("\n\n");

            // write out line
            output = fopen("output.txt","a");
            fprintf(output,"%s,",temp_buff);
            for(int i=0; i<num_hw_events+num_hw_cache_events; i++){
                fprintf(output, "%lld", counts[i]);
                if(i+1==num_hw_events+num_hw_cache_events){break;}
                fprintf(output, ",");
            }
            fprintf(output,"\n");
            fclose(output);
            loop_counter++;

        }

    }


	for(int i=0;i<num_hw_events+num_hw_cache_events;i++){ioctl(fd_arr[i], PERF_EVENT_IOC_DISABLE,0);}
	for(int i=0;i<num_hw_events+num_hw_cache_events;i++){read(fd_arr[i], &counts[i], sizeof(long long));}
	for(int i=0;i<num_hw_events;i++){printf("Used %lld %s\t", counts[i], hw_name_arr[i]);}
	for(int i=0;i<num_hw_cache_events;i++){printf("Used %lld %s\t", counts[i+num_hw_events], hw_cache_name_arr[i]);}
	for(int i=0;i<num_hw_events;i++){close(fd_arr[i]);}
	return 0;
}

