import csv
import glob
import os
import matplotlib.pyplot as plt

plt.style.use('dark_background')

csv_files      = sorted(glob.glob("build/*.csv"))
stride_counts  = {}
stride_raw     = {}
live_peak_raw  = {}

for path in csv_files:
	with open(path, newline='') as csvfile:
		reader    = csv.DictReader(csvfile)
		phase_raw = {}
		for row in reader:
			bucket = int(row["bucket"])
			count  = int(row["count"])
			raw    = int(row["raw_bytes"])
			stride_counts[bucket]  = stride_counts.get(bucket, 0) + count
			stride_raw[bucket]     = stride_raw.get(bucket, 0) + raw
			phase_raw[bucket]      = phase_raw.get(bucket, 0) + raw

		for bucket, raw in phase_raw.items():
			live_peak_raw[bucket] = max(live_peak_raw.get(bucket, 0), raw)

active_strides = sorted(stride_counts.keys())
indices        = range(len(active_strides))
counts         = [stride_counts[s] for s in active_strides]
raws           = [stride_raw[s] for s in active_strides]
live_raws      = [live_peak_raw.get(s, 0) for s in active_strides]

fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize = (12, 10), sharex = True)

ax1.bar(
	indices,
	counts,
	width      = 1,
	align      = 'center',
	alpha      = 1.0,
	color      = 'none',
	edgecolor  = 'white',
	hatch      = 'xx',
	linewidth  = 1.2
)

ax1.set_ylabel("allocation count")
ax1.set_yscale("log")
ax1.grid(True, axis = 'y', linestyle = '--', alpha = 0.4)
ax1.tick_params(axis = 'x', colors = 'white')
ax1.tick_params(axis = 'y', colors = 'white')
ax1.yaxis.label.set_color('white')
ax1.yaxis.labelpad = 1

ax2.bar(
	indices,
	raws,
	width      = 1,
	align      = 'center',
	alpha      = 1.0,
	color      = 'none',
	edgecolor  = 'white',
	hatch      = '//',
	linewidth  = 1.2
)

ax2.set_ylabel("raw bytes")
ax2.set_yscale("log")
ax2.grid(True, axis = 'y', linestyle = '--', alpha = 0.4)
ax2.tick_params(axis = 'x', colors = 'white')
ax2.tick_params(axis = 'y', colors = 'white')
ax2.yaxis.label.set_color('white')
ax2.yaxis.labelpad = 1

ax3.bar(
	indices,
	live_raws,
	width      = 1,
	align      = 'center',
	alpha      = 1.0,
	color      = 'none',
	edgecolor  = 'white',
	hatch      = 'O',
	linewidth  = 1.2
)

ax3.set_ylabel("peak memory pressure")
ax3.set_yscale("log")
ax3.grid(True, axis = 'y', linestyle = '--', alpha = 0.4)
ax3.set_xlabel("stride size (bytes)")
ax3.tick_params(axis = 'x', colors = 'white')
ax3.tick_params(axis = 'y', colors = 'white')
ax3.yaxis.label.set_color('white')
ax3.yaxis.labelpad = 1

tick_interval = max(1, len(active_strides) // 32)
ax3.set_xticks(range(0, len(active_strides), tick_interval))
ax3.set_xticklabels(
	[str(active_strides[i]) for i in range(0, len(active_strides), tick_interval)],
	rotation = 45
)

plt.suptitle("memory phase disassociation / stride pressure signature", color = 'white')
plt.tight_layout(pad = 1.5, h_pad = 0.8)
plt.savefig("alloc_profile.svg", format = "svg")
plt.show()
