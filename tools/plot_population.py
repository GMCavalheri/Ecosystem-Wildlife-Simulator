#!/usr/bin/env python3
"""Plot population history CSV (time,prey,predator,avg_vegetation) from ecosystem_sim.

Usage: python3 tools/plot_population.py [population_history.csv] [output.png]
"""

import csv
import sys


def main() -> None:
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "population_history.csv"
    out_path = sys.argv[2] if len(sys.argv) > 2 else "population_history.png"

    time, prey, predator, vegetation = [], [], [], []
    with open(csv_path, newline="") as f:
        reader = csv.DictReader(f)
        has_vegetation = "avg_vegetation" in (reader.fieldnames or [])
        for row in reader:
            time.append(float(row["time"]))
            prey.append(int(row["prey"]))
            predator.append(int(row["predator"]))
            if has_vegetation:
                vegetation.append(float(row["avg_vegetation"]))

    import matplotlib.pyplot as plt

    ncols = 3 if vegetation else 2
    fig, axes = plt.subplots(1, ncols, figsize=(6 * ncols, 5))
    ax_time, ax_phase = axes[0], axes[1]

    ax_time.plot(time, prey, label="Prey", color="tab:green")
    ax_time.plot(time, predator, label="Predator", color="tab:red")
    ax_time.set_xlabel("Simulated time")
    ax_time.set_ylabel("Population")
    ax_time.set_title("Population vs. time")
    ax_time.legend()

    ax_phase.plot(prey, predator, color="tab:blue", linewidth=0.8)
    ax_phase.set_xlabel("Prey")
    ax_phase.set_ylabel("Predator")
    ax_phase.set_title("Phase portrait (predator vs. prey)")

    if vegetation:
        ax_veg = axes[2]
        ax_veg.plot(time, vegetation, color="tab:olive")
        ax_veg.set_xlabel("Simulated time")
        ax_veg.set_ylabel("Grid-average vegetation density")
        ax_veg.set_title("Vegetation vs. time")
        ax_veg.set_ylim(0, 1.05)

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    print(f"Wrote {out_path}")


if __name__ == "__main__":
    main()
