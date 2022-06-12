#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLS 10
#define ROWS 10
#define ITER 154
#define DEBUG 1
#define BOARD_FILE "./resources/10x10/LifeGameInit_10x10_iter0.txt"
// "/share/apps/files/lifegame/Examples/5000x5000/LifeGameInit_5000x5000_iter0.txt"
struct task {
    int row;
    int board[COLS];
};

void loadBoard(int **);
void assembyMsg(struct task *, int, int *);
void copyNeighborRow(struct task *, int *);
int countNeighbors(int, int, int *);
int evaluateCellNex(int, int);

enum boolean { false,
               true };
enum rank_roles { master,
                  worker };
enum state { dead,
             alive };

int main(int argc, char **argv) {
    int *board;
    FILE *fp;
    loadBoard(&board);
    int i, j, k;
    if (DEBUG) {
        fp = fopen("results/10x10/LifeGameInit_10x10_iter0.txt", "w+");
        for (i = 0; i < ROWS; i++) {
            for (j = 0; j < COLS; j++) {
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

    // https://rookiehpc.com/mpi/docs/mpi_allgatherv.php
    // Calculate the work load + displacements for each rank
    int workload[nproc];
    int shifts[nproc];

    int rowsPerRank = ROWS / nproc;
    int shift = 0;
    for (i = 0; i < nproc; i++) {
        shifts[i] = shift;
        if (i < ROWS % nproc)
            workload[i] = (rowsPerRank + 1) * COLS;
        else
            workload[i] = rowsPerRank * COLS;
        shift += workload[i];
        // if (iproc == 0) printf("%d %d\n", workload[i], shifts[i]);
    }

    // Calculate start and end positions [start,end)
    int start = 0;
    int end = 0;

    for (i = 0; i < iproc; i++) {
        start += ROWS / nproc;
        if (i < ROWS % nproc) start++;
    }
    if (iproc < ROWS % nproc)
        end = start + ROWS / nproc;
    else
        end = start + ROWS / nproc - 1;
    end++;
    // printf("[Process %d] Start in %d and End in %d\n", iproc, start, end);

    int remainingRows = COLS % nproc;

    MPI_Barrier(MPI_COMM_WORLD);

    double begin, elapsed;
    MPI_Request request[2];
    int neighbors[2] = {iproc - 1, iproc + 1};
    struct task *msg = malloc(sizeof(struct task));
    int *boardTmp = malloc((end - start + 2) * sizeof(int) * COLS);

    k = 1;
    for (i = start; i < end; i++) {
        for (j = 0; j < COLS; j++) {
            boardTmp[k * COLS + j] = board[i * COLS + j];
        }
        k++;
    }
    free(board);

    if (neighbors[0] == -1) neighbors[0] = nproc - 1;
    if (neighbors[1] == nproc) neighbors[1] = 0;

    int *nextIter = malloc((end - start) * sizeof(int) * COLS);
    int cellState = 0;
    if (iproc == 0) {
        begin = MPI_Wtime();
    }
    int row, col, cell;
    for (i = 0; i < ITER; i++) {
        assembyMsg(msg, start, &boardTmp[COLS]);
        MPI_Isend(msg, 1, task_type, neighbors[0], 0, MPI_COMM_WORLD, &request[0]);
        assembyMsg(msg, end - 1, &boardTmp[(end - start) * COLS]);
        MPI_Isend(msg, 1, task_type, neighbors[1], 0, MPI_COMM_WORLD, &request[1]);
        // printf("Process %d receives from %d and %d\n", iproc, neighbors[0], neighbors[1]);
        MPI_Recv(msg, 1, task_type, neighbors[0], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        copyNeighborRow(msg, &boardTmp[0]);
        MPI_Recv(msg, 1, task_type, neighbors[1], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        copyNeighborRow(msg, &boardTmp[(end - start + 1) * COLS]);
        for (row = 1; row < (end - start) + 1; ++row) {
            for (col = 0; col < COLS; col++) {
               
                cellState = countNeighbors(row, col, boardTmp);
                nextIter[(row - 1) * COLS + col] = evaluateCellNex(boardTmp[row * COLS + col], cellState);
            }
        }
        
        for (row = 1; row < (end - start) + 1; ++row) {
            for (col = 0; col < COLS; col++) {
                boardTmp[row * COLS + col] = nextIter[(row - 1) * COLS + col];
            }
        }
    }
    free(boardTmp);

    if (iproc == 0) {
        board = malloc(sizeof(int) * COLS * ROWS);
        MPI_Gatherv(nextIter, workload[iproc], MPI_INT, board, workload, shifts, MPI_INT, master, MPI_COMM_WORLD);
        elapsed = MPI_Wtime() - begin;
        printf("With board %dx%d and %d iterations, I took %f seconds with %d ranks\n", ROWS, COLS, ITER, elapsed, nproc);
        if (DEBUG) {
            char path[255];
            sprintf(path, "results/10x10/LifeGameEnd_10x10_iter%d.txt", ITER - 1);
            fp = fopen(path, "w+");
            for (i = 0; i < ROWS; i++) {
                for (j = 0; j < COLS; j++) {
                    fprintf(fp, " %d", board[i * COLS + j]);
                }
                fprintf(fp, "\n");
            }
            fclose(fp);
        }
    } else {
        MPI_Gatherv(nextIter, workload[iproc], MPI_INT, NULL, NULL, NULL, MPI_INT, master, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
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
    int i;
    for (i = 0; i < COLS; i++) {
        assembly->board[i] = board[i];
    }
}

void copyNeighborRow(struct task *msg, int *board) {
    int i;
    for (i = 0; i < COLS; i++) {
        board[i] = msg->board[i];
    }
}
int countNeighbors(int row, int col, int *board) {
    int neighbors = 0;

    neighbors += board[(row - 1) * COLS + col];  // N
    neighbors += board[(row + 1) * COLS + col];  // S
    if (col == COLS - 1) {
        neighbors += board[(row - 1) * COLS];  // NE
        neighbors += board[(row)*COLS];        // E
        neighbors += board[(row + 1) * COLS];  // SE
    } else {
        neighbors += board[(row - 1) * COLS + col + 1];  // NE
        neighbors += board[(row)*COLS + col + 1];        // E
        neighbors += board[(row + 1) * COLS + col + 1];  // SE
    }
    if (col == 0) {
        neighbors += board[(row - 1) * COLS + COLS - 1];  // NW
        neighbors += board[(row)*COLS + COLS - 1];        // W
        neighbors += board[(row + 1) * COLS + COLS - 1];  // SW
    } else {
        neighbors += board[(row - 1) * COLS + col - 1];  // NW
        neighbors += board[(row)*COLS + col - 1];        // W
        neighbors += board[(row + 1) * COLS + col - 1];  // SW
    }

    return neighbors;
}
int evaluateCellNex(int state, int neighbors) {
    /*
    • Cell without life: if the cell has less than two living neighbors.
    • Live cell: if the cell has exactly two living neighbors.
    • Cell is born: if a cell has exactly 3 living neighbors.
    • Death: if a living cell has more than 3 living neighbors (overpopulation).
    */
    if (neighbors < 2)
        return dead;
    else if (neighbors == 2 && state == alive)
        return alive;
    else if (neighbors == 3)
        return alive;
    else
        return dead;
}
