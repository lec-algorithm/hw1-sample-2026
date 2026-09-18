/* 정렬 비교 — 삽입 / 버블 / 블록.
 * 실행: make run-c
 *
 * 부르는 쪽은 정렬 이름을 하나도 적지 않는다. 구현 표(SORT_ALGORITHMS)를
 * 훑을 뿐이다. 정렬을 하나 더 만들어 표에 넣으면 이 표도 한 줄 늘어난다.
 */
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"
#include "sort.h"

#define ROW_FORMAT "%-14s %9.3f %12zu %12zu %6zu B %8zu %5s %6s\n"
#define ROW_HEADER "알고리즘        시간(ms)         비교         이동   메모리 재귀깊이  정렬 안정성\n"
#define ROW_RULE   "---------------------------------------------------------------------------------\n"

static void printRow(const BenchResult *r) {
    printf(ROW_FORMAT, r->algo->name, r->millis, r->stats.compares, r->stats.moves,
           r->stats.extraBytes, r->stats.maxDepth, r->sorted ? "yes" : "NO!",
           r->stable ? "yes" : "no");
}

/* 구현 표가 뭐라고 주장하는지 먼저 보여 준다. 아래 측정과 견줘 보라고. */
static void printDeclarations(void) {
    printf("구현 표 (SortAlgorithm이 주장하는 값)\n");
    /* 한글은 터미널에서 두 칸을 쓴다. %-14s는 바이트를 세므로 머리글은 손으로 맞춘다. */
    printf("알고리즘       시간복잡도     메모리     안정성\n");
    printf("%s", ROW_RULE);
    for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
        const SortAlgorithm *algo = &SORT_ALGORITHMS[k];
        printf("%-14s %-14s %-10s %s\n", algo->name, algo->timeComplexity,
               algo->spaceComplexity, algo->stable ? "stable" : "unstable");
    }
    printf("\n");
}

/* 입력 모양을 바꿔 가며 같은 n으로 잰다. */
static void compareInputKinds(size_t n, int reps) {
    Record *input = (Record *)malloc(n * sizeof(Record));
    if (input == NULL) {
        return;
    }
    printf("입력 모양별 비교 (n = %zu, %d회 평균)\n", n, reps);
    for (int kind = 0; kind < INPUT_KIND_COUNT; kind++) {
        makeInput(input, n, (InputKind)kind, 20260901u);
        printf("\n[%s]\n", inputKindName((InputKind)kind));
        printf("%s", ROW_HEADER);
        printf("%s", ROW_RULE);
        for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
            BenchResult r = benchRun(&SORT_ALGORITHMS[k], input, n, reps);
            printRow(&r);
        }
    }
    printf("\n");
    free(input);
}

/* n을 배로 늘리며 잰다. 시간이 4배로 뛰면 O(n^2)다. */
static void compareGrowth(void) {
    static const size_t SIZES[] = {1000, 2000, 4000, 8000};
    const size_t count = sizeof(SIZES) / sizeof(SIZES[0]);

    printf("n을 키우며 (무작위 입력)\n");
    for (size_t s = 0; s < count; s++) {
        size_t n = SIZES[s];
        Record *input = (Record *)malloc(n * sizeof(Record));
        if (input == NULL) {
            return;
        }
        makeInput(input, n, INPUT_RANDOM, 20260901u);
        printf("\n[n = %zu]\n", n);
        printf("%s", ROW_HEADER);
        printf("%s", ROW_RULE);
        for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
            BenchResult r = benchRun(&SORT_ALGORITHMS[k], input, n, 1);
            printRow(&r);
        }
        free(input);
    }
    printf("\n");
}

int main(void) {
    printf("=== 정렬 비교: 삽입 · 버블 · 블록 ===\n");
    printf("원소는 (key, tag) %zu바이트. key로 정렬하고 tag로 안정성을 본다.\n\n",
           sizeof(Record));

    printDeclarations();
    compareInputKinds(4000, 3);
    compareGrowth();

    printf("읽는 법\n");
    printf("  시간   : 같은 기계에서만 견준다. 비교·이동 횟수가 더 믿을 만하다.\n");
    printf("  메모리 : 셋 다 제자리 정렬이라 원소 한 칸(%zu B)뿐이다. 블록 정렬은\n",
           sizeof(Record));
    printf("           그 값을 지키면서 비교 횟수를 O(n^2)에서 끌어내린 것이 핵심이다.\n");
    printf("           대신 재귀 깊이가 1이 아니다 — 그만큼 스택을 쓴다.\n");
    printf("  안정성 : 표의 주장이 아니라 tag 순서로 실측한 값이다.\n");
    return 0;
}
