#!/bin/bash
dir=`mktemp -d`
tests=trace*.txt
timer="timeout -s KILL 20s"
passed=0
total=17
passed17=0

{
  if [ "$1" != "" ]; then
    first=$1
    last=$1
    if [ "$2" != "" ]; then
      last=$2
    fi  
    total=$(($last-$first+1))
    tests=`seq -f "trace%02g.txt" $first $last`
  fi
  for file in $tests; do
      printf "Testing $file  "

      # Run tshref also with timeout to have the same ps output 
      $timer ./sdriver.pl -t "$file" -s ./tshref -a "-p" > $dir/"$file".ref

      # 137 means KILLED
      if [ $? -eq 137 ]; then
        echo "Reference shell timeout! You should adjust timer to higher value."
        echo "Output can be found at $dir"
        exit 1
      fi
      # Run tsh to avoid hanging, SIGKILL to avoid signal blocking
      $timer ./sdriver.pl -t "$file" -s ./tsh -a "-p" > $dir/$file

      # 137 means KILLED
      if [ $? -eq 137 ]; then
        printf "✘ (Timeout!)"
        delete=false
        echo "TIMEOUT!!!" >> $dir/$file
        continue
      fi

      # Replace [jid] (pidnum) ... with [jid] () ...
      sed -i -E 's/\([0-9]+\)/(?)/' $dir/"$file".ref
      sed -i -E 's/\([0-9]+\)/(?)/' $dir/"$file"

      # Replace pid pts/ with ? pts/
      sed -i -E 's/[0-9]+ pts/? pts/' $dir/"$file".ref
      sed -i -E 's/[0-9]+ pts/? pts/' $dir/"$file"

      # Replace pid tty with ? tty
      sed -i -E 's/[0-9]+ tty/? tty/' $dir/"$file".ref
      sed -i -E 's/[0-9]+ tty/? tty/' $dir/"$file"

      # Replace tshref with tsh
      sed -i -E 's/tshref/tsh/' $dir/"$file".ref
      sed -i -E 's/tshref/tsh/' $dir/"$file"

      if [ "`diff $dir/$file $dir/$file.ref`" != "" ]; then
        printf "✘\n"
        diff -y --suppress-common-lines $dir/$file $dir/$file.ref
        delete=false
      else
        if [ $file = "trace17.txt" ]; then
          passed17=1
        fi
        passed=$(($passed+1))
        printf "✓\n"
      fi  
  done

  printf "Passed: %d/%d - Score: %.2lf\n" $passed $total $(bc -l <<< $(($passed-$passed17))/16*90+10*$passed17)

# Delete the directory if no errors
  if $delete; then
      rm -r $dir
  else
      echo "Output can be found at $dir"
  fi
} 2> /dev/null
