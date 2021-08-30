#!/usr/bin/env bash
# batch file to build all maya plugins at once
BASEDIR=$(dirname "$0")
echo "$BASEDIR"

cd "$BASEDIR"

python Generate.py
echo "build Generate.inc"


function pause(){
   read -p "$*"
}
 
pause 'Press [Enter] key to continue...'

array=( 2017 2018 2019 2020 2022)


mkdir -p "$BASEDIR/build"
cd "$BASEDIR/build"
pause 'check if not everything is removed'

rm -r *


for i in "${array[@]}"
do
	cmake -g "Unix makefiles" -DMAYA_VERSION=$i ../
	cmake --build . --config Release --target install
	rm -r *
done


pause  'all plugins built press [Enter] to exit'
