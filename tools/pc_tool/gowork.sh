#!/bin/bash
case $1 in
    "-cl")
        GOTODEST='code/local/'
        ;;
    "-cb")
        GOTODEST='code/broad/'
        ;;
    "-t")
        GOTODEST='tools/'
        ;;
    *)
        GOTODEST=''
        ;;
esac


DESTPATH="/home/dinggy/Desktop/Project/$GOTODEST"
cd $DESTPATH
if [ $? = 0 ]; then
    echo "[+] we are in `echo $DESTPATH | egrep -o '[^/]*/$'`"
else
    echo "[-] $DESTPATH not exited!!"
fi

