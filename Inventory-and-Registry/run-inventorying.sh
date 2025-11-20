echo 'localhost' | bash setup.sh -a "org1.example.com"

WORKSPACE=$PWD
cd inventoring/
docker build -t gradle-java11-project .
docker run --rm -v "$(pwd)":/app gradle-java11-project

cd $WORKSPACE
bash setup.sh -x ./inventoring/build/install/inventoring

bash setup.sh -r ./ChainREST/