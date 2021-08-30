#!/usr/bin/env bash
# batch file to build all maya plugins at once
python Generate.py
echo "build resources.inc"

function pause(){
   read -p "$*"
}
 
pause 'Press [Enter] key to continue...'

array=( 2017 2018 2019 2020 2022)

mkdir -p "../build"
cd "../build"
pause 'check if not everything is removed'
rm -r *


for i in "${array[@]}"
do
	cmake3 -g "Unix makefiles" -DMAYA_VERSION=$i ../
	cmake3 --build . --config Release --target install
	rm -r *
done

pause  'all plugins built press [Enter] to exit'
