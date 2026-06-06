#!/bin/bash
set -e

IMAGE_NAME="drone_fleet:latest"
CONTAINER_NAME="drone_fleet_run"

# ── Build the image if it doesn't exist yet ───────────────────────────────────
if ! docker image inspect "$IMAGE_NAME" &>/dev/null; then
    echo "[run.sh] Building Docker image $IMAGE_NAME ..."
    docker build -t "$IMAGE_NAME" .
fi

echo "[run.sh] Starting drone fleet container..."
echo "[run.sh] Fleet report every 5 seconds | Health diagnostics every 10 seconds"
echo "[run.sh] Gamma will hit critical battery in ~30 seconds"
echo ""

# ── Run the container ─────────────────────────────────────────────────────────
docker run \
    --rm \
    -it \
    --name "$CONTAINER_NAME" \
    --net=host \
    -e ROS_DOMAIN_ID=42 \
    -v "$(pwd)/fleet_ws/src:/fleet_ws/src:ro" \
    "$IMAGE_NAME"
