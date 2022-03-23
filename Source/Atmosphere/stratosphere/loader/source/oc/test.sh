#!/bin/bash

fw_dir="/Volumes/RAM/NX-14.0.0/"
tmp_dir="/Volumes/RAM/"
oc_test_dir="$HOME/Source/Switch-OC-Suite/Source/Atmosphere/stratosphere/loader/source/oc"
prodkeys="$HOME/.switch/prod.keys"
hactool_exe="$HOME/Source/hactool/hactool"
nx2elf_exe="$HOME/Source/nx2elf/nx2elf"

echo -e "\nExtracting nca..."
out_pcv="${tmp_dir}pcv/"
out_ptm="${tmp_dir}ptm/"
cd $fw_dir
for nca_file in ./*.nca
do
    file_size=`wc -c "$nca_file" | awk '{print $1}'`
    if [[ "$nca_file" == *".cnmt."*  || file_size -lt 16384 ]]; then
        continue
    fi

    titleid=`$hactool_exe -k $prodkeys --disablekeywarns -t nca $nca_file | grep 'Title ID'`

    if [[ $titleid == *"010000000000001a"* ]]; then
        echo "$nca_file (pcv) -> $out_pcv"
        $hactool_exe -k $prodkeys --disablekeywarns -t nca $nca_file --exefsdir "$out_pcv" 1> /dev/null
    fi

    if [[ $titleid == *"0100000000000010"* ]]; then
        echo "$nca_file (ptm) -> $out_ptm"
        $hactool_exe -k $prodkeys --disablekeywarns -t nca $nca_file --exefsdir "$out_ptm" 1> /dev/null
    fi
done

echo -e "\nConverting nca to elf..."
$nx2elf_exe "${out_pcv}main" 1> /dev/null
$nx2elf_exe "${out_ptm}main" 1> /dev/null

echo -e "\nBuilding and testing..."
cd $oc_test_dir
make test 1> /dev/null
./test pcv "${out_pcv}main.elf"
./test ptm "${out_ptm}main.elf"
make clean 1> /dev/null

rm -fr $out_pcv
rm -fr $out_ptm
