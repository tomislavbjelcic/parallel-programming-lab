#include <iostream>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include "board.h"
#include <chrono>

#define TASK_DONE_TAG 9
#define REQUEST_TAG 10
#define NO_MORE_TASKS_TAG 11
#define TASK_SENT 12

#define START 'S'
#define TERM 'T'

struct Task {
    int m1;
    int m2;
    double eval;
    Task() {}
    Task(int m1, int m2) : m1(m1), m2(m2) {}
};

double Evaluate(Board Current, data LastMover, int iLastCol, int iDepth);

void p(int *arr, int len) {
	for (int i=0; i<len; i++) {
		std::cout << arr[i] << ' ';
	}
	std::cout << std::endl;
}

int main(int argc, char *argv[]) {
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    const int nitems=3;
    int          blocklengths[nitems] = {1,1,1};
    MPI_Datatype types[nitems] = {MPI_INT, MPI_INT, MPI_DOUBLE};
    MPI_Datatype MPI_TASK_TYPE;
    MPI_Aint     offsets[nitems];

    offsets[0] = offsetof(Task, m1);
    offsets[1] = offsetof(Task, m2);
    offsets[2] = offsetof(Task, eval);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MPI_TASK_TYPE);
    MPI_Type_commit(&MPI_TASK_TYPE);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // postavi sjeme generatora slucajnih brojeva drukcije za svaki proces
    srand(time(NULL) + 10000*rank);

    char dummy;
    MPI_Status status;
    int rows = 6;
    int cols = 7;
    int DEPTH = atoi(argv[1]);

    int boardbuf[2 + rows*cols + cols];
    int bufsize = 2 + rows*cols + cols;


    if (rank == 0) {
        Board b(rows, cols);
        b.Print();
        int inputCol;
        bool end = false;
        while (1) {
            
            while (1) {
                std::cout << "stupac: ";
                std::cin >> inputCol;
                if (inputCol == -1) {
                    end = true;
                    break;
                }
                if (!b.MoveLegal(inputCol)) {
                    std::cout << "ilegalan potez.\n";
                } else break;
            }
            if (end) {
                // posalji svima poruku da terminiraju
                dummy = TERM;
                MPI_Bcast(&dummy, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
                break;
            }
            b.Move(inputCol, HUMAN);
            b.Print();
            if (b.GameEnd(inputCol)) {
                std::cout << "Pobijedio si.\n";
                end = true;
            }

            if (end) {
                // posalji svima poruku da terminiraju
                dummy = TERM;
                MPI_Bcast(&dummy, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
                break;
            }
            
            std::cout << "CPU racuna svoj sljedeci potez...\n";


            auto t1 = std::chrono::high_resolution_clock::now();
            

            // stvori zadatke
            Task tasks[cols*cols];
            int numtasks = 0;
            bool immediate_win = false;
            int best = -1;
            for (int i=0; i<b.Columns(); i++) {
                if (!b.MoveLegal(i)) continue;
                b.Move(i, CPU);
                if (b.GameEnd(i)) {
                    immediate_win=true;
                    best = i;
                    b.UndoMove(i);
                    break;
                }
                for (int j=0; j<b.Columns(); j++) {
                    if (b.MoveLegal(j)) {
                        tasks[numtasks] = Task(i, j);
                        numtasks++;
                    }
                }
                b.UndoMove(i);
            }

            
            
            if (!immediate_win) {
                dummy = START;
                MPI_Bcast(&dummy, 1, MPI_CHAR, 0, MPI_COMM_WORLD);


                //posalji svima stanje ploce
                b.tobuf(boardbuf);
                MPI_Bcast(boardbuf, bufsize, MPI_INT, 0, MPI_COMM_WORLD);
                // pocni primati zahtjeve
                int remaining_tasks = numtasks;
                int working = world_size - 1;
                Task task;
                Task task_dummy;
                Task result;
                double mat[cols][cols];
                for (int i=0;i<cols;i++) {
                    for (int j=0;j<cols;j++) {
                        mat[i][j] = -4.0;
                    }
                }

                while (working) {
                    MPI_Recv(&result, 1, MPI_TASK_TYPE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                    if (status.MPI_TAG == REQUEST_TAG) {
                        if (remaining_tasks > 0) {
                            // posalji zadatak jer ga ima
                            task = tasks[remaining_tasks-1];
                            remaining_tasks--;
                            MPI_Send(&task, 1, MPI_TASK_TYPE, status.MPI_SOURCE, TASK_SENT, MPI_COMM_WORLD);
                            //std::cout << "Poslao " << task << " radniku " << status.MPI_SOURCE << std::endl;
                        } else {
                            // posalji nema vise
                            working--;
                            MPI_Send(&task_dummy, 1, MPI_TASK_TYPE, status.MPI_SOURCE, NO_MORE_TASKS_TAG, MPI_COMM_WORLD);
                        }
                    }
                    if (status.MPI_TAG == TASK_DONE_TAG) {
                        // dobio sam rezultat
                        //std::cout << "Od radnika " << status.MPI_SOURCE << " sam dobio rezultat " << result << std::endl;
                        mat[result.m1][result.m2] = result.eval;
                    }
                    
                }
                // obradi matricu
                /*
                for (int i=0; i<cols; i++) {
                    for (int j=0; j<cols; j++) {
                        std::cout << mat[i][j] << ' '; 
                    }
                    std::cout << '\n';
                }*/


                
                double highest_score = -2.0;
                for (int i=0;i<cols;i++) {
                    double total_score = 0.0;
                    int valid = 0;
                    for (int j=0;j<cols;j++) {
                        if (mat[i][j] == -4.0) continue;
                        valid++;
                        total_score += mat[i][j];
                    }
                    if (valid == 0) continue;
                    double score = total_score / valid;
                    if (score > highest_score) {
                        highest_score = score;
                        best = i;
                    }
                }
            }

            auto t2 = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1);
            std::cout << "Potez izracunat u " << ms.count() << " milisekundi" << std::endl;

            

            b.Move(best, CPU);
            b.Print();
            if (b.GameEnd(best)) {
                std::cout << "CPU pobijedio.\n";
                end = true;
            }
            if (end) {
                // posalji svima poruku da terminiraju
                dummy = TERM;
                MPI_Bcast(&dummy, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
                break;
            }
            
        }


    } else {
        // slaves
        while (1) {
            MPI_Bcast(&dummy, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
            if (dummy == TERM)
                break;
            
            MPI_Bcast(boardbuf, bufsize, MPI_INT, 0, MPI_COMM_WORLD);
            Board b(boardbuf);


            //std::cout << "Radnik " << rank << " dobio obavijest, stanje ploce u nastavku." << std::endl;
            //p(boardbuf, bufsize);
            // pocni dohvacati zadatke
            
            

            Task task;
            Task result;
            Task task_dummy;
            int tasks_done = 0;

            while (1) {
                // posalji zahtjev za zadatkom
                MPI_Send(&task_dummy, 1, MPI_TASK_TYPE, 0, REQUEST_TAG, MPI_COMM_WORLD);
                // cekaj odgovor
                MPI_Recv(&task, 1, MPI_TASK_TYPE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                if (status.MPI_TAG == NO_MORE_TASKS_TAG)
                    break;
                
                // rijesi zadatak
                b.Move(task.m1, CPU);
                b.Move(task.m2, HUMAN);
                result.eval = Evaluate(b, HUMAN, task.m2, DEPTH);
                result.m1 = task.m1;
                result.m2 = task.m2;
                b.UndoMove(task.m2);
                b.UndoMove(task.m1);
                tasks_done++;
                // rijesi zadatak

                // posalji rezultat masteru
                MPI_Send(&result, 1, MPI_TASK_TYPE, 0, TASK_DONE_TAG, MPI_COMM_WORLD);
            }
            //std::cout << "Radnik " << rank << " ide doma nakon obavljenih " << tasks_done << " zadataka" << std::endl;
        }
        

    }

    // Finalize the MPI environment.
    MPI_Type_free(&MPI_TASK_TYPE);
    MPI_Finalize();
    return 0;
}



// rekurzivna funkcija: ispituje sve moguce poteze i vraca ocjenu dobivenog stanja ploce
// Current: trenutno stanje ploce
// LastMover: HUMAN ili CPU
// iLastCol: stupac prethodnog poteza
// iDepth: dubina se smanjuje do 0
double Evaluate(Board Current, data LastMover, int iLastCol, int iDepth)
{
	double dResult, dTotal;
	data NewMover;
	bool bAllLose = true, bAllWin = true;
	int iMoves;
	
	if(Current.GameEnd(iLastCol))	// igra gotova?
		if(LastMover == CPU)
			return 1;	// pobjeda
		else //if(LastMover == HUMAN)
			return -1;	// poraz
	// nije gotovo, idemo u sljedecu razinu
	if(iDepth == 0)
		return 0;	// a mozda i ne... :)
	iDepth--;
	if(LastMover == CPU)	// tko je na potezu
		NewMover = HUMAN;
	else
		NewMover = CPU;
	dTotal = 0;
	iMoves = 0;	// broj mogucih poteza u ovoj razini
	for(int iCol=0; iCol<Current.Columns(); iCol++)
	{	if(Current.MoveLegal(iCol))	// jel moze u stupac iCol
		{	iMoves++;
			Current.Move(iCol, NewMover);
			dResult = Evaluate(Current, NewMover, iCol, iDepth);
			Current.UndoMove(iCol);
			if(dResult > -1)
				bAllLose = false;
			if(dResult != 1)
				bAllWin = false;
			if(dResult == 1 && NewMover == CPU)
				return 1;	// ako svojim potezom mogu doci do pobjede (pravilo 1)
			if(dResult == -1 && NewMover == HUMAN)
				return -1;	// ako protivnik moze potezom doci do pobjede (pravilo 2)
			dTotal += dResult;
		}
	}
	if(bAllWin == true)	// ispitivanje za pravilo 3.
		return 1;
	if(bAllLose == true)
		return -1;
	dTotal /= iMoves;	// dijelimo ocjenu s brojem mogucih poteza iz zadanog stanja
	return dTotal;
}