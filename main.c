#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include "mpi.h"

int main(int argc, char **argv)
{
    int choice, nOfCandidates, nOfVoters, nOfProcesses, rank, rem, portionSize, i, j;
    int *votes, *portionVotes, *localResult, *final_results;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nOfProcesses);
    MPI_Status status;
    //while (choice != 0){
    if (rank == 0)
    {
        printf("Enter 1 to Generate the data file\nOR\nEnter 2 to Calculate the result and print the winner\nOR\nEnter 0 to exit\n");
        scanf("%d", &choice);
        ///BROADCAST CHOICE TO SLAVES
        MPI_Bcast(&choice, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }

    ///RECEIVE CHOICE FROM MASTER
    MPI_Bcast(&choice, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (choice == 1)
    {
        if(rank==0)
        {

            printf("Enter The number of Candidates: ");
            scanf("%d",&nOfCandidates);
            printf("Enter The number of voters: ");
            scanf("%d",&nOfVoters);
            FILE* elections = fopen("elections.txt", "w");
            fprintf(elections, "%d", nOfCandidates);
            fputs("\r\n", elections);
            fprintf(elections, "%d", nOfVoters);
            fputs("\r\n", elections);
            for (i = 0; i < nOfVoters; i++)
            {
                srand(time(NULL));
                for (j = 0; j < nOfCandidates; j++)
                {
                    int r = rand() % nOfCandidates +1;
                    fprintf(elections, "%d", r);
                    fputs(" ", elections);
                }
                fputs("\r\n", elections);
            }
            fclose(elections);
        }
    }
    else if (choice == 2 )
    {
        FILE* elections = fopen("elections.txt", "r");
        char tempLine[256];
        fgets(tempLine, sizeof(tempLine), elections);
        nOfCandidates = atoi(tempLine);
		//sscanf(tempLine,"%d",&nOfCandidates);
        fgets(tempLine, sizeof(tempLine), elections);
        nOfVoters = atoi(tempLine);
		//sscanf(tempLine,"%d",&nOfVoters);

        votes = malloc(nOfCandidates*nOfVoters * sizeof(int));
        localResult = malloc(nOfCandidates * sizeof(int));
        final_results = malloc(nOfCandidates * sizeof(int));
        portionSize = (nOfCandidates*nOfVoters) / nOfProcesses;
        portionVotes = malloc(portionSize * sizeof(int));


        //to do: read the votes in parallel
        int u = 0;
        int lines = 1;
        while (fgets(tempLine, sizeof(tempLine), elections))
        {
            int k = 0;
            if (lines > 2)
            {
                for (k; k < strlen(tempLine) - 2; k++)
                {
                    if (tempLine[k] != ' ')
                    {
                        votes[u] = tempLine[k];
                        u++;
                    }
                }
            }
            lines++;
        }
        MPI_Scatter(votes, portionSize, MPI_INT, portionVotes, portionSize, MPI_INT, 0, MPI_COMM_WORLD);
        for (i = 0; i < nOfCandidates; i++)
        {
            localResult[i] = 0;
            final_results[i] = 0;
        }
        for (i = 0; i < portionSize; i++)
            localResult[portionVotes[i] ] ++;

        //remainder
        if (rank == 0)
        {
            for (i = (portionSize * nOfProcesses); i < (portionSize * nOfProcesses) + rem; i++ )
            {
                localResult[portionVotes[i] ] ++;
            }
        }

        MPI_Allreduce(localResult, final_results, nOfCandidates, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        bool round2 = false;
        int max = -1, count = 0, index, index2;
        if (rank == 0)
        {
            printf("ROUND 1: \n");
            for (i = 0; i < nOfCandidates; i++)
            {
                int percent = (final_results[i] * 100) / nOfVoters;
                printf("Candidate [%d] got %d/%d which is %d %% \n", i+1, final_results[i], nOfVoters, percent);
                if (max < percent)
                {
                    max = percent;
                    count = 1;
                    index = i;
                }
                if ((max == percent)&&(i != index))
                {
                    index2 = i;
                    count ++;
                }
            }
            if (count == 1)
                printf("So Candidate %d wins in round 1 \n", index+1);
            else
            {
                printf("So second round will take place between candidates %d and %d \n", index, index2);
                round2 = true;
            }
        }
        /*
        if (round2)
        {
            int *votes2 = malloc(2*nOfVoters * sizeof(int));
            j = 0;
            for (i = 0; i < nOfCandidates*nOfVoters; i+=nOfVoters)
            {
                votes2 [j] = votes [i+index];
                j++;
                votes2 [j] = votes [i+index2];
                j++;
            }
            portionSize = (2*nOfVoters) / nOfProcesses;
            rem = (2*nOfVoters) - portionSize;
            free(portionVotes);
            free(localResult);
            free(final_results);
           // free();
            portionVotes = malloc(portionSize * sizeof(int));
            localResult = malloc(2 * sizeof(int));
            final_results = malloc(2 * sizeof(int));
            MPI_Scatter(votes2, portionSize, MPI_INT, portionVotes, portionSize, MPI_INT, 0, MPI_COMM_WORLD);
            for (i = 0; i < 2; i++)
            {
                localResult[i] = 0;
                final_results[i] = 0;
            }
            for (i = 0; i < portionSize; i++)
                localResult[portionVotes[i] ] ++;

            //remainder
            if (rank == 0)
            {
                for (i = (portionSize * nOfProcesses); i < (portionSize * nOfProcesses) + rem; i++ )
                {
                    localResult[portionVotes[i]] ++;
                }
            }
            MPI_Allreduce(localResult, final_results, 2, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
            if (rank == 0)
            {
                int percent1 = (final_results[0] * 100) / nOfVoters;
                int percent2 = (final_results[1] * 100) / nOfVoters;
                printf("Candidate [%d] got %d/2 which is %d %% \n", index+1, final_results[0], percent1);
                printf("Candidate [%d] got %d/2 which is %d %% \n", index2+1, final_results[1], percent2);
                if (percent1 > percent2)
                    printf("So Candidate %d wins in round 2 \n", index+1);
                else if (percent2 > percent1)
                    printf("So Candidate %d wins in round 2 \n", index2+1);
                else
                    printf("IT's DRAW. \n");
            }
        }
        */
    }
    else if ((choice != 0)&&(choice != 1)&&(choice != 2))
        printf("Wrong choice! Please try again...\n");


    MPI_Finalize();

}
