#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* CITS2002 Project 1 2019
   Name(s):             Misha Soirs ( Theo Andily)
   Student number(s):   22496477 ( 22764884 )
*/


//  besttq (v1.0)
//  Written by Chris.McDonald@uwa.edu.au, 2019, free for all to copy and modify

//  Compile with:  cc -std=c99 -Wall -Werror -o besttq besttq.c


//  THESE CONSTANTS DEFINE THE MAXIMUM SIZE OF TRACEFILE CONTENTS (AND HENCE
//  JOB-MIX) THAT YOUR PROGRAM NEEDS TO SUPPORT.  YOU'LL REQUIRE THESE
//  CONSTANTS WHEN DEFINING THE MAXIMUM SIZES OF ANY REQUIRED DATA STRUCTURES.

#define MAX_DEVICES             4
#define MAX_DEVICE_NAME         20
#define MAX_PROCESSES           50
// DO NOT USE THIS - #define MAX_PROCESS_EVENTS      1000
#define MAX_EVENTS_PER_PROCESS	100

#define TIME_CONTEXT_SWITCH     5
#define TIME_ACQUIRE_BUS        5


//  NOTE THAT DEVICE DATA-TRANSFER-RATES ARE MEASURED IN BYTES/SECOND,
//  THAT ALL TIMES ARE MEASURED IN MICROSECONDS (usecs),
//  AND THAT THE TOTAL-PROCESS-COMPLETION-TIME WILL NOT EXCEED 2000 SECONDS
//  (SO YOU CAN SAFELY USE 'STANDARD' 32-BIT ints TO STORE TIMES).

int optimal_time_quantum                = 0;
int total_process_completion_time       = 0;

//  ----------------------------------------------------------------------

#define CHAR_COMMENT            '#'
#define MAXWORD                 20

char devices[MAX_DEVICES][MAX_DEVICE_NAME];						//DECLARE DEVICES ARRAY[][]
char TR[MAX_DEVICES][MAXWORD];								//DECLARE TRANSFER RATE ARRAY AS TR[][]
int ndev = 0;										//DECLARE NUMBER OF DEVICES AS ndev AND INITIALISE IT TO 0
char process[MAX_PROCESSES][MAXWORD];							//DECLARE PROCESSES AS process[][]			
char PRT[MAX_PROCESSES][MAXWORD];							//DECLARE PROCESS TRANSFER RATE AS PRT[][]
int npro = 0;										//DECLARE NUMBER OF PROCESS AS npro AND INITIALISE IT TO 0
char ERT[MAX_EVENTS_PER_PROCESS][MAXWORD];						//DECLARE EVENT REBOOT TIME AS ERT[][]
char EDN[MAX_EVENTS_PER_PROCESS][MAXWORD];						//DECLARE EVENT DEVICE NAME AS EDN[][]
char TS[MAX_EVENTS_PER_PROCESS][MAXWORD];						//DECLARE SIZE OF TRANSFER AS TS[][]
char exitTime[MAX_PROCESSES][MAXWORD];							//DECLARE EXIT TIME AS exitTime[][]
int neve = 0;										//DECLARE NUMBER OF EVENTS AS neve AND INITIALISE IT TO 0
int eventPerProcess[MAX_EVENTS_PER_PROCESS];
char EXRT[MAX_PROCESSES][MAXWORD];
int m = 0;
int noevenperpro = 0;

