#!/usr/bin/env python3

import os
import sys
import csv
import glob
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm

preferred_fonts = ['DIN Alternate', 'JetBrains Mono', 'DejaVu Sans']
available_fonts = {f.name for f in fm.fontManager.ttflist}
for font in preferred_fonts:
	if font in available_fonts:
		plt.rcParams['font.family'] = font
		break

plt.style.use('dark_background')

if len(sys.argv) != 2:
	print("usage: plot_trace.py <trace_folder>")
	sys.exit(1)

trace_dir = sys.argv[1]
if not os.path.isdir(trace_dir):
	print(f"error: not a directory: {trace_dir}")
	sys.exit(1)

csv_files = sorted(glob.glob(os.path.join(trace_dir, "*.csv")))
if not csv_files:
	print(f"error: no trace files found in {trace_dir}/")
	sys.exit(1)

trace_data = {}

for path in csv_files:
	with open(path, newline='') as csvfile:
		reader = csv.DictReader(csvfile)
		phase_total = {}
		for row in reader:
			try:
				raw_size = int(row["raw_size"])
				alignment = int(row["alignment"])
				proxy = int(row["proxy_index"])
				count = int(row["count"])
				fallbacks = int(row["fallbacks"])
				raw_bytes = int(row["raw_total_bytes"])
				stride_bytes = int(row["stride_total_bytes"])
			except ValueError:
				continue

			key = (raw_size, proxy)
			if key not in trace_data:
				trace_data[key] = {
					"count": 0,
					"fallback_count": 0,
					"raw_total": 0,
					"stride_total": 0,
					"peak": 0
				}

			trace_data[key]["count"] += count
			trace_data[key]["fallback_count"] += fallbacks
			trace_data[key]["raw_total"] += raw_bytes
			trace_data[key]["stride_total"] += stride_bytes
			phase_total[key] = phase_total.get(key, 0) + stride_bytes

		for key, total in phase_total.items():
			trace_data[key]["peak"] = max(trace_data[key]["peak"], total)

if not trace_data:
	print("error: no allocation data found in trace files")
	sys.exit(1)

active_keys = sorted(trace_data.keys(), key=lambda x: x[0])
raw_sizes = sorted(set(k[0] for k in active_keys))
indices = range(len(raw_sizes))

size_counts   = {s: 0 for s in raw_sizes}
fallbacks     = {s: 0 for s in raw_sizes}
raw_totals    = {s: 0 for s in raw_sizes}
stride_totals = {s: 0 for s in raw_sizes}
live_peaks    = {s: 0 for s in raw_sizes}

for (raw_size, _), data in trace_data.items():
	size_counts[raw_size]   += data["count"]
	fallbacks[raw_size]     += data["fallback_count"]
	raw_totals[raw_size]    += data["raw_total"]
	stride_totals[raw_size] += data["stride_total"]
	live_peaks[raw_size]     = max(live_peaks[raw_size], data["peak"])

counts      = [size_counts[s] for s in raw_sizes]
raw_data    = [raw_totals[s] for s in raw_sizes]
stride_data = [stride_totals[s] for s in raw_sizes]
peaks       = [live_peaks[s] for s in raw_sizes]

fig, (ax1, ax2, ax3, ax4) = plt.subplots(4, 1, figsize=(12, 10), sharex=True)

for ax, data, label, hatch in zip(
	(ax1, ax2, ax3, ax4),
	(counts, raw_data, stride_data, peaks),
	("ALLOCATION COUNT", "RAW TOTAL BYTES", "STRIDE TOTAL BYTES", "PEAK MEMORY PRESSURE"),
	('x', '.', '//', 'O')
):
	ax.bar(
		indices,
		data,
		width=1,
		align='center',
		alpha=1.0,
		color='none',
		edgecolor='white',
		hatch=hatch,
		linewidth=1.2
	)
	ax.set_ylabel(label, fontsize=10)
	ax.set_yscale("log")
	ax.grid(True, axis='y', linestyle='--', alpha=0.4)
	ax.tick_params(axis='x', colors='white')
	ax.tick_params(axis='y', colors='white')
	ax.yaxis.label.set_color('white')
	ax.yaxis.labelpad = 1

ax4.set_xlabel("RAW SIZE (BYTES)", fontsize=10)
tick_interval = max(1, len(raw_sizes) // 32)
ax4.set_xticks(range(0, len(raw_sizes), tick_interval))
ax4.set_xticklabels(
	[str(raw_sizes[i]) for i in range(0, len(raw_sizes), tick_interval)],
	rotation=45,
	color='white',
	fontsize=8
)

plt.suptitle("MEMORY PHASE DISSOCIATION: RAW / STRIDE TRACE SIGNATURE", color='white', fontsize=12)
plt.tight_layout(pad=1.5, h_pad=1.0)

svg_path = os.path.join(trace_dir, "alloc_trace.svg")
plt.savefig(svg_path, format="svg")

print("\nmetapool trace data points:\n")

rows = []
headers = ["raw_size", "proxy", "count", "fallbacks", "raw_total", "stride_total", "peak"]
col_widths = [len(h) for h in headers]
head = min(5, len(active_keys))
tail = min(5, len(active_keys) - head)
important_indices = list(range(head)) + list(range(len(active_keys) - tail, len(active_keys)))

for idx in important_indices:
	raw_size, proxy = active_keys[idx]
	data = trace_data[(raw_size, proxy)]
	row = (
		raw_size,
		proxy,
		data["count"],
		data["fallback_count"],
		data["raw_total"],
		data["stride_total"],
		data["peak"]
	)
	rows.append(row)
	for i, val in enumerate(row):
		col_widths[i] = max(col_widths[i], len(str(val)))

header_line = " | ".join(f"{h:>{w}}" for h, w in zip(headers, col_widths))
print(header_line)
print("-" * len(header_line))

for row in rows:
	line = " | ".join(f"{str(v):>{w}}" for v, w in zip(row, col_widths))
	print(line)

plt.show()

