# taskset
This program is inspired by [taskset](http://man7.org/linux/man-pages/man1/taskset.1.html) from `Linux`, and is used to bond process with dedicated CPUs on [DragonFly BSD](https://www.dragonflybsd.org/).  

## Usage  

    // Bond running process with CPU list:
    # taskset -c cpu_list -p pid
    // Execute command bonding with CPU list:
    # taskset -c cpu_list command argument ...
    // Query running process's bonding CPU list:
    # taskset -p pid

## Example

	# taskset -c 0,2-4 -p 37224
	# taskset -c 0,2-4 ./test_thread
	# taskset -p 37224

## Compile and run

	# git clone https://github.com/NanXiao/taskset.git
	# cd taskset
	# mkdir build
	# cd build
	# cmake ..
	# make