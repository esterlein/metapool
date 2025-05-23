import csv
import glob
import matplotlib.pyplot as plt

plt.style.use('dark_background')
plt.rcParams['font.family'] = 'DIN Alternate'

csv_files = sorted(glob.glob("build/*.csv"))
stride_counts = {}
stride_raw = {}
live_peak_raw = {}

for path in csv_files:
	with open(path, newline='') as csvfile:
		reader = csv.DictReader(csvfile)
		phase_raw = {}
		for row in reader:
			bucket = int(row["bucket"])
			count = int(row["count"])
			raw = int(row["raw_bytes"])
			stride_counts[bucket] = stride_counts.get(bucket, 0) + count
			stride_raw[bucket] = stride_raw.get(bucket, 0) + raw
			phase_raw[bucket] = phase_raw.get(bucket, 0) + raw

		for bucket, raw in phase_raw.items():
			live_peak_raw[bucket] = max(live_peak_raw.get(bucket, 0), raw)

active_strides = sorted(stride_counts.keys())
indices = range(len(active_strides))
counts = [stride_counts[s] for s in active_strides]
raws = [stride_raw[s] for s in active_strides]
live_raws = [live_peak_raw.get(s, 0) for s in active_strides]

fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 8), sharex=True)

for ax, data, label, hatch in zip(
	(ax1, ax2, ax3),
	(counts, raws, live_raws),
	("ALLOCATION COUNT", "RAW BYTES", "PEAK MEMORY PRESSURE"),
	('x', '//', 'O')
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

ax3.set_xlabel("STRIDE SIZE (BYTES)", fontsize=10)
tick_interval = max(1, len(active_strides) // 32)
ax3.set_xticks(range(0, len(active_strides), tick_interval))
ax3.set_xticklabels(
	[str(active_strides[i]) for i in range(0, len(active_strides), tick_interval)],
	rotation=45,
	color='white',
	fontsize=8
)

plt.suptitle("MEMORY PHASE DISASSOCIATION / STRIDE PRESSURE SIGNATURE", color='white', fontsize=12)
plt.tight_layout(pad=1.5, h_pad=0.8)
plt.savefig("alloc_profile.svg", format="svg")

print("\nIMPORTANT STRIDE DATA POINTS:")
important_indices = list(range(5)) + list(range(len(active_strides) - 5, len(active_strides)))
for idx in important_indices:
	stride = active_strides[idx]
	count = stride_counts[stride]
	raw = stride_raw[stride]
	peak = live_peak_raw[stride]
	print(f"STRIDE {stride:>6} | COUNT {count:>8} | RAW {raw:>10} | PEAK {peak:>10}")

plt.show()
