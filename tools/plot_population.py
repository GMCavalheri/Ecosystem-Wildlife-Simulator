#!/usr/bin/env python3
"""Plot population history CSV from ecosystem_sim: time,prey,predator,
avg_vegetation,avg_prey_speed.

Usage: python3 tools/plot_population.py [population_history.csv] [output.png]
"""

import csv
import sys


def main() -> None:
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "population_history.csv"
    out_path = sys.argv[2] if len(sys.argv) > 2 else "population_history.png"

    time, prey, predator, vegetation, speed = [], [], [], [], []
    with open(csv_path, newline="") as f:
        reader = csv.DictReader(f)
        fields = reader.fieldnames or []
        has_vegetation = "avg_vegetation" in fields
        has_speed = "avg_prey_speed" in fields
        for row in reader:
            time.append(float(row["time"]))
            prey.append(int(row["prey"]))
            predator.append(int(row["predator"]))
            if has_vegetation:
                vegetation.append(float(row["avg_vegetation"]))
            if has_speed:
                speed.append(float(row["avg_prey_speed"]))

    import matplotlib.pyplot as plt

    ncols = 2 + (1 if vegetation else 0) + (1 if speed else 0)
    fig, axes = plt.subplots(1, ncols, figsize=(6 * ncols, 5))
    ax_time, ax_phase = axes[0], axes[1]
    next_axis = 2

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
        ax_veg = axes[next_axis]
        next_axis += 1
        ax_veg.plot(time, vegetation, color="tab:olive")
        ax_veg.set_xlabel("Simulated time")
        ax_veg.set_ylabel("Grid-average vegetation density")
        ax_veg.set_title("Vegetation vs. time")
        ax_veg.set_ylim(0, 1.05)

    if speed:
        ax_speed = axes[next_axis]
        next_axis += 1
        ax_speed.plot(time, speed, color="tab:purple")
        ax_speed.axhline(1.0, color="gray", linestyle="--", linewidth=0.8, label="Founder speed")
        ax_speed.set_xlabel("Simulated time")
        ax_speed.set_ylabel("Mean prey speed")
        ax_speed.set_title("Trait drift vs. time")
        ax_speed.legend()

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    print(f"Wrote {out_path}")


if __name__ == "__main__":
    main()
