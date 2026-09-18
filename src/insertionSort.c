/* 삽입 정렬 — 앞쪽을 정렬된 상태로 유지하며 원소를 하나씩 끼워 넣는다.
 *
 * O(n^2)이지만 거의 정렬된 입력에서는 O(n)에 가깝다. 짧은 배열에서 제일
 * 빠르기도 해서, 블록 정렬이 블록 하나를 정리할 때도 이 함수를 쓴다.
 */
#include "sort.h"

#include "sortctx.h"

/* a[lo..hi)를 삽입 정렬한다. */
void insertionSortRange(SortCtx *c, size_t lo, size_t hi) {
    for (size_t i = lo + 1; i < hi; i++) {
        /* 앞 원소가 더 크지 않으면 이미 제자리다. 이 검사 덕분에 정렬된
         * 입력에서는 비교 n-1번으로 끝난다. */
        if (sortCompareAt(c, i - 1, i) <= 0) {
            continue;
        }
        /* a[i]를 따로 들고, 그보다 큰 원소들을 한 칸씩 뒤로 민다. */
        sortMove(c, c->tmp, sortElemAt(c, i));
        size_t j = i;
        /* '>'로 비교해 같은 값은 넘지 않는다. 그래서 안정 정렬이다. */
        while (j > lo && sortCompareTmp(c, j - 1) > 0) {
            sortMove(c, sortElemAt(c, j), sortElemAt(c, j - 1));
            j--;
        }
        sortMove(c, sortElemAt(c, j), c->tmp);
    }
}

void insertionSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    SortCtx c;
    if (!sortBegin(&c, base, n, size, cmp, stats)) {
        return;
    }
    insertionSortRange(&c, 0, n);
    sortEnd(&c);
}
