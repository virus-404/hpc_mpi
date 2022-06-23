//
//  mandelbrot.c
//  
//
//  The Mandelbrot calculation is to iterate the equation
//  z = z*z + c, where z and c are complex numbers, z is initially
//  zero, and c is the coordinate of the point being tested. If
//  the magnitude of z remains less than 2 for ever, then the point
//  c is in the Mandelbrot set. In this code We write out the number of iterations
//  before the magnitude of z exceeds 2, or UCHAR_MAX, whichever is
//  smaller.//
//
//

#include <stdio.h>

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <omp.h>
#include <mpi.h>

// Structure to keep the color, it was already done
struct colour {
    int red, green, blue;
};

// Structure to keep the real and imaginary part, makes the code cleaner and easier to understand
struct complex {
    double real, imag;
};

void color(int red, int green, int blue)
{
    fputc((char)red, stdout);
    fputc((char)green, stdout);
    fputc((char)blue, stdout);
}

int h_final(int rank, int numtasks, int o_h){
    if(rank != numtasks-1)
    {
        return (o_h/(numtasks-1))*rank;
    }
    else
    {
        return o_h;
    }
}

int h_init(int rank, int numtasks, int o_h){
    if(rank > 1)
    {
        return ((o_h/(numtasks-1))*(rank-1));
    }
    else
    {
        return 0;
    }
}


int main(int argc, char *argv[]) 
{
    // Getting parameters from command line execution, else we finish the program.
    int numtasks, rank;
    int w, o_h, maxIterations = 0;
    if(argc == 4)
    {
        w = atoi(argv[1]);
        o_h = atoi(argv[2]);
        maxIterations = atoi(argv[3]);
    }
    else {
        printf("Execution of the program must be ./program width heigt maxIterations ");
        return 1;
    }
    double start_time, elapsed_time;
    double zoom = 1, moveX = -0.5, moveY = 0;
    
    // We have move these operations outside as they will no change during the execution
    // Makes the code cleaner and easier to understand
    double realDenom = (0.5 * zoom * w);
    double imagDenom = (0.5 * zoom * o_h);

    MPI_Init (&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);   // get number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);       // get current process id
    
    /*
    Serializable struct for MPI
    */
    const int nitems = 3;
    int blocklengths[3] = {1, 1, 1};
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Datatype mpi_colour_type;
    const MPI_Aint offsets[5] = {0, sizeof(int), sizeof(int)*2};

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_colour_type);
    MPI_Type_commit(&mpi_colour_type);

    if(rank == 0){
        start_time = MPI_Wtime();
        printf("P6\n# CREATOR: Eric R. Weeks MODIFIED BY: Jordi R. Onrubia and Marcel Ortiz/ mandel program\n");
        printf("%d %d\n255\n",w, o_h);
        MPI_Status status;
        int count;
        fprintf(stderr, "H: %d W: %d Iteratiosn: %d NumProc %d \n", o_h, w, maxIterations, numtasks);
        /*
        * The Master will wait for each worker to send its results, results are printed after recieving them
        * as this version is static we do not need to send each time the next batch to process we can just print them
        * in order they are received
        */
        for(count=1; count<numtasks; count++){
            int ini_h = h_init(count, numtasks, o_h);
            int h = h_final(count, numtasks, o_h);
            struct colour *mndlbrt_recv = malloc(sizeof(struct colour)*w*(h-ini_h));
            MPI_Recv(mndlbrt_recv, (h-ini_h)*w, mpi_colour_type, count, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int i, j;
            for(i = 0; i < (h-ini_h); i++)
            {
                for(j = 0; j < w; j++)
                {
                    color((mndlbrt_recv + i*w + j)->red, (mndlbrt_recv + i*w + j)->green, (mndlbrt_recv + i*w + j)->blue);
                }
            }
            free(mndlbrt_recv);
        }
        elapsed_time = MPI_Wtime() - start_time;
        fprintf(stderr, "Elapsed time: %.2f seconds.\n", elapsed_time);
    }
    else
    {
        struct complex p, c;
        struct colour clr;
        int y,x;
        // As the process are static each worker can calculate its own load, thus reducing communications.
        int ini_h = h_init(rank, numtasks, o_h);
        int h = h_final(rank, numtasks, o_h);
        struct colour *mndlbrt_send = malloc(sizeof(struct colour)*w*(h-ini_h));
        #pragma omp parallel shared(mndlbrt_send) private(y, x, p, c, clr)
        {   
            #pragma omp for schedule(dynamic)
            for(y = ini_h; y < h; y++)
            {
                for(x = 0; x < w; x++)
                {
                    p.real = p.imag = 0; 
                    // Simplified the formula by extracting the static part, this is only for cleaning and easier understanding
                    c.real = (1.5 * (x - w / 2) / realDenom) + moveX;
                    c.imag = ((y - o_h / 2) / imagDenom) + moveY;

                    int k = 0;
                    double temp, lengthsq;
                    // We have changed the for loop as a do while is more suitable for the situation due the scaping condition
                    do{
                        // The inside of the formula has been changed following the model shown in the struct example as it
                        // is cleaner and easier of understand
                        temp = p.real * p.real - p.imag * p.imag + c.real;
                        p.imag = 2 * p.real * p.imag + c.imag;
                        p.real = temp;
                        lengthsq = p.real * p.real + p.imag * p.imag;
                        ++k;
                    }while(lengthsq < 4 && k < maxIterations);
                    
                    // Removed loop as now after the calculation we just save the value in the array, also changed the operations
                    // to work with the structures
                    if(k == maxIterations){
                        clr.red = clr.green = clr.blue = 0;   
                    }
                    else
                    {
                        double z = sqrt(p.real * p.real + p.imag * p.imag);
                        int brightness = 256 * log2(1.75 + k - log2(log2(z))) / log2((double)maxIterations);
                        clr.red = clr.green = brightness;
                        clr.blue = 255;
                    }
                    int ypos = y-ini_h;
                    *(mndlbrt_send + ypos*w + x) = clr;
                }
            }
        }
        // We send the results to the master so they can be printed
        MPI_Send(mndlbrt_send, (h-ini_h)*w, mpi_colour_type, 0, 1, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}