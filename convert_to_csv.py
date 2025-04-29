import os
import csv

folder = "results"
output_csv = "parsed_benchmark_results.csv"

algorithms = ["Baseline", "Bruck", "Circulant"]
header = [
    "Nodes", "Tasks", "Algorithm", "Type",
    "m=1", "m=10", "m=100", "m=1000", "m=10000", "m=100000"
]

rows = []

for filename in sorted(os.listdir(folder)):
    if not filename.endswith(".out"):
        continue

    try:
        nodes, tasks = filename.split("_")[0].lower().split("x")
    except ValueError:
        continue

    filepath = os.path.join(folder, filename)
    with open(filepath, "r") as f:
        lines = [line.strip() for line in f if line.strip()]

    line_idx = 0
    for algo in algorithms:
        for type_id in range(3):
            # Skip lines until we reach a data line (every second line in the block)
            while line_idx < len(lines):
                if "&" in lines[line_idx] and "\\" in lines[line_idx]:
                    parts = lines[line_idx].split("&")
                    if len(parts) >= 8:
                        try:
                            values = [float(p.strip().replace("\\", ""))
                                      for p in parts[2:8]]
                            rows.append([nodes, tasks, algo, type_id] + values)
                            line_idx += 1
                            break
                        except ValueError:
                            break
                line_idx += 1
        # skip the \hline or separator line
        while line_idx < len(lines) and not lines[line_idx].startswith("\\hline"):
            line_idx += 1
        line_idx += 1

# Write to CSV
with open(output_csv, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(header)
    writer.writerows(rows)
