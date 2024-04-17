#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h> // pid_t type
#include <sys/wait.h>
#include <time.h>

typedef struct arguments
{
    int L;  // počet lyžařů, L < 20'000
    int Z;  // počet nástupních zastávek 0 < Z <= 10
    int K;  // kapacita skibusu, 10 <= K <= 100
    int KL; // Maximální čas v mikrosekundách, který lyžař čeká, než přijde na zastávku, 0 <= TL <= 10'000
    int TB; // Maximální doba jízdy autobusu mezi dvěma zastávkami, 0 <= TB <= 1'000
} Arguments;

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
    args->KL = atoi(argv[4]);
    args->TB = atoi(argv[5]);

    return 0;
}

int skibus_process(Arguments *args)
{
    printf("skibus(%d) created, it's parent is %d\n    > needs to go to %d stops\n    > capacity: %d skiers\n", getpid(), getppid(), args->Z, args->K);
    return 0;
}

int skier_process(int idZ, int i)
{
    printf("%d. skier(%d) created, it's parent is %d and it's bus stop is: %d\n", i, getpid(), getppid(), idZ);
    return 0;
}

int main(int argc, char **argv)
{
    Arguments args;
    if (handle_arguments(&args, argc, argv) != 0)
    {
        print_usage();
        return 1;
    }

    setbuf(stdout, NULL);

    // Proces vytváří ihned po spuštění proces skibus.
    if (fork() == 0)
    {
        skibus_process(&args);
        exit(0);
    }

    // Dále vytvoří L procesů lyžařů.
    for (int i = 0; i < args.L; i++)
    {
        if (fork() == 0)
        {
            // Každému lyžaři při jeho spuštění náhodně přidělí nástupní zastávku.
            srand(getpid() + time(NULL));
            int idZ = (rand() % args.Z) + 1;

            skier_process(idZ, i);

            exit(0);
        }
    }

    // Poté čeká na ukončení všech procesů, které aplikace vytváří.
    while (wait(NULL) > 0)
        ;

    // Jakmile jsou tyto procesy ukončeny, ukončí se i hlavní proces s kódem (exit code) 0.
    return 0;
}
