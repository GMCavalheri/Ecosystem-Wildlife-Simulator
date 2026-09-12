#!/usr/bin/env python3
"""Plot population history CSV (time,prey,predator) produced by ecosystem_sim.

Usage: python3 tools/plot_population.py [population_history.csv] [output.png]
"""

import csv
import sys


def main() -> None:
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "population_history.csv"
    out_path = sys.argv[2] if len(sys.argv) > 2 else "population_history.png"

    time, prey, predator = [], [], []
    with open(csv_path, newline="") as f:
        for row in csv.DictReader(f):
            time.append(float(row["time"]))
            prey.append(int(row["prey"]))
            predator.append(int(row["predator"]))

    import matplotlib.pyplot as plt

    fig, (ax_time, ax_phase) = plt.subplots(1, 2, figsize=(12, 5))

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

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    print(f"Wrote {out_path}")


if __name__ == "__main__":
    main()
