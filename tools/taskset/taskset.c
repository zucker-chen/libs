#define _GNU_SOURCE
#define __USE_GNU
#include <sched.h>
#include <unistd.h>

#include <stdbool.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

static void __attribute__((__noreturn__)) usage(void);
static int convert_str_to_int(char* begin);
static void parse_cpu_list(char* cpu_list, cpu_set_t* cpu_set);

static void __attribute__((__noreturn__)) usage(void)
{
    fprintf(stderr, "Usage:\n"
                    "    Bond running process with CPU list:\n"
                    "        taskset -c cpu_list -p pid\n"
                    "    Execute command bonding with CPU list:\n"
                    "        taskset -c cpu_list command argument ...\n"
                    "    Query running process's bonding CPU list:\n"
                    "        taskset -p pid\n");
    exit(1);
}

/* Both Process and CPU ids should be positive numbers. */
static int convert_str_to_int(char* begin)
{
    if (!begin)
    {
        errx(1, "Invalid arguments for %s", __func__);
    }

    errno = 0;
    char *end = NULL;
    long num = strtol(begin, &end, 10);
    if (errno || (*end != '\0') || (num > INT_MAX) || (num < 0))
    {
        errx(1, "Invalid integer: %s", begin);
    }
    return (int)num;
}

/*
 * The cpu list should like 1-3,6
 */
static void parse_cpu_list(char* cpu_list, cpu_set_t* cpu_set)
{
    if (!cpu_list || !cpu_set)
    {
        errx(1, "Invalid arguments for %s", __func__);
    }

    char* begin = cpu_list;
    while (1)
    {
        bool last_token = false;
        char* end = strchr(begin, ',');
        if (!end)
        {
            last_token = true;
        }
        else
        {
            *end = '\0';
        }

        char* hyphen = strchr(begin, '-');
        if (hyphen)
        {
            *hyphen = '\0';
            int first_cpu = convert_str_to_int(begin);
            int last_cpu = convert_str_to_int(hyphen + 1);
            if ((first_cpu > last_cpu) || (last_cpu >= CPU_SETSIZE))
            {
                errx(1, "Invalid cpu list: %s", cpu_list);
            }
            for (int i = first_cpu; i <= last_cpu; i++)
            {
                CPU_SET(i, cpu_set);
            }
        }
        else
        {
            CPU_SET(convert_str_to_int(begin), cpu_set);
        }

        if (last_token)
        {
            break;
        }
        else
        {
            begin = end + 1;
        }
    }
}

int main(int argc, char**argv)
{
    cpu_set_t cpu_set;
    pid_t pid;
    int c = 0;
    bool cflag = false;
    bool pflag = false;

    CPU_ZERO(&cpu_set);
    while ((c = getopt(argc, argv, "c:p:h")) != -1)
    {
        switch (c)
        {
            case 'c':
            {
                parse_cpu_list(optarg, &cpu_set);
                cflag = true;
                break;
            }
            case 'p':
            {
                pid = (pid_t)convert_str_to_int(optarg);
                pflag = true;
                break;
            }
            case 'h':
            case '?':
            default:
            {
                usage();
            }
        }
    }

    if (pflag)
    {
        /* pid and command are exclusive */
        if (optind != argc)
        {
            usage();
        }

        if (cflag)
        {
            if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set))
            {
                err(1, "sched_setaffinity");
            }
        }
        else
        {
            if (sched_getaffinity(pid, sizeof(cpu_set_t), &cpu_set))
            {
                err(1, "sched_getaffinity");
            }
            printf("Process (%d) bonds to CPU:", pid);
            for (int i = 0; i < CPU_SETSIZE; i++)
            {
                if (CPU_ISSET(i, &cpu_set))
                {
                    printf(" %d", i);
                }
            }
            printf("\n");
        }
    }
    else
    {
        if ((optind == argc) || (!cflag))
        {
            usage();
        }

        if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set))
        {
            err(1, "sched_setaffinity");
        }
        if (execvp(argv[optind], &argv[optind]))
        {
            err(1, "execvp");
        }
    }

    return 0;
}
