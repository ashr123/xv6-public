#include "user.h"
#include "mmu.h"

int main()
{
    int i, j, pid, array_size = 20;
    int c = 1;
    int *arr[array_size];
    for (i = 0; i < array_size; i++)
    {
        arr[i] = malloc(PGSIZE);
        for (j = 0; j < 1; j++)
        {
            arr[i][j] = c++;
        }
    }
    printf(1, "Before fork\n");
    if ((pid = fork()) > 0)
    {
        printf(1, "In child proc\n");
        for (i = 0; i < array_size; i++)
        {
            if (arr[i][0] != i + 1)
                break;
            free(arr[i]);
        }
        if (i < array_size - 1)
            printf(2, "Fork not copied the pages correctly, index: %d.\n", i);
        else
            printf(1, "Fork copied all the pages correctly.\n");
    }
    else if (pid == 0)
    {
        for (i = 0; i < array_size; i++)
        {
            if (arr[i][0] != i + 1)
            {
                printf(2, "Expected: %d, found: %d\n", (i + 1), (arr[i][0]));
                break;
            }
            free(arr[i]);
        }
        if (i < array_size - 1)
            printf(2, "Some problem with paging, with i: %d.\n", i);
        else
            printf(1, "Paging working correctly.\n");
        wait();
    }
    else
        printf(2, "Fork failed!\n");
    exit();
}