#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h> // pid_t type
#include <sys/wait.h>
#include <time.h>
#include <semaphore.h>
#include <sys/mman.h>

// TODO POLE SEMAFORŮ

#define MAX(x, y) (((x) >= (y)) ? (x) : (y))
#define MIN(x, y) (((x) <= (y)) ? (x) : (y))

#define MAP(pointer) mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)
#define SLEEP(time)                     \
    {                                   \
        if (time != 0)                  \
            sleep((rand() % time) + 1); \
    }
#define UNMAP(pointer) munmap((pointer), sizeof((pointer)))

typedef struct arguments
{
    int L;  // počet lyžařů, L < 20'000
    int Z;  // počet nástupních zastávek 0 < Z <= 10
    int K;  // kapacita skibusu, 10 <= K <= 100
    int TL; // Maximální čas v mikrosekundách, který lyžař čeká, než přijde na zastávku, 0 <= TL <= 10'000
    int TB; // Maximální doba jízdy autobusu mezi dvěma zastávkami, 0 <= TB <= 1'000
} Arguments;

FILE *output_file;
int *action_number = NULL;
int *num_of_skiers_in_bus = NULL;
int *bus_capacity = NULL;
int *waiting = NULL;
int *waiting_sum = NULL;
sem_t *mutex = NULL;
sem_t *bus = NULL;
sem_t *boarded = NULL;
sem_t *bus_stops_semaphores = NULL;

void print_usage()
{
    fprintf(stderr, "Usage: ./proj2 L Z K TL TB\n- L: počet lyžařů, L < 20'000\n- Z: počet nástupních zastávek 0 < Z <= 10\n- K: kapacita skibusu, 10 <= K <= 100\n- TL: Maximální čas v mikrosekundách, který lyžař čeká, než přijde na zastávku, 0 <= TL <= 10'000\n- TB: Maximální doba jízdy autobusu mezi dvěma zastávkami, 0 <= TB <= 1'000\n");
}

int check_argument_validity(char *argument, int argument_number, int from, int to)
{
    char *valid_numbers = "0123456789";

    if ((strspn(argument, valid_numbers) != strlen(argument)) || (atoi(argument) < from) || (atoi(argument) > to))
    {
        return argument_number;
    }

    return 0;
}

int handle_arguments(Arguments *args, int argc, char **argv)
{
    if (argc != 6)
    {
        return 1;
    }

    int from[5] = {0, 0 + 1, 10, 0, 0};
    int to[5] = {20000 - 1, 10, 100, 10000, 1000};

    for (int i = 1; i < argc; i++)
    {
        if (check_argument_validity(argv[i], i, from[i - 1], to[i - 1]) != 0)
        {
            return i;
        }
    }

    args->L = atoi(argv[1]);
    args->Z = atoi(argv[2]);
    args->K = atoi(argv[3]);
    args->TL = atoi(argv[4]);
    args->TB = atoi(argv[5]);

    return 0;
}

int skibus_process(Arguments *args)
{
    // INICIALIZACE
    // ----------------------------
    // waiting = 0
    // mutex = new Semaphore(1)
    // bus = new Semaphore(0)
    // boarded = new Semaphore(0)

    // BUS
    // ----------------------------
    // mutex.wait()
    // n = min(waiting, 50)
    // for i in range(n):
    //     bus.signal()
    //     boarded.wait()
    //
    // waiting = max(waiting-50, 0)
    // mutex.signal()
    //
    // depart()

    // Po spuštění vypíše: A: BUS: started
    fprintf(output_file, "%d: BUS: started\n", *action_number);

    // (#) idZ = 1 (identifikace zastávky)
    do
    {
        int idZ = 1;
        *num_of_skiers_in_bus = 0;

        // TODO tohle je možná do while
        while (idZ <= args->Z)
        {
            // (*) Jede na zastávku idZ---čeká pomocí volání usleep náhodný čas v intervalu <0, TB>
            usleep(rand() % args->TB);

            // Vypíše: A: BUS: arrived to idZ
            fprintf(output_file, "%d: BUS: arrived to %d\n", ++(*action_number), idZ);

            // Nechá nastoupit všechny čekající lyžaře do kapacity autobusu
            while (waiting[idZ - 1] > 0)
            {
                ;
            }

            // depart()

            // Vypíše: A: BUS: leaving idZ
            fprintf(output_file, "%d: BUS: leaving %d\n", ++(*action_number), idZ);

            // Pokud idZ<Z, tak idZ=idZ+1 a pokračuje bodem (*)
            if (idZ < args->Z)
            {
                idZ++;
            }
            else
            {
                break;
            }
        }

        // Jinak jede na výstupní zastávku---čeká pomocí volání usleep náhodný čas v intervalu <0,TB>.
        usleep(rand() % args->TB);

        // Vypíše: A: BUS: arrived to final
        fprintf(output_file, "%d: BUS: arrived to final\n", ++(*action_number));

        // Nechá vystoupit všechny lyžaře

        // Vypíše: A: BUS: leaving final
        fprintf(output_file, "%d: BUS: leaving final\n", ++(*action_number));

        // Pokud ještě nějací lyžaři čekají na některé ze zastávek/mohou přijít na zastávku, tak pokračuje bodem (#)
    } while (*waiting_sum > 0);

    // Jinak vypíše: A: BUS: finish
    fprintf(output_file, "%d: BUS: finish\n", ++(*action_number));

    // Proces končí
    return 0;
}

int skier_process(Arguments *args, int idZ, int i)
{
    // LYŽAŘI
    // ----------------
    // mutex.wait()
    //     waiting += 1
    // mutex.signal()
    //
    // bus.wait()
    // board()
    // boarded.signal()

    // Každý lyžař je jednoznačně identifikován číslem idL, 0<idL<=L
    int idL = i;

    // Po spuštění vypíše: A: L idL: started
    fprintf(output_file, "%d: L %d: started\n", ++(*action_number), idL);

    // Dojí snídani---čeká v intervalu <0,TL> mikrosekund.
    usleep(rand() % args->TL);

    // Jde na přidělenou zastávku idZ.

    // Vypíše: A: L idL: arrived to idZ
    fprintf(output_file, "%d: L %d: arrived to %d\n", ++(*action_number), idL, idZ);

    waiting[idZ - 1]++;
    (*waiting_sum)++;

    // Čeká na příjezd skibusu
    sem_post(&bus_stops_semaphores[idZ - 1]);

    // Po příjezdu skibusu nastoupí (pokud je volná kapacita)
    if (*num_of_skiers_in_bus <= args->K)
    {
        (*num_of_skiers_in_bus)++;
        waiting[idZ - 1]--;
        (*waiting_sum)--;
    }

    // Vypíše: A: L idL: boarding
    fprintf(output_file, "%d: L %d: boarding\n", ++(*action_number), idL);

    // Vyčká na příjezd skibusu k lanovce.

    // Vypíše: A: L idL: going to ski
    fprintf(output_file, "%d: L %d: going to ski\n", ++(*action_number), idL);

    // Proces končí
    return 0;
}

int initialize_semaphore(sem_t *semaphore, int initialization_value)
{
    semaphore = MAP(semaphore);
    if (semaphore == MAP_FAILED)
    {
        fprintf(stderr, "Creating mutual exclusion semaphore failed\n");
        return 1;
    }
    else if (sem_init(semaphore, 1, initialization_value) == -1)
    {
        fprintf(stderr, "Initialization of mutual exclusion semaphore failed\n");
        return 1;
    }
    return 0;
}

void alocate_shared_memory(Arguments *args)
{
    action_number = MAP(action_number); // chyba MAP_ANONYMOUS protože windows, může se ignorovat
    (*action_number)++;
    num_of_skiers_in_bus = MAP(num_of_skiers_in_bus);
    waiting = (int *)mmap(NULL, sizeof(*waiting) * args->Z, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    waiting_sum = MAP(waiting_sum);
    bus_capacity = MAP(bus_capacity);
    bus_stops_semaphores = (sem_t *)mmap(NULL, sizeof(*bus_stops_semaphores) * args->Z, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

void unallocate_shared_memory(Arguments *args)
{
    UNMAP(action_number);
    UNMAP(num_of_skiers_in_bus);
    munmap(waiting, sizeof(int) * args->Z);
    UNMAP(waiting_sum);
    UNMAP(bus_capacity);
    UNMAP(mutex);
    UNMAP(bus);
    UNMAP(boarded);
}

int initialize_semaphores(Arguments *args)
{
    if (initialize_semaphore(mutex, 1) != 0)
    {
        return 1;
    }
    if (initialize_semaphore(bus, 0) != 0)
    {
        return 1;
    }
    if (initialize_semaphore(boarded, 0) != 0)
    {
        return 1;
    }

    for (int i = 0; i < args->Z; i++)
    {
        if (sem_init(&bus_stops_semaphores[i], 1, 0) == -1)
        {
            perror("sem_init");
            return 1;
        }
    }

    return 0;
}

void destroy_semaphores(Arguments *args)
{
    sem_destroy(mutex);
    sem_destroy(bus);
    sem_destroy(boarded);

    for (int i = 0; i < args->Z; i++)
    {
        sem_destroy(&bus_stops_semaphores[i]);
    }
}

int main(int argc, char **argv)
{
    Arguments args;
    if (handle_arguments(&args, argc, argv) != 0)
    {
        print_usage();
        return 1;
    }

    if ((output_file = fopen("proj2.out", "w+")) == NULL)
    {
        fprintf(stderr, "Cannot open output.out file\n");
        return 2;
    }

    // Vypnutí bufferování výstupu
    setbuf(output_file, NULL);
    setbuf(stderr, NULL);

    // Alokování sdílená paměti
    alocate_shared_memory(&args);

    if (initialize_semaphores(&args) != 0)
    {
        return 1;
    }

    // Proces vytváří ihned po spuštění proces skibus.
    pid_t skibusPID = fork();
    if (skibusPID < 0)
    {
        fprintf(stderr, "Skibus proces problém\n");
        return 1;
    }
    else if (skibusPID == 0)
    {
        skibus_process(&args);

        return 0;
    }

    // Dále vytvoří L procesů lyžařů.
    for (int i = 0; i < args.L; i++)
    {
        pid_t skierPID = fork();
        if (skierPID < 0)
        {
            fprintf(stderr, "Skibus proces problém\n");
            return 1;
        }
        else if (skierPID == 0)
        {
            // Každému lyžaři při jeho spuštění náhodně přidělí nástupní zastávku.
            srand(getpid() * time(NULL));
            int idZ = (rand() % args.Z) + 1;

            skier_process(&args, idZ, i + 1);

            return 0;
        }
    }

    // Poté čeká na ukončení všech procesů, které aplikace vytváří.
    while (wait(NULL) > 0)
    {
        ;
    }

    if (fclose(output_file) == EOF)
    {
        fprintf(stderr, "Error closing output.out");
        return 3;
    }

    // znič semafory
    destroy_semaphores(&args);

    // Odalokování sdílené paměti
    unallocate_shared_memory(&args);

    // Jakmile jsou tyto procesy ukončeny, ukončí se i hlavní proces s kódem (exit code) 0.
    return 0;
}
