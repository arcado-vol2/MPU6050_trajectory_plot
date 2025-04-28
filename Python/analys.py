import numpy as np
from scipy import signal
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from mpl_toolkits.mplot3d import Axes3D
from pyquaternion import Quaternion  # Install via pip install pyquaternion

def load_data(filename):
    """Load data from file: time, q0,q1,q2,q3,accX,accY,accZ"""
    with open(filename) as f:
        lines = f.readlines()
    
    # Skip header if present
    if not lines[0][0].isdigit():
        lines = lines[1:]
    
    data = []
    for line in lines:
        parts = line.strip().split(',')
        time_str = parts[0]
        quat_acc = list(map(float, parts[1:8]))
        timestamp = float(time_str)  / 1000
        data.append([timestamp] + quat_acc)
    
    data = np.array(data)
    timestamps = data[:, 0]
    quaternions = data[:, 1:5]  # Columns 1-4: quaternion [w, x, y, z]
    acc = data[:, 5:8]         # Columns 5-7: accelerometer [X, Y, Z]
    
    
    return quaternions, acc, timestamps

def quaternion_to_rotation_matrix(q):
    """Convert quaternion to rotation matrix"""
    q = q / np.linalg.norm(q)  # Normalization
    w, x, y, z = q
    return np.array([
        [1 - 2*y**2 - 2*z**2,     2*x*y - 2*z*w,     2*x*z + 2*y*w],
        [2*x*y + 2*z*w,     1 - 2*x**2 - 2*z**2,     2*y*z - 2*x*w],
        [2*x*z - 2*y*w,     2*y*z + 2*x*w,     1 - 2*x**2 - 2*y**2]
    ])

def plot_data(acc, title):
    """Visualize accelerometer data"""
    plt.figure(figsize=(10, 6))
    plt.plot(acc[:, 0], 'r', label='X')
    plt.plot(acc[:, 1], 'g', label='Y')
    plt.plot(acc[:, 2], 'b', label='Z')
    plt.xlabel('Sample')
    plt.ylabel('Acceleration (g)')
    plt.title(title)
    plt.legend()
    plt.grid(True)
    plt.show()

def six_dof_animation(positions, rotations, sample_plot_freq=8):
    """6DOF motion animation with full trajectory"""
    fig = plt.figure(figsize=(12, 8))
    ax = fig.add_subplot(111, projection='3d')
    
    # Set axis limits based on entire trajectory
    margin = 0.1
    ax.set_xlim([np.min(positions[:, 0])-margin, np.max(positions[:, 0])+margin])
    ax.set_ylim([np.min(positions[:, 1])-margin, np.max(positions[:, 1])+margin])
    ax.set_zlim([np.min(positions[:, 2])-margin, np.max(positions[:, 2])+margin])
    ax.set_xlabel('X (m)')
    ax.set_ylabel('Y (m)')
    ax.set_zlabel('Z (m)')
    ax.set_title('6DOF Animation with Full Trajectory')
    
    # Create full trajectory once (it won't change)
    full_trajectory = ax.plot(positions[:, 0], positions[:, 1], positions[:, 2], 
                            'grey', linewidth=1, alpha=0.5)[0]
    
    # Create "progress" part of trajectory (will grow)
    progress_trajectory = ax.plot([], [], [], 'r-', linewidth=2)[0]
    
    # Create orientation axis objects
    x_axis = ax.plot([], [], [], 'r-', linewidth=2)[0]
    y_axis = ax.plot([], [], [], 'g-', linewidth=2)[0]
    z_axis = ax.plot([], [], [], 'b-', linewidth=2)[0]
    
    def init():
        # Initialize only mutable objects
        progress_trajectory.set_data([], [])
        progress_trajectory.set_3d_properties([])
        x_axis.set_data([], [])
        x_axis.set_3d_properties([])
        y_axis.set_data([], [])
        y_axis.set_3d_properties([])
        z_axis.set_data([], [])
        z_axis.set_3d_properties([])
        return progress_trajectory, x_axis, y_axis, z_axis
    
    def update(frame):
        idx = frame * sample_plot_freq
        if idx >= len(positions):
            idx = len(positions) - 1
        
        current_pos = positions[idx]
        R = rotations[idx]
        
        # Orientation axis length (5% of scene size)
        axis_length = 0.4 * np.max([
            np.ptp(positions[:, 0]),
            np.ptp(positions[:, 1]),
            np.ptp(positions[:, 2])
        ])
        
        # Update orientation axes
        x_end = current_pos + R[:, 0] * axis_length
        y_end = current_pos + R[:, 1] * axis_length
        z_end = current_pos + R[:, 2] * axis_length
        
        x_axis.set_data([current_pos[0], x_end[0]], [current_pos[1], x_end[1]])
        x_axis.set_3d_properties([current_pos[2], x_end[2]])
        
        y_axis.set_data([current_pos[0], y_end[0]], [current_pos[1], y_end[1]])
        y_axis.set_3d_properties([current_pos[2], y_end[2]])
        
        z_axis.set_data([current_pos[0], z_end[0]], [current_pos[1], z_end[1]])
        z_axis.set_3d_properties([current_pos[2], z_end[2]])
        
        # Update "progress" part of trajectory
        progress_trajectory.set_data(positions[:idx+1, 0], positions[:idx+1, 1])
        progress_trajectory.set_3d_properties(positions[:idx+1, 2])
        
        return progress_trajectory, x_axis, y_axis, z_axis
    
    frames = len(positions) // sample_plot_freq
    anim = FuncAnimation(fig, update, frames=frames, init_func=init,
                        blit=True, interval=50, repeat=False)
    
    plt.tight_layout()
    plt.show()
    return anim

