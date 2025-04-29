import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter

# Load and prepare data
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

# Unique sorted values
message_sizes_sorted = sorted(melted["MessageSize"].unique())
node_sizes_sorted = sorted(melted["Nodes"].unique())

# Set up the grid: rows = message sizes, columns = node sizes
fig, axes = plt.subplots(len(message_sizes_sorted), len(node_sizes_sorted),
                         figsize=(len(node_sizes_sorted)*4,
                                  len(message_sizes_sorted)*3),
                         sharey=False)

# Formatter for scientific notation
formatter = ScalarFormatter(useMathText=True)
formatter.set_scientific(True)
formatter.set_powerlimits((-2, 2))

# Plot
for i, msg_size in enumerate(message_sizes_sorted):
    for j, nodes in enumerate(node_sizes_sorted):
        ax = axes[i, j]
        cfg = melted[(melted["Nodes"] == nodes) & (
            melted["MessageSize"] == msg_size)]
        if cfg.empty:
            ax.axis("off")
            continue
        sns.lineplot(data=cfg, x="Tasks", y="Runtime",
                     hue="Algorithm", marker="o", ax=ax, palette="colorblind")
        ax.set_title(f"Nodes={nodes}, m={msg_size}")
        ax.set_xlabel("Tasks per Node")
        ax.set_ylabel("Runtime (s)" if j == 0 else "")
        ax.yaxis.set_major_formatter(formatter)
        ax.ticklabel_format(style='sci', axis='y', scilimits=(-2, 2))
        ax.legend().remove()

# Add global legend
handles, labels = ax.get_legend_handles_labels()
fig.legend(handles, labels, loc='upper right', title="Algorithm")

fig.suptitle(
    "Weak Scaling: Runtime vs Tasks per Node\n(separated by Nodes and Message Size)", fontsize=16)
plt.tight_layout(rect=[0, 0, 1, 0.96])
plt.savefig("weak_scaling_grid.pdf")
