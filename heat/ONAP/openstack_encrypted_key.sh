#!/bin/bash

usage () {
  echo "Usage:"
  echo "   ./$(basename $0) your_openstack_password"
  exit 1
}

if [ "$#" -ne 1 ]; then
  echo "Wrong number of input parameters"
  usage
fi

SO_ENCRYPTION_KEY=aa3871669d893c7fb8abbcda31b88b4f
OPENSTACK_API_KEY=$1

echo -n "$OPENSTACK_API_KEY" | openssl aes-128-ecb -e -K $SO_ENCRYPTION_KEY -nosalt | xxd -c 256 -p
