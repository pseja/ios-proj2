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
#include <stdbool.h>

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
int *bus_stop = NULL;
int *num_of_skiers_in_bus = NULL;
int *waiting = NULL;
int *waiting_sum = NULL;
int *bus_capacity = NULL;
int *skiers_not_skiing = NULL;

sem_t *sem_file_write = NULL;
sem_t *sem_skier_waiting = NULL;
sem_t *bus_stops_semaphores = NULL;
sem_t *boarded = NULL;
sem_t *unboarded = NULL;

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
    // Po spuštění vypíše: A: BUS: started
    sem_wait(sem_file_write);
    fprintf(output_file, "%d: BUS: started\n", ++(*action_number));
    sem_post(sem_file_write);

    int idZ = 1;
    bool bus_finished_run = false;

    do
    {
        // (#) idZ = 1 (identifikace zastávky)
        if (*skiers_not_skiing > 0 && bus_finished_run)
        {
            idZ = 1;
            bus_finished_run = false;
        }

        // (*) Jede na zastávku idZ---čeká pomocí volání usleep náhodný čas v intervalu <0, TB>
        srand(getpid() + time(NULL));
        usleep(rand() % args->TB);

        // Vypíše: A: BUS: arrived to idZ
        sem_wait(sem_file_write);
        fprintf(output_file, "%d: BUS: arrived to %d\n", ++(*action_number), idZ);
        sem_post(sem_file_write);

        // Nechá nastoupit všechny čekající lyžaře do kapacity autobusu
        while (*num_of_skiers_in_bus < args->K && waiting[idZ - 1] > 0)
        {
            sem_post(&bus_stops_semaphores[idZ - 1]);
            sem_wait(&bus_stops_semaphores[idZ - 1]);
        }

        // Vypíše: A: BUS: leaving idZ
        sem_wait(sem_file_write);
        fprintf(output_file, "%d: BUS: leaving %d\n", ++(*action_number), idZ);
        sem_post(sem_file_write);

        // Pokud idZ<Z, tak idZ=idZ+1 a pokračuje bodem (*)
        if (idZ < args->Z)
        {
            idZ++;
            continue;
        }

        // Jinak jede na výstupní zastávku---čeká pomocí volání usleep náhodný čas v intervalu <0,TB>.
        usleep(rand() % args->TB);

        // Vypíše: A: BUS: arrived to final
        sem_wait(sem_file_write);
        fprintf(output_file, "%d: BUS: arrived to final\n", ++(*action_number));
        sem_post(sem_file_write);

        // Nechá vystoupit všechny lyžaře
        int skiers_in_bus_save = *num_of_skiers_in_bus;

        while (*num_of_skiers_in_bus > 0)
        {
            sem_post(boarded);
            (*num_of_skiers_in_bus)--;
        }

        for (int i = 0; i < skiers_in_bus_save; i++)
        {
            sem_wait(unboarded);
        }

        // Vypíše: A: BUS: leaving final
        sem_wait(sem_file_write);
        fprintf(output_file, "%d: BUS: leaving final\n", ++(*action_number));
        fprintf(output_file, "skiers_not_skiing: %d\n", *skiers_not_skiing);
        sem_post(sem_file_write);

        // Pokud ještě nějací lyžaři čekají na některé ze zastávek/mohou přijít na zastávku, tak pokračuje bodem (#)
        bus_finished_run = true;
    } while (*skiers_not_skiing > 0);

    // Jinak vypíše: A: BUS: finish
    sem_wait(sem_file_write);
    fprintf(output_file, "%d: BUS: finish\n", ++(*action_number));
    sem_post(sem_file_write);

    // Proces končí
    exit(0);
}

