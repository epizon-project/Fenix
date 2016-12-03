/*
//@HEADER
// ************************************************************************
//
//
//            _|_|_|_|  _|_|_|_|  _|      _|  _|_|_|  _|      _|
//            _|        _|        _|_|    _|    _|      _|  _|
//            _|_|_|    _|_|_|    _|  _|  _|    _|        _|
//            _|        _|        _|    _|_|    _|      _|  _|
//            _|        _|_|_|_|  _|      _|  _|_|_|  _|      _|
//
//
//
//
// Copyright (C) 2016 Rutgers University and Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY RUTGERS UNIVERSITY and SANDIA CORPORATION
// "AS IS" AND ANY // EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE // IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RUTGERS 
// UNIVERISY, SANDIA CORPORATION OR THE CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author Marc Gamell, Eric Valenzuela, Keita Teranishi, Manish Parashar
//        and Michael Heroux
//
// Questions? Contact Keita Teranishi (knteran@sandia.gov) and
//                    Marc Gamell (mgamell@cac.rutgers.edu)
//
// ************************************************************************
//@HEADER
*/

#include <fenix.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

const int kCount = 300;
const int kTag = 1;
const int kKillID = 2;
int kNumIterations = 2;

void my_recover_callback(MPI_Comm new_comm, int error, void *callback_data) {
  int rank;
  double *y = (double *) callback_data;
  MPI_Comm_rank(new_comm, &rank);
  // printf("Callback %d %.2f\n", rank, y[2]);
}