def runge_kutta_integration(acc, time_deltas):
    vel = np.zeros_like(acc)
    for i in range(1, len(acc)):
        dt = time_deltas[i]
        k1 = acc[i-1]
        k2 = acc[i-1] + 0.5 * k1 * dt
        k3 = acc[i-1] + 0.5 * k2 * dt
        k4 = acc[i-1] + k3 * dt
        vel[i] = vel[i-1] + (k1 + 2*k2 + 2*k3 + k4) / 6 * dt
    return vel

def main():
    # 1. Load data with timestamps
    quaternions, acc, time_deltas = load_data('../C++/main/out/build/x64-Debug/data_example/recording_20250401_235146.csv')
    print(time_deltas)
    # Average sampling rate (for filtering)
    sample_rate = 1 / np.mean(time_deltas)
    filter_cutoff = 0.1  # Filter cutoff frequency (Hz)
    
    # 2. Visualize raw data
    plot_data(acc, 'Raw Accelerometer Data')
    
    # 3. Calculate rotation matrices
    rotations = np.zeros((len(quaternions), 3, 3))
    for i, q in enumerate(quaternions):
        rotations[i] = quaternion_to_rotation_matrix(q)
    
    # 4. Tilt compensation for accelerometer
    tc_acc = np.zeros_like(acc)
    for i in range(len(acc)):
        tc_acc[i] = rotations[i] @ acc[i]
    
    plot_data(tc_acc, 'Tilt-Compensated Accelerometer')
    
    # 5. Calculate linear acceleration (without gravity)
    lin_acc = tc_acc - np.array([0, 0, 1])  # Subtract gravity
    lin_acc *= 9.81  # Convert g to m/s²
    
    # 6. Calculate linear velocity (acceleration integration)
    lin_vel = runge_kutta_integration(lin_acc, time_deltas)

    # 7. Velocity filtering (remove drift)
    nyquist = 0.5 * sample_rate
    normal_cutoff = filter_cutoff / nyquist
    
    # Verify frequency is in valid range
    if normal_cutoff >= 1.0:
        normal_cutoff = 0.99  # Set slightly below 1
    elif normal_cutoff <= 0.0:
        normal_cutoff = 0.01  # Set slightly above 0
    
    b, a = signal.butter(1, normal_cutoff, 'high')
    lin_vel_hp = signal.filtfilt(b, a, lin_vel, axis=0)
    
    # 8. Calculate position (velocity integration)
    lin_pos = runge_kutta_integration(lin_vel_hp, time_deltas)
    
    # 9. Position filtering (remove drift)
    lin_pos_hp = signal.filtfilt(b, a, lin_pos, axis=0)
    
    # 10. Visualize results
    fig, axs = plt.subplots(3, 1, figsize=(10, 8))
    titles = ['Linear Acceleration', 'Linear Velocity', 'Linear Position']
    data = [lin_acc, lin_vel_hp, lin_pos_hp]
    units = ['m/s²', 'm/s', 'm']
    
    for ax, title, d, unit in zip(axs, titles, data, units):
        ax.plot(d[:, 0], 'r', label='X')
        ax.plot(d[:, 1], 'g', label='Y')
        ax.plot(d[:, 2], 'b', label='Z')
        ax.set_xlabel('Sample')
        ax.set_ylabel(unit)
        ax.set_title(title)
        ax.legend()
        ax.grid(True)
    
    plt.tight_layout()
    plt.show()
    
    # 11. Motion animation
    six_dof_animation(lin_pos_hp, rotations)

if __name__ == "__main__":
    main()