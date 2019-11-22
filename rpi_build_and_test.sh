#!/bin/bash
set -e
bash build_and_test.sh "$@"

if [ "${RPI_ADDRESS}" == "" ]; then
  echo "error: RPI_ADDRESS not defined"
  exit 1
fi

DEST_DIR=/home/pi/remote_builds/cleo

echo "synchronising..."
ssh ${RPI_ADDRESS} "mkdir -p ${DEST_DIR}"
rsync -av --delete --exclude xcode --exclude Debug --exclude Release --exclude FastDebug --exclude local_inst * ${RPI_ADDRESS}:${DEST_DIR}/

echo "building..."
ssh ${RPI_ADDRESS} "cd ${DEST_DIR}; ./build_and_test.sh $@"
