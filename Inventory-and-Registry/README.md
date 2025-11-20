# easy-setup-Hyperledger Fabric Deployment




## Inventorying and registring
 Para lanzar todo los componentes necesarios (REST, fabric y smartcontract) basta con lanzar el script:
 ```bash
 bash run-inventorying.sh
 ```

## Resumen

El script `setup.sh` es capaz de ejecutar diferentes pasos para el despliegue de un escenario funcional de Hyperledger Fabric.

1. Generar criptografía.
1. Generar ficheros de configuración (bloques genesis).
1. Generar ficheros docker-compose que contienen los diferentes servicios/roles de hyperledger Fabric.
1. Desplegar los contenedores asociados a los docker-compose.
1. Instalar el smartcontract en el despliegue.
1. Generar imagen del servicio REST y lanzar el contenedor asociado.

### Comando help -h

Para obtener un mensaje de ayuda que indique como usar el comando tienes la opción -h:

```
./setup.sh -h
mkdir: cannot create directory ‘workspace’: File exists
mkdir: cannot create directory ‘chaincode’: File exists
clave en procesamiento: -h
Este script genera los archivos de configuración y los Genesis Blocks necesarios para crear una red de Hyperledger Fabric completamente funcional.

Uso: ./setup.sh [ORGANIZATION1] [ORGANIZATION2] ... [ORGANIZATIONN] [-acdlh] [-i RUTA_AL_PROYECTO_JAVA] [-x RUTA_AL_PROYECTO_JAVA] -r  [RUTA_A_LA_CARPETA_NODEJS]

Opciones:
  -h  Mostrar ayuda
  -a  Generar criptografía, configuración, Docker Compose y lanzar Hyperledger
  -b  Generar criptografía
  -c  Generar configuración
  -d  Generar Docker Compose
  -l  Lanzar Hyperledger
  -i  [RUTA_AL_PROYECTO_JAVA] Compilar e instalar Chaincode/smartcontract
  -x  [RUTA_AL_PROYECTO_JAVA] Instalar Chaincode/smartcontract
  -r  [RUTA_A_LA_CARPETA_NODEJS] Lanzar REST

Ejemplo: ./setup.sh org1.example.com org2.example.com -acdl -i /ruta/al/proyecto/java


Organizaciones:
- org1.example.com
- org2.example.com

```

`Nota: Actualmente -i no funciona, encargado de compilar automaticamente el proyecto java del smartcontract e instalarlo. Hay que compilar a mano y despues ejecutar el comando con -x`

## Instalar un smartcontract

Suponemos que tu smartcontract (proyecto java gradle) está ubicado en la ruta `~/SC/`

1. Compilar el smartcontract. Para compilar el smartcontract hay que ejecutar el siguiente script:

```
bash gradlew installDist
```

Este comando genera la carpeta `build/install/nombredelproyecto` que se utilizará en el comando setup.sh -x.

2. Instalar el SmartContract en el despliegue de hyperledger. Para ello vamos a ejecutar el script setup con el parámetro -x indicando la ruta anterior:

```
./setup.sh  -x ~/SC/build/install/SC/
```

Automaticamente el script se conectara con todos los nodos/peers de las organizaciones y comenzará el proceso de endorsement o aprovación de un chaincode dentro del canal. Al terminar el script, la respuesta docker ps debe contener tantos contenedores dev-NOMBRE_DEL_PEER-NOMBRE_DEL_SC-HASH como numero de peers hayan desplegados. Ejemplo:

```
docker ps
CONTAINER ID   IMAGE                                                                                                                                                                COMMAND                  CREATED        STATUS        PORTS                                                                                                                             NAMES
3eb30b7c39cc   dev-peer0.org2.example.com-flsc2-1de0850f2efed7923c8aceccfd3388dd8ffbe7cbb3534e42c59f38c079c06e89-6f43be23383246c1d2e17042fdc4872009025b0edf238b555f195bbfaa0ca3e7   "/root/chaincode-jav…"   25 hours ago   Up 25 hours                                                                                                                                     dev-peer0.org2.example.com-FLSC2-1de0850f2efed7923c8aceccfd3388dd8ffbe7cbb3534e42c59f38c079c06e89
c78ef41a6dd2   dev-peer0.org1.example.com-flsc2-1de0850f2efed7923c8aceccfd3388dd8ffbe7cbb3534e42c59f38c079c06e89-2c7133f7ac8726321c2f79b6d0117194ea2ff5bbca834319021ad25b847a4e40   "/root/chaincode-jav…"   25 hours ago   Up 25 hours

```

## Despliegue del REST

El servicio REST asociado a este proyecto, necesita la carpeta de la criptografía y modificar un fichero config con el wallet asociado a una organización para poder comunicarse (invocar smartcontracts) con el blockchain. Para ello, el script automaticamente realiza las tareas necesarias, genera/regenera la imagen de docker y lanza el servicio REST HTTP.

</p>
Asumiendo que el proyecto nodejs está en la siguiente ruta: `~/ChainREST`:

```
./setup.sh  -r ~/ChainREST/
```

Al finalizar el script se ejecutará un nuevo contenedor de docker con el servicio escuchando en el puerto 3000:

```
docker ps
CONTAINER ID   IMAGE                                                                                                                                                                COMMAND                  CREATED        STATUS        PORTS                                                                                                                             NAMES
fd1560559fd4   chainapi/node-web-app                                                                                                                                                "docker-entrypoint.s…"   24 hours ago   Up 24 hours   0.0.0.0:3000->3000/tcp, :::3000->3000/tcp
```

### Probar el REST

El REST se despliega en el puerto 3000. Las rutas/endpoints que a las que se pueden hacer peticiones HTTP están definidas en el fichero routes/chain.js. Aquí hay algunos ejemplos:

- POST: subir un JSON al LEDGER

```
curl -X POST -H "Content-Type:application/json" --location '155.54.95.237:3000/chain/json' -d '{"n":"mapping","t":1680537951,"v":10 , "serial": "IPXXXXXXX", "created": "1680537951", "id":"id1"}'
```
- GET: solicitar una o varias entradas del ledger siempre y cuando coincidan con un elemento del JSON `id`.

```
curl --location '155.54.95.237:3000/chain/json?id=id1'

```

# Anexo:

## Generación de los ficheros necesarios para el despliegue de Hyperledger Fabric

Para desplegar de 0 un escenario de Fabric, hay que borrar la carpeta `workspace`, para que el script regenere todos los ficheros necesarios.

</p>
El script espera una lista de parámetros que se corresponden con el nombre de las organizaciones en formato de dominio/url:

```
setup.sh "org1.example.com" "org2.example.com" "org3.example.com" "orgN.example.com"
```

Siempre siguiendo el formato `"NOMBRE.DOMINIO.EXT"`. Al anterior comando hay que añadirle los parámetros de generación de ficheros, que están resumidos en el parámetro -a. por lo que para hacer un despliegue de 2 organizaciones el comando sería:

```
setup.sh "org1.example.com" "org2.example.com" -a
##### o #####
setup.sh "org1.example.com" "org2.example.com" -bcd # equivalente
```

Una vez terminado el script tendrías:
2 organizaciones (cada una con un peer/nodo desplegado). Siempre se utiliza un solo orderer con el protocolo RAFT :S TODO permitir configuración de número de orderers en el RAFT.

</p>
Cada peer de cada organización estaría unido a un canal, en el que todas las organizaciones son participantes.

## Como Modificar el SmartContract:

Suponiendo que tu Smartcontract tiene su archivo main en: `~/FLSC/src/main/java/contracts/FLSaver.java` siendo FLSC el nombre de tu proyecto gradle/nombre del smartcontract.
El fichero JAVA necesita varias anotacionas para definir el smart contract: Hay una explicación por encima de las anotaciones que necesita en el siguiente link: [documentación](https://github.com/agustin-marin/Precept-Blockchain/tree/divided-roles/setup/hyperledger/chaincode/PreceptSC#code)

</p>
</p>

En Hyperledger Fabric, los contratos inteligentes se escriben como chaincodes, que son esencialmente programas que se ejecutan en la red de blockchain. Cuando un contrato inteligente recibe una transacción, se ejecuta en un entorno de sandbox aislado llamado "chaincode container". El código de la cadena de bloques se comunica con el código de la aplicación a través de un objeto "stub", que es proporcionado por la plataforma de Hyperledger Fabric.

</p>
En Java, el objeto stub se proporciona a través del objeto de contexto ("Context"), que se utiliza para interactuar con la red de Hyperledger Fabric y el entorno de ejecución de chaincode. Para utilizar el stub desde el contexto, primero debes obtener una instancia del contexto utilizando el constructor de la clase "ChaincodeStub" y el método "getStub" del contexto:

```
    public String pushData(final Context ctx, final String key, final String data) {
        ChaincodeStub stub = ctx.getStub();
```

Una vez que tengas una instancia del contexto, puedes utilizar el objeto "stub" para interactuar con la red de Hyperledger Fabric. El stub proporciona una variedad de métodos para acceder y modificar el estado de la cadena de bloques, incluyendo:

- "getStringState" y "putStringState" para acceder y actualizar el estado de la cadena de bloques como cadenas de caracteres.
- "getState" y "putState" para acceder y actualizar el estado de la cadena de bloques como bytes.
- "getArgs" para obtener los argumentos de la transacción que se está procesando.
- "getTxId" para obtener el identificador único de la transacción que se está procesando.

```
        stub.putStringState(key + l, data);
```

En Hyperledger Fabric se suele utilizar couchdb para hacer consultas mas complejas en el código utilizando [consultas enriquecidas de couchdb](https://docs.couchdb.org/en/stable/api/database/find.html). Se utilizan:

-     stub.getQueryResult(queryString); Siendo queryString el JSON selector referente a las consultas de couchdb
-     stub.getQueryResultWithPagination(queryString, pageSize, bookmark); Lo mismo, pero usando paginación, para mejorar enormemente la eficiencia de las consultas #NECESARIO 100%
  Nota: `En caso de no usar paginación, las consultas pueden escalar a horas para obtener unos pocos resultados. Es necesario al 100% usar paginación.`

## Como Modificar el REST:

El proyecto ChainREST está compuesto esencialmente por 4 o 5 ficheros:

- app.js - fichero main
- routes/chain.js - define endpoints/paths y genera hilos para cada path definido en el rest.
- routes/worker-get.js - código del hilo que ejecuta una petición de lectura al blockchain
- routes/worker-put.js - código del hiilo que ejecuta una petición de escritura al blockchain.

Dentro de los worker-XXX.js el método que realiza la llamada al smartcontract del blockchain es:

```
  const queryResult = await queryChaincode(contract, "pullData", [id]);

  await queryChaincode(contract, "pushData", ["key1", JSON.stringify(body)]);


```

Donde contract es la variable que mantiene la conexión con el blockchain, seguido de 2 parámetros:

1. el método que se quiere invocar
2. un array con los parámetros del método. Es esencial que el número y tipo de los parámetros dentro del array coincida con el del método definido en el proyecto java con @Transaction para que la librería sea capaz de ejecutarlo.
