#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLS 10
#define ROWS 10
#define BOARD_FILE "./resources/10x10/LifeGameInit_10x10_iter0.txt"

struct task {
    int stop;
    int row;
    int board[COLS * 3];
};

void load_board(int[ROWS][COLS]);
void assembly_task(struct task*, int, int[ROWS][COLS]);
int calculate(int[COLS * 3], int);

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int nproc, iproc;
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);

    enum rank_roles { MASTER, WORKER };
    enum boolean { FALSE, TRUE };
    enum state { DEAD, ALIVE };

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

    if (iproc == MASTER) {
        int i, j, k, init_row;
        int board[ROWS][COLS];
        MPI_Request sent[COLS];
        struct task work[COLS];
        struct task done;

        load_board(board);
        k = WORKER;

        for (i = 0; i < ROWS; i++) {
            assembly_task(&work[i], i, board);

            

            work[i].stop = FALSE;
            MPI_Isend(&work[i], 1, task_type, k, 0, MPI_COMM_WORLD, &sent[i]);
            k++;
            if (k == nproc) k = 1;
        }

        k = WORKER;
        for (i = 0; i < ROWS; i++) {
            MPI_Recv(&done, 1, task_type, k, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (j = 0; j < COLS; j++) {
                board[done.row][j] = done.board[j + COLS];
            }
            k++; 
            if (k == nproc) k = 1;
        }
        
        for (i = 0; i < ROWS; i++)
        {
            for (j = 0; j < COLS; j++)
            {
                printf("|%d",board[i][j]);
            }
            printf("|\n");

        }
        

        // getResutlTask
        // Broadcasting the end --> MPI_Bcast was not used since it locks the
        // program (sometimes)
        for (i = WORKER; i < nproc; i++) {
            work[i].stop = TRUE;
            MPI_Send(&work[i], 1, task_type, i, 0, MPI_COMM_WORLD);
        }

        printf("Hello World \n");

    } else {
        struct task received, sent;
        MPI_Request outbound;
        int i, j, neighbors;

        while (TRUE) {
            MPI_Recv(&received, 1, task_type, MASTER, 0, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            if (received.stop == TRUE)
                break;  // Always on n_iteration + 1 is TRUE

            for (i = 0; i < COLS; i++) {
                neighbors = 0;
                neighbors += received.board[i];                     // N
                neighbors += received.board[i + COLS * 2];         // S
                if (i == COLS - 1) {
                    neighbors += received.board[0];                // NE
                    neighbors += received.board[COLS];             // E
                    neighbors += received.board[COLS*2];           // SE
                } else {
                    neighbors += received.board[i + 1];            // NE
                    neighbors += received.board[i + COLS + 1];     // E
                    neighbors += received.board[i + COLS * 2 + 1]; // SE
                } 
                if (i == 0){
                    neighbors += received.board[COLS * 3 - 1];     //SW
                    neighbors += received.board[COLS * 2 - 1];     //W
                    neighbors += received.board[COLS - 1];         //NW
                } else {
                    neighbors += received.board[i + COLS * 2 - 1]; //SW
                    neighbors += received.board[i + COLS - 1];     //W
                    neighbors += received.board[i - 1];            //NW
                }
                /*
                • Cell without life: if the cell has less than two living neighbors.
                • Live cell: if the cell has exactly two living neighbors.
                • Cell is born: if a cell has exactly 3 living neighbors.
                • Death: if a living cell has more than 3 living neighbors (overpopulation).
                */
                if (neighbors < 2)
                    sent.board[i + COLS] = DEAD;
                else if (neighbors == 2 && received.board[i + COLS] == ALIVE)
                    sent.board[i + COLS] = ALIVE;
                else if (neighbors == 3)
                    sent.board[i + COLS] = ALIVE;
                else
                    sent.board[i + COLS] = DEAD;
            }
            sent.row = received.row;
            sent.stop = received.stop;
            MPI_Send(&sent, 1, task_type, MASTER, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}

void load_board(int board[ROWS][COLS]) {
    int i, j;

    FILE* fp;
    char* token;
    const char* mode = "r";
    char buffer[COLS * 2 + 2];  // + 2 -->"\o" +"\n"

    i = 0;
    fp = fopen(BOARD_FILE, mode);
    while (fgets(buffer, sizeof(buffer), fp)) {
        token = strtok(buffer, " ");
        while (token != NULL) {
            board[i][j] = atoi(token);
            token = strtok(NULL, " ");
            j++;
        }
        i++;
        j = 0;
    }
    fclose(fp);
}

void assembly_task(struct task* assembly, int row, int board[ROWS][COLS]) {
    int i, j, k;

    i = row == 0? ROWS - 1 : row - 1;
    assembly->row = row;
    
    for (k = 0; k < 3; k++) {
        for (j = 0; j < COLS; j++) {
            assembly->board[j + k * COLS] = board[i][j];
        }
        i = (i + 1) % ROWS;
    }
}
