_script_dir="$(dirname "$0")"
_repo_dir=$(realpath "$_script_dir/..")
_output_dir="$_repo_dir/build"
_board_name='seeed_xiao_rp2040'

#parse arguments
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
    -rp2350)
        _board_name="seeed_xiao_rp2350"
        shift
        shift
        ;;
    *)
        echo "Unknown option: $1"
        exit 1
        ;;
    esac
done

pushd "$_script_dir"

mkdir -p $_output_dir

set -e

OUTPUT_DIR=$_output_dir PICO_BOARD=$_board_name docker compose -f docker-compose-compile.yml up --build

set +e

popd
