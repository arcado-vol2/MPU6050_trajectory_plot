import matplotlib.pyplot as plt
import numpy as np

# Reading data from file
data = []
with open('data.txt', 'r') as file:
    for line in file:
        if line.strip():  # skip empty lines
            row = list(map(float, line.strip().split()))
            data.append(row)

# Convert to numpy array
points = np.array(data)
x = points[:, 0]
y = points[:, 1]
z = points[:, 2]

# Create 3D plot
fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, projection='3d')

# Display line with points
# First draw the line
ax.plot(x, y, z, c='b', linestyle='-', linewidth=1, alpha=0.5)
# Then draw points on top of the line
scatter = ax.scatter(x, y, z, c=z, cmap='viridis', marker='o', s=50)

# Configure axes
ax.set_xlabel('X coordinate')
ax.set_ylabel('Y coordinate')
ax.set_zlabel('Z coordinate')
ax.set_title('3D visualization of points with connecting line')

# Add colorbar
cbar = fig.colorbar(scatter, ax=ax, shrink=0.5, aspect=5)
cbar.set_label('Z value')

# Rotate plot for better view
ax.view_init(elev=20, azim=45)

plt.tight_layout()
plt.show()