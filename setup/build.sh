#!/bin/bash

#
# build script
# @author Tobias Weber (orcid: 0000-0002-7230-1932)
# @date july-2022
# @license see 'LICENSE.EUPL' file
#

#export CXX=g++
#export CXX=clang++-13


echo -e "\nPreparing build directory..."

rm -rfv build
mkdir -v build
if [ ! -d build ]; then
	echo "Build directory does not exist."
	exit -1
fi
cd build



echo -e "\nPrebuilding..."

if ! cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-march=native" ..
then
	echo "Prebuilding failed."
	exit -1
fi



num_procs=$(($(nproc) / 2))
echo -e "\nBuilding using ${num_procs} processes..."

if ! cmake --build . --parallel ${num_procs}
then
	echo "Building failed."
	exit -1
fi



echo -e "\nBuilding examples..."

if ! ./expr_prec_create; then
	echo "Building expression example failed."
	exit -1
fi

if ! ./script_create; then
	echo "Building scripting example failed."
	exit -1
fi

touch ../src/examples/expr_prec.cpp
touch ../src/examples/script.cpp

if ! cmake --build . --parallel ${num_procs}
then
	echo "Building of examples failed."
	exit -1
fi
