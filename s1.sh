#!/usr/bin/env bash
tmdir=$HOME/TravelMapping
execs=()
e=''
v=''
t=''
#t="-t "`nproc 2>/dev/null` #linux
#if [ "$t" == '-t ' ]; then
#  t="-t "`sysctl hw.ncpu | cut -f2 -d' '` #BSD
#  if [ "$t" == '-t ' ]; then
#    t=''
#  fi
#fi

# process commandline args
while  [ $# -gt 0 ]; do
  case $1 in
    -e) e='-e';;
    -v) v='-v';;
    -t) t="-t $2"; shift;;
    -y) tmdir=$HOME/ytm;;
    *)  execs+=("$1");;
  esac
  shift
done

# if no execs specified, default to vanilla siteupdate
if [ `echo "${execs[@]}" | wc -w` == 0 ]; then
  execs=('')
fi

# clear output dirs & execute each
for X in "${execs[@]}"; do
  rm -f  ./$X/TravelMapping.sql
  rm -fr ./$X/logs;		mkdir -p ./$X/logs/users
  rm -fr ./$X/nmp_merged;	mkdir -p ./$X/nmp_merged
  rm -fr ./$X/stats;		mkdir -p ./$X/stats
  rm -fr ./$X/graphs;		mkdir -p ./$X/graphs
  echo ./siteupdate$X $e $v $t -l ./$X/logs -n ./$X/nmp_merged -c ./$X/stats -g ./$X/graphs -u $tmdir/UserData/list_files -w $tmdir/HighwayData '|' tee ./$X/logs/siteupdate.log
       ./siteupdate$X $e $v $t -l ./$X/logs -n ./$X/nmp_merged -c ./$X/stats -g ./$X/graphs -u $tmdir/UserData/list_files -w $tmdir/HighwayData  |  tee ./$X/logs/siteupdate.log
done
