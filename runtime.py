import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter

# Load and melt data
df = pd.read_csv("results.csv")
melted = df.melt(
    id_vars=["Nodes", "Tasks", "Algorithm", "Type"],
    var_name="MessageSize",
    value_name="Runtime"
)

# Extract numeric message size
melted["MessageSize"] = melted["MessageSize"].str.extract(
    r"m=(\d+)")[0].astype(int)

# Keep only Type 2 messages
melted = melted[melted["Type"] == 2]

# Clean theme
sns.set_theme(style="whitegrid")

# Unique values for grid layout
tasks_sorted = sorted(melted["Tasks"].unique())
nodes_sorted = sorted(melted["Nodes"].unique())

# Grid layout
fig, axes = plt.subplots(len(tasks_sorted), len(nodes_sorted),
                         figsize=(len(nodes_sorted)*4, len(tasks_sorted)*4),
                         sharey=False)

# Formatter for scientific notation
formatter = ScalarFormatter(useMathText=True)
formatter.set_scientific(True)
formatter.set_powerlimits((-2, 2))

# Plot
for i, tasks in enumerate(tasks_sorted):
    for j, nodes in enumerate(nodes_sorted):
        ax = axes[i, j]
        cfg = melted[(melted["Nodes"] == nodes) & (melted["Tasks"] == tasks)]
        if cfg.empty:
            ax.axis("off")
            continue
        sns.lineplot(data=cfg, x="MessageSize", y="Runtime",
                     hue="Algorithm", marker="o", ax=ax, palette="colorblind")
        ax.set_title(f"{nodes}x{tasks}")
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xlabel("Message Size (log, per processor)")
        ax.set_ylabel("Runtime (s)" if j == 0 else "")
        ax.yaxis.set_major_formatter(formatter)
        ax.ticklabel_format(style='sci', axis='y', scilimits=(-2, 2))
        ax.legend().remove()

# Add global legend
handles, labels = ax.get_legend_handles_labels()
fig.legend(handles, labels, loc='upper right', title="Algorithm")

fig.suptitle(
    "Runtime by Algorithm for all configurations (M. Type 2 only)", fontsize=14)
plt.tight_layout(rect=[0, 0, 1, 0.95])
plt.savefig("runtime_grid_logy.pdf")
