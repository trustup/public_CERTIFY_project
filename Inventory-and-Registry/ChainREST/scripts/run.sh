    echo "Running ChainREST..."
    echo "Generating docker image for ChainREST"
    bash generate-docker-image.sh
    echo "Running docker compose"
    bash run-chain-REST.sh