#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

#define DEBUG 2

/* Translation of the DNA bases
A -> 0
C -> 1
G -> 2
T -> 3
N -> 4*/

#define M  11// Number of sequences
#define N  1  // Number of bases per sequence

unsigned int g_seed = 0;

int fast_rand(void) {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16) % 5;
}

// The distance between two bases
int base_distance(int base1, int base2){

if((base1 == 4) || (base2 == 4)){
    return 3;
}

if(base1 == base2) {
    return 0;
}

if((base1 == 0) && (base2 == 3)) {
    return 1;
}

if((base2 == 0) && (base1 == 3)) {
    return 1;
}

if((base1 == 1) && (base2 == 2)) {
    return 1;
}

if((base2 == 2) && (base1 == 1)) {
    return 1;
}

return 2;
}

int main(int argc, char *argv[] ) {

    int i, j;
    int *sendcounts, *displs;
    int *result, *data1, *data2;
    int *recvbuf1, *recvbuf2;
    struct timeval  tv1, tv2;
    int my_rank, numprocs, acumulado = 0, recvcount;
    int *resultp;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    int cant = M/numprocs;
    int rest = M-cant*numprocs;

    if(my_rank == 0){
        sendcounts = malloc(numprocs*sizeof(int));
        displs = (int *) malloc(sizeof(int) * numprocs);
        

        for(i=0; i<numprocs-1; i++){
            sendcounts[i] = cant*N;
            displs[i] = acumulado*N;
            acumulado += cant;
        }
        sendcounts[numprocs-1] = M - cant*(numprocs-1);
        displs[numprocs-1] = acumulado;

        //data reparto
        data1 = (int *) malloc(M*N*sizeof(int));
        data2 = (int *) malloc(M*N*sizeof(int));
        result = (int *) malloc(M*sizeof(int));
        /* Initialize Matrices */
        for(i=0;i<M;i++) {
            for(j=0;j<N;j++) {
                /* random with 20% gap proportion */
                data1[i*N+j] = fast_rand();
                data2[i*N+j] = fast_rand();
            }
        }

    }
    
    if(my_rank == numprocs-1) recvcount = M - cant*(numprocs-1);
    else recvcount = cant;
    printf("%d  %d\n", my_rank, recvcount);

    recvbuf1 = malloc(recvcount*N*sizeof(int));
    recvbuf2 = malloc(recvcount*N*sizeof(int));

    MPI_Scatterv(data1, sendcounts, displs, MPI_INT, recvbuf1, recvcount, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(data2, sendcounts, displs, MPI_INT, recvbuf2, recvcount, MPI_INT, 0, MPI_COMM_WORLD);

    resultp = malloc(recvcount * sizeof(int));
    
    /*
    recvbuf1 = malloc(sendcounts[my_rank]*sizeof(int));
    recvbuf2 = malloc(sendcounts[my_rank]*sizeof(int));
    int recvcount = sendcounts[my_rank];
    MPI_Scatterv(data1, sendcounts, displs, MPI_INT, recvbuf1, recvcount, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(data2, sendcounts, displs, MPI_INT, recvbuf2, recvcount, MPI_INT, 0, MPI_COMM_WORLD);
    */
    gettimeofday(&tv1, NULL);

    for(i=0; i<recvcount; i++){
        resultp[i] = 0;
        for(j=i * N; j<i*N+N; j++){
            resultp[i] += base_distance(recvbuf1[j], recvbuf2[j]);
        }
    }

    gettimeofday(&tv2, NULL);
        
    int microseconds = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

    for(i=0; i<recvcount*N; i++){
        printf("%d --> d1 %d\n",my_rank, recvbuf1[i]);
    }
    for(i=0; i<recvcount*N; i++){
        printf("%d --> d2 %d\n",my_rank, recvbuf2[i]);
    }

    if(my_rank == 0){
        acumulado = 0;
        for(i=0; i< numprocs-1; i++){
            sendcounts[i] = cant;
            displs[i] = acumulado;
            acumulado += cant;
        }
        sendcounts[numprocs-1] = M - cant*(numprocs-1);
        displs[numprocs-1] = acumulado;
    }

    
    MPI_Gatherv(resultp , recvcount*N, MPI_INT, result, sendcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);

    if(my_rank == 0){
        //Display result 
        if (DEBUG == 1) {
            int checksum = 0;
            for(i=0;i<M;i++) {
                checksum += result[i];
            }
            printf("Checksum: %d\n ", checksum);
        } else if (DEBUG == 2) {
            for(i=0;i<M;i++) {
                printf(" %d \t ",result[i]);
            }
            printf("\n");
        } else {
            printf ("Time (seconds) = %lf\n", (double) microseconds/1E6);
        }
        
    }

    free(data1); free(data2);  free(resultp); free(recvbuf1); free(recvbuf2); free(sendcounts); free(displs);

    MPI_Finalize();
    return 0;
}
