"""비교 결과를 그래프(SVG)로 그린다.

    make charts          # 또는
    python3 tools/plot.py

src/main.out --csv 를 돌려 측정값을 받아 report/ 아래에 SVG를 쓴다. 사람이 읽는
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
OUT_DIR = ROOT / "report"
ALGOS = ["insertionSort", "bubbleSort", "blockSort"]
KIND_LABEL = {
    "random": "무작위",
    "sorted": "정렬됨",
    "reversed": "역순",
    "few-unique": "중복많음",
}


def run_csv(flag):
    """측정 프로그램을 돌려 CSV를 읽는다."""
    if not BINARY.exists():
        subprocess.run(["make", "src/main.out"], cwd=ROOT, check=True)
    result = subprocess.run([str(BINARY), flag], cwd=ROOT, check=True,
                            capture_output=True, text=True)
    return list(csv.DictReader(io.StringIO(result.stdout)))


def load_rows():
    """측정 프로그램을 돌려 CSV를 읽는다. 없으면 make가 만들게 한다."""
    rows = run_csv("--csv")
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

    # 같은 자료를 두 축으로 그린다. 선형은 격차의 크기를, 로그는 작은 값과
    # 자라는 속도를 보여 준다. 어느 한쪽만으로는 절반씩 놓친다.
    #
    # 1. n에 따라 자라는 모양
    for field, unit, noun, stem in (
            ("compares", "비교 횟수", "비교 횟수", "growth-compares"),
            ("millis", "시간 (ms)", "걸린 시간", "growth-time")):
        data = by_algo(growth, field, sizes, "n")
        made.append(svgchart.line_chart(
            OUT_DIR / f"{stem}.svg",
            f"n이 커질 때 {noun} — 선형 축",
            "무작위 입력 · 격차가 그대로 보이는 대신, 바닥에 깔린 값은 읽히지 않는다",
            sizes, data, "n (원소 개수)", unit, log_axes=False))
        made.append(svgchart.line_chart(
            OUT_DIR / f"{stem}-log.svg",
            f"n이 커질 때 {noun} — 로그-로그 축",
            "기울기가 곧 복잡도 지수다 (2.0이면 n^2, 1.0이면 n)",
            sizes, data, "n (원소 개수)", unit))

    # 2. 입력 모양별 — 표의 네 열을 각각 그래프로 (n = 4,000)
    shape_labels = [KIND_LABEL[k] for k in kind_keys]
    columns = (
        ("millis", "시간 (ms)", "input-shapes-time", "걸린 시간", svgchart.ms),
        ("compares", "비교 횟수", "input-shapes-compares", "비교 횟수", svgchart.si),
        ("moves", "이동 횟수", "input-shapes-moves", "이동 횟수", svgchart.si),
    )
    for field, unit, stem, label, fmt in columns:
        data = by_algo(kinds, field, kind_keys, "input")
        made.append(svgchart.grouped_bar_chart(
            OUT_DIR / f"{stem}.svg",
            f"입력 모양에 따른 {label} — 선형 축",
            "n = 4,000 · 막대 높이가 곧 값의 비율이다. 대신 작은 값은 바닥에 붙는다",
            shape_labels, data, unit, value_label=fmt))
        made.append(svgchart.grouped_bar_chart(
            OUT_DIR / f"{stem}-log.svg",
            f"입력 모양에 따른 {label} — 로그 축",
            "같은 자료. 선형 축에서 사라졌던 작은 값이 여기서는 읽힌다",
            shape_labels, data, unit, log_scale=True, value_label=fmt))

    # 3. 재귀 깊이. 값이 1~24라 로그가 필요 없다.
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

    # 5. 블록 크기 실험 — 이 값이 왜 상수인지를 보여 준다
    blocks = run_csv("--blocks")
    for row in blocks:
        for key in ("n", "block", "compares", "moves"):
            row[key] = int(row[key])
        row["millis"] = float(row["millis"])
    with open(OUT_DIR / "block-size.csv", "w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(blocks[0]))
        w.writeheader()
        w.writerows(blocks)
    xs = sorted({r["block"] for r in blocks if r["block"] <= 512})
    series = {}
    for n in sorted({r["n"] for r in blocks}):
        series[f"n = {n:,}"] = [
            next((r["compares"] for r in blocks if r["n"] == n and r["block"] == b), 0)
            for b in xs
        ]
    made.append(svgchart.line_chart(
        OUT_DIR / "block-size.svg",
        "블록 크기를 바꿔 가며 — 비교 횟수",
        "무작위 입력 · 로그-로그 · 최적 블록 크기는 n이 커져도 8~32에서 움직이지 않는다",
        xs, series, "블록 크기 (원소 수)", "비교 횟수",
        annotate_slope=False, vline=32, vline_label="지금 쓰는 값 32"))

    for path in made:
        print(f"wrote {Path(path).relative_to(ROOT)}")


if __name__ == "__main__":
    main()
