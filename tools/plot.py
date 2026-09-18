"""비교 결과를 그래프(SVG)로 그린다.

    make charts          # 또는
    python3 tools/plot.py

src/main.out --csv 를 돌려 측정값을 받아 docs/ 아래에 SVG를 쓴다. 사람이 읽는
표를 파싱하지 않고 CSV를 쓰는 이유는, 표의 모양이 바뀌어도 그래프가 깨지지
않게 하려는 것이다.

표준 모듈만 쓴다. 그림은 tools/svgchart.py가 직접 찍어 낸다.
"""

import csv
import io
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import svgchart  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
BINARY = ROOT / "src" / "main.out"
OUT_DIR = ROOT / "docs"
ALGOS = ["insertionSort", "bubbleSort", "blockSort"]
KIND_LABEL = {
    "random": "무작위",
    "sorted": "정렬됨",
    "reversed": "역순",
    "few-unique": "중복많음",
}


def load_rows():
    """측정 프로그램을 돌려 CSV를 읽는다. 없으면 make가 만들게 한다."""
    if not BINARY.exists():
        subprocess.run(["make", "src/main.out"], cwd=ROOT, check=True)
    result = subprocess.run([str(BINARY), "--csv"], cwd=ROOT, check=True,
                            capture_output=True, text=True)
    rows = list(csv.DictReader(io.StringIO(result.stdout)))
    for row in rows:
        for key in ("n", "compares", "moves", "extraBytes", "maxDepth"):
            row[key] = int(row[key])
        row["millis"] = float(row["millis"])
    return rows


def pick(rows, **conditions):
    return [r for r in rows if all(r[k] == v for k, v in conditions.items())]


def by_algo(rows, field, keys, key_field):
    """{알고리즘: [키 순서대로의 값]} 으로 모은다."""
    table = {}
    for algo in ALGOS:
        values = []
        for key in keys:
            match = [r for r in rows if r["algo"] == algo and r[key_field] == key]
            values.append(match[0][field] if match else 0)
        table[algo] = values
    return table


def main():
    OUT_DIR.mkdir(exist_ok=True)
    rows = load_rows()
    # 그래프와 보고서의 표가 같은 실행에서 나오도록 측정값을 그대로 남긴다.
    with open(OUT_DIR / "results.csv", "w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    growth = pick(rows, scope="growth")
    kinds = pick(rows, scope="kinds")
    sizes = sorted({r["n"] for r in growth})
    kind_keys = ["random", "sorted", "reversed", "few-unique"]
    made = []

    # 1. 비교 횟수가 n에 따라 어떻게 자라는가 (로그-로그)
    made.append(svgchart.log_line_chart(
        OUT_DIR / "growth-compares.svg",
        "비교 횟수가 n에 따라 자라는 모양",
        "무작위 입력 · 로그-로그 축에서는 기울기가 곧 복잡도 지수다 "
        "(2.0이면 n^2, 1.0이면 n)",
        sizes, by_algo(growth, "compares", sizes, "n"),
        "n (원소 개수)", "비교 횟수"))

    # 2. 같은 것을 시간으로
    made.append(svgchart.log_line_chart(
        OUT_DIR / "growth-time.svg",
        "걸린 시간이 n에 따라 자라는 모양",
        "무작위 입력 · 같은 기계에서 잰 값이다 (컨테이너, gcc -O2)",
        sizes, by_algo(growth, "millis", sizes, "n"),
        "n (원소 개수)", "시간 (ms)"))

    # 3. 입력 모양별 — 표의 네 열을 각각 그래프로 (n = 4,000)
    shape_labels = [KIND_LABEL[k] for k in kind_keys]

    # 3-1. 걸린 시간. 정렬된 입력과 최악이 4자릿수 차이라 세로축을 로그로 둔다.
    made.append(svgchart.grouped_bar_chart(
        OUT_DIR / "input-shapes-time.svg",
        "입력 모양에 따른 걸린 시간",
        "n = 4,000 · 3회 평균 · 세로축 로그 (0.004ms와 29ms를 한 축에 담아야 한다)",
        shape_labels, by_algo(kinds, "millis", kind_keys, "input"),
        "시간 (ms)", log_scale=True, value_label=svgchart.ms))

    # 3-2. 비교 횟수
    made.append(svgchart.grouped_bar_chart(
        OUT_DIR / "input-shapes-compares.svg",
        "입력 모양에 따른 비교 횟수",
        "n = 4,000 · 세로축 로그 · 정렬된 입력에서는 셋 다 n-1번으로 끝난다",
        shape_labels, by_algo(kinds, "compares", kind_keys, "input"),
        "비교 횟수", log_scale=True))

    # 3-3. 이동 횟수. 정렬된 입력은 0이라 로그 축에서 막대가 사라진다(그것이 답이다).
    made.append(svgchart.grouped_bar_chart(
        OUT_DIR / "input-shapes-moves.svg",
        "입력 모양에 따른 이동 횟수",
        "n = 4,000 · 세로축 로그 · 정렬된 입력은 0회라 막대가 없다",
        shape_labels, by_algo(kinds, "moves", kind_keys, "input"),
        "이동 횟수", log_scale=True))

    # 3-4. 재귀 깊이. 값이 작아 로그가 필요 없다.
    made.append(svgchart.grouped_bar_chart(
        OUT_DIR / "input-shapes-depth.svg",
        "입력 모양에 따른 재귀 깊이",
        "n = 4,000 · 삽입·버블은 반복문뿐이라 늘 1이다. 블록 정렬만 입력을 탄다",
        shape_labels, by_algo(kinds, "maxDepth", kind_keys, "input"),
        "재귀 깊이"))

    # 4. 비교는 비슷한데 이동이 다르다 — 버블 정렬이 느린 진짜 이유
    reversed_rows = pick(kinds, input="reversed")
    made.append(svgchart.grouped_bar_chart(
        OUT_DIR / "compares-vs-moves.svg",
        "비교 횟수는 비슷한데 이동 횟수가 다르다",
        "n = 4,000 역순 입력 · 교환 한 번이 이동 세 번이라 버블 정렬만 3배로 뛴다",
        ["비교", "이동"],
        {algo: [pick(reversed_rows, algo=algo)[0]["compares"],
                pick(reversed_rows, algo=algo)[0]["moves"]] for algo in ALGOS},
        "횟수"))

    for path in made:
        print(f"wrote {Path(path).relative_to(ROOT)}")


if __name__ == "__main__":
    main()
