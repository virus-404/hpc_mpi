#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLS 10
#define ROWS 10
#define ITER 1
#define DEBUG 0
#define BOARD_FILE "./resources/10x10/LifeGameInit_10x10_iter0.txt"
// "/share/apps/files/lifegame/Examples/5000x5000/LifeGameInit_5000x5000_iter0.txt"
struct task {
    int row;
    int board[COLS];
};

void loadBoard(int**);
void assembyMsg(struct task*, int, int*);
void copyNeighborRow(struct task*, int*);


enum boolean { false, true };
enum rank_roles { master,worker };
enum state { dead, alive }; 

int main(int argc, char **argv) {
    int *board;
    loadBoard(&board);

    if (DEBUG) {
        FILE *fp;
        fp = fopen("results/10x10/LifeGameInit_10x10_iter0.txt", "w+");
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLS; j++) {
                fprintf(fp, " %d", board[i * COLS + j]);
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    }

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

    //Calculate the work load
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
    
    if(DEBUG) printf("[Process %d] Start in %d and End in %d\n", iproc, start, end);
    MPI_Barrier(MPI_COMM_WORLD);

    double begin, elapsed;
    MPI_Request request [2];
    int neighbors[2] = {iproc - 1, iproc + 1};
    struct task *msg = malloc(sizeof(struct task));
    int *boardTmp = malloc((end - start + 2) * sizeof(int) * COLS);

    int k = 1;
    for (int i = start; i < end; i++) {
        for (int j = 0; j < COLS; j++) {
            boardTmp[k * COLS + j] = board[i* COLS + j];
        }
        k++;
    }
    free(board);

    if(neighbors[0] == -1) neighbors[0] = nproc - 1;
    if(neighbors[1] == nproc) neighbors[1] = 0;

    for (int i = 0; i < ITER; i++) {
        
        assembyMsg(msg, start, &boardTmp[COLS]);
        MPI_Isend(msg, 1, task_type, neighbors[0], 0, MPI_COMM_WORLD, &request[0]);
        assembyMsg(msg, end - 1, &boardTmp[(end - start) * COLS]);
        MPI_Isend(msg, 1, task_type, neighbors[1], 0, MPI_COMM_WORLD, &request[1]);
        if(DEBUG) printf("Process %d receives from %d and %d\n", iproc, neighbors[0], neighbors[1]);
        MPI_Recv(msg, 1, task_type, neighbors[0], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        copyNeighborRow(msg, &boardTmp[0]);
        MPI_Recv(msg, 1, task_type, neighbors[1], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        copyNeighborRow(msg, &boardTmp[(end - start + 1) * COLS]);
        
    }
    MPI_Finalize();
}
void loadBoard(int **board) {
    int i, j;
    FILE *fp;
    char *token;
    const char *mode = "r";
    char buffer[COLS * 2 + 2];  // + 2 -->"\o" +"\n"

    *board = malloc(sizeof(int) * COLS * ROWS);
    i = 0;
    fp = fopen(BOARD_FILE, mode);
    while (fgets(buffer, sizeof(buffer), fp)) {
        token = strtok(buffer, " ");
        while (token != NULL) {
            (*board)[i] = atoi(token);
            token = strtok(NULL, " ");
            i++;
        }
    }
    fclose(fp);
}

void assembyMsg(struct task *assembly, int row, int *board) {
    assembly->row = row;
    for (int i = 0; i < COLS; i++) {
        assembly->board[i] = board[i];
    }
}

void copyNeighborRow(struct task *msg, int *board) {
    for (int i = 0; i < COLS; i++) {
        board[i] = msg->board[i];
    }
}
