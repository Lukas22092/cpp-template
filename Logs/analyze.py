from collections import defaultdict
import os  # <-- Added for directory management
import re
import matplotlib.pyplot as plt

current_dir = os.path.dirname(os.path.abspath(__file__))

# Join it with the Logs folder and file name
file_path = os.path.join(current_dir, "logs.log")

# --- 0. Setup Output Directory ---
# Creates a folder named 'Logs' in your current working directory if it doesn't exist
output_dir = 'Logs'
os.makedirs(output_dir, exist_ok=True)
output_file_path = os.path.join(output_dir, 'performance_dashboard.png')

with open(file_path, 'r') as file:
    log_data = file.read()

# 1. Regex to extract function name and duration from EXIT lines
pattern = r"([\w\.]+)\s*\|\s*EXIT\s*-\s*([\d\.]+)s"

# Dictionaries to track execution times
function_times = defaultdict(list)
class_times = defaultdict(list)

# Parse lines
for line in log_data.strip().split('\n'):
    match = re.search(pattern, line)
    if match:
        full_name = match.group(1)
        duration = float(match.group(2)) * 1000  # Convert seconds to milliseconds
        
        function_times[full_name].append(duration)
        
        # Track by Class group
        if '.' in full_name:
            class_name, _ = full_name.split('.', 1)
            class_times[class_name].append(duration)
        else:
            class_times['Global / Standalone'].append(duration)

# 2. Process metrics (Averages & Invocations)
func_averages = {}
func_counts = {}
for func, times in function_times.items():
    func_averages[func] = sum(times) / len(times)
    func_counts[func] = len(times)  # Count of how often it was entered/exited

class_averages = {cls: sum(times)/len(times) for cls, times in class_times.items()}

# Sort datasets and grab top 10 to keep graphs clear
sorted_funcs_time = dict(sorted(func_averages.items(), key=lambda x: x[1], reverse=True)[:10])
sorted_funcs_count = dict(sorted(func_counts.items(), key=lambda x: x[1], reverse=True)[:10])
sorted_classes = dict(sorted(class_averages.items(), key=lambda x: x[1], reverse=True))


# 3. Plot the 3-Panel Dashboard
fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(20, 7))

# --- Graph 1: Top Slowest Functions ---
colors_f1 = ['#ff4d4d' if x > 10 else '#33cc33' for x in sorted_funcs_time.values()]
bars1 = ax1.barh(list(sorted_funcs_time.keys()), list(sorted_funcs_time.values()), color=colors_f1, edgecolor='black')
ax1.set_title("Top 10 Slowest Functions", fontsize=12, fontweight='bold', pad=10)
ax1.set_xlabel("Avg Duration (Milliseconds)", fontsize=10)
ax1.invert_yaxis()
ax1.grid(axis='x', linestyle='--', alpha=0.5)

# Add label with duration AND invocation count
for bar, func_name in zip(bars1, sorted_funcs_time.keys()):
    w = bar.get_width()
    count = func_counts[func_name]
    ax1.text(w + (w * 0.02 + 0.1), bar.get_y() + bar.get_height()/2, f'{w:.3f} ms ({count}x)', 
             va='center', ha='left', fontsize=8, fontweight='bold')


# --- Graph 2: Most Frequently Called Functions ---
bars2 = ax2.barh(list(sorted_funcs_count.keys()), list(sorted_funcs_count.values()), color='#ff9f43', edgecolor='black')
ax2.set_title("Top 10 Most Frequently Entered", fontsize=12, fontweight='bold', pad=10)
ax2.set_xlabel("Invocation Count (Total Entries)", fontsize=10)
ax2.invert_yaxis()
ax2.grid(axis='x', linestyle='--', alpha=0.5)

# Add count label
for bar in bars2:
    w = bar.get_width()
    ax2.text(w + (w * 0.02 + 0.1), bar.get_y() + bar.get_height()/2, f'{int(w)} calls', 
             va='center', ha='left', fontsize=8, fontweight='bold')


# --- Graph 3: Aggregated Performance by Class ---
colors_classes = ['#4a90e2' if cls != 'Global / Standalone' else '#7f8c8d' for cls in sorted_classes.keys()]
bars3 = ax3.barh(list(sorted_classes.keys()), list(sorted_classes.values()), color=colors_classes, edgecolor='black')
ax3.set_title("Avg Performance by Class", fontsize=12, fontweight='bold', pad=10)
ax3.set_xlabel("Avg Duration (Milliseconds)", fontsize=10)
ax3.invert_yaxis()
ax3.grid(axis='x', linestyle='--', alpha=0.5)

# Add duration label
for bar in bars3:
    w = bar.get_width()
    ax3.text(w + (w * 0.02 + 0.1), bar.get_y() + bar.get_height()/2, f'{w:.3f} ms', 
             va='center', ha='left', fontsize=8, fontweight='bold')


# Final Dashboard Tweak
plt.suptitle("Mindstorms Party Application Performance Metrics", fontsize=15, fontweight='bold', y=0.98)
plt.tight_layout()

# --- 4. Save and Show ---
plt.savefig(output_file_path, dpi=300)  # <-- Saves the figure to Logs/performance_dashboard.png
print(f"Dashboard successfully saved to: {output_file_path}")

plt.show()