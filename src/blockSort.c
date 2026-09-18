/* 블록 정렬 — 배열을 작은 블록으로 잘라 각각 정렬한 뒤, 그 블록들을 배로
 * 넓혀 가며 병합한다.
 *
 * 핵심은 병합에 보조 배열을 쓰지 않는 것이다. 대신 **회전(rotation)** 으로
 * 원소 덩어리를 통째로 옮긴다. 그래서 병합 정렬의 장점(안정 · 적은 비교
 * 횟수)을 O(1) 메모리로 얻는다.
 *
 * 여기 있는 것은 수업용으로 줄인 판이다. 실제 block sort(WikiSort)는 배열
 * 안에 내부 버퍼를 잡아 병합을 O(n log n)까지 끌어내리지만, 이 구현은 회전
 * 병합만 써서 O(n log^2 n)이다. 대신 200줄이 아니라 이 파일 하나로 읽힌다.
 */
#include "sort.h"

#include <assert.h>

#include "sortctx.h"

/* 블록 크기 — n과 무관한 상수다.
 *
 * 진짜 block sort(WikiSort)는 ceil(sqrt(n))을 쓴다. 다만 거기서 sqrt(n)은
 * 배열 안에 잡는 **내부 버퍼**의 크기이지 초기 블록의 길이가 아니다. 이
 * 구현은 내부 버퍼를 쓰지 않으므로 sqrt(n)을 따를 이유가 없다.
 *
 * 블록 하나를 삽입 정렬하는 비용은 O(B^2)이고 블록이 n/B개라 합이 O(n*B)다.
 * B에 정비례해 늘어난다. 반면 B를 키워서 아끼는 병합 비용은 log 수준으로만
 * 준다. 그래서 최적 B는 n과 무관한 작은 상수다. 재 보면 8~32에서 평평하다
 * (보고서 3.2 「블록 크기는 얼마가 좋은가」). timsort의 minrun이 32~64인 것도
 * 같은 이유다.
 *
 * 그 실험이 이 값을 바꿔 가며 재기 때문에 const가 아니다. */
size_t blockSortBlockSize = 32;

/* a[lo..hi)를 뒤집는다. */
static void reverseRange(SortCtx *c, size_t lo, size_t hi) {
    while (lo + 1 < hi) {
        sortSwap(c, lo, hi - 1);
        lo++;
        hi--;
    }
}

/* a[lo..mid)와 a[mid..hi)의 자리를 맞바꾼다. 뒤집기 세 번이면 된다.
 * 추가 메모리가 필요 없어서, 이 회전이 제자리 병합의 유일한 도구다. */
static void rotateRange(SortCtx *c, size_t lo, size_t mid, size_t hi) {
    if (lo == mid || mid == hi) {
        return;
    }
    reverseRange(c, lo, mid);
    reverseRange(c, mid, hi);
    reverseRange(c, lo, hi);
}

/* a[lo..hi) 중 a[key] 이상이 처음 나오는 자리. */
static size_t lowerBound(SortCtx *c, size_t lo, size_t hi, size_t key) {
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (sortCompareAt(c, mid, key) < 0) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

/* a[lo..hi) 중 a[key] 초과가 처음 나오는 자리. */
static size_t upperBound(SortCtx *c, size_t lo, size_t hi, size_t key) {
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (sortCompareAt(c, mid, key) <= 0) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo;
}

/* 이미 정렬된 a[lo..mid)와 a[mid..hi)를 제자리에서 병합한다.
 *
 * 긴 쪽의 한가운데 원소를 고르고, 반대쪽에서 그 원소가 들어갈 자리를 이분
 * 탐색으로 찾은 다음, 사이 구간을 회전시켜 그 원소를 확정한다. 남은 두
 * 구간은 같은 방법으로 다시 병합한다.
 *
 * 경계를 왼쪽에서 고를 때는 lowerBound, 오른쪽에서 고를 때는 upperBound를
 * 쓴다. 같은 값일 때 왼쪽 것이 앞에 남게 하려는 것이고, 이 선택 하나가
 * 안정성을 지킨다. */
static void mergeInPlace(SortCtx *c, size_t lo, size_t mid, size_t hi, size_t depth) {
    if (c->stats != NULL && depth > c->stats->maxDepth) {
        c->stats->maxDepth = depth;
    }
    if (lo >= mid || mid >= hi) {
        return;
    }
    /* 이어 붙이기만 하면 되는 상태면 할 일이 없다. */
    if (sortCompareAt(c, mid - 1, mid) <= 0) {
        return;
    }

    size_t i, j;
    /* 짧지 않은 쪽의 한가운데를 골라 반대쪽에 끼워 넣는다. 늘 긴 쪽을 반으로
     * 자르므로 재귀 깊이가 O(log n)으로 묶인다.
     *
     * '>' 가 아니라 '>=' 인 것이 중요하다. 길이가 같을 때 오른쪽으로 보내면,
     * 두 런이 모두 길이 1인 경우 j = mid + 0 = mid 가 되어 회전할 구간이
     * 비고, 같은 인자로 다시 불려 무한 재귀가 된다. 왼쪽으로 보내면 앞의
     * 이어붙이기 검사에서 a[mid-1] > a[mid] 임이 이미 확인됐으므로
     * lowerBound가 반드시 mid보다 큰 자리를 돌려주어 회전이 비지 않는다. */
    if (mid - lo >= hi - mid) {
        /* 왼쪽이 더 길거나 같다. 왼쪽 한가운데를 오른쪽에 끼워 넣는다. */
        i = lo + (mid - lo) / 2;
        j = lowerBound(c, mid, hi, i);
    } else {
        /* 오른쪽이 더 길다. 길이가 2 이상이므로 j > mid 가 보장된다. */
        j = mid + (hi - mid) / 2;
        i = upperBound(c, lo, mid, j);
    }
    /* a[i..mid)와 a[mid..j)를 맞바꾼다. 둘 중 한 자리는 반드시 움직이므로
     * 재귀는 매번 짧아진다. */
    rotateRange(c, i, mid, j);
    size_t newMid = i + (j - mid);
    /* 진행 보장: 아래 두 호출이 반드시 더 짧아야 한다. newMid > lo 가 그 조건이고,
     * 위의 '>=' 가 그것을 지킨다. 조건을 '>' 로 바꾸면 여기서 걸린다 — 그렇지
     * 않으면 같은 인자로 영원히 다시 불려 멈추지 않는다. */
    assert(newMid > lo);
    mergeInPlace(c, lo, i, newMid, depth + 1);
    mergeInPlace(c, newMid, j, hi, depth + 1);
}

void blockSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    SortCtx c;
    if (!sortBegin(&c, base, n, size, cmp, stats)) {
        return;
    }

    size_t block = blockSortBlockSize < 1 ? 1 : blockSortBlockSize;
    /* 1단계: 블록마다 삽입 정렬. 짧은 배열에서는 삽입 정렬이 제일 빠르다. */
    for (size_t lo = 0; lo < n; lo += block) {
        size_t hi = lo + block < n ? lo + block : n;
        insertionSortRange(&c, lo, hi);
    }
    /* 2단계: 이웃한 두 덩어리를 병합하며 폭을 배로 넓힌다. */
    for (size_t width = block; width < n; width *= 2) {
        for (size_t lo = 0; lo + width < n; lo += 2 * width) {
            size_t mid = lo + width;
            size_t hi = lo + 2 * width < n ? lo + 2 * width : n;
            mergeInPlace(&c, lo, mid, hi, 1);
        }
    }
    sortEnd(&c);
}
