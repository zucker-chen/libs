#/bin/sh


# filename:         unjffs2.sh
# last modified:    2017-6-21
#
# unzip mtd files, like: jffs2.

sh_args=$@
mtd_size=16384	# K
img_file=rootfs.jffs2 # jffs2...
img_type=jffs2
mnt_dir=./mnt
out_dir=./out


usage()
{
    printf "Usage: %s [-t <arg>] [-s <arg>] [-o <arg>] [-h] <file>\n" "$(basename "$0")"
    printf "\n"
    printf "Options:\n"
    printf "    -t <arg>  arg is image type(jffs2...), default jffs2\n"
    printf "    -s <arg>  arg is mtd total size(K), default 16MB\n"
    printf "    -o <arg>  output dir name, default ./out\n"
    printf "    -h        print usage and exit\n"
    printf "    -v        verbose logging\n"
}

opts_parse()
{
    while getopts :t:s:o::h option; do
        case "${option}" in
            t)
                img_type="${OPTARG}"
                ;;

            s)
                mtd_size="${OPTARG}"
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
	
	echo "
sh_args=$sh_args
mtd_size=$mtd_size
img_file=$img_file
img_type=$img_type
mnt_dir=$mnt_dir
out_dir=$out_dir
	"
}


# root permission check, this shell will be executed again if no permission
# 0: successed, 1: failed
root_check()
{
	if [ $UID -ne 0 ]; then
		echo "sorry, you must have super privilege!" >&2
		select choose in 'I have root passwd' 'I have sudo privilege' 'try again' 'ignore' 'aboart' 
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


mtdram_mount()
{
	# modprobe mtdram, with mtd_size(default 4MB), create /proc/mtd
	modprobe mtdram total_size=$mtd_size

	# modprobe mtdchar
	modprobe mtdchar
	
	# write the image to /dev/mtd0
	dd if=$img_file of=/dev/mtd0
	
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
	rmmod mtdchar
	rmmod mtdram
}



main()
{
	opts_parse $sh_args
	
	root_check
	[ $? -ne 0 ] && exit 0		# exit current shell if no permission
		
	mtdram_mount
	
	mtdram_umount

}

main






