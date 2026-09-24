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
    # sequential `compose run` (not `up --abort-on-container-exit`): each job
    # is a batch container; run propagates its exit code without killing
    # siblings when another service exits
    OUTPUT_DIR=$_output_dir docker compose -f docker-compose-test.yml \
        build pico-builder-unit-tests
    OUTPUT_DIR=$_output_dir docker compose -f docker-compose-test.yml \
        run --rm pico-builder-unit-tests
}

run_integration() {
    OUTPUT_DIR=$_output_dir docker compose -f docker-compose-test.yml \
        build pico-tests-integration
    OUTPUT_DIR=$_output_dir docker compose -f docker-compose-test.yml \
        run --rm pico-builder-emu-fw
    OUTPUT_DIR=$_output_dir docker compose -f docker-compose-test.yml \
        run --rm pico-tests-integration
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
