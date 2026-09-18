/* 공통 인터페이스를 따르는 정렬 구현. 표준 라이브러리만 쓴다. */
#include "sort.h"

#include <stdlib.h>
#include <string.h>

void sortStatsReset(SortStats *stats) {
    if (stats == NULL) {
        return;
    }
    stats->compares = 0;
    stats->moves = 0;
    stats->extraBytes = 0;
    stats->maxDepth = 1; /* 재귀를 쓰지 않아도 깊이는 1로 센다 */
}

int sortCompareInt(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y); /* 뺄셈은 overflow가 날 수 있어 쓰지 않는다 */
}

/* --- 구현들이 함께 쓰는 작업 문맥 ------------------------------------- */

/* 정렬 함수의 인자 다섯 개를 한 덩어리로 들고 다닌다. tmp는 원소 하나를
 * 잠시 담아 두는 자리다. 여기 말고 추가로 잡는 메모리는 없다. */
typedef struct Ctx {
    char *base;
    size_t size;
    SortCompare cmp;
    SortStats *stats;
    char *tmp;
} Ctx;

static char *elemAt(const Ctx *c, size_t i) {
    return c->base + i * c->size;
}

static int compareAt(Ctx *c, size_t i, size_t j) {
    if (c->stats != NULL) {
        c->stats->compares++;
    }
    return c->cmp(elemAt(c, i), elemAt(c, j));
}

/* a[i]와 tmp에 담아 둔 원소를 비교한다. */
static int compareWithTmp(Ctx *c, size_t i) {
    if (c->stats != NULL) {
        c->stats->compares++;
    }
    return c->cmp(elemAt(c, i), c->tmp);
}

static void moveElem(Ctx *c, void *dst, const void *src) {
    memcpy(dst, src, c->size);
    if (c->stats != NULL) {
        c->stats->moves++;
    }
}

static void swapAt(Ctx *c, size_t i, size_t j) {
    moveElem(c, c->tmp, elemAt(c, i));
    moveElem(c, elemAt(c, i), elemAt(c, j));
    moveElem(c, elemAt(c, j), c->tmp);
}

/* 작업 문맥을 차린다. 실패하면 0. */
static int ctxInit(Ctx *c, void *base, size_t size, SortCompare cmp, SortStats *stats) {
    c->base = (char *)base;
    c->size = size;
    c->cmp = cmp;
    c->stats = stats;
    c->tmp = (char *)malloc(size);
    if (c->tmp == NULL) {
        return 0;
    }
    if (stats != NULL) {
        stats->extraBytes = size;
    }
    return 1;
}

static void ctxFree(Ctx *c) {
    free(c->tmp);
    c->tmp = NULL;
}

/* 정렬 함수 셋이 똑같이 하는 앞처리. 정렬할 것이 없으면 0을 돌려준다. */
static int sortBegin(Ctx *c, void *base, size_t n, size_t size,
                     SortCompare cmp, SortStats *stats) {
    sortStatsReset(stats);
    if (base == NULL || cmp == NULL || size == 0 || n < 2) {
        return 0;
    }
    return ctxInit(c, base, size, cmp, stats);
}

/* --- 삽입 정렬 --------------------------------------------------------- */

/* a[lo..hi)를 삽입 정렬한다. 블록 정렬이 블록 하나를 정리할 때도 부른다. */
static void insertionSortRange(Ctx *c, size_t lo, size_t hi) {
    for (size_t i = lo + 1; i < hi; i++) {
        /* 앞 원소가 더 크지 않으면 이미 제자리다. 이 검사 덕분에 정렬된
         * 입력에서는 비교 n-1번으로 끝난다. */
        if (compareAt(c, i - 1, i) <= 0) {
            continue;
        }
        /* a[i]를 따로 들고, 그보다 큰 원소들을 한 칸씩 뒤로 민다. */
        moveElem(c, c->tmp, elemAt(c, i));
        size_t j = i;
        /* '>'로 비교해 같은 값은 넘지 않는다. 그래서 안정 정렬이다. */
        while (j > lo && compareWithTmp(c, j - 1) > 0) {
            moveElem(c, elemAt(c, j), elemAt(c, j - 1));
            j--;
        }
        moveElem(c, elemAt(c, j), c->tmp);
    }
}

void insertionSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    Ctx c;
    if (!sortBegin(&c, base, n, size, cmp, stats)) {
        return;
    }
    insertionSortRange(&c, 0, n);
    ctxFree(&c);
}

/* --- 버블 정렬 --------------------------------------------------------- */

void bubbleSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    Ctx c;
    if (!sortBegin(&c, base, n, size, cmp, stats)) {
        return;
    }
    /* 한 번 훑을 때마다 가장 큰 값이 뒤로 밀려 자리를 잡는다. */
    for (size_t end = n; end > 1; end--) {
        int swapped = 0;
        for (size_t j = 1; j < end; j++) {
            /* '>'로 비교해 같은 값끼리는 자리를 바꾸지 않는다 (안정). */
            if (compareAt(&c, j - 1, j) > 0) {
                swapAt(&c, j - 1, j);
                swapped = 1;
            }
        }
        /* 한 바퀴 동안 교환이 없었다면 이미 정렬된 것이다. */
        if (!swapped) {
            break;
        }
    }
    ctxFree(&c);
}

/* --- 구현 표 ----------------------------------------------------------- */

const SortAlgorithm SORT_ALGORITHMS[] = {
    {"insertionSort", "O(n^2)", "O(1)", 1, insertionSort},
    {"bubbleSort",    "O(n^2)", "O(1)", 1, bubbleSort},
};

const size_t SORT_ALGORITHM_COUNT = sizeof(SORT_ALGORITHMS) / sizeof(SORT_ALGORITHMS[0]);
