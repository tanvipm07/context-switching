#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>         /* For syscall, pipe, fork, write, & read */
#include <sys/types.h>      /* For syscall */
#include <sys/time.h>       /* For gettimeofday */
#include <sched.h>          /* For CPU_ZERO, CPU_SET, & sched_setaffinity() */
#include <string.h>         /* For strlen */

// Number of times (loops) to complete the context switch
#define NUM 5

int main()
{
    // Declare set of active CPUs
    cpu_set_t mask;
    // Clear set of active CPUs
    CPU_ZERO(&mask);
    // Add a CPU to the active set
    CPU_SET(0, &mask);

    //Bind PID to the active CPU set
    if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &mask))
    {
        fprintf(stderr, "sched_setaffinity error\n");
        exit(EXIT_FAILURE);
    }

    // Declare pipes
    int p1[2];
    int p2[2];
    int p3[2];

    // Declare messages & buffers
    char * msg1 = "Message from parent";
    char * msg2 = "Message from child";
    char inbuf1[strlen(msg1) + 1];
    char inbuf2[strlen(msg2) + 1];

    // Declare time variables
    struct timeval start, finish;
    unsigned long elapsed;
    int t = sizeof(struct timeval);

    /* struct timeval{
        long tv_sec;
        long tv_usec;
    }; */
    
    // Loop counter
    int i;

    // Declare pid
    int pid = -1;

    // Create & check the first pipe
    if (pipe(p1) < 0)
    {
        fprintf(stderr, "Parent failed to create first pipe\n");
        exit(EXIT_FAILURE);
    }

    // Create & check the second pipe
    if (pipe(p2) < 0)
    {
        fprintf(stderr, "Parent failed to create second pipe\n");
        exit(EXIT_FAILURE);
    }

    // Create & check the pipe that will keep the time
    if (pipe(p3) < 0)
    {
        fprintf(stderr, "Parent failed to create third pipe\n");
        exit(EXIT_FAILURE);
    }

    // Create & check the fork
    if ((pid = fork()) < 0)
        perror("fork");
    else if (pid == 0)
    // Child process
    {
       // printf("Child pid = %d\n", getpid());

        // Read & write a string to each pipe
        for (i = 0; i < NUM; i++)
        {
            read(p1[0], inbuf1, sizeof(inbuf1));
            printf("Message 1: %s %d\n", inbuf1, (NUM - i));
            write(p2[1], msg2, strlen(msg2) + 1);
        }
        // Stop the timer
        gettimeofday(&finish, NULL);

        // Write & check the stop time to the third pipe
        if (write(p3[1], &finish, sizeof(struct timeval)) != t)
        {
            fprintf(stderr, "Child failed to write to third pipe\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    // Parent Process
    {
        //printf("Parent pid = %d\n", getpid());

        // Start the timer
        gettimeofday(&start, NULL);

        // Read & write a string to each pipe
        for (i = 0; i < NUM; i++)
        {
            write(p1[1], msg1, strlen(msg1) + 1);
            read(p2[0], inbuf2, sizeof(inbuf2));
            printf("Message 2: %s\n", inbuf2);
        }
        

        // Read & check the stop time from the third pipe
       if (read(p3[0], &finish, sizeof(struct timeval)) != t)
        {
            fprintf(stderr, "Parent failed to read from third pipe\n");
            exit(EXIT_FAILURE);
        }

        //printf("Finish: %lu\n",finish.tv_usec);
        //printf("Start: %lu\n",start.tv_usec);

        // Determine the time to complete the total processes
        elapsed = (finish.tv_sec - start.tv_sec) * 1000000;
        elapsed += (finish.tv_usec - start.tv_usec);

        // Print the average time taken to complete one context switch
        printf("Average time for a context switch = %f microseconds\n",
               (float) elapsed / (float) (NUM * 2));
    }
    
    return 0;
}