int skier_process(Arguments *args, int idZ, int idL)
{
    // Každý lyžař je jednoznačně identifikován číslem idL, 0<idL<=L
    // parametr funkce

    // Po spuštění vypíše: A: L idL: started
    sem_wait(sem_file_write);
    fprintf(output_file, "%d: L %d: started\n", ++(*action_number), idL);
    sem_post(sem_file_write);

    // Dojí snídani---čeká v intervalu <0,TL> mikrosekund.
    srand(getpid() + time(NULL));
    usleep(rand() % args->TL);

    // Jde na přidělenou zastávku idZ.

    // Čeká na příjezd skibusu
    sem_wait(&sem_skier_waiting[idZ - 1]);
    waiting[idZ - 1]++;
    (*waiting_sum)++;
    sem_post(&sem_skier_waiting[idZ - 1]);

    // Vypíše: A: L idL: arrived to idZ
    sem_wait(sem_file_write);
    fprintf(output_file, "%d: L %d: arrived to %d\n", ++(*action_number), idL, idZ);
    sem_post(sem_file_write);

    // Po příjezdu skibusu nastoupí (pokud je volná kapacita)
    sem_wait(&bus_stops_semaphores[idZ - 1]);
    (*num_of_skiers_in_bus)++;
    waiting[idZ - 1]--;
    (*waiting_sum)--;

    // Vypíše: A: L idL: boarding
    sem_wait(sem_file_write);
    fprintf(output_file, "%d: L %d: boarding\n", ++(*action_number), idL);
    sem_post(sem_file_write);
    sem_post(&bus_stops_semaphores[idZ - 1]);

    // Vyčká na příjezd skibusu k lanovce.
    sem_wait(boarded);

    // Vypíše: A: L idL: going to ski
    sem_wait(sem_file_write);
    fprintf(output_file, "%d: L %d: going to ski\n", ++(*action_number), idL);
    (*skiers_not_skiing)--;
    sem_post(sem_file_write);

    sem_post(unboarded);

    // Proces končí
    exit(0);
}

void alocate_shared_memory(Arguments *args)
{
    // shared variables
    action_number = MAP(action_number); // chyba MAP_ANONYMOUS protože windows, může se ignorovat
    bus_stop = MAP(bus_stop);
    num_of_skiers_in_bus = MAP(num_of_skiers_in_bus);
    waiting = (int *)mmap(NULL, sizeof(int) * args->Z, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    waiting_sum = MAP(waiting_sum);
    *waiting_sum = 0;
    bus_capacity = MAP(bus_capacity);
    skiers_not_skiing = MAP(skiers_not_skiing);
    *skiers_not_skiing = args->L;

    // shared semaphores
    sem_file_write = MAP(sem_file_write);
    sem_skier_waiting = (sem_t *)mmap(NULL, sizeof(sem_t) * args->Z, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    bus_stops_semaphores = (sem_t *)mmap(NULL, sizeof(sem_t) * args->Z, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    boarded = MAP(boarded);
    unboarded = MAP(unboarded);
}

void unallocate_shared_memory(Arguments *args)
{
    // unmap variables
    UNMAP(action_number);
    UNMAP(bus_stop);
    UNMAP(num_of_skiers_in_bus);
    munmap(waiting, sizeof(int) * args->Z);
    UNMAP(waiting_sum);
    UNMAP(bus_capacity);
    UNMAP(skiers_not_skiing);

    // unmap semaphores
    UNMAP(sem_file_write);
    munmap(sem_skier_waiting, sizeof(sem_t) * args->Z);
    munmap(bus_stops_semaphores, sizeof(sem_t) * args->Z);
    UNMAP(boarded);
    UNMAP(unboarded);
}

int initialize_semaphores(Arguments *args)
{
    if (sem_init(sem_file_write, 1, 1) == -1)
    {
        fprintf(stderr, "Initialization of semaphore sem_file_write failed\n");
        return 1;
    }

    if (sem_init(boarded, 1, 0) == -1)
    {
        fprintf(stderr, "Initialization of semaphore boarded failed\n");
        return 1;
    }

    if (sem_init(unboarded, 1, 0) == -1)
    {
        fprintf(stderr, "Initialization of semaphore unboarded failed\n");
        return 1;
    }

    for (int i = 0; i < args->Z; i++)
    {
        if (sem_init(&sem_skier_waiting[i], 1, 1) == -1)
        {
            perror("sem_init");
            return 1;
        }
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
    sem_destroy(sem_file_write);
    sem_destroy(boarded);
    sem_destroy(unboarded);

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
        exit(1);
    }
    else if (skibusPID == 0)
    {
        skibus_process(&args);

        exit(0);
    }

    // Dále vytvoří L procesů lyžařů.
    for (int i = 0; i < args.L; i++)
    {
        pid_t skierPID = fork();
        if (skierPID < 0)
        {
            fprintf(stderr, "Skibus proces problém\n");
            exit(1);
        }
        else if (skierPID == 0)
        {
            // Každému lyžaři při jeho spuštění náhodně přidělí nástupní zastávku.
            srand(getpid() * time(NULL));
            int idZ = (rand() % args.Z) + 1;

            skier_process(&args, idZ, i + 1);

            exit(0);
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
