#!/bin/bash
a=$2
b=$3 # url peer
c=$4 # channel
ORDERER_CA=/etc/crypto-config/ordererOrganizations/example.com/orderers/orderer.example.com/msp/tlscacerts/tlsca.example.com-cert.pem
rutachaincode=/etc/chaincode/
PEERADDRESSES=$5


fn () {
    set -x
    peer lifecycle chaincode checkcommitreadiness --channelID $c --name $1 --version $a --sequence $a --tls --cafile $ORDERE_CA --output json  
    echo
    peer lifecycle chaincode commit -o orderer.example.com:7050 --channelID $c --name $1 --version $a --sequence $a --tls --cafile $ORDERER_CA $PEERADDRESSES
    echo
    peer lifecycle chaincode querycommitted --channelID $c --name $1 --cafile $ORDERER_CA 
    echo
}

fn $1
