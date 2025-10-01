#!/bin/bash

toolchains=$(readlink -f "../../toolchains")
pythonbin="/bin/python3"
miracllib=$(readlink -f "ta/lib/p-abc-main/lib/pfecCwrapper/lib/Miracl_Core")
aceunitlib=$(readlink -f "host/lib/aceunit")
dpabc_psmslib=$(readlink -f "ta/lib/p-abc-main")
applicationdir=$(readlink -f $(pwd))

export AARCH64_COMPILER="$toolchains/aarch64/bin/aarch64-none-linux-gnu-gcc"
export AARCH32_COMPILER="$toolchains/aarch32/bin/arm-none-linux-gnueabihf-gcc"

if [ $# -ne 1 ]; then
	echo "use: config.sh [32 | 64]"
	exit 1
fi

if [ "$1" == "-h" ]; then
	echo "use: config.sh [32 | 64]"
	exit 0
fi

if [ ! -f "$pythonbin" ]; then
	echo "Could not find a python3 instalation (try 'sudo apt get python3')"
	exit 1
fi

echo Entering directory: $aceunitlib
cd $aceunitlib
make clean
make

echo Entering directory: $applicationdir
cd $applicationdir

if [ "$1" = "32" ]; then
	pwd
	sed -i 's/pfec_Miracl_Bls381_64/pfec_Miracl_Bls381_32/g' "CMakeLists.txt"
	sed -i 's/pfec_Miracl_Bls381_64/pfec_Miracl_Bls381_32/g' ta/sub.mk
	echo Entering directory: $miracllib
	cd $miracllib
	$pythonbin config32.py
	export ENV_WRAPPER_INSTANTIATION="pfec_Miracl_Bls381_32"
	echo "Entering directory: $aceunitlib/lib"
	cd "$aceunitlib/lib"
	make clean
	make libs CROSS_COMPILE="$toolchains/aarch32/bin/arm-none-linux-gnueabihf-"

elif [ "$1" = "64" ]; then
	sed -i 's/pfec_Miracl_Bls381_32/pfec_Miracl_Bls381_64/g' "CMakeLists.txt"
	sed -i 's/pfec_Miracl_Bls381_32/pfec_Miracl_Bls381_64/g' ta/sub.mk
	echo Entering directory: $miracllib
	cd $miracllib
    $pythonbin config64.py
	export ENV_WRAPPER_INSTANTIATION="pfec_Miracl_Bls381_64"
	echo "Entering directory: $aceunitlib/lib"
	cd "$aceunitlib/lib"
	make clean
	make libs CROSS_COMPILE="$toolchains/aarch64/bin/aarch64-none-linux-gnu-"

else
    echo "use: config.sh [32 | 64]"
    exit 1
fi
 
