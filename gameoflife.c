#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLS 10
#define ROWS 10
#define ITER 154
#define DEBUG 0
#define BOARD_FILE "./resources/10x10/LifeGameInit_10x10_iter0.txt" 
// "/share/apps/files/lifegame/Examples/5000x5000/LifeGameInit_5000x5000_iter0.txt"
//"./resources/10x10/LifeGameInit_10x10_iter0.txt"

struct task
{
    int stop;
    int row;
    int board[COLS * 3];
};

void load_board(int*);
void assembly_task(struct task *, int, int*);

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int nproc, iproc;
    double start, elapsed;
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);

    enum rank_roles
    {
        MASTER,
        WORKER
    };
    enum boolean
    {
        FALSE,
        TRUE
    };
    enum state
    {
        DEAD,
        ALIVE
    };

    // Create the datatype  SOURCE:
    // https://www.rookiehpc.com/mpi/docs/mpi_type_create_struct.php
    MPI_Datatype task_type;
    int lengths[3] = {1, 1, COLS * 3};

    // Calculate displacements
    MPI_Aint displacements[3];
    struct task task_message;
    MPI_Aint base_address;
    MPI_Get_address(&task_message, &base_address);
    MPI_Get_address(&task_message.stop, &displacements[0]);
    MPI_Get_address(&task_message.row, &displacements[1]);
    MPI_Get_address(&task_message.board[0], &displacements[2]);

    // Recalulate displacements including padding (if any)
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);
    displacements[2] = MPI_Aint_diff(displacements[2], base_address);

    // Committing MPI_Datatype
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(3, lengths, displacements, types, &task_type);
    MPI_Type_commit(&task_type);
    
    if (iproc == MASTER)
    {
        int i, j, k, init_row, iter;
        int *board = malloc((ROWS * COLS) * sizeof(int));
        MPI_Request *sent = malloc(ROWS* sizeof(MPI_Request));
        struct task *work = malloc(sizeof(struct task));
        struct task *done = malloc(sizeof(struct task));
        char path[255];
        FILE *fp;
        
        load_board(board);
        
        if(DEBUG){
            fp = fopen("results/10x10/LifeGameInit_10x10_iter0.txt", "w+");

            for (i = 0; i < ROWS; i++)
            {
                for (j = 0; j < COLS; j++)
                {
                    fprintf(fp," %d", board[i*COLS + j]);
                }
                fprintf(fp,"\n");
            }
            fclose(fp);
        }     
        
        start = MPI_Wtime();
        for (iter = 1; iter < ITER; iter++)
        {
            k = WORKER;
            for (i = 0; i < ROWS; i++)
            {
                assembly_task(work, i, board);
                work->stop = FALSE;
                MPI_Isend(work, 1, task_type, k, 0, MPI_COMM_WORLD, &sent[i]);
                k++;
                if (k == nproc)
                    k = WORKER;
            }

            k = WORKER;
            for (i = 0; i < ROWS; i++)
            {
                MPI_Recv(done, 1, task_type, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (j = 0; j < COLS; j++)
                {
                    *(board + done->row * COLS + j) = done->board[j + COLS];
                }
                k++;
                if (k == nproc)
                    k = WORKER;
            }
            
            if(DEBUG){
                sprintf(path, "results/10x10/LifeGame_10x10_iter%d.txt", iter);
                fp = fopen(path, "w+");
                for (i = 0; i < ROWS; i++)
                {
                    for (j = 0; j < COLS; j++)
                    {
                        fprintf(fp, " %d", *(board + i* COLS + j));
                    }
                    fprintf(fp, "\n");
                }
                fclose(fp);
            }
            
        }
        elapsed = MPI_Wtime() - start;
        printf("With board %dx%d and %d iterations, I took %f seconds with %d ranks\n", ROWS, COLS, ITER, elapsed, nproc);
        
	if(DEBUG){
            sprintf(path, "results/10x10/LifeGameEnd_10x10_iter%d.txt", ITER - 1);
            fp = fopen(path, "w+");
            for (i = 0; i < ROWS; i++)
            {
                for (j = 0; j < COLS; j++)
                {
                    fprintf(fp, " %d", *(board + i * COLS + j));
                }
                fprintf(fp, "\n");
            }
            fclose(fp);
        }
            
        for (i = WORKER; i < nproc; i++)
        {
            work->stop = TRUE;
            MPI_Send(work, 1, task_type, i, 0, MPI_COMM_WORLD);
        }
        //
    }
    else
    {
        struct task *received= malloc(sizeof(struct task));
        struct task *sent = malloc(sizeof(struct task));
        MPI_Request outbound;
        int i, j, neighbors;
        int iter = 0;
        while (TRUE)
        {
            MPI_Recv(received, 1, task_type, MASTER, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            
            iter++;
            if (received->stop == TRUE)
                break; // Always on n_iteration + 1 is TRUE

            for (i = 0; i < COLS; i++)
            {
                neighbors = 0;
                neighbors += *(received->board + i);            // N
                neighbors += *(received->board + i + COLS * 2); // S
                if (i == COLS - 1)
                {
                    neighbors += *(received->board + 0);        // NE
                    neighbors += *(received->board + COLS);     // E
                    neighbors += *(received->board + COLS * 2); // SE
                }
                else
                {
                    neighbors += *(received->board + i + 1);            // NE
                    neighbors += *(received->board + i + COLS + 1);     // E
                    neighbors += *(received->board + i + COLS * 2 + 1); // SE
                }
                if (i == 0)
                {
                    neighbors += *(received->board + COLS * 3 - 1); // SW
                    neighbors += *(received->board + COLS * 2 - 1); // W
                    neighbors += *(received->board + COLS - 1);     // NW
                }
                else
                {
                    neighbors += *(received->board + i + COLS * 2 - 1); // SW
                    neighbors += *(received->board + i + COLS - 1);     // W
                    neighbors += *(received->board + i - 1);            // NW
                }
                /*
                • Cell without life: if the cell has less than two living neighbors.
                • Live cell: if the cell has exactly two living neighbors.
                • Cell is born: if a cell has exactly 3 living neighbors.
                • Death: if a living cell has more than 3 living neighbors (overpopulation).
                */
                if (neighbors < 2)
                    *(sent->board+ i + COLS) = DEAD;
                else if (neighbors == 2 && *(received->board + i + COLS) == ALIVE)
                    *(sent->board+ i + COLS) = ALIVE;
                else if (neighbors == 3)
                    *(sent->board+ i + COLS) = ALIVE;
                else
                    *(sent->board+ i + COLS) = DEAD;
            }
            sent->row = received->row;
            sent->stop = received->stop;
            MPI_Send(sent, 1, task_type, MASTER, 0, MPI_COMM_WORLD);     
        }
    }
    MPI_Finalize();
    return 0;
}

void load_board(int *board)
{
    int i, j;

    FILE *fp;
    char *token;
    const char *mode = "r";
    char buffer[COLS * 2 + 2]; // + 2 -->"\o" +"\n"

    i = 0;
    j = 0;
    fp = fopen(BOARD_FILE, mode);
    while (fgets(buffer, sizeof(buffer), fp))
    {
        token = strtok(buffer, " ");
        while (token != NULL)
        {
            *(board + i * COLS + j) = atoi(token);
            token = strtok(NULL, " ");
            j++;
        }
        i++;
        j = 0;
    }
    
    fclose(fp);
}
//[0..ROWS-1]  
void assembly_task(struct task *assembly, int row, int *board)
{
    int i, j, k;

    i = row == 0 ? ROWS - 1 : row - 1;
    assembly->row = row;

    for (k = 0; k < 3; k++)
    {
        for (j = 0; j < COLS; j++)
        {
            assembly->board[j + k * COLS] = *(board + i*COLS + j);
        }
        i = (i + 1) % ROWS;
    }
}
