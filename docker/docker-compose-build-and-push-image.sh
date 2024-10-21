_script_dir=$(dirname "$0")
_should_push_images=0
while [ "$1" != "" ]; do
    case $1 in
    -p | --push) _should_push_images=1 ;;
    esac
    shift
done

echo "script_dir: $_script_dir"
pushd "$_script_dir"

_docker_compose_cmd='docker compose'

rm -f output/*
mkdir -p output

set -e

$_docker_compose_cmd -f docker-compose-image-build.yml build --pull
if [ $_should_push_images -eq 1 ]; then
    $_docker_compose_cmd -f docker-compose-image-build.yml push
fi

set +e

popd
