/* 유닛 테스트 — 외부 프레임워크 없이 표준 C만 쓴다.
 * 실행: make test-c
 *
 * 테스트도 공통 인터페이스로 쓴다. 구현 표(SORT_ALGORITHMS)를 훑으며
 * 모든 정렬에 같은 검사를 돌리므로, 정렬을 하나 더 넣어도 테스트는 그대로다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sort.h"

static int checks = 0;
static int failures = 0;

static void report(const char *algo, const char *name, int ok) {
    checks++;
    if (ok) {
        printf("ok    %-14s %s\n", algo, name);
        return;
    }
    failures++;
    printf("FAIL  %-14s %s\n", algo, name);
}

/* --- int 배열 --------------------------------------------------------- */

static void expectSorted(const SortAlgorithm *algo, const char *name,
                         const int input[], const int want[], size_t n) {
    int a[32];
    SortStats stats;

    memcpy(a, input, n * sizeof(int));
    algo->sort(a, n, sizeof(a[0]), sortCompareInt, &stats);

    int ok = (n == 0) || memcmp(a, want, n * sizeof(int)) == 0;
    report(algo->name, name, ok);
    if (!ok) {
        printf("      got :");
        for (size_t i = 0; i < n; i++) {
            printf(" %d", a[i]);
        }
        printf("\n      want:");
        for (size_t i = 0; i < n; i++) {
            printf(" %d", want[i]);
        }
        printf("\n");
    }
}

/* --- 안정성 ----------------------------------------------------------- */

/* key로 정렬하고 tag에는 입력 순서를 담아 둔다. 정렬 뒤에도 같은 key끼리
 * tag가 오름차순이면 안정 정렬이다. */
typedef struct Tagged {
    int key;
    int tag;
} Tagged;

static int taggedCompare(const void *a, const void *b) {
    int x = ((const Tagged *)a)->key;
    int y = ((const Tagged *)b)->key;
    return (x > y) - (x < y);
}

static void expectStable(const SortAlgorithm *algo) {
    enum { N = 60 };
    Tagged a[N];
    SortStats stats;

    /* key는 0~4만 쓴다. 중복이 많아야 안정성이 드러난다. */
    for (int i = 0; i < N; i++) {
        a[i].key = (i * 7) % 5;
        a[i].tag = i;
    }
    algo->sort(a, N, sizeof(a[0]), taggedCompare, &stats);

    int ok = 1;
    for (int i = 1; i < N; i++) {
        if (a[i - 1].key > a[i].key) {
            ok = 0; /* 정렬조차 안 됐다 */
        }
        if (a[i - 1].key == a[i].key && a[i - 1].tag > a[i].tag) {
            ok = 0; /* 같은 key인데 입력 순서가 뒤집혔다 */
        }
    }
    /* 구현 표의 stable 값이 실측과 맞는지 함께 본다. */
    report(algo->name, "안정성 (표의 stable 값과 일치)", ok == algo->stable);
}

/* --- 난수 배열을 qsort 결과와 맞춰 본다 ------------------------------- */

static void expectMatchesQsort(const SortAlgorithm *algo) {
    enum { N = 500 };
    int *a = malloc(N * sizeof(int));
    int *want = malloc(N * sizeof(int));
    SortStats stats;

    srand(20260901); /* 씨앗을 고정해 매번 같은 입력을 쓴다 */
    for (int i = 0; i < N; i++) {
        a[i] = rand() % 100; /* 중복이 섞이도록 좁은 범위를 쓴다 */
        want[i] = a[i];
    }
    qsort(want, N, sizeof(want[0]), sortCompareInt);
    algo->sort(a, N, sizeof(a[0]), sortCompareInt, &stats);

    report(algo->name, "난수 500개가 qsort 결과와 같다",
           memcmp(a, want, N * sizeof(int)) == 0);
    free(a);
    free(want);
}

/* --- 측정값이 채워지는지 --------------------------------------------- */

static void expectStats(const SortAlgorithm *algo) {
    int a[] = {5, 1, 4, 2, 3};
    SortStats stats;

    algo->sort(a, 5, sizeof(a[0]), sortCompareInt, &stats);
    report(algo->name, "측정값이 채워진다",
           stats.compares > 0 && stats.moves > 0 &&
           stats.extraBytes == sizeof(a[0]) && stats.maxDepth >= 1);
}

/* --- 전부 돌린다 ------------------------------------------------------ */

int main(void) {
    for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
        const SortAlgorithm *algo = &SORT_ALGORITHMS[k];
        {
            const int a[] = {6, 8, 5, 9, 10, 1, 7, 2, 4, 3};
            const int want[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            expectSorted(algo, "섞인 배열", a, want, 10);
        }
        {
            const int a[] = {1, 2, 3, 4, 5};
            const int want[] = {1, 2, 3, 4, 5};
            expectSorted(algo, "이미 정렬된 배열", a, want, 5);
        }
        {
            const int a[] = {5, 4, 3, 2, 1};
            const int want[] = {1, 2, 3, 4, 5};
            expectSorted(algo, "역순 배열", a, want, 5);
        }
        {
            const int a[] = {3, 1, 3, 1, 2};
            const int want[] = {1, 1, 2, 3, 3};
            expectSorted(algo, "중복이 있는 배열", a, want, 5);
        }
        {
            const int a[] = {2, 2, 2, 2};
            const int want[] = {2, 2, 2, 2};
            expectSorted(algo, "모두 같은 값", a, want, 4);
        }
        {
            const int a[] = {42};
            const int want[] = {42};
            expectSorted(algo, "원소 하나", a, want, 1);
        }
        {
            /* n = 0이면 배열을 건드리지 않는다. */
            const int a[1] = {0};
            const int want[1] = {0};
            expectSorted(algo, "빈 배열", a, want, 0);
        }
        expectStable(algo);
        expectMatchesQsort(algo);
        expectStats(algo);
        printf("\n");
    }

    printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
