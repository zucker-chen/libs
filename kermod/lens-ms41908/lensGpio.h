
#ifndef __LENS_GPIO__
#define __LENS_GPIO__

#define DIR_OUT		22
#define DIR_IN		11

extern int set_gpio_dir(unsigned int group, unsigned int id, unsigned int dir);
extern int set_gpio_value(unsigned int group, unsigned int id, unsigned int value);
extern int get_gpio_value(unsigned int group, unsigned int id);

#define LENS_GpioSet(group,id,value) \
			do{						\
				set_gpio_dir(group, id, DIR_OUT); \
				set_gpio_value(group, id, value); \
			}while(0)
				
#define LENS_GpioGet(group,id,value) \
			do{						\
				set_gpio_dir(group, id, DIR_IN); \
				value = get_gpio_value(group, id); \
			}while(0)
#endif
