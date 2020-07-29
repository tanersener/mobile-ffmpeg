#!/bin/sh

set -eu
. "${0%/*}/util.sh"

valgrind_test 264x130 10 --bitrate=500000 -p0 -r1 --owf=1 --threads=2 --rd=0 --no-rdoq --no-deblock --no-sao --no-signhide --subme=0 --pu-depth-inter=1-3 --pu-depth-intra=2-3
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 512x512 30 --bitrate=100000 -p0 -r1 --owf=1 --threads=2 --rd=0 --no-rdoq --no-deblock --no-sao --no-signhide --subme=2 --pu-depth-inter=1-3 --pu-depth-intra=2-3 --bipred; fi
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 264x130 10 --bitrate=500000 -p0 -r1 --owf=1 --threads=2 --rd=0 --no-rdoq --no-deblock --no-sao --no-signhide --subme=0 --pu-depth-inter=1-3 --pu-depth-intra=2-3 --bipred --gop 8 --rc-algorithm oba --no-intra-bits --no-clip-neighbour; fi
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 264x130 10 --bitrate=500000 -p0 -r1 --owf=1 --threads=2 --rd=0 --no-rdoq --no-deblock --no-sao --no-signhide --subme=0 --pu-depth-inter=1-3 --pu-depth-intra=2-3 --bipred --gop 8 --rc-algorithm oba --intra-bits --clip-neighbour; fi
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 264x130 10 --bitrate=500000 -p0 -r1 --owf=1 --threads=2 --rd=0 --no-rdoq --no-deblock --no-sao --no-signhide --subme=0 --pu-depth-inter=1-3 --pu-depth-intra=2-3 --bipred --gop lp-g8d4t1 --rc-algorithm oba --no-intra-bits --no-clip-neighbour; fi
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 264x130 10 --bitrate=500000 -p0 -r1 --owf=1 --threads=2 --rd=0 --no-rdoq --no-deblock --no-sao --no-signhide --subme=0 --pu-depth-inter=1-3 --pu-depth-intra=2-3 --bipred --gop lp-g8d4t1 --rc-algorithm oba --intra-bits --clip-neighbour; fi

