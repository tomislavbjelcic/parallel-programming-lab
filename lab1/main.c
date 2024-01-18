#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <windows.h>

#define SECS_LOW 3
#define SECS_HIGH 6
#define MSECS_SLEEP_INTERVAL 1000

#define REQUEST_TAG 11      // oznaka zahtjeva vilice
#define SEND_TAG 12         // oznaka poruke u kojem saljemo cistu vilicu

#define LEFT 'L'
#define RIGHT 'R'

int printf_flush(const char *format, ...) {
    va_list args;
    va_start(args, format);

    int ret = vprintf(format, args);
    fflush(stdout);
    va_end(args);
    return ret;
}

int main(int argc, char **argv)
{
    
    // inicijaliziraj MPI okolinu
    MPI_Init(&argc, &argv);
    
    // dohvati indeks svakog procesa
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // dohvati broj procesa
    int n;
    MPI_Comm_size(MPI_COMM_WORLD, &n);

    // postavi sjeme generatora slucajnih brojeva drukcije za svaki proces
    srand(time(NULL) + 10000*rank);

    // alociraj varijable
    MPI_Status status;
    int flag;
    char dummy;     // slat cemo poruke s ovakvim sadrzajem da nikad ne bude prazna poruka
    int left_neighbor = (rank+1)%n; // smjer kazaljke sata: pomicanje u lijevo povecava indeks, osim zadnjeg
    int right_neighbor = (rank == 0) ? n-1 : rank-1;
    int left_fork = rank != n-1;    // na pocetku vilicu izmedu sebe i onog slijeva imam ja osim ako nisam zadnji
    int right_fork = rank == 0;     // vilicu izmedju sebe i onog sdesna nemam ja, osim ako nisam prvi
    int left_fork_clean = 0;        // na pocetku sve vilice su prljave
    int right_fork_clean = 0;
    int left_neighbor_requesting = 0;         // postoji li neobrađen zahtjev od susjeda
    int right_neighbor_requesting = 0;
    int left_pending = 0;                 // cekam li na odgovor lijevog susjeda
    int right_pending = 0;

    char tabs[rank+1];
    for (int i=0; i<rank; i++) {
        tabs[i] = '\t';
    }
    tabs[rank] = 0;

    while (1) {
        // misli slucajan broj sekundi
        printf_flush("%s%d: mislim...\n", tabs, rank);
        int ms = (rand() % (SECS_HIGH - SECS_LOW + 1) + SECS_LOW) * 1000;
        do {
            // misli neko fiksno vrijeme
            Sleep(MSECS_SLEEP_INTERVAL);
            ms -= MSECS_SLEEP_INTERVAL;
            // provjeri zahtjeve
            while (1) {
                MPI_Iprobe(MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &flag, &status);
                if (!flag) break;
                // prvo "konzumiraj poruku"
                MPI_Recv(&dummy, 1, MPI_CHAR, status.MPI_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &status);
                // ima zahtjeva, udovolji im - posalji im ciste vilice
                MPI_Send(&dummy, 1, MPI_CHAR, status.MPI_SOURCE, SEND_TAG, MPI_COMM_WORLD);
                // printf_flush("%s%d: poslao vilicu prema %d\n", tabs, rank, status.MPI_SOURCE);
                if (status.MPI_SOURCE == left_neighbor && dummy==LEFT) {
                    left_fork = 0;
                } else {
                    right_fork = 0;
                }
                
            }
            
        } while (ms > 0);

        while (!left_fork || !right_fork) {
            if (!left_fork && !left_pending) {
                // posalji zahtjev lijevom susjedu za njegovom desnom vilicom
                dummy = RIGHT;
                MPI_Send(&dummy, 1, MPI_CHAR, left_neighbor, REQUEST_TAG, MPI_COMM_WORLD);
                left_pending = 1;
                printf_flush("%s%d: trazim vilicu od %d\n", tabs, rank, left_neighbor);
            }
            if (!right_fork && !right_pending) {
                // posalji zahtjev desnom susjedu za njegovom lijevom vilicom
                dummy = LEFT;
                MPI_Send(&dummy, 1, MPI_CHAR, right_neighbor, REQUEST_TAG, MPI_COMM_WORLD);
                right_pending = 1;
                printf_flush("%s%d: trazim vilicu od %d\n", tabs, rank, right_neighbor);
            }

            MPI_Recv(&dummy, 1, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == SEND_TAG) {
                // dobio sam cistu vilicu
                // printf_flush("%s%d: dobio vilicu od %d\n", tabs, rank, status.MPI_SOURCE);
                if (status.MPI_SOURCE == left_neighbor && dummy==RIGHT) {
                    left_fork = 1;
                    left_fork_clean = 1;
                    left_pending = 0;
                } else {
                    right_fork = 1;
                    right_fork_clean = 1;
                    right_pending = 0;
                }
            }
            if (status.MPI_TAG == REQUEST_TAG) {
                if (status.MPI_SOURCE == left_neighbor && dummy==LEFT) {
                    if (left_fork_clean) {
                        // ako je vilica cista nemoj je slati, ali zabiljezi da postoji zahtjev
                        left_neighbor_requesting = 1;
                    } else {
                        // ako nije cista posalji je
                        MPI_Send(&dummy, 1, MPI_CHAR, left_neighbor, SEND_TAG, MPI_COMM_WORLD);
                        // printf_flush("%s%d: poslao vilicu prema %d\n", tabs, rank, left_neighbor);
                        left_fork = 0;
                        left_neighbor_requesting = 0;
                    }
                }
                if (status.MPI_SOURCE == right_neighbor && dummy==RIGHT) {
                    if (right_fork_clean) {
                        // ako je vilica cista nemoj je slati, ali zabiljezi da postoji zahtjev
                        right_neighbor_requesting = 1;
                    } else {
                        // ako nije cista posalji je
                        MPI_Send(&dummy, 1, MPI_CHAR, right_neighbor, SEND_TAG, MPI_COMM_WORLD);
                        // printf_flush("%s%d: poslao vilicu prema %d\n", tabs, rank, right_neighbor);
                        right_fork = 0;
                        right_neighbor_requesting = 0;
                    }
                }
            }
        }

        printf_flush("%s%d: jedem...\n", tabs, rank);
        left_fork_clean = 0;
        right_fork_clean = 0;

        // odgovori na postojece zahtjeve
        if (right_neighbor_requesting) {
            dummy = RIGHT;
            MPI_Send(&dummy, 1, MPI_CHAR, right_neighbor, SEND_TAG, MPI_COMM_WORLD);
            // printf_flush("%s%d: poslao vilicu prema %d\n", tabs, rank, right_neighbor);
            right_fork = 0;
            right_neighbor_requesting = 0;
        }
        if (left_neighbor_requesting) {
            dummy = LEFT;
            MPI_Send(&dummy, 1, MPI_CHAR, left_neighbor, SEND_TAG, MPI_COMM_WORLD);
            // printf_flush("%s%d: poslao vilicu prema %d\n", tabs, rank, left_neighbor);
            left_fork = 0;
            right_neighbor_requesting = 0;
        }



    }
    


    MPI_Finalize();
    return 0;
}