void parse_tracefile(char program[], char tracefile[])
{
//  ATTEMPT TO OPEN OUR TRACEFILE, REPORTING AN ERROR IF WE CAN'T
    FILE *fp    = fopen(tracefile, "r");

    if(fp == NULL) {
        printf("%s: unable to open '%s'\n", program, tracefile);
        exit(EXIT_FAILURE);
    }

    char line[BUFSIZ];
    int  lc     = 0;

//  READ EACH LINE FROM THE TRACEFILE, UNTIL WE REACH THE END-OF-FILE
    while(fgets(line, sizeof line, fp) != NULL) {
        ++lc;

//  COMMENT LINES ARE SIMPLY SKIPPED
        if(line[0] == CHAR_COMMENT) {
            continue;
        }

//  ATTEMPT TO BREAK EACH LINE INTO A NUMBER OF WORDS, USING sscanf()
        char    word0[MAXWORD], word1[MAXWORD], word2[MAXWORD], word3[MAXWORD];
        int nwords = sscanf(line, "%s %s %s %s", word0, word1, word2, word3);

//      printf("%i = %s", nwords, line);

//  WE WILL SIMPLY IGNORE ANY LINE WITHOUT ANY WORDS
        if(nwords <= 0) {
            continue;
        }

//  LOOK FOR LINES DEFINING DEVICES, PROCESSES, AND PROCESS EVENTS
        if(nwords == 4 && strcmp(word0, "device") == 0) {
	 	// FOUND A DEVICE DEFINITION, WE'LL NEED TO STORE THIS SOMEWHERE
        	strcpy(devices[ndev], word1);						// PLACING DEVICES NAME INTO device[][]
		strcpy(TR[ndev],  word2);						// PLACING TRANSFER RATE INTO transferRate[][]
	    	++ndev;
        }
	else if(nwords == 1 && strcmp(word0, "reboot") == 0) {            
	    ;   // NOTHING REALLY REQUIRED, DEVICE DEFINITIONS HAVE FINISHED
        }
        else if(nwords == 4 && strcmp(word0, "process") == 0) {
                // FOUND THE START OF A PROCESS'S EVENTS, STORE THIS SOMEWHERE
		strcpy(process[npro], word1); 						// PLACING PROCESS NUMBER INTO  process[] 
		strcpy(PRT[npro], word2);						// PLACING REBOOT TIME INTO PRT[]
	    	++npro;
	}   
        else if(nwords == 4 && strcmp(word0, "i/o") == 0) {
	    	//  AN I/O EVENT FOR THE CURRENT PROCESS, STORE THIS SOMEWHERE
	    	strcpy(ERT[neve], word1);						//PLACING EVENT REBOOT TIME INTO ERT[]
	    	strcpy(EDN[neve], word2);						//PLACING EVENT DEVICES NAME INTO EDN[]
	    	strcpy(TS[neve], word3);						//PLACING SIZE OF TRANSFER INTO TS[]
	    	neve++;
		++noevenperpro;
	}
        else if(nwords == 2 && strcmp(word0, "exit") == 0) {
            	//  PRESUMABLY THE LAST EVENT WE'LL SEE FOR THE CURRENT PROCESS
	    	strcpy(EXRT[neve], word1);	    					//PLACING ENDING TIME INTO exitTime[][]
		noevenperpro++;
	    	++neve;
		eventPerProcess[m] = noevenperpro;
		m++;
		noevenperpro = 0;
        }
        else if(nwords == 1 && strcmp(word0, "}") == 0) {
   //  JUST THE END OF THE CURRENT PROCESS'S EVENTS
		;
        }
        else {
            printf("%s: line %i of '%s' is unrecognized",
                        program, lc, tracefile);
            exit(EXIT_FAILURE);
        }
	
    }/*    
    for(int i = 0; i < MAX_DEVICES; i++)
    {
	printf("devices %s\n",devices[i]);
    }*/
//    printf("number of process: %d\n", npro);
    fclose(fp);
}

#undef  MAXWORD
#undef  CHAR_COMMENT

//  ----------------------------------------------------------------------

//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, FOR THE GIVEN TIME-QUANTUM



int readyQueue[MAX_PROCESSES];							//DECLARE READY QUEUE as readyQueue[]
int running[1] = {0};								//DECLARE RUNNING STATE  as running[							
char IO[1][20];									//FOR EACH ROW IS AN I/O AND IT IS ORDERED BY TRANSFER RATE
char CPU[1][20];								//DECLARE A CPU ARRAY FOR EACH I/O IN THE CPU
int TOMICRO = 1000000;
char BUS[MAX_EVENTS_PER_PROCESS][20];
int OTR[MAX_DEVICES];
int location = 1;
char ODN[MAX_DEVICES][20];
int TIME = 0;
int TOTALCOMPLETIONTIME[100000];
int z = 0;
int PTIME = 0;


int maximum(int size)
{
    int max = atoi(TR[0]);
    for(int i = 1; i <= size; i++)
    {
	if(atoi(TR[i]) > max)
	{
	    max = atoi(TR[i]);
	}
    }
    return max;
}

int locationOfMax(int size)
{
    int location = 0;
    int maximum = atoi(TR[0]);
    for (int i  = 1; i < size;i++)
    {
	if (atoi(TR[i]) > maximum)
	{
	    maximum  = atoi(TR[i]);
	    location = i;
	}
    }
    return location;
}

void OrderTransferRate(void)
{
    int size = sizeof(devices) / sizeof(devices[0]);
    int location;
    for(int i = 0; i < size; i++)
    {
	OTR[i] = maximum(size);
	location = locationOfMax(size);
	strcpy(ODN[i], devices[location]);
	strcpy(TR[location], "0");
    }
}

void placeInReadyQueue(void)
{
    for(int m = 0; m < MAX_PROCESSES; m++)
    {
	int processes = atoi(process[m]);
	readyQueue[m] =  processes;				
    }
}
void addToRunning(void)
{
    running[0] = readyQueue[0];
}

void addToCpu(char currentEvent[20])
{
    strcpy(CPU[0],  currentEvent);
}

int calculations(char name[20])
{
    int time = 0;
    for(int i = 0; i < sizeof(devices); i++)
    {
	
	if(strcmp(name, devices[i]) == 0)
	{
	    time = time + ceil((atoi(TS[i]) / (atoi(TR[i]) / TOMICRO)));
	}
    }
    return time;
}

int addToBus(char currentEvent[20])
{
    strcpy(BUS[0], currentEvent);
    int time = calculations(currentEvent);
    return time;
}

void addToIO(char currentEvent[20])
{
    strcpy(IO[0], currentEvent);
}

