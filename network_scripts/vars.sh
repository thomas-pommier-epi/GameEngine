#!/bin/bash

TAP_NAME="tap0"
NAMESPACE="rtype"
IPV4_MID_16="42"
IPV4_LAST_16="$IPV4_MID_16.100"

FULL_IP="192.168.$IPV4_LAST_16"
GATEWAY="192.168.$IPV4_MID_16.1"

VETH_OUT="veth0"
VETH_IN="veth1"