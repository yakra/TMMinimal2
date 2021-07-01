#!/usr/bin/env bash
pass=0
echo -en "0"
verdict="Total run time"
while [ "$verdict" = "Total run time" ]; do
  verdict=`./s1.sh $@ | tail -n 1 | cut -f1 -d:`
  pass=`expr $pass + 1`
  echo -en "\r$pass"
done
echo
