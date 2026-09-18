/* 버블 정렬 — 이웃한 두 원소를 견주며 큰 값을 뒤로 밀어낸다.
 *
 * 비교 횟수는 삽입 정렬과 비슷하지만 이동이 훨씬 많다. 교환 한 번이 이동
 * 세 번이기 때문이다. 셋 중에 제일 느린 것은 그래서다.
 */
#include "sort.h"

#include "sortctx.h"

void bubbleSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    SortCtx c;
    if (!sortBegin(&c, base, n, size, cmp, stats)) {
        return;
    }
    /* 한 번 훑을 때마다 가장 큰 값이 뒤로 밀려 자리를 잡는다. */
    for (size_t end = n; end > 1; end--) {
        int swapped = 0;
        for (size_t j = 1; j < end; j++) {
            /* '>'로 비교해 같은 값끼리는 자리를 바꾸지 않는다 (안정). */
            if (sortCompareAt(&c, j - 1, j) > 0) {
                sortSwap(&c, j - 1, j);
                swapped = 1;
            }
        }
        /* 한 바퀴 동안 교환이 없었다면 이미 정렬된 것이다. */
        if (!swapped) {
            break;
        }
    }
    sortEnd(&c);
}
