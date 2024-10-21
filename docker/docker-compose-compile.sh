_script_dir="$(dirname "$0")"
_repo_dir=$(realpath "$_script_dir/..")
_output_dir="$_repo_dir/build"

pushd "$_script_dir"

mkdir -p $_output_dir

set -e

OUTPUT_DIR=$_output_dir docker compose -f docker-compose-compile.yml up --build

set +e

popd
