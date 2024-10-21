_script_dir="$(dirname "$0")"
_repo_dir=$(realpath "$_script_dir/..")

pushd "$_script_dir"

set -e

docker compose -f docker-compose-compile.yml up --build

set +e

popd
