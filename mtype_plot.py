import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter

# Load and melt the data
df = pd.read_csv("results.csv")
melted = df.melt(
    id_vars=["Nodes", "Tasks", "Algorithm", "Type"],
    var_name="MessageSize",
    value_name="Runtime"
)

# Choose one message size for clarity in plots
msg_size = "m=10"
subset = melted[melted["MessageSize"] == msg_size]

# Use a clean modern theme
sns.set_theme(style="whitegrid")

# Define unique values for sorting grid layout
tasks_per_node_sorted = sorted(subset["Tasks"].unique())
nodes_sorted = sorted(subset["Nodes"].unique())

# Create the grid
fig, axes = plt.subplots(len(tasks_per_node_sorted), len(nodes_sorted),
                         figsize=(len(nodes_sorted)*4,
                                  len(tasks_per_node_sorted)*4),
                         sharey=False)

# Formatter for scientific notation
sci_formatter = ScalarFormatter(useMathText=True)
sci_formatter.set_scientific(True)
sci_formatter.set_powerlimits((-2, 2))

# Plot each config in the grid
for i, tasks in enumerate(tasks_per_node_sorted):
    for j, nodes in enumerate(nodes_sorted):
        ax = axes[i, j]
        cfg_data = subset[(subset["Nodes"] == nodes) &
                          (subset["Tasks"] == tasks)]
        if cfg_data.empty:
            ax.axis('off')
            continue
        sns.barplot(data=cfg_data, x="Algorithm",
                    y="Runtime", hue="Type", ax=ax, palette="colorblind")
        ax.set_title(f"{nodes}x{tasks}")
        ax.set_xlabel("")
        ax.set_ylabel("Runtime (s)" if j == 0 else "")
        ax.yaxis.set_major_formatter(sci_formatter)
        ax.legend().remove()

# Final touches
handles, labels = ax.get_legend_handles_labels()
fig.legend(handles, labels, loc='upper right', ncol=3, title="Type")
fig.suptitle(
    f"Runtime Differences by Message Type for {msg_size}", fontsize=14)
plt.tight_layout(rect=[0, 0, 1, 0.95])
plt.savefig("m_types.pdf")
