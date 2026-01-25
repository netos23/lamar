cd ./../docker || exit 1
docker build --platform=linux/amd64 -t dlama:1.0.0 .