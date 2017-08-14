#/bin/sh


# filename:         unjffs2.sh
# last modified:    2017-6-21
#
# unzip mtd files, like: jffs2.

sh_args=$@
mtd_size=16384	# K
img_file=rootfs.jffs2   # jffs2...
img_type=jffs2          # file -b rootfs.jffs2 | cut -d ' ' -f2
uzip_mode=mtdram
mnt_dir=./mnt
out_dir=./out


usage()
{
    printf "Usage: %s [-t <type>] [-m <mode>]  [-o <dir>] [-s] [-h] <file>\n" "$(basename "$0")"
    printf "\n"
    printf "Options:\n"
    printf "    file      input image file, like: rootfs.jffs2\n"
    printf "    -t <type> image type(jffs2...), default jffs2\n"
    printf "    -m <mode> uzip mode(mtdram or loop), default mtdram, tips: mtdram need enough ram capacity\n"
    printf "    -o <dir>  output dir name, default ./out\n"
    printf "    -s        assign use file size to mtd total size(K), default 16MB\n"
    printf "    -h        print usage and exit\n"
}

opts_parse()
{
	local s_parm=0

    while getopts :t:m:o::sh option;
	do
		
        case "${option}" in
            t)
                img_type="${OPTARG}"
                ;;

            s)
				s_parm=1
                ;;

            m)
                uzip_mode="${OPTARG}"
                ;;

            o)
                out_dir="${OPTARG}"
                ;;
            h)
                usage
                exit 0
                ;;

            --)
                ;;

            \?)
                printf "Error: Invalid option: -%s\n" "${OPTARG}" >&2
                usage
                exit 1
                ;;

            :)
                printf "Error: Option -%s requires an argument\n" "${OPTARG}" >&2
                usage
                exit 1
                ;;
        esac
    done

    shift $((OPTIND - 1))

    if [ -z "$1" ]; then
        printf "Error: Missing argument\n" >&2
        usage
        exit 1
    fi

    img_file="$1"
	[ $s_parm -eq 1 ] && mtd_size=$(du -bk $img_file | awk '{print $1}')
	
	echo "\
sh_args=$sh_args
mtd_size=$mtd_size
img_file=$img_file
img_type=$img_type
mnt_dir=$mnt_dir
out_dir=$out_dir\
	" 1>/dev/null 2>&1
}


# root permission check, this shell will be executed again if no permission
# 0: successed, 1: failed
root_check()
{
	if [ $(id -u) -ne 0 ]; then
		echo "sorry, you must have super privilege!" >&2
		#select choose in 'I have root passwd' 'I have sudo privilege' 'try again' 'ignore' 'aboart' 
		printf "Input your choose: 'root' or 'sudo' or 'try' or 'ignore' or 'aboart' ?\n>"
		while read choose
		do
			case $choose in
			*root*)
				su -c "sh $0 $sh_args"
				break
				;;
			*sudo*)
				sudo sh $0 $sh_args	# force
				break
				;;
			try*)
				eval sh $0 $sh_args
				[ $? == 0 ] && break
				;;
			ignore)
				return 0
				;;
			aboart)
				break
				;;
			*)
				echo "Invalid select, please try again!" >&2
				continue
				;;
			esac

			echo "Install cross tools failed!" >&2
		done
		
		return 1
	fi
	
	return 0
}


loop_mount()
{
	# make a block device with image file
	loop=$(losetup -f)
	losetup $loop $img_file
	
	# convert it to a mtd device
	modprobe block2mtd block2mtd=$loop,65536	# 65536 = erasesize 64K, must equal erasesize of 'mkfs.jffs2 -e'

	# modprobe mtdblock, create /dev/mtdblock0
	modprobe mtdblock

	# modprobe jffs2, support mount -t auto ...
	#modprobe jffs2
	
	# mount
	[ ! -d $mnt_dir ] && mkdir $mnt_dir
	mount -t $img_type -o ro /dev/mtdblock0 $mnt_dir
	
	# copy dir
	cp -raT $mnt_dir $out_dir
	chmod 777 $out_dir	
}

loop_unmount()
{
	umount $mnt_dir
	[ -d $mnt_dir ] && rm -r $mnt_dir
	rmmod mtdblock
	rmmod block2mtd
	losetup -d $loop

}


mtdram_mount()
{
	# modprobe mtdram, with mtd_size(unit KB), create /proc/mtd
	modprobe mtdram total_size=$mtd_size

	# modprobe mtdchar
	#modprobe mtdchar
	
	# write the image to /dev/mtd0
	dd if=$img_file of=/dev/mtd0 1>/dev/null 2>&1
	
	# modprobe mtdblock, create /dev/mtdblock0
	modprobe mtdblock
	
	# mount
	[ ! -d $mnt_dir ] && mkdir $mnt_dir
	mount -t $img_type -o ro /dev/mtdblock0 $mnt_dir
	
	# copy dir
	cp -raT $mnt_dir $out_dir
	chmod 777 $out_dir
}

mtdram_umount()
{
	umount $mnt_dir
	[ -d $mnt_dir ] && rm -r $mnt_dir
	rmmod mtdblock
	#rmmod mtdchar
	rmmod mtdram
}



main()
{
	opts_parse $sh_args

	root_check
	[ $? -ne 0 ] && exit 0		# exit current shell if no permission


	if [ $uzip_mode = "mtdram" ]; then
		mtdram_mount
		mtdram_umount
		echo "<mtdram>: uzip $img_type file $img_file done, output to $out_dir."
	elif [ $uzip_mode = "loop" ]; then
		loop_mount
		loop_unmount
		echo "<loop>: uzip $img_type file $img_file done, output to $out_dir."
	else
		echo "Error uzip mode input, only support 'mtdram' or 'loop'"
	fi
}

main



# tips:
# mtdram need enough ram capacity.
# loop need assign erasesize, must equal the erasesize of 'mkfs.jffs2 -e'
# try change unzip mode '-m mtdram|loop' if unzip failed


