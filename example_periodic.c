/* 
	Compilation flags need to be changed in the /liblitmus/inc/config.makefile
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
/*
 * The main header to include for functionality specific to LITMUSRT is
 * litmus.h, which provided by liblitmus.
 */
#include <litmus.h>


#define rand ((float) random())/(float)(RAND_MAX)

#define CALL( exp ) do { \
        int ret; \
        ret = exp; \
        if (ret != 0) \
            fprintf(stderr, "%s failed: %m\n", #exp); \
        else \
            fprintf(stderr, "%s ok.\n", #exp); \
    } while(0)

#define P_EDF

// Shared variables
static float *baterry[2], *capacity;
unsigned int *miss, *stop, *B;
sem_t *mutex, *mutexMiss;

typedef struct attribute {
    double C, D, P;
    #ifdef P_EDF
        int cpu;
    #endif
} attribute;

// Energy prodoce model
float energy(struct timespec start){
    struct timespec now;
    float a, b;
    unsigned long int s, n;

    clock_gettime(CLOCK_REALTIME, &now);

    s = start.tv_sec; 
    n = now.tv_sec;
    
    //				           0.7*pi 	   	0.1*pi
    a = (float) 10*rand*sin(s/2.19911)*sin(s/0.31415);
    b = (float) 10*rand*sin(n/2.19911)*sin(n/0.31415);

    if(a < 0) a *= (-1);
    if(b < 0) b *=(-1);

    return b-a > 0 ? b-a : 0;
}

#ifdef P_EDF
    void set_atributes(attribute attr[], int index, double C, double D, double P, int cpu)
#else
    void set_atributes(attribute attr[], int index, double C, double D, double P)
#endif
{
    attr[index].C = C; attr[index].D = D; attr[index].P = P;
    #ifdef P_EDF
        attr[index].cpu = cpu;
    #endif
    index++;
}


// Energy consumption model
int job(unsigned int c)
{
    int i;
    for ( i = 0; i < c; i++){

        if(*B == 2){
            sem_wait(mutex);
            if((*baterry[sched_getcpu()] -= *capacity) < 0.0){
            	*baterry[sched_getcpu()] = 0.0;
                sem_post(mutex);
                return 1;
            }
            sem_post(mutex);
        }else{
            sem_wait(mutex);
            
            if((*baterry[0] -= *capacity) < 0.0){
            	*baterry[0] = 0.0;
                sem_post(mutex);
                return 1;
            }
            sem_post(mutex);
        }
    }

    return 0;
}

/*
	Initialize task in real-time mode
*/
#ifdef P_EDF
	void create_rt_task(long long int c, long long int period, long long int deadline, unsigned int cpu)
#else
	void create_rt_task(long long int c, long long int period, long long int deadline)
#endif
{
	/* Used to signal the exit condition and end the loop */
    int do_exit = 0;

    /* Hold all task-related information needed by the kernel */
    struct rt_task params;

    /*
     * Initializes the data structures within liblitmus and is mandatory in
     * order to correctly use liblitmus.
    */
  
    CALL(init_litmus());

    init_rt_task_param(&params);
    params.exec_cost =  ms2ns(c); 
    params.period = ms2ns(period); 
    params.relative_deadline =  ms2ns(deadline);
    params.priority = LITMUS_LOWEST_PRIORITY;

    #ifdef P_EDF
        params.cpu = cpu;
        CALL(be_migrate_to_cpu(params.cpu));
    #endif

    CALL(set_rt_task_param(gettid(), &params));

   
    CALL(task_mode(LITMUS_RT_TASK));

    // Loop for task periodicity in LITMUSRT
    do {
        if(*stop) break;
        sleep_next_period();
        if(*stop) break;

        do_exit = job(c);

    } while(!do_exit);

    struct timespec t; 

    // If task interrupt its executions, so a energy deadline loss happened
    if(do_exit){
        sem_wait(mutexMiss);
        *miss += 1;
        sem_post(mutexMiss);
        printf("(%lld, %lld, %d) miss deadline in time %d:%d\n", c, period, sched_getcpu(), (int)t.tv_sec, (int)t.tv_nsec);
    }

    // Task put itself in convencional mode
    CALL(task_mode(BACKGROUND_TASK));

}

int main(int argc, char *argv[])
{
    unsigned int i, n, p, c;
    #ifdef P_EDF
        unsigned int cpu;
    #endif
    int pid;
    struct timespec start;

    FILE *arq = fopen(argv[1], "wt"); // Log for tests
    
    if (arq == NULL){
    	printf("Missing log file...\n");
    	return 0;
    }

    pid = scanf("%u", &n);
    attribute attr[n];

    // Populate the task set attr by input
    for (i = 0; i < n; i++){
        #ifdef P_EDF
            pid = scanf("%u %u %u", &p, &c, &cpu);
            set_atributes(attr, i, c, p, p, cpu);
        #else
        	pid = scanf("%u %u", &p, &c);
        	set_atributes(attr, i, c, p, p);
        #endif
    }
    

    srandom(time(NULL));


    // Share
    baterry[0] = mmap(NULL, sizeof *baterry, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    baterry[1] = mmap(NULL, sizeof *baterry, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    stop = mmap(NULL, sizeof *stop, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    B = mmap(NULL, sizeof *B, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    miss = mmap(NULL, sizeof *miss, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    capacity = mmap(NULL, sizeof *capacity, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
   
    
    mutex = mmap(NULL, sizeof *mutex, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    mutexMiss = mmap(NULL, sizeof *mutexMiss, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  
    if(sem_init(mutex, 1, 1) < 0){
        printf("erro mutex\n");
    }

    if(sem_init(mutexMiss, 1, 1) < 0){
        printf("erro mutex\n");
    }

    *B = atoi(argv[2]);
    *capacity = atoi(argv[3])/1000.00;
    *stop = *miss = 0;

    // Fully charged
    *baterry[0] = 100.00;
    *baterry[1] = 100.00;
 

 	// Create the task set attrr as real-time
    for(i = 0; i < n; i++){
        if((pid = fork()) == 0){
                pid = 0; 
                #ifdef P_EDF
                	create_rt_task(attr[i].C, attr[i].P, attr[i].D, attr[i].cpu);
                #else
                	create_rt_task(attr[i].C, attr[i].P, attr[i].D);
                #endif
                break;
            }
        }

    clock_gettime(CLOCK_REALTIME, &start);
    	
  
    if(pid > 0){
    	if((pid = fork()) == 0){
    		sleep(595);
    		*stop = 1;
    		return 1;
		}
		while(1){

			/* Energy storage model */

	        sleep(1); //Refresh batteries
	        if(*B == 2){
	        	sem_wait(mutex);
		        if((*baterry[0] += energy(start)) > 100.0){
		            *baterry[0] = 100.0;
		        }
		        if((*baterry[1] += energy(start)) > 100.0){
		            *baterry[1] = 100.0;
		        }
		    }else{
		    	sem_wait(mutex);
		        if((*baterry[0] += energy(start)) > 100.0){
		            *baterry[0] = 100.0;
		        }
		    }
		    sem_post(mutex);
		    
		    clock_gettime(CLOCK_REALTIME, &start);
		    
		    /* LOG TESTS */
		    if(!*stop){		    	
		    	sem_wait(mutex);
		    	if(*B == 2) fprintf(arq, "%f %f %d:%d\n", *baterry[0], *baterry[1], (int) start.tv_sec, (int) start.tv_nsec);
		    	else fprintf(arq, "%f %d:%d\n", *baterry[0], (int) start.tv_sec, (int) start.tv_nsec);
		    	sem_post(mutex);
		    
		    }else{
		    	//store miss
		    	sem_wait(mutexMiss);
		    	fprintf(arq, "Miss = %d\n", *miss);
		    	sem_post(mutexMiss);

		    	fclose(arq);
		    	sem_destroy(mutex);
		    	sem_destroy(mutexMiss);

		    	sleep(10); //Wait script kill all task
		    	break;
		    }
		}
    }

    return 0;
}