int main(int argc, char **argv) {

  void (*recPtr)(MPI_Comm, int, void *);
  recPtr = &my_recover_callback;
  double x[4] = {0, 0, 0, 0};
  int i;
  int inmsg[300]  ;
  int outmsg[300] ;
  int recovered = 0;
  int reset = 0;
  MPI_Status status;

  int fenix_role;
  MPI_Comm world_comm;
  MPI_Comm new_comm;
  int spare_ranks = atoi(*++argv);
  MPI_Info info = MPI_INFO_NULL;
  int num_ranks;
  int rank;
  int error;
  int my_group = 0;
  int my_timestamp = 0;
  int my_depth = 0;

  MPI_Init(&argc, &argv);
  MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
  Fenix_Init(&fenix_role, world_comm, &new_comm, &argc, &argv,
             spare_ranks, 0, info, &error);

  MPI_Comm_size(new_comm, &num_ranks);
  MPI_Comm_rank(new_comm, &rank);

  /* This is called even for recovered/survived ranks */
  /* If called by SURVIVED ranks, make sure the group has been exist */
  /* Recovered rank needs to initalize the data                      */
  Fenix_Data_group_create( my_group, new_comm, my_timestamp, my_depth );

  if (fenix_role == FENIX_ROLE_INITIAL_RANK) {

    for (i = 0; i < kCount; i++) {
        inmsg[i] = -1;  
    }

    for( i = 4; i < kCount; i++ ) {
      outmsg[i] = i+rank *2;
    }
    for( i = 0; i < 4; i++ ) {
      x[i] = (double)(i+1)/(double)3.0;
    }
    outmsg[0] = rank + 1;
    outmsg[1] = rank + 2;
    outmsg[2] = rank + 3;
    outmsg[3] = -1000;

    int v_print;
    for (v_print = 0; v_print < kCount; v_print++) {
        //printf("outmsg[%d]: %d; rank: %d; count: %d\n", v_print, outmsg[v_print], rank, kCount);        
    }
    Fenix_Data_member_create(my_group, 777, outmsg, kCount, MPI_INT);
    Fenix_Data_member_store(my_group, 777, FENIX_DATA_SUBSET_FULL);
    Fenix_Data_member_create(my_group, 778, x, 4, MPI_DOUBLE);
    Fenix_Data_member_store(my_group, 778, FENIX_DATA_SUBSET_FULL);
    Fenix_Data_member_create(my_group, 779, inmsg, kCount, MPI_INT);
    Fenix_Data_member_store(my_group, 779, FENIX_DATA_SUBSET_FULL);
    Fenix_Data_commit(my_group, &my_timestamp);
    recovered = 0;
    reset = 0;
    
    if (rank == 2) {
      printf("NOT-RECOVERED rank %d, inmsg[0]: %d, inmsg[1]: %d, inmsg[2]: %d; ROLE: %d\n", rank, inmsg[0], inmsg[1], inmsg[2], fenix_role);
      printf("NOT-RECOVERED rank %d, outmsg[0]: %d, outmsg[1]: %d, outmsg[2]: %d; ROLE: %d\n", rank, outmsg[0], outmsg[1], outmsg[2], fenix_role);
    }

  } else {
    int out_flag = 0;
    Fenix_Data_member_restore(my_group, 777, outmsg, kCount, 1);
    /* Should throw error if kCount is greater than 4 */
    Fenix_Data_member_restore(my_group, 778, x, 4, 1);
    Fenix_Data_member_restore(my_group, 779, inmsg, kCount, 1);
    recovered = 1;
    reset = 1;
    if(rank == 2) {
      printf("RECOVERED rank %d, inmsg[0]: %d, inmsg[1]: %d, inmsg[2]: %d; ROLE: %d\n", rank, inmsg[0], inmsg[1], inmsg[2], fenix_role);
      printf("RECOVERED rank %d, outmsg[0]: %d, outmsg[1]: %d, outmsg[2]: %d; ROLE: %d\n", rank, outmsg[0], outmsg[1], outmsg[2], fenix_role);
    } 
  }

#if 0
  if (rank == kKillID && recovered == 0) {
    pid_t pid = getpid();
    kill(pid, SIGKILL);
  }
#endif

  for (i = 0; i < kNumIterations; i++) {
 
    //printf("rank: %d; i: %d; role: %d\n", rank, i, fenix_role);
  
    if (rank == 0) {
      MPI_Send(outmsg, kCount, MPI_INT, 1, kTag, new_comm); // send to rank # 1
      MPI_Recv(inmsg, kCount, MPI_INT, (num_ranks - 1), kTag, new_comm,
               &status); // recv from last rank #
    }
    else {
      MPI_Recv(inmsg, kCount, MPI_INT, (rank - 1), kTag, new_comm,
               &status); // recv from prev rank #
      outmsg[0] = inmsg[0] + 1;
      outmsg[1] = inmsg[1] + 1;
      outmsg[2] = inmsg[2] + 1;
      outmsg[3] = inmsg[3] ;
      MPI_Send(outmsg, kCount, MPI_INT, ((rank + 1) % num_ranks), kTag,
               new_comm); // send to next rank #
    }
  }

  int checksum[300];
  MPI_Allreduce(inmsg, checksum, kCount, MPI_INT, MPI_SUM, new_comm);
  MPI_Barrier(new_comm);
  if (rank == 0) {
    printf("DONE\n");
  }

#if 1
  Fenix_Data_member_store(my_group, 777,FENIX_DATA_SUBSET_FULL);
  Fenix_Data_member_store(my_group, 778,FENIX_DATA_SUBSET_FULL);
  Fenix_Data_member_store(my_group, 779,FENIX_DATA_SUBSET_FULL);
  Fenix_Data_commit(my_group, &my_timestamp);
#endif 
 
  if (rank < 14) {
    int sum = (num_ranks * (num_ranks + 1)) / 2;
    printf("num_ranks: %d; sum: %d; checksum: %d\n", num_ranks, sum, checksum[0]);
    printf("Checksum: %d %d %d %d %d\n", checksum[0], checksum[1], checksum[2], checksum[3], checksum[100]);
    printf("End of the program %d\n", rank);
  }

  Fenix_Finalize();
  MPI_Finalize();
  return 0;
}