#!/bin/bash
a=$2
b=$3 # url peer
c=$4 # channel
ORDERER_CA=/etc/crypto-config/ordererOrganizations/example.com/orderers/orderer.example.com/msp/tlscacerts/tlsca.example.com-cert.pem
rutachaincode=/etc/chaincode/
#PEERADDRESSES="--peerAddresses peer0.org1.example.com:7051 --tlsRootCertFiles  ${rutapeer}/crypto/peerOrganizations/org1.example.com/peers/peer0.org1.example.com/tls/ca.crt"


fn () {
    set -x

    peer lifecycle chaincode package $1package$a.tar.gz --path $rutachaincode/$1/  --lang java --label $1$a
    echo
    peer lifecycle chaincode install $1package$a.tar.gz
    echo
    peer lifecycle chaincode queryinstalled
    PACKAGEID=$( peer lifecycle chaincode queryinstalled | grep "$1$a" | cut -d" " -f3 | cut -f1 -d",")
    echo
    peer lifecycle chaincode approveformyorg -o orderer.example.com:7050 --channelID $c --name $1 --version $a --package-id $PACKAGEID --sequence $a --tls --cafile $ORDERER_CA
    echo

    peer lifecycle chaincode checkcommitreadiness --channelID $c --name $1 --version $a --sequence $a --tls --cafile $ORDERE_CA --output json  
    echo
    peer lifecycle chaincode commit -o orderer.example.com:7050 --channelID $c --name $1 --version $a --sequence $a --tls --cafile $ORDERER_CA 
    echo
    peer lifecycle chaincode querycommitted --channelID $c --name $1 --cafile $ORDERER_CA 
    echo
}





fn $1

echo 'USO DEL SCRIPT: ./installOne.sh [nombre_de_la_carpeta_que_contiene_el_chaincode] [version_del_chaincode]'
#peer chaincode invoke -o 10.208.211.47:7050 --tls --cafile $ORDERER_CA -C mychannel -n name  -c '{"function":"publicarconfig","Args":[]}'
