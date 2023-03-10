#!/bin/bash

script_folder=/home/victor/xlang/test/unittest
xlang_exec=/home/victor/xlang/out/xlang
cd $script_folder

if [[ ! -d $script_folder/rpt ]]
then
   mkdir $script_folder/rpt
fi

cd $script_folder/rpt

if [[ ! -f "comparison.rpt" ]]
then
   touch comparison.rpt
fi

#for file in `ls $folder | egrep '*.x'`
for script_file in $script_folder/*.x;
do  
    cd $script_folder/rpt
    
    #echo $script_file
    
    #basename $script_file 
    #script_base =$(basename -- $script_file)  
    #script_base = "${script_file##*/}"
    #echo $script_base

    #script_base=${script_base%%.*}
    #echo $script_base

    script_base=$(basename "$script_file" | cut -d. -f1)
    #echo $script_base
    python_rpt=$script_base_p.rpt 
    xlang_rpt=$script_base_x.rpt 
    : '
    if [ -f $python_rpt ];
    then
      echo "python result exists"
      rm -i $python_rpt
    fi

    if [ -f $xlang_rpt ];
    then
      echo "xlang result exists"
      rm -i $python_rpt
    fi
    '
    python $script_file >> $python_rpt
    xlang_exec $script_file >> $xlang_rpt

    diff $python_rpt $xlang_rpt >> comparison.rpt

done
