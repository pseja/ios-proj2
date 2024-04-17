#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct arguments
{
    int L;  // počet lyžařů
    int Z;  // počet nástupních zastávek
    int K;  // kapacita skibusu
    int KL; // maximální čas v mikrosekundách, který lyžař čeká, než přijde na zastávku
    int TB; // maximální doba jízdy autobusu mezi dvěma zastávkami
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

int main(int argc, char **argv)
{
    Arguments args;
    if (handle_arguments(&args, argc, argv) != 0)
    {
        print_usage();
        return 1;
    }

    return 0;
}
