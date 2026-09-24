#!/usr/bin/env bash
# rp2040js test entry point. Same script locally (WSL) and in CI/docker.
#
#   ./run.sh [firmware.uf2|firmware.elf] [--smoke|--ci] [extra args]
#
#   firmware  defaults to build/debug/pwm_controller_pcie.uf2 (repo-relative)
#   --smoke   run the firmware and print captured log lines live (default)
#   --ci      run scenarios/ and exit 0/1
set -euo pipefail

_script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
_repo_dir=$(realpath "$_script_dir/../..")
# The simulator package lives beside this script, unless overridden (the
# docker test image bakes an extracted copy at /opt/tests/simulator -- the
# repo mount there is read-only, so setup would fail).
_sim_dir="${PWM_SIM_DIR:-$_script_dir/simulator}"

FW_PATH=""
MODE="--smoke"

while [[ $# -gt 0 ]]; do
    case "$1" in
    --smoke|--ci)
        MODE="$1"
        shift
        ;;
    -*)  # future flags
        echo "error: unknown option: $1" >&2
        exit 1
        ;;
    *)
        if [[ -n "$FW_PATH" ]]; then
            echo "error: unexpected extra argument: $1 (only one firmware path)" >&2
            exit 1
        fi
        FW_PATH="$1"
        shift
        ;;
    esac
done

if [[ -z "$FW_PATH" ]]; then
    FW_PATH="$_repo_dir/build/debug/pwm_controller_pcie.uf2"
fi
if [[ ! -f "$FW_PATH" ]]; then
    # the compile compose drops artifacts under build/<debug|release>/exec
    for candidate in "$_repo_dir/build/debug/exec/pwm_controller_pcie.uf2" \
                     "$_repo_dir/build/release/exec/pwm_controller_pcie.elf"; do
        if [[ -f "$candidate" ]]; then
            FW_PATH="$candidate"
            break
        fi
    done
fi
if [[ ! -f "$FW_PATH" ]]; then
    echo "error: firmware not found: $FW_PATH" >&2
    echo "build it first:  docker/docker-compose-compile.sh" >&2
    exit 1
fi
FW_PATH=$(realpath "$FW_PATH")

if ! command -v node > /dev/null 2>&1; then
    echo "error: node (v20+) not found in PATH" >&2
    exit 1
fi

# One-time offline setup: extract the vendored tarballs into node_modules
# (no npm / registry involved -- see simulator/setup.sh).
if [[ ! -d "$_sim_dir/node_modules/rp2040js" ]]; then
    echo "setting up vendored simulator (offline)..."
    bash "$_sim_dir/setup.sh"
fi

cd "$_script_dir"
export PWM_TEST_FW="$FW_PATH"

if [[ "$MODE" == "--ci" ]]; then
    exec node run-all.js
else
    exec node smoke.js
fi
