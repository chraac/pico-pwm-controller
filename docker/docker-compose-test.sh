_script_dir="$(dirname "$0")"
_repo_dir=$(realpath "$_script_dir/..")
_output_dir="$_repo_dir/build"

#parse arguments
_cmd='all'
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
    unit)
        _cmd='unit'
        shift
        ;;
    integration)
        _cmd='integration'
        shift
        ;;
    all)
        _cmd='all'
        shift
        ;;
    *)
        echo "Usage: $0 [unit|integration|all]"
        exit 1
        ;;
    esac
done

pushd "$_script_dir"

mkdir -p $_output_dir

set -e

run_unit() {
    OUTPUT_DIR=$_output_dir docker compose -f docker-compose-test.yml \
        up --build --abort-on-container-exit --exit-code-from pico-builder-unit-tests \
        pico-builder-unit-tests
}

run_integration() {
    OUTPUT_DIR=$_output_dir docker compose -f docker-compose-test.yml \
        up --build --abort-on-container-exit --exit-code-from pico-tests-integration \
        pico-builder-emu-fw pico-tests-integration
}

case $_cmd in
unit)
    run_unit
    ;;
integration)
    run_integration
    ;;
all)
    run_unit
    run_integration
    ;;
esac

set +e

popd
