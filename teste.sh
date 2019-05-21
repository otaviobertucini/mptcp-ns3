#!/bin/bash

N=30
Z=1.96
dev=0.6
err=`echo "" | gawk -v dev=$dev -v z=$Z -v n=$N '{printf "%f", dev*1.96*sqrt(n)}'`

echo $err
