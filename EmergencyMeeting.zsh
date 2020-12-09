#!/bin/zsh

for n in `ipcs -b -m | egrep ^m | awk '{ print $2; }'`; do ipcrm -m $n; done

ipcs