void simulate_job_mix(int time_quantum)
{
    while(npro > 0)											//WHILE THERE IS A PROCESS 
	{
	    printf("number of process: %d\n", npro);
	    placeInReadyQueue();						        		//PLACE PROCESSES INTO READY QUEUE ASSUMING THEY ARE IN ORDER
	    int j = 0;
	    for(int i = 0; i < npro; i++)								//FOR EACH PROCESS
	    {
		j = i;	    
		printf("event per process: %d\n", eventPerProcess[i]);
//		if(eventPerProcess[i] > 0)								//IF THERE IS AN EVENT AND IF PROCESS TIME < TIME QUANTUM
//		{
		    if(running[0] == 0)									//PLACE INTO RUNNING IF NOTHING IS RUNNING
		    {
			addToRunning();
			if(eventPerProcess[i] > 1 && PTIME < time_quantum)
			{
			/*	for(int i = 0; i < atoi(EXRT[i]) / time_quantum; i++)
				{
					PTIME = PTIME + TIME_CONTEXT_SWITCH;				//ADD TIME CONTEXT SWITCH TO THE PROCESS TIME
				}
			*/	printf("TIME : %d\n", PTIME);
				PTIME = PTIME + TIME_CONTEXT_SWITCH;
			}
			else if(eventPerProcess[i] == 1 && npro == 1 && PTIME < time_quantum)
			{
				PTIME = PTIME + TIME_CONTEXT_SWITCH;
				printf("TIME : %d\n", PTIME);

			}
			else
			{
			//	PTIME = PTIME + TIME_CONTEXT_SWITCH;
				printf("%d    %d\n",atoi(EXRT[i]), time_quantum);
				float size = ceil(atof(EXRT[i]) / time_quantum);
				printf("%f\n", size);
				int k = 0;
				while(k < size)
				{
					PTIME = PTIME + TIME_CONTEXT_SWITCH;				//ADD TIME CONTEXT SWITCH TO THE PROCESS TIME
					k = k  + 1;
				}
				printf("TIME : %d\n", PTIME);

			}
			for(int i = 0; i < MAX_EVENTS_PER_PROCESS; i++)					//CHECK IF THERE IS MORE THAN ONE EVENT THEN ORDER THE DEVICE NAME
			{
			    if(eventPerProcess[i] > 1)
			    {
				OrderTransferRate();							//ORDER THE TRANFER RATE IN DESC ORDER
				break;
			    }
			}
			for(int l = 0; l <eventPerProcess[i] ; l++)					//FOR EACH EVENTS IN EACH PROCESS
			{
			    printf("event per process: %d\n", eventPerProcess[i]);
			    if(eventPerProcess[i] == 1)							//IF THERE IS ONLY ONE EVENT
			    {
				if(atoi(EXRT[i]) > time_quantum)
				{
					printf("%d\n", i);
					PTIME = PTIME + atoi(EXRT[i]);
					printf("TIME : %d\n", PTIME);
					running[0] = 0;
				}
			        else
				{	
					PTIME = PTIME +  atoi(EXRT[i]);						//ADD EXIT TIME TO PROCESS TIME
					printf("TIME : %d\n", PTIME);
					running[0] = 0;
				}
			    }
		            else
			    {
				if(l == 0)								//IF IT IS THE FIRST EVENT
				{
    				    if(strcmp(EDN[l],ODN[l]) == 0)					//CHECK FOR THE EVENT NAME AGAINST THE ORDERED NAME
				    {
					addToCpu(EDN[l]);						//PLACE THE EVENT NAME INTO THE CPU
					PTIME = PTIME + atoi(ERT[l]);					//ADD CPU TIME TO PROCESS TIME
					strcpy(CPU[0], "");						//CLEAR THE CPU						
					addToIO(EDN[l]);						//PLACE THE CURRENT EVENT NAME TO I/O QUEUE
					PTIME = PTIME + addToBus(EDN[l]);				//ADD FILE TRANSFER TIME TO PROCESS TIME
					PTIME = PTIME + TIME_ACQUIRE_BUS;				//ADD TIME TO ACQUIRE BUS TO THE PROCESS TIME
					strcpy(IO[0], "");						//CLEAR I/O QUEUE
					running[0] = 0;							//CLEAR RUNNING QUEUE
					printf("TIME : %d\n", TIME);
				    }
				}
				else									//IF IT IS NOT THE FIRST EVENT IN PROCESS
				{
				    if(strcmp(EDN[l],ODN[l]) == 0)					//CHECK FOR THE EVENT NAME AGAINST THE ORDERED NAME 
				    {
					addToCpu(EDN[l]);						//PLACE THE EVENT NAME INTO THE CPU
					PTIME = PTIME + (atoi(ERT[l]) - atoi(ERT[l-1]));		//ADD CPU TIME TO PROCESS TIME
					strcpy(CPU[0], "");						//CLEAR THE CPU
					addToIO(EDN[l]);						//PLACE THE CURRENT EVENT NAME TO I/O QUEUE
					PTIME = PTIME + addToBus(EDN[l]);				//ADD FILE TRANSFER TIME TO PROCESS TIME
					PTIME = PTIME + TIME_ACQUIRE_BUS;				//ADD TIME TO ACQUIRE BUS TO THE PROCESS TIME
					strcpy(IO[0], "");						//CLEAR I/O QUEUE
					running[0] = 0;							//CLEAR RUNNING QUEUE
				//	printf("TIME : %d\n", TIME);
				    }
				}
				PTIME = PTIME + atoi(ERT[i]);
			//	printf("TIME : %d\n", TIME);
			    }
			    PTIME = PTIME + atoi(ERT[i]);
		//	    printf("TIME : %d\n", TIME);
			}
		    }
		    else
		    {
	//		PTIME = PTIME + atoi(ERT[0]);
			printf("TIME : %d\n", PTIME);
		    }
		    npro = npro - 1;
	    
	    }
	    printf("%d\n", npro); 
	    if(PTIME < atoi(PRT[j+1]))
	    {
		    PTIME = atoi(PRT[j+1]) - atoi(PRT[j]);
	    }
	}
    	printf("TIME : %d\n", PTIME);  
        printf("running simulate_job_mix( time_quantum = %i usecs )\n", time_quantum);
}


//  ----------------------------------------------------------------------

void usage(char program[])
{
    printf("Usage: %s tracefile TQ-first [TQ-final TQ-increment]\n", program);
    exit(EXIT_FAILURE);
}

int main(int argcount, char *argvalue[])
{
    int TQ0 = 0, TQfinal = 0, TQinc = 0;

//  CALLED WITH THE PROVIDED TRACEFILE (NAME) AND THREE TIME VALUES

    if(argcount == 5) {
        TQ0     = atoi(argvalue[2]);
        TQfinal = atoi(argvalue[3]);
        TQinc   = atoi(argvalue[4]);

        if(TQ0 < 1 || TQfinal < TQ0 || TQinc < 1) {
            usage(argvalue[0]);
        }
    }
//  CALLED WITH THE PROVIDED TRACEFILE (NAME) AND ONE TIME VALUE
    else if(argcount == 3) {
        TQ0     = atoi(argvalue[2]);
        if(TQ0 < 1) {
            usage(argvalue[0]);
        }
        TQfinal = TQ0;
        TQinc   = 1;
    }
//  CALLED INCORRECTLY, REPORT THE ERROR AND TERMINATE
    else {
        usage(argvalue[0]);
    }

//  READ THE JOB-MIX FROM THE TRACEFILE, STORING INFORMATION IN DATA-STRUCTURES
    parse_tracefile(argvalue[0], argvalue[1]);
	
//  SIMULATE THE JOB-MIX FROM THE TRACEFILE, VARYING THE TIME-QUANTUM EACH TIME.
//  WE NEED TO FIND THE BEST (SHORTEST) TOTAL-PROCESS-COMPLETION-TIME
//  ACROSS EACH OF THE TIME-QUANTA BEING CONSIDERED
    int TOTALCOMPLETIONTIME[TQfinal/TQinc];
    int tq[TQfinal/TQinc];
    int i = 0;
    for(int time_quantum=TQ0 ; time_quantum<=TQfinal ; time_quantum += TQinc) {
        simulate_job_mix(time_quantum);
	TOTALCOMPLETIONTIME[i] =  PTIME;
	tq[i] = time_quantum;
	i = i + 1;
    }
    if(TQfinal == TQ0  && TQinc == 1)
    {
	total_process_completion_time = total_process_completion_time + PTIME;
	optimal_time_quantum = optimal_time_quantum + TQ0;
    }
    else
    {
	int minimum = TOTALCOMPLETIONTIME[0];
	int location = 0;
	for(int i  = 1; i < (sizeof(TOTALCOMPLETIONTIME) / sizeof(TOTALCOMPLETIONTIME[0])); i++)
	{
	    if(minimum == TOTALCOMPLETIONTIME[i])
	    {
		    minimum = TOTALCOMPLETIONTIME[i];;
		    for(int i =0; i < sizeof(tq) / sizeof(tq[0]); i++)
		    {
			    if(tq[i] == TQfinal)
			    {
				    location = i;
				    break;
			    }
		    }
		    break;
	    }
	    else
	    {
		    minimum = TOTALCOMPLETIONTIME[TQfinal];
		    location  = i + 1;
		    break;
	    }  
	}
	total_process_completion_time = minimum;
	optimal_time_quantum = tq[location];
    }

//  PRINT THE PROGRAM'S RESULT

    printf("best %i %i\n", optimal_time_quantum, total_process_completion_time);

    exit(EXIT_SUCCESS);
}

//vim: ts=8 sw=4
