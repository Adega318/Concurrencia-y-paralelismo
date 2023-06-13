#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

#define DEBUG 1

/* Translation of the DNA bases
A -> 0
C -> 1
G -> 2
T -> 3
N -> 4*/

#define M  1000// Number of sequences
#define N  200  // Number of bases per sequence

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
        

        for(i=0; i<numprocs; i++){
            sendcounts[i] = cant*N;
            displs[i] = acumulado*N;
            if(i < rest){
                sendcounts[i] += N;
                acumulado += 1;
            }
            acumulado += cant;
        }

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
    
    recvcount = cant;
    if(my_rank < rest) recvcount += 1;

    recvbuf1 = malloc(recvcount*N*sizeof(int));
    recvbuf2 = malloc(recvcount*N*sizeof(int));

    gettimeofday(&tv1, NULL);

    MPI_Scatterv(data1, sendcounts, displs, MPI_INT, recvbuf1, recvcount*N, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(data2, sendcounts, displs, MPI_INT, recvbuf2, recvcount*N, MPI_INT, 0, MPI_COMM_WORLD);

    gettimeofday(&tv2, NULL);

    int tiempoScatter = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

    resultp = malloc(recvcount * sizeof(int));


    gettimeofday(&tv1, NULL);

    for(i=0; i<recvcount; i++){
        resultp[i] = 0;
        for(j=i * N; j<i*N+N; j++){
            resultp[i] += base_distance(recvbuf1[j], recvbuf2[j]);
        }
    }

    gettimeofday(&tv2, NULL);
        
    int tiempoEjecut = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

    if(my_rank == 0){
        acumulado = 0;
        for(i=0; i< numprocs; i++){
            sendcounts[i] = cant;
            displs[i] = acumulado;
            if(i < rest){
                sendcounts[i] += 1;
                acumulado += 1;
            }
            acumulado += cant;
        }
    }
    gettimeofday(&tv1, NULL);
    
    MPI_Gatherv(resultp , recvcount, MPI_INT, result, sendcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);

    gettimeofday(&tv2, NULL);

    int tiempoGather = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);
    
    
    //Display result 

    printf ("Proces %d time com = %lf\ttime ejec = %lf\n",my_rank, (double) ((tiempoGather+tiempoScatter)/1E6), (double) (tiempoEjecut/1E6));

    if (my_rank==0 && DEBUG == 1) {
        int checksum = 0;
        for(i=0;i<M;i++) {
            checksum += result[i];
        }
        printf("Checksum: %d\n ", checksum);
    } else if (my_rank==0 && DEBUG == 2) {
        for(i=0;i<M;i++) {
            printf(" %d \t ",result[i]);
        }
        printf("\n");
    }
        
    free(data1); free(data2);  free(resultp); free(recvbuf1); free(recvbuf2); free(sendcounts); free(displs);

    MPI_Finalize();
    return 0;
}
