import matplotlib.pyplot as plt
import numpy as np


def plot_3D(title: str, cloud: np.array):
    x = cloud[:, 0]
    y = cloud[:, 1]
    z = cloud[:, 2]

    plt.figure("Point cloud")
    ax = plt.axes(projection="3d")
    plt.grid()
    ax.set_xlabel("X", fontsize=8)
    ax.set_ylabel("Y", fontsize=8)
    ax.set_zlabel("Z", fontsize=8)
    ax.tick_params(axis="both", labelsize=8)
    plt.title(title)

    ax.scatter3D(
        x,
        y,
        z,
        s=[0.5 for i in range(len(x))],
        c=-z,
        cmap="rainbow",
        marker="o",
    )
    ax.xaxis.pane.fill = False
    ax.yaxis.pane.fill = False
    ax.zaxis.pane.fill = False
    ax.set_box_aspect([np.ptp(coord) for coord in [x, y, z]])
    ax.view_init(azim=-155, elev=20)

    plt.tight_layout()

    plt.show(block=False)
