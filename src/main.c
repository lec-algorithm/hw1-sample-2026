/* 실행: make run-c */
#include <stdio.h>

#include "sort.h"

int main(void) {
    /* 구현 표를 훑기만 하면 된다. 부르는 쪽은 어떤 정렬인지 몰라도 된다. */
    for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
        const SortAlgorithm *algo = &SORT_ALGORITHMS[k];
        int a[] = {6, 8, 5, 9, 10, 1, 7, 2, 4, 3};
        size_t n = sizeof(a) / sizeof(a[0]);
        SortStats stats;

        algo->sort(a, n, sizeof(a[0]), sortCompareInt, &stats);

        printf("%-14s", algo->name);
        for (size_t i = 0; i < n; i++) {
            printf(" %d", a[i]);
        }
        printf("   (비교 %zu, 이동 %zu)\n", stats.compares, stats.moves);
    }
    return 0;
}
