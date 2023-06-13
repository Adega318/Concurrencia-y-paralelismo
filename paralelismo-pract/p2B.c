#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

void inicializaCadena(char *cadena, int n){
  int i;
  for(i=0; i<n/2; i++){
    cadena[i] = 'A';
  }
  for(i=n/2; i<3*n/4; i++){
    cadena[i] = 'C';
  }
  for(i=3*n/4; i<9*n/10; i++){
    cadena[i] = 'G';
  }
  for(i=9*n/10; i<n; i++){
    cadena[i] = 'T';
  }
}

int MPI_BinomialColectiva(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int MPI_FlattreeColectiva(void *buff, void *recvbuff, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);

int main(int argc, char *argv[])
{
  if(argc != 3){
    printf("Numero incorrecto de parametros\nLa sintaxis debe ser: program n L\n  program es el nombre del ejecutable\n  n es el tamaño de la cadena a generar\n  L es la letra de la que se quiere contar apariciones (A, C, G o T)\n");
    exit(1); 
  }
    
  int i, n, count=0;
  char *cadena;
  char L;
  int numprocs, my_rank;
  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  if(my_rank == 0){
    n = atoi(argv[1]);
    L = *argv[2];
    /*
    práctica 1
    for(i = 1; i < numprocs; i++){
      MPI_Send (&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Send (&L, 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);
    }*/

    MPI_BinomialColectiva(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_BinomialColectiva(&L, 1, MPI_INT, 0, MPI_COMM_WORLD);
  }else{
    /*
    práctica 1
    MPI_Recv (&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    MPI_Recv (&L, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
    */

    MPI_BinomialColectiva(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_BinomialColectiva(&L, 1, MPI_INT, 0, MPI_COMM_WORLD);
  }
  
  cadena = (char *) malloc(n*sizeof(char));
  inicializaCadena(cadena, n);
  
  for(i = my_rank; i < n; i += numprocs){
    if(cadena[i] == L){
      count++;
    }
  }
  int totalcount = 0;
  if(my_rank == 0){
    /*
    práctica 1
    totalcount = count;

    for(i = 1; i < numprocs; i++){
      MPI_Recv (&count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
      totalcount += count;
    }
    */
    MPI_FlattreeColectiva(&count, &totalcount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    
    printf("El numero de apariciones de la letra %c es %d\n", L, totalcount);
  }else{
    /*
    práctica 1
    MPI_Send (&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    */
    MPI_FlattreeColectiva(&count, &totalcount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  }

  free(cadena);

  MPI_Finalize();

  exit(0);
}

int MPI_BinomialColectiva(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm){
  int numprocs, my_rank;
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Status status;

  int error;

  if(my_rank == root){
    for(int i = 0; pow(2, i) < numprocs; i++){
      error = MPI_Send(buf, count, datatype, (my_rank + (int)pow (2, i))%numprocs, 0, comm);
      if (error != 0){
        return error;
      }
    }
  }else{
    for(int i = 0; pow(2,i) < numprocs; i++){
      if(my_rank < 2 * pow(2,i)){
        if(my_rank < pow(2,i)){
          error = MPI_Send(buf, count, datatype, (my_rank + (int)pow (2, i))%numprocs, 0, comm);
          if (error != 0){
            return error;
          }
        }else{
          error = MPI_Recv(buf, count, datatype, (my_rank - (int)pow (2, i))%numprocs, 0, comm, &status);
          if (error != 0){
            return error;
          }        
        }
      }
    }
  }
  return error;
}

int MPI_FlattreeColectiva(void *buff, void *recvbuff, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm){
  int numprocs, my_rank;
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Status status;

  int error;

  if(op == MPI_SUM){
    if(my_rank == root){
      for(int i = 0; i < numprocs; i++){
        if(i != my_rank){
          error = MPI_Recv (buff, count, datatype, i, 0, comm, &status);
          if (error != 0){
            return error;
          }  
        }
        *((int*)recvbuff) += *((int*)buff);
      }
    }else{
      error = MPI_Send (buff, count, datatype, root, 0, comm);
      if (error != 0){
        return error;
      }      
    }
    return 0;
  }else{
    return -1;
  }
}