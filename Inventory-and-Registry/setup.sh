#!/bin/bash
PWD_PATH=${PWD}
mkdir workspace
cd workspace
mkdir chaincode
WORKSPACE=${PWD}
CONFIGTX_PATH=configtx.yaml
CHANNEL_NAME=mychannel
CRYPTO_CONFIG_PATH=${PWD}/crypto-config
SCRIPTS_PATH=${PWD}/scripts
CHANNEL_ARTIFACTS_PATH=${PWD}/channel-artifacts

# Añadir la carpeta bin al PATH
export PATH="$PWD/bin:$PATH"

execute_generate_cryptography=false
execute_generate_configuration=false
execute_generate_docker_compose=false
execute_lanzar_hyperledger=false
execute_instalar_cc=false
execute_instalar_ccc=false
execute_lanzar_rest=false
chaincode_project_path=""

ORDERER_DOMAIN="orderer.example.com"
CONNECTION_PROFILE_FILE="connection-profile.json"
HOSTS_FILE="hosts"
mkdir chaincode

function help {
  echo "Este script genera los archivos de configuración y los Genesis Blocks necesarios para crear una red de Hyperledger Fabric completamente funcional."
  echo ""
  echo "Uso: $0 [ORGANIZATION1] [ORGANIZATION2] ... [ORGANIZATIONN] [-acdlh] [-i RUTA_AL_PROYECTO_JAVA] [-x RUTA_AL_PROYECTO_JAVA] -r  [RUTA_A_LA_CARPETA_NODEJS]"
  echo ""
  echo "Opciones:"
  echo "  -h  Mostrar ayuda"
  echo "  -a  Generar criptografía, configuración, Docker Compose y lanzar Hyperledger"
  echo "  -b  Generar cryptografía"
  echo "  -c  Generar configuración"
  echo "  -d  Generar Docker Compose"
  echo "  -l  Lanzar Hyperledger"
  echo "  -i  [RUTA_AL_PROYECTO_JAVA] Compilar e instalar Chaincode/smartcontract"
  echo "  -x  [RUTA_AL_PROYECTO_JAVA] Instalar Chaincode/smartcontract"
  echo "  -r  [RUTA_A_LA_CARPETA_NODEJS] Lanzar REST"
  echo ""
  echo "Ejemplo: $0 org1.example.com org2.example.com -acdl -i /ruta/al/proyecto/java"
  echo ""
}


declare -a org_names=()
workspace_path="./crypto-config/peerOrganizations/"
while [[ $# -ge 1 ]] ; do
  echo "clave en procesamiento: $1"
  key="$1"
  case $key in
    -a)
      execute_generate_cryptography=true
      execute_generate_configuration=true
      execute_generate_docker_compose=true
      execute_lanzar_hyperledger=true
      ;;
    -b)
      execute_generate_cryptography=true
      ;;
    -c)
      execute_generate_configuration=true
      ;;
    -d)
      execute_generate_docker_compose=true
      ;;
    -r)
      execute_lanzar_rest=true
      shift
      rest_project_path=$PWD_PATH/$1
      if [ ! -d "$rest_project_path" ]; then
        echo "Error: La ruta a la carpeta del proyecto REST de Node.js no existe."
        echo "Use la opción -h para ver la ayuda."
        help
        exit 1
      fi
      ;;
    -l)
      execute_lanzar_hyperledger=true
      ;;
    -h)
      help
      ;;
    -i)
      execute_instalar_cc=true
      shift
      chaincode_project_path=$PWD_PATH/$1
      if [ ! -d "$chaincode_project_path" ]; then
        echo "Error: La ruta al proyecto Java para instalar el chaincode no existe."
        echo "Use la opción -h para ver la ayuda."
        help
        exit 1
      fi
      ;;
    -x)
      execute_instalar_ccc=true
      shift
      chaincode_project_path=$PWD_PATH/$1
      echo $chaincode_project_path
      if [ ! -d "$chaincode_project_path" ]; then
        echo "Error: La ruta al proyecto Java para instalar el chaincode no existe."
        echo "Use la opción -h para ver la ayuda."
        help
        exit 1
      fi
      ;;
      *)
        if [[ -n "$key" && ${key:0:1} != '-' ]]; then
          org_names+=("$key")
        else
          echo "Opción inválida: $key"
          help
          exit 1
        fi
        ;;
    esac
  shift
  echo "${org_names[@]}"
done

if [ ${#org_names[@]} -eq 0 ]; then
  if [ -d "$workspace_path" ]; then
    org_folders=$(ls -d "$workspace_path"*/)
    if [ -n "$org_folders" ]; then
      for folder in $org_folders; do
        folder_name=$(basename "$folder")
        org_names+=("$folder_name")
      done
    else
      echo "Error: El directorio no contiene ninguna carpeta."
      echo "El usuario necesita introducir al menos el nombre de una organización."
      echo "Use la opción -h para ver la ayuda."
      exit 1
    fi
  else
    echo "Error: El directorio no existe o está vacío."
    echo "El usuario necesita introducir al menos el nombre de una organización."
    echo "Use la opción -h para ver la ayuda."
    exit 1
  fi
fi

echo "Organizaciones:"
for org in "${org_names[@]}"; do
  echo "- $org"
done



function check_bin_folder() {
  # Comprobar si la carpeta bin existe
  if [[ ! -d "./bin" ]]; then
    echo "La carpeta bin no existe, instalando los binarios de Hyperledger Fabric..."
    
    # Descargar e instalar los binarios de Hyperledger Fabric
    curl -sSLO https://raw.githubusercontent.com/hyperledger/fabric/main/scripts/install-fabric.sh && chmod +x install-fabric.sh
    ./install-fabric.sh -f 2.4 docker
    ./install-fabric.sh  binaries
    echo "Los binarios de Hyperledger Fabric se han instalado correctamente en la carpeta bin."
  fi
}


# Generar la criptografía para las organizaciones y el nodo ordenador
function generar_criptografia {
  # Obtener el array de nombres de organizaciones como parámetroF
  rm -rf crypto-config
  # Crear el archivo de configuración crypto-config.yaml
  echo "OrdererOrgs:" > crypto-config.yaml
  echo "  - Name: Orderer" >> crypto-config.yaml
  echo "    Domain: example.com" >> crypto-config.yaml
  echo "    Specs:" >> crypto-config.yaml
  echo "      - Hostname: orderer" >> crypto-config.yaml
  echo "        SANS:" >> crypto-config.yaml
  echo "          - \"localhost\"" >> crypto-config.yaml
  echo "          - \"127.0.0.1\"" >> crypto-config.yaml
  echo "          - \"orderer.example.com\"" >> crypto-config.yaml
  echo "" >> crypto-config.yaml
  echo "PeerOrgs:" >> crypto-config.yaml
  for org_name in "${org_names[@]}"; do
    echo "  - Name: ${org_prefix}" >> crypto-config.yaml
    echo "    Domain: $org_name" >> crypto-config.yaml
    echo "    EnableNodeOUs: true" >> crypto-config.yaml
    echo "    Specs:" >> crypto-config.yaml
    echo "      - Hostname: peer0" >> crypto-config.yaml
    echo "        SANS:" >> crypto-config.yaml
    echo "          - 'peer0.${org_name}'" >> crypto-config.yaml
    echo "          - 'peer0'" >> crypto-config.yaml
    echo "          - 'localhost'" >> crypto-config.yaml
    echo "          - '127.0.0.1'" >> crypto-config.yaml
    echo "    Template:" >> crypto-config.yaml
    echo "      Count: 1" >> crypto-config.yaml
    echo "    Users:" >> crypto-config.yaml
    echo "      Count: 1" >> crypto-config.yaml
  done

  # Crear la carpeta crypto-config/ordererOrganizations
  mkdir -p crypto-config/ordererOrganizations/example.com

  # Generar la criptografía para las organizaciones y el nodo ordenador
  cryptogen generate --config=crypto-config.yaml --output=crypto-config
}

# Generar el archivo configtx.yaml y los Genesis Blocks del Orderer y del Canal
function generar_configuracion {
  rm -rf configtx.yaml channel-artifacts
  # Generar el archivo configtx.yaml
    echo "---
Organizations:" > configtx.yaml
    # Agregar perfil de organización para orderer
    echo "  - &OrdererOrg" >> $CONFIGTX_PATH
    echo "    Name: OrdererOrg" >> $CONFIGTX_PATH
    echo "    ID: OrdererMSP" >> $CONFIGTX_PATH
    echo "    MSPDir: crypto-config/ordererOrganizations/example.com/msp" >> $CONFIGTX_PATH
    echo "    Policies:" >> $CONFIGTX_PATH
    echo "      Readers:" >> $CONFIGTX_PATH
    echo "        Type: Signature" >> $CONFIGTX_PATH
    echo "        Rule: \"OR('OrdererMSP.member')\"" >> $CONFIGTX_PATH
    echo "      Writers:" >> $CONFIGTX_PATH
    echo "        Type: Signature" >> $CONFIGTX_PATH
    echo "        Rule: \"OR('OrdererMSP.member')\"" >> $CONFIGTX_PATH
    echo "      Admins:" >> $CONFIGTX_PATH
    echo "        Type: Signature" >> $CONFIGTX_PATH
    echo "        Rule: \"OR('OrdererMSP.admin')\"" >> $CONFIGTX_PATH

for org_name in "${org_names[@]}"; do
  org_prefix="${org_name%%.*}"
  org_prefix="${org_prefix^}"  # convierte la primera letra a mayúscula
  echo "  - &${org_prefix}MSP" >> "$CONFIGTX_PATH"
  echo "    Name: ${org_prefix}MSP" >> "$CONFIGTX_PATH"
  echo "    ID: ${org_prefix}MSP" >> "$CONFIGTX_PATH"
  echo "    MSPDir: crypto-config/peerOrganizations/${org_name}/msp" >> "$CONFIGTX_PATH"
  echo "    Policies:" >> "$CONFIGTX_PATH"
  echo "      Readers:" >> "$CONFIGTX_PATH"
  echo "        Type: Signature" >> "$CONFIGTX_PATH"
  echo "        Rule: \"OR('${org_prefix}MSP.admin', '${org_prefix}MSP.peer', '${org_prefix}MSP.client')\"" >> "$CONFIGTX_PATH"
  echo "      Writers:" >> "$CONFIGTX_PATH"
  echo "        Type: Signature" >> "$CONFIGTX_PATH"
  echo "        Rule: \"OR('${org_prefix}MSP.admin', '${org_prefix}MSP.client')\"" >> "$CONFIGTX_PATH"
  echo "      Admins:" >> "$CONFIGTX_PATH"
  echo "        Type: Signature" >> "$CONFIGTX_PATH"
  echo "        Rule: \"OR('${org_prefix}MSP.admin')\"" >> "$CONFIGTX_PATH"
  echo "      Endorsement:" >> "$CONFIGTX_PATH"
  echo "        Type: Signature" >> "$CONFIGTX_PATH"
  echo "        Rule: \"OR('${org_prefix}MSP.peer')\"" >> "$CONFIGTX_PATH"
  echo "    AnchorPeers:" >> "$CONFIGTX_PATH"
  echo "      - Host: peer0.${org_name}" >> "$CONFIGTX_PATH"
  echo "        Port: 7051" >> "$CONFIGTX_PATH"
done



    # Agregar sección de Capabilities
    echo "Capabilities:" >> $CONFIGTX_PATH
    echo "  Channel: &ChannelCapabilities" >> $CONFIGTX_PATH
    echo "    V2_0: true" >> $CONFIGTX_PATH
    echo "  Orderer: &OrdererCapabilities" >> $CONFIGTX_PATH
    echo "    V2_0: true" >> $CONFIGTX_PATH
    echo "  Application: &ApplicationCapabilities" >> $CONFIGTX_PATH
    echo "    V2_0: true" >> $CONFIGTX_PATH

    # Agregar sección de ApplicationDefaults
    echo "Application: &ApplicationDefaults" >> $CONFIGTX_PATH
    echo "  Organizations:" >> $CONFIGTX_PATH
    echo "  Policies:" >> $CONFIGTX_PATH
    echo "    Readers:" >> $CONFIGTX_PATH
    echo "      Type: ImplicitMeta" >> $CONFIGTX_PATH
    echo "      Rule: \"ANY Readers\"" >> $CONFIGTX_PATH
    echo "    Writers:" >> $CONFIGTX_PATH
    echo "      Type: ImplicitMeta" >> $CONFIGTX_PATH
    echo "      Rule: \"ANY Writers\"" >> $CONFIGTX_PATH
    echo "    Admins:" >> $CONFIGTX_PATH
    echo "      Type: ImplicitMeta" >> $CONFIGTX_PATH
    echo "      Rule: \"MAJORITY Admins\"" >> $CONFIGTX_PATH
    echo "    LifecycleEndorsement:" >> $CONFIGTX_PATH
    echo "      Type: ImplicitMeta" >> $CONFIGTX_PATH
    echo "      Rule: \"MAJORITY Endorsement\"" >> $CONFIGTX_PATH
    echo "    Endorsement:" >> $CONFIGTX_PATH
    echo "      Type: ImplicitMeta" >> $CONFIGTX_PATH
    echo "      Rule: \"MAJORITY Endorsement\"" >> $CONFIGTX_PATH
    echo "  Capabilities:" >> $CONFIGTX_PATH
    echo "    <<: *ApplicationCapabilities" >> $CONFIGTX_PATH
    # Agregar seccion ordererdefault
    cat <<EOF >> "$CONFIGTX_PATH"
Orderer: &OrdererDefaults
  OrdererType: etcdraft
  Addresses:
    - orderer.example.com:7050
  EtcdRaft:
    Consenters:
      - Host: orderer.example.com
        Port: 7050
        ClientTLSCert: ./crypto-config/ordererOrganizations/example.com/orderers/orderer.example.com/tls/server.crt
        ServerTLSCert: ./crypto-config/ordererOrganizations/example.com/orderers/orderer.example.com/tls/server.crt
  BatchTimeout: 500ms
  BatchSize:
    MaxMessageCount: 10
    AbsoluteMaxBytes: 99 MB
    PreferredMaxBytes: 512 KB
  Organizations:
  Policies:
    Readers:
      Type: ImplicitMeta
      Rule: "ANY Readers"
    Writers:
      Type: ImplicitMeta
      Rule: "ANY Writers"
    Admins:
      Type: ImplicitMeta
      Rule: "MAJORITY Admins"
    BlockValidation:
      Type: ImplicitMeta
      Rule: "ANY Writers"
EOF

    # Agregar sección de ChannelDefaults
    echo "Channel: &ChannelDefaults"  >> $CONFIGTX_PATH
    echo "  Policies:" >> $CONFIGTX_PATH
    echo "    Readers:" >> $CONFIGTX_PATH
    echo "      Type: ImplicitMeta" >> $CONFIGTX_PATH
    echo "      Rule: \"ANY Readers\"" >> $CONFIGTX_PATH
    echo "    Writers:" >> $CONFIGTX_PATH
    echo "      Type: ImplicitMeta" >> $CONFIGTX_PATH
    echo "      Rule: \"ANY Writers\"" >> $CONFIGTX_PATH
    echo "    Admins:" >> $CONFIGTX_PATH
    echo "      Type: ImplicitMeta" >> $CONFIGTX_PATH
    echo "      Rule: \"MAJORITY Admins\"" >> $CONFIGTX_PATH
    echo "  Capabilities:" >> $CONFIGTX_PATH
    echo "    <<: *ChannelCapabilities" >> $CONFIGTX_PATH


    echo "Profiles:" >> "$CONFIGTX_PATH"
    echo "  OneOrgOrdererGenesis:" >> "$CONFIGTX_PATH"
    echo "    <<: *ChannelDefaults" >> "$CONFIGTX_PATH"
    echo "    Orderer:" >> "$CONFIGTX_PATH"
    echo "      <<: *OrdererDefaults" >> "$CONFIGTX_PATH"
    echo "      Organizations:" >> "$CONFIGTX_PATH"
    echo "        - *OrdererOrg" >> "$CONFIGTX_PATH"
    echo "      Capabilities:" >> "$CONFIGTX_PATH"
    echo "        <<: *OrdererCapabilities" >> "$CONFIGTX_PATH"
    echo "    Consortiums:" >> "$CONFIGTX_PATH"
    echo "      SampleConsortium:" >> "$CONFIGTX_PATH"
    echo "        Organizations:" >> "$CONFIGTX_PATH"

    for org_name in "${org_names[@]}"; do
      org_prefix="${org_name%%.*}"
      org_prefix="${org_prefix^}"  # convierte la primera letra a mayúscula
      echo "          - *${org_prefix}MSP" >> "$CONFIGTX_PATH"
    done

    echo "  MultiOrgChannel:" >> "$CONFIGTX_PATH"
    echo "    Consortium: SampleConsortium" >> "$CONFIGTX_PATH"
    echo "    <<: *ChannelDefaults" >> "$CONFIGTX_PATH"
    echo "    Application:" >> "$CONFIGTX_PATH"
    echo "      <<: *ApplicationDefaults" >> "$CONFIGTX_PATH"
    echo "      Organizations:" >> "$CONFIGTX_PATH"

    for org_name in "${org_names[@]}"; do
      org_prefix="${org_name%%.*}"
      org_prefix="${org_prefix^}"  # convierte la primera letra a mayúscula
      echo "        - *${org_prefix}MSP" >> "$CONFIGTX_PATH"
    done

    echo "      Capabilities:" >> "$CONFIGTX_PATH"
    echo "        <<: *ApplicationCapabilities" >> "$CONFIGTX_PATH"
    echo "" >> configtx.yaml
    # Generar el Genesis Block del Orderer
    mkdir -p channel-artifacts/
    configtxgen -profile OneOrgOrdererGenesis -channelID system-channel -outputBlock channel-artifacts/genesis.block
    # Generar el archivo de configuración del canal
    configtxgen -profile MultiOrgChannel -outputCreateChannelTx channel-artifacts/${CHANNEL_NAME}.tx -channelID ${CHANNEL_NAME}
    # Crear el archivo de configuración para actualizar el anchor peer

    for org_name in "${org_names[@]}"; do
        org_prefix="${org_name%%.*}"
        org_prefix="${org_prefix^}"  # convierte la primera letra a mayúscula
        configtxgen -profile MultiOrgChannel -outputAnchorPeersUpdate "channel-artifacts/${org_prefix}MSPanchors.tx" -channelID ${CHANNEL_NAME} -asOrg "${org_prefix}MSP"
        done
    # Crear objeto para almacenar el perfil de conexión
    PROFILE='{ "name": "'"$CHANNEL_NAME"'", "version": "1.0.0", "channels": { "'"$CHANNEL_NAME"'": { "orderers": [ "'"$ORDERER_DOMAIN"'" ], "peers": {'

# Añadir cada organización al objeto del perfil de conexión
  for org_name in "${org_names[@]}"
    do
      PROFILE+=' "'"$org_name"'.peer": {}'
    done

  # Añadir cada organización como objeto en la sección de organizaciones
  PROFILE+=' }, "organizations": {'

  for org_name in "${org_names[@]}"
    do
      PROFILE+=' "'"$org_name"'": { "mspid": "'"$org_name"'MSP", "peers": [ "peer.'"$org_name"'" ], "certificateAuthorities": [ ""ca.'"$org_name"'" ] },'
    done

  # Eliminar la última coma de la lista de organizaciones y cerrar la sección
  PROFILE=${PROFILE%?}
  PROFILE+=' },'

  # Añadir cada peer de cada organización a la sección de peers
  PROFILE+=' "peers": {'
  for org_name in "${org_names[@]}"
  do
    peer_name="peer.$org_name"
    PROFILE+=' "'"$peer_name"'": { "url": "grpc://'"$peer_name"':7051", "grpcOptions": { "ssl-target-name-override": "'"$peer_name"'" }, "tlsCACerts": { "path": "'"$CRYPTO_CONFIG_PATH"'/peerOrganizations/'"$org_name"'/tlsca/tlsca.'"$org_name"'.pem" } },'
  done

  # Eliminar la última coma de la lista de peers y cerrar la sección
  PROFILE=${PROFILE%?}
  PROFILE+=' },'

  # Añadir cada orderer a la sección de orderers
  PROFILE+=' "orderers": { "'"$ORDERER_DOMAIN"'": { "url": "grpc://'"$ORDERER_DOMAIN"':7050", "grpcOptions": { "ssl-target-name-override": "'"$ORDERER_DOMAIN"'" }, "tlsCACerts": { "path": "'"$CRYPTO_CONFIG_PATH"'/ordererOrganizations/example.com/tlsca/tlsca.example.com-cert.pem" } } }'

  # Cerrar el objeto del perfil de conexión
  PROFILE+=' }'

  # Guardar el perfil de conexión en un archivo
  echo "$PROFILE" > "$CONNECTION_PROFILE_FILE"

  # Generar archivo de hosts
  echo "Ingresa la dirección IP donde se lanzarán los servicios:"
  read IP_ADDRESS

  echo "$IP_ADDRESS  $ORDERER_DOMAIN" > "$HOSTS_FILE"

  for org_name in "${org_names[@]}"
  do
    peer_name="$peer.org_name"
    echo "$IP_ADDRESS  $peer_name" >> "$HOSTS_FILE"
  done

    echo "Archivos generados:"
    echo "$CONNECTION_PROFILE_FILE"
    echo "$HOSTS_FILE"


}
# Función para detener y eliminar todos los contenedores de Hyperledger Fabric en ejecución,
# borrar los volúmenes y lanzar todos los docker-compose de la carpeta actual
function lanzar_hyperledger() {

  # Detener y eliminar todos los contenedores de Hyperledger Fabric en ejecución
  echo "Deteniendo y eliminando los contenedores de Hyperledger Fabric en ejecución..."
docker ps -a | grep hyperledger | awk '{print $1}' | xargs docker rm -f
docker ps -a | grep couchdb | awk '{print $1}' | xargs docker rm -f
docker container prune -f 
docker volume prune -f


  # Inicializar contador de intentos
  n=1
  max_attempts=2

  # Recorrer la carpeta docker-composes y lanzar todos los docker-compose
  find docker-composes -name "*.yml" | while read file
  do
    echo "Lanzando docker-compose de $file..."
    docker-compose -f "$file" up -d
  done

  # Comprobar si están todos los contenedores en ejecución
  failed_containers=()
  while [ $(docker ps -q | wc -l) -ne $(find docker-composes -name "*.yml" | wc -l) ] && [ $n -le $max_attempts ]
  do
  echo "Esperando a que se inicien todos los contenedores... Intento $n de $max_attempts"
    sleep 5
    ((n++))
    failed_containers=($(for file in $(find docker-composes -name "*.yml"); do basename "$file" .yml; done | grep -v $(docker ps --format '{{.Names}}') || true))
  done

# Mostrar información de contenedores no iniciados
if [ ${#failed_containers[@]} -gt 0 ]
then
  echo "Los siguientes contenedores no se han iniciado correctamente:"
  printf '%s\n' "${failed_containers[@]}"
  exit 1
fi
  for org_name in "${org_names[@]}"
    do
      org_prefix="${org_name%%.*}"
      org_prefix="${org_prefix^}"  # convierte la primera letra a mayúscula
      echo "Instalando canal en peer0.${org_name}..."
set -x
docker exec -e CORE_PEER_ADDRESS=peer0.${org_name}:7051 \
          -e CORE_PEER_LOCALMSPID=${org_prefix}MSP \
          -e CORE_PEER_TLS_CERT_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/server.crt \
          -e CORE_PEER_TLS_KEY_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/server.key \
          -e CORE_PEER_TLS_ROOTCERT_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/ca.crt \
          -e CORE_PEER_MSPCONFIGPATH=/etc/crypto-config/peerOrganizations/${org_name}/users/Admin@${org_name}/msp \
          cli bash -c "/etc/scripts/crearCanal.sh $CHANNEL_NAME ${org_prefix}MSPanchors.tx"
          

set +x

    
    done


  # Comprobar que todos los contenedores están en ejecución
  echo "Todos los contenedores se han iniciado correctamente."
}


function generar_docker_compose {
  index=6;
  for org_name in "${org_names[@]}"; do
    org_prefix="${org_name%%.*}"
    org_prefix="${org_prefix^}"  # convierte la primera letra a mayúscula

 ((index++))
    # Crear la carpeta para el peer de la organización
    mkdir -p docker-composes/peers/${org_name}/
    mkdir -p docker-composes/ca/${org_name}/

    # Crear el archivo de docker-compose.yml para el peer de la organización
    cat <<EOF > "docker-composes/peers/${org_name}/docker-compose.yml"
version: '3.4'

networks:
  local:
    name: fabric_local

volumes:
  peer0.${org_name}:

services:
  peer0.${org_name}:
    container_name: peer0.${org_name}
    image: hyperledger/fabric-peer:2.4
    environment:
      - CORE_VM_ENDPOINT=unix:///host/var/run/docker.sock
      - CORE_CHAINCODE_EXECUTETIMEOUT=30s
      - CORE_PEER_KEEPALIVE_DELIVERYCLIENT_TIMEOUT=30s
      - CORE_PEER_KEEPALIVE_CLIENT_TIMEOUT=30s
      - CORE_VM_DOCKER_HOSTCONFIG_NETWORKMODE=fabric_local
      - CORE_PEER_CHAINCODELISTENADDRESS=0.0.0.0:7052
      - FABRIC_LOGGING_SPEC=info
      - CORE_PEER_TLS_ENABLED=true
      - CORE_PEER_PROFILE_ENABLED=true
      - CORE_PEER_TLS_CERT_FILE=/etc/hyperledger/fabric/tls/server.crt
      - CORE_PEER_TLS_KEY_FILE=/etc/hyperledger/fabric/tls/server.key
      - CORE_PEER_TLS_ROOTCERT_FILE=/etc/hyperledger/fabric/tls/ca.crt
      - CORE_PEER_ID=peer0.${org_name}
      - CORE_PEER_ADDRESS=peer0.${org_name}:7051
      - CORE_PEER_GOSSIP_USELEADERELECTION=false
      - CORE_PEER_GOSSIP_ORGLEADER=true
      - CORE_PEER_GOSSIP_EXTERNALENDPOINT=peer0.${org_name}:7051
      - CORE_PEER_LOCALMSPID=${org_prefix}MSP
      - CORE_VM_DOCKER_ATTACHSTDOUT=true
      - CORE_CHAINCODE_STARTUPTIMEOUT=1200s
      - CORE_CHAINCODE_EXECUTETIMEOUT=800s
      - CORE_LEDGER_STATE_STATEDATABASE=CouchDB
      - CORE_LEDGER_STATE_COUCHDBCONFIG_COUCHDBADDRESS=couchdb.example.com:5984
      # The CORE_LEDGER_STATE_COUCHDBCONFIG_USERNAME and CORE_LEDGER_STATE_COUCHDBCONFIG_PASSWORD
      # provide the credentials for ledger to connect to CouchDB.  The username and password must
      # match the username and password set for the associated CouchDB.
      - CORE_LEDGER_STATE_COUCHDBCONFIG_USERNAME=admin
      - CORE_LEDGER_STATE_COUCHDBCONFIG_PASSWORD=adminpw
    restart: always
    working_dir: /opt/gopath/src/github.com/hyperledger/fabric/peer
    command: peer node start
    networks:
      - local

    volumes:
        - /var/run/:/host/var/run/
        - ${CRYPTO_CONFIG_PATH}/peerOrganizations/${org_name}/peers/peer0.${org_name}/msp:/etc/hyperledger/fabric/msp
        - ${CRYPTO_CONFIG_PATH}/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls:/etc/hyperledger/fabric/tls
        - peer0.${org_name}:/var/hyperledger/production
    ports:
      - "$((index))051:7051"
      - "$((index))052:7052"
      - "$((index))053:7053"

EOF


    cat <<EOF > "docker-composes/ca/${org_name}/docker-compose.yml"
version: '3.4'

networks:
  local:
    name: fabric_local

services:
  ca.${org_name}:
    container_name: ca.${org_name}
    image: hyperledger/fabric-ca
    environment:
      - FABRIC_CA_HOME=/etc/hyperledger/fabric-ca-server
      - FABRIC_CA_SERVER_CA_NAME=ca-${org_prefix}
      - FABRIC_CA_SERVER_TLS_ENABLED=true
      - FABRIC_CA_SERVER_TLS_CERTFILE=/etc/hyperledger/fabric-ca-server-config/ca.${org_name}-cert.pem
      - FABRIC_CA_SERVER_TLS_KEYFILE=/etc/hyperledger/fabric-ca-server-config/priv_sk
      - FABRIC_CA_SERVER_PORT=7054
    ports:
      - "$((index))054:7054"
    command: sh -c 'fabric-ca-server start --ca.certfile /etc/hyperledger/fabric-ca-server-config/ca.${org_name}-cert.pem --ca.keyfile /etc/hyperledger/fabric-ca-server-config/priv_sk -b admin:adminpw -d'
    volumes:
      - ${WORKSPACE}/crypto-config/peerOrganizations/${org_name}/ca/:/etc/hyperledger/fabric-ca-server-config
    networks:
      - local
    restart: always
EOF
  done
}
function generar_cli_compose {

    mkdir -p docker-composes/cli/

    # Crear el archivo de docker-compose.yml para el peer de la organización
    cat <<EOF > "docker-composes/cli/docker-compose.yml"
version: '3.4'

networks:
  local:
    name: fabric_local

volumes:
  cli:


services:
  cli:
      container_name: cli
      image: hyperledger/fabric-tools:2.4
      tty: true
      stdin_open: true

      environment:

        - GOPATH=/opt/gopath
        - CORE_VM_ENDPOINT=unix:///host/var/run/docker.sock
        - FABRIC_LOGGING_SPEC=INFO
        - CORE_PEER_ID=cli
        - CORE_PEER_TLS_ENABLED=true
      working_dir: /etc/
      restart: always
      command: /bin/bash
      volumes:
          - /var/run/:/host/var/run/
          - ${WORKSPACE}/chaincode:/etc/chaincode:rw
          - ${WORKSPACE}/crypto-config:/etc/crypto-config
          - ${WORKSPACE}/scripts:/etc/scripts
          - ${WORKSPACE}/channel-artifacts:/etc/channel-artifacts
      networks:
        - local
EOF
}

function crear_scripts() {
  
    mkdir -p scripts
    touch scripts/crearCanal.sh # Crea el archivo si no existe
    chmod +x scripts/crearCanal.sh # Añade permisos de ejecución al archivo
    # Crear el archivo de docker-compose.yml para el peer de la organización
    cat <<EOF > "scripts/crearCanal.sh"
#!/bin/bash
if [ \$# -ne 2 ]; then
    echo "Usage: \$0 [CHANNEL_NAME] [ANCHOR_NAME]"
    exit 1
fi

CHANNEL_NAME="\$1"
ORDERER_CA=/etc/crypto-config/ordererOrganizations/example.com/orderers/orderer.example.com/msp/tlscacerts/tlsca.example.com-cert.pem

TX_FILE="/etc/channel-artifacts/${CHANNEL_NAME}.tx"
BLOCK_FILE="/etc/channel-artifacts/${CHANNEL_NAME}.block"
ANCHOR_FILE="/etc/channel-artifacts/\$2"

if [ ! -f "\$BLOCK_FILE" ]; then
    echo "Block file \$BLOCK_FILE does not exist, creating channel..."
    peer channel create -o orderer.example.com:7050 -c "\$CHANNEL_NAME" -f "\$TX_FILE" --tls --cafile "\$ORDERER_CA"
    mv \$CHANNEL_NAME.block \$BLOCK_FILE
    sleep 5
else
    echo "Block file \$BLOCK_FILE exists, skipping channel creation..."
fi

peer channel join  -o orderer.example.com:7050  -b "\$BLOCK_FILE" --tls --cafile "\$ORDERER_CA"
peer channel update -o orderer.example.com:7050 -c "${CHANNEL_NAME}" --tls --cafile "\$ORDERER_CA" -f \$ANCHOR_FILE #/etc/hyperledger/configtx/${org_prefix}MSPanchors.tx 
EOF

    touch scripts/installSC.sh # Crea el archivo si no existe
    chmod +x scripts/installSC.sh # Añade permisos de ejecución al archivo
    # Crear el archivo de docker-compose.yml para el peer de la organización
    cat <<EOF > "scripts/installSC.sh"
#!/bin/bash
a=\$2
b=\$3 # url peer
c=\$4 # channel
ORDERER_CA=/etc/crypto-config/ordererOrganizations/example.com/orderers/orderer.example.com/msp/tlscacerts/tlsca.example.com-cert.pem
rutachaincode=/etc/chaincode/
#PEERADDRESSES="--peerAddresses peer0.org1.example.com:7051 --tlsRootCertFiles  \${rutapeer}/crypto/peerOrganizations/org1.example.com/peers/peer0.org1.example.com/tls/ca.crt"


fn () {
    set -x

    peer lifecycle chaincode package \$1package\$a.tar.gz --path \$rutachaincode/\$1/  --lang java --label \$1\$a
    echo
    peer lifecycle chaincode install \$1package\$a.tar.gz
    echo
    peer lifecycle chaincode queryinstalled
    PACKAGEID=\$( peer lifecycle chaincode queryinstalled | grep "\$1\$a" | cut -d" " -f3 | cut -f1 -d",")
    echo
    peer lifecycle chaincode approveformyorg -o orderer.example.com:7050 --channelID \$c --name \$1 --version \$a --package-id \$PACKAGEID --sequence \$a --tls --cafile \$ORDERER_CA
    echo

    peer lifecycle chaincode checkcommitreadiness --channelID \$c --name \$1 --version \$a --sequence \$a --tls --cafile \$ORDERE_CA --output json  
    echo
    peer lifecycle chaincode commit -o orderer.example.com:7050 --channelID \$c --name \$1 --version \$a --sequence \$a --tls --cafile \$ORDERER_CA 
    echo
    peer lifecycle chaincode querycommitted --channelID \$c --name \$1 --cafile \$ORDERER_CA 
    echo
}





fn \$1

echo 'USO DEL SCRIPT: ./installOne.sh [nombre_de_la_carpeta_que_contiene_el_chaincode] [version_del_chaincode]'
#peer chaincode invoke -o 10.208.211.47:7050 --tls --cafile \$ORDERER_CA -C mychannel -n name  -c '{"function":"publicarconfig","Args":[]}'
EOF

    cat <<EOF > "scripts/commit.sh"
#!/bin/bash
a=\$2
b=\$3 # url peer
c=\$4 # channel
ORDERER_CA=/etc/crypto-config/ordererOrganizations/example.com/orderers/orderer.example.com/msp/tlscacerts/tlsca.example.com-cert.pem
rutachaincode=/etc/chaincode/
PEERADDRESSES=\$5


fn () {
    set -x
    peer lifecycle chaincode checkcommitreadiness --channelID \$c --name \$1 --version \$a --sequence \$a --tls --cafile \$ORDERE_CA --output json  
    echo
    peer lifecycle chaincode commit -o orderer.example.com:7050 --channelID \$c --name \$1 --version \$a --sequence \$a --tls --cafile \$ORDERER_CA \$PEERADDRESSES
    echo
    peer lifecycle chaincode querycommitted --channelID \$c --name \$1 --cafile \$ORDERER_CA 
    echo
}

fn \$1
EOF

}

function generate_orderer_docker_compose() {
  mkdir -p docker-composes/orderers/
  cat > docker-composes/orderers/docker-compose-orderer.yml << EOF
version: '3.4'

networks:
  local:
    name: fabric_local

volumes:
  orderer.example.com:

services:
  orderer.example.com:
    container_name: orderer.example.com
    image: hyperledger/fabric-orderer:2.4
    environment:
      - FABRIC_LOGGING_SPEC=info
      - ORDERER_GENERAL_LISTENADDRESS=0.0.0.0
      - ORDERER_GENERAL_GENESISMETHOD=file
      - ORDERER_GENERAL_GENESISFILE=/var/hyperledger/orderer/orderer.genesis.block
      - ORDERER_GENERAL_LOCALMSPID=OrdererMSP
      - ORDERER_GENERAL_LOCALMSPDIR=/var/hyperledger/orderer/msp
      - ORDERER_GENERAL_TLS_ENABLED=true
      - ORDERER_GENERAL_TLS_PRIVATEKEY=/var/hyperledger/orderer/tls/server.key
      - ORDERER_GENERAL_TLS_CERTIFICATE=/var/hyperledger/orderer/tls/server.crt
      - ORDERER_GENERAL_TLS_ROOTCAS=[/var/hyperledger/orderer/tls/ca.crt]
      - ORDERER_GENERAL_CLUSTER_CLIENTCERTIFICATE=/var/hyperledger/orderer/tls/server.crt
      - ORDERER_GENERAL_CLUSTER_CLIENTPRIVATEKEY=/var/hyperledger/orderer/tls/server.key
      - ORDERER_GENERAL_CLUSTER_ROOTCAS=[/var/hyperledger/orderer/tls/ca.crt]
    working_dir: /opt/gopath/src/github.com/hyperledger/fabric/orderer
    command: orderer
    volumes:
      - ${CRYPTO_CONFIG_PATH}/ordererOrganizations/example.com/orderers/orderer.example.com/msp:/var/hyperledger/orderer/msp
      - ${CRYPTO_CONFIG_PATH}/ordererOrganizations/example.com/orderers/orderer.example.com/tls/:/var/hyperledger/orderer/tls
      - ${CHANNEL_ARTIFACTS_PATH}/genesis.block:/var/hyperledger/orderer/orderer.genesis.block
    ports:
      - "7050:7050"
    networks:
      - local
EOF
}

function generate_couchdb_docker_compose() {
  mkdir -p docker-composes/database/
  cat > docker-composes/database/docker-compose-couchdb.yml << EOF
version: '3.4'

networks:
  local:
    name: fabric_local

services:
  couchdb.example.com:
    container_name: couchdb.example.com
    image: couchdb:3.1.0
    environment:
      - COUCHDB_USER=admin
      - COUCHDB_PASSWORD=adminpw
    ports:
      - "5984:5984"
    networks:
      - local
    volumes:
      - ./couchdb-data:/opt/couchdb/data
EOF
}
function instalar_cc() {
cd $chaincode_project_path
  echo "compiling smart contract"
  ruta=$chaincode_project_path
  nombre_ultima_carpeta=$(basename $(readlink -f "$ruta"))
  ./gradlew installDist
    cp -r META-INF build/install/$nombre_ultima_carpeta/

  cd build/install
  rm -r $WORKSPACE/chaincode/$nombre_ultima_carpeta
  cp -r $nombre_ultima_carpeta $WORKSPACE/chaincode/$nombre_ultima_carpeta


  echo "running script to install smartcontract on peer container"
  #run script installSC on container named cli and redirect error to output

  for org_name in "${org_names[@]}"; do
    org_prefix="${org_name%%.*}"
    org_prefix="${org_prefix^}"  # convierte la primera letra a mayúscula
    output=$(docker exec -e CORE_PEER_ADDRESS=peer0.${org_name}:7051 \
            -e CORE_PEER_LOCALMSPID=${org_prefix}MSP \
            -e CORE_PEER_TLS_CERT_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/server.crt \
            -e CORE_PEER_TLS_KEY_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/server.key \
            -e CORE_PEER_TLS_ROOTCERT_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/ca.crt \
            -e CORE_PEER_MSPCONFIGPATH=/etc/crypto-config/peerOrganizations/${org_name}/users/Admin@${org_name}/msp \
            -it cli bash -c  "sh /etc/scripts/installSC.sh $nombre_ultima_carpeta 1" 2>&1)
    echo $output
    # if output contains "but new definition must be sequence" then
    if [[ $output == *"but new definition must be sequence"* ]]; then
      echo "smart contract already installed, adding new version"
      # get number from substring
      number=$(echo $output | grep -o -E 'but new definition must be sequence [[:digit:]]+' | grep -o -E '[[:digit:]]+')
      docker exec -e CORE_PEER_ADDRESS=peer0.${org_name}:7051 \
            -e CORE_PEER_LOCALMSPID=${org_prefix}MSP \
            -e CORE_PEER_TLS_CERT_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/server.crt \
            -e CORE_PEER_TLS_KEY_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/server.key \
            -e CORE_PEER_TLS_ROOTCERT_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/ca.crt \
            -e CORE_PEER_MSPCONFIGPATH=/etc/crypto-config/peerOrganizations/${org_name}/users/Admin@${org_name}/msp \
            -it cli bash -c "/etc/scripts/installSC.sh $nombre_ultima_carpeta $number" 2>&1
      # but new definition must be sequence \d+
    fi
  done
  set +x
}
function instalar_ccc() {
cd $chaincode_project_path
  ruta=$chaincode_project_path
  nombre_ultima_carpeta=$(basename $(readlink -f "$ruta"))
  cp -r ../$nombre_ultima_carpeta $WORKSPACE/chaincode/$nombre_ultima_carpeta



  echo "running script to install smartcontract on peer container"
  #run script installSC on container named cli and redirect error to output
  declare -a peer_addresses=()
echo "running script to install smartcontract on peer container"
echo "number of organizations: ${#org_names[@]}"

  for org_name in "${org_names[@]}"
    do
  echo "processing organization: $org_name"
  org_prefix="${org_name%%.*}"
  org_prefix="${org_prefix^}"
  echo "org_prefix value: $org_prefix"

  echo "creating temporary variable for peer address and TLS root certificate file"
  peer_address="--peerAddresses peer0.${org_name}:7051 --tlsRootCertFiles /etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/ca.crt"
  echo "peer_address value: $peer_address"

  echo "adding temporary variable to peer addresses array"
  peer_addresses+=("$peer_address")
echo "peer_addresses array: ${peer_addresses[@]}"
docker container restart $(docker container ls -q -f name=cli)
sleep 10s
  echo "executing command to install smart contract on peer container"
  echo "CORE_PEER_ADDRESS: peer0.${org_name}:7051"
echo "CORE_PEER_LOCALMSPID: ${org_prefix}MSP"
echo "CORE_PEER_TLS_CERT_FILE: /etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/server.crt"
echo "CORE_PEER_TLS_KEY_FILE: /etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/server.key"
echo "CORE_PEER_TLS_ROOTCERT_FILE: /etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/ca.crt"
echo "CORE_PEER_MSPCONFIGPATH: /etc/crypto-config/peerOrganizations/${org_name}/users/Admin@${org_name}/msp"

  output=$(docker exec -e CORE_PEER_ADDRESS=peer0.${org_name}:7051 \
            -e CORE_PEER_LOCALMSPID=${org_prefix}MSP \
            -e CORE_PEER_TLS_CERT_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/server.crt \
            -e CORE_PEER_TLS_KEY_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/server.key \
            -e CORE_PEER_TLS_ROOTCERT_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/ca.crt \
            -e CORE_PEER_MSPCONFIGPATH=/etc/crypto-config/peerOrganizations/${org_name}/users/Admin@${org_name}/msp \
            -it cli bash -c  "/etc/scripts/installSC.sh $nombre_ultima_carpeta 1 peer0.${org_name} $CHANNEL_NAME" 2>&1)
echo "output of command to install smart contract on peer container:"
echo "$output"
number=1
  echo "checking if smart contract is already installed"
  if [[ $output == *"but new definition must be sequence"* ]]; then
    echo "smart contract already installed, adding new version"
    echo $output

# Guardar todas las ocurrencias del patrón en un array
mapfile -t numbers < <(echo "$output" | awk -F'but new definition must be sequence ' 'NF>1{print $2}' | grep -o -E '[[:digit:]]+')

# Acceder a la primera ocurrencia del array
number="${numbers[0]}"

    echo "NUMBER***"
    echo $number
    echo "NUMBER***"
    docker exec -e CORE_PEER_ADDRESS=peer0.${org_name}:7051 \
            -e CORE_PEER_LOCALMSPID=${org_prefix}MSP \
            -e CORE_PEER_TLS_CERT_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/server.crt \
            -e CORE_PEER_TLS_KEY_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/server.key \
            -e CORE_PEER_TLS_ROOTCERT_FILE=/etc/crypto-config/peerOrganizations/${org_name}/peers/peer0.${org_name}/tls/ca.crt \
            -e CORE_PEER_MSPCONFIGPATH=/etc/crypto-config/peerOrganizations/${org_name}/users/Admin@${org_name}/msp \
            -it cli bash -c  "/etc/scripts/installSC.sh $nombre_ultima_carpeta $number peer0.${org_name} $CHANNEL_NAME"  2>&1
  fi
done

echo "combining peer addresses into a single string"
PEERADDRESSES=$(echo "${peer_addresses[*]}")

org_prefix="${org_names[0]%%.*}"
org_prefix="${org_prefix^}"

echo "executing command to commit smart contract to the channel"
docker exec -e CORE_PEER_ADDRESS=peer0.${org_names[0]}:7051 \
            -e CORE_PEER_LOCALMSPID=${org_prefix}MSP \
            -e CORE_PEER_TLS_CERT_FILE=/etc/crypto-config/peerOrganizations/${org_names[0]}/peers/peer0.${org_names[0]}/tls/server.crt \
            -e CORE_PEER_TLS_KEY_FILE=/etc/crypto-config/peerOrganizations/${org_names[0]}/peers/peer0.${org_names[0]}/tls/server.key \
            -e CORE_PEER_TLS_ROOTCERT_FILE=/etc/crypto-config/peerOrganizations/${org_names[0]}/peers/peer0.${org_names[0]}/tls/ca.crt \
            -e CORE_PEER_MSPCONFIGPATH=/etc/crypto-config/peerOrganizations/${org_names[0]}/users/Admin@${org_names[0]}/msp \
            -it cli bash -c  "bash /etc/scripts/commit.sh $nombre_ultima_carpeta $number peer0.${org_names[0]} $CHANNEL_NAME \"$PEERADDRESSES\""

}

function change_crypto() {
cd $WORKSPACE
org1=${org_names[0]}
certificate=$( sed  -z -e 's|\n|\\\\n|g' ./crypto-config/peerOrganizations/$org1/users/User1@$org1/msp/signcerts/User1@$org1-cert.pem)
priv=$(sed  -z -e 's|\n|\\\\n|g' ./crypto-config/peerOrganizations/$org1/users/User1@$org1/msp/keystore/priv_sk)
s1="s|certificate:.*|certificate: \"$certificate\",|"
s2="s|privateKey:.*|privateKey: \"$priv\"|"

sed -ir "$s1" "${rest_project_path}/routes/common.js"
sed -ir "$s2" "${rest_project_path}/routes/common.js"
rm -rf $rest_project_path/crypto-config
cp -R crypto-config $rest_project_path
cat $rest_project_path/routes/common.js | grep "privateKey:"
echo should have "$priv"


}

function run_rest() {
cd $rest_project_path
cd scripts
bash generate-docker-image.sh
bash run-chain-REST.sh
}

check_bin_folder # comprobar prerequisitos
crear_scripts # crear carpeta scripts

# Ejecutar las funciones seleccionadas
if [[ "$execute_generate_cryptography" == true ]]; then
  rm -rf crypto-config
  generar_criptografia
fi

if [[ "$execute_generate_configuration" == true ]]; then
  rm -rf channel-artifacts
  generar_configuracion
fi
if [[ "$execute_generate_docker_compose" == true ]]; then
  rm -rf docker-composes
  generar_cli_compose # generate cli compose
  generar_docker_compose
  generate_orderer_docker_compose
  generate_couchdb_docker_compose
fi

if [[ "$execute_lanzar_hyperledger" == true ]]; then
  lanzar_hyperledger
fi

if [[ "$execute_instalar_cc" == true ]]; then
  rm -rf chaincode/*
  instalar_cc
fi

if [[ "$execute_instalar_ccc" == true ]]; then
  rm -rf chaincode/*
  instalar_ccc
fi

if [[ "$execute_lanzar_rest" == true ]]; then
  change_crypto
  run_rest
fi

