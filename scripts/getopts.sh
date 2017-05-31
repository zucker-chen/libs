#!/bin/sh
# filename:         getopts.sh
# last modified:    2017-05-13
#
# Example usage of getopts in a POSIX-compliant way.

usage()
{
    printf "Usage: %s [-a <arg>] [-b [<arg>]] [-hv] <file>\n" "$(basename "$0")"
    printf "\n"
    printf "Options:\n"
    printf "    -a <arg>  arg is required here\n"
    printf "    -b <arg>  arg is optional here\n"
    printf "    -h        print usage and exit\n"
    printf "    -v        verbose logging\n"
}


main()
{
    a_arg=""
    b_arg=""
    verbose=false
    file=""

    while getopts :a:b::hv option; do
        case "${option}" in
            a)
                a_arg="${OPTARG}"
                ;;

            b)
                case "${OPTARG}" in
                    "")
                        b_arg=""
                        ;;
                    *)
                        b_arg="${OPTARG}"
                        ;;
                esac
                ;;

            h)
                usage
                exit 0
                ;;

            v)
                verbose=true
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

    file="$1"

    printf "a_arg=%s\n" "${a_arg}"
    printf "b_arg=%s\n" "${b_arg}"
    printf "verbose=%s\n" "${verbose}"
    printf "file=%s\n" "${file}"
}

main "$@"
