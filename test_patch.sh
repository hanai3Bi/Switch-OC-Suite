#!/bin/bash

fw_dir="/Volumes/RAM/NX-14.0.0/"
tmp_dir="/Volumes/RAM/"
repack_out_dir="/Volumes/RAM/out/"
oc_test_dir="$HOME/Source/Switch-OC-Suite/Source/Atmosphere/stratosphere/loader/source/oc"
prodkeys="$HOME/.switch/prod.keys"
hactool_exe="$HOME/Source/hactool/hactool"
nx2elf_exe="$HOME/Source/nx2elf/nx2elf"
elf2nso_exe="$HOME/Source/switch-tools/elf2nso"
hacpack_exe="$HOME/Source/hacPack/hacpack"

should_remove_tmp="Y"
should_save_repack="Y"
option_Mariko_Erista="M"

echo -e "\nExtracting nca..."
out_pcv="${tmp_dir}pcv/"
out_ptm="${tmp_dir}ptm/"
mkdir -p "${out_pcv}"
mkdir -p "${out_ptm}"
cd $fw_dir

pcv_nca_name=""
ptm_nca_name=""
for file_00 in ./*.nca/00
do
    if [ -e "${file_00}" ]; then
        echo "Processing \"*.nca/00\" files..."
        find "${fw_dir}" -type f -name "00" -exec sh -c 'DIR=$(dirname "{}"); FW_DIR=$(dirname "${DIR}"); mv "{}" "${FW_DIR}/00"; rm -r "${DIR}"; mv "${FW_DIR}/00" "${DIR}"' \;
    fi
    break
done

for nca_file in ./*.nca
do
    file_size=`wc -c "$nca_file" | awk '{print $1}'`
    if [[ "$nca_file" == *".cnmt."*  || file_size -lt 16384 ]]; then
        continue
    fi

    titleid=`$hactool_exe -k $prodkeys --disablekeywarns -t nca $nca_file | grep 'Title ID'`

    if [[ $titleid == *"010000000000001a"* ]]; then
        pcv_nca_name="$(basename $nca_file)"
        echo "$pcv_nca_name (pcv) -> $out_pcv"
        $hactool_exe -k $prodkeys --disablekeywarns -t nca $nca_file --exefsdir "$out_pcv" 1> /dev/null
    fi

    if [[ $titleid == *"0100000000000010"* ]]; then
        ptm_nca_name="$(basename $nca_file)"
        echo "$ptm_nca_name (ptm) -> $out_ptm"
        $hactool_exe -k $prodkeys --disablekeywarns -t nca $nca_file --exefsdir "$out_ptm" 1> /dev/null
    fi
done

echo -e "\nConverting nca to elf..."
$nx2elf_exe "${out_pcv}main" 1> /dev/null
$nx2elf_exe "${out_ptm}main" 1> /dev/null

echo -e "\nBuilding..."
cd $oc_test_dir
make test 1> /dev/null

echo -e "\nPatching..."

[ -z "$should_save_repack" ] && read -p "Save and repack to nca (y/N)? " should_save_repack
SAVE_OPT=" "
case $should_save_repack in
    Y|y ) SAVE_OPT="-s ";;
esac

./test pcv $SAVE_OPT "${out_pcv}main.elf"
./test ptm $SAVE_OPT "${out_ptm}main.elf"
make clean 1> /dev/null

if [ ! -z $SAVE_OPT ]; then
    case $should_save_repack in
        Y|y )
            patched_ext=".mariko"
            [ -z "$option_Mariko_Erista" ] && read -p "[M]ariko (Default) | [E]rista ? " option_Mariko_Erista
            case $option_Mariko_Erista in
                E|e ) patched_ext=".erista";;
            esac
            mkdir -p "${repack_out_dir}"
            cd "${tmp_dir}"
            echo -e "\nRepacking pcv to ${repack_out_dir}${pcv_nca_name}..."
            TMP="${out_pcv}temp/"
            mkdir -p "${TMP}"
            $elf2nso_exe "${out_pcv}main.elf${patched_ext}" "${TMP}main" 1> /dev/null
            cp "${out_pcv}main.npdm" "${TMP}main.npdm"
            $hacpack_exe -k $prodkeys -o "${TMP}nca" --type nca --ncatype program --titleid 010000000000001A --exefsdir "${TMP}" 1> /dev/null
            find "${TMP}nca" -name "*.nca" -exec mv {} "${repack_out_dir}${pcv_nca_name}" \;

            if [[ $patched_ext == ".mariko" ]]; then
                echo -e "\nRepacking ptm (Mariko Only) to ${repack_out_dir}${ptm_nca_name}..."
                TMP="${out_ptm}temp/"
                mkdir -p "${TMP}"
                $elf2nso_exe "${out_ptm}main.elf${patched_ext}" "${TMP}main" 1> /dev/null
                cp "${out_ptm}main.npdm" "${TMP}main.npdm"
                $hacpack_exe -k $prodkeys -o "${TMP}nca" --type nca --ncatype program --titleid 0100000000000010 --exefsdir "${TMP}" 1> /dev/null
                find "${TMP}nca" -name "*.nca" -exec mv {} "${repack_out_dir}${ptm_nca_name}" \;
            fi
        ;;
    esac
fi

[ -z "$should_remove_tmp" ] && read -p "Remove temp files (Y/n)? " should_remove_tmp
case $should_remove_tmp in
    N|n )
        exit;;
esac

rm -fr $out_pcv
rm -fr $out_ptm
rm -fr "${tmp_dir}hacpack_backup"

echo -e "\nDone!"
