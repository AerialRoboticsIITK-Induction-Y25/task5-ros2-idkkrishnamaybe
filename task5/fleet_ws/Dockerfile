# Stage 1: Build stage
FROM osrf/ros:humble-desktop AS builder

SHELL ["/bin/bash", "-c"]

WORKDIR /workspace

# Copy workspace source files
COPY src/ /workspace/src/

# Build the workspace
RUN source /opt/ros/humble/setup.bash && \
    colcon build

# Stage 2: Runtime stage
FROM ros:humble-ros-base AS runtime

WORKDIR /workspace

# Copy the built install directory
COPY --from=builder /workspace/install/ /workspace/install/

# Entrypoint sources the workspace setup and runs the fleet launch file
ENTRYPOINT ["/bin/bash", "-c", "source /opt/ros/humble/setup.bash && source /workspace/install/setup.bash && ros2 launch drone_fleet fleet.launch.py"]
