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

void loadBoard(int *);

struct task {
    int row;
    int board[COLS];
};

enum boolean { false, true };
enum rank_roles { master,worker };

int main(int argc, char **argv) {
    int *board;
    loadBoard(board);

    int nproc, iproc;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);

    // Create the datatype  SOURCE:
    // https://www.rookiehpc.com/mpi/docs/mpi_type_create_struct.php
    MPI_Datatype task_type;
    int lengths[3] = {1, COLS};

    // Calculate displacements
    MPI_Aint displacements[2];
    struct task task_message;
    MPI_Aint base_address;
    MPI_Get_address(&task_message, &base_address);
    MPI_Get_address(&task_message.row, &displacements[0]);
    MPI_Get_address(&task_message.board[0], &displacements[1]);

    // Recalulate displacements including padding (if any)
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);

    // Committing MPI_Datatype
    MPI_Datatype types[2] = {MPI_INT, MPI_INT};
    MPI_Type_create_struct(2, lengths, displacements, types, &task_type);
    MPI_Type_commit(&task_type);


    int start = 0;
    int end = 0; 

    for (int i = 0; i < iproc; i++)
    {
        start += ROWS / nproc;
        if (i <  ROWS % nproc) start++;
    }
    end = start + ROWS / nproc;
    if (iproc <  ROWS % nproc) end++;

    int remainingRows = COLS%nproc;

    MPI_Barrier(MPI_COMM_WORLD);

    printf("[Process %d] Start in %d and End in %d\n", iproc, start, end);

    double begin, elapsed;
    for (int i; i < ITER; i++) {
        
    }
}
void loadBoard(int *board) {
    int i, j;

    board = malloc(sizeof(int) * COLS * ROWS);
    FILE *fp;
    char *token;
    const char *mode = "r";
    char buffer[COLS * 2 + 2];  // + 2 -->"\o" +"\n"

    i = 0;
    fp = fopen(BOARD_FILE, mode);
    while (fgets(buffer, sizeof(buffer), fp)) {
        token = strtok(buffer, " ");
        while (token != NULL) {
            *(board + i * COLS + j) = atoi(token);
            token = strtok(NULL, " ");
            j++;
        }
        i++;
        j = 0;
    }
    fclose(fp);
}
