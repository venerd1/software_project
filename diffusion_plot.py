import numpy as np
import matplotlib.pyplot as plt

# Read data from 'initial.dat' and 'final.dat'
initial_grid = np.loadtxt('heat_diffusion/initial.dat')
final_grid = np.loadtxt('heat_diffusion/final.dat')

# Find the minimum and maximum value between the two dataset for the colour scale
vmin = min(initial_grid.min(), final_grid.min())
vmax = max(initial_grid.max(), final_grid.max())

# Crete the figure and the sub-plots
fig, axs = plt.subplots(1, 2, figsize=(12, 6))

# First plot: initial.dat
im1 = axs[0].imshow(initial_grid, cmap='hot', interpolation='nearest', vmin=vmin, vmax=vmax)
axs[0].set_title('Heat Diffusion - Initial')
axs[0].set_xlabel('X Axis')
axs[0].set_ylabel('Y Axis')
fig.colorbar(im1, ax=axs[0], label='Temperature')

# Second plot: final.dat
im2 = axs[1].imshow(final_grid, cmap='hot', interpolation='nearest', vmin=vmin, vmax=vmax)
axs[1].set_title('Heat Diffusion - Final')
axs[1].set_xlabel('X Axis')
axs[1].set_ylabel('Y Axis')
fig.colorbar(im2, ax=axs[1], label='Temperature')

#save the figure
fig.savefig("heat_diffusion/heat_diffusion_comparison.png", dpi=300)

# Show the figure
plt.tight_layout()
plt.show()

