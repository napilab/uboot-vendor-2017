/*
 * Copyright (c) 2017, ASUS CORPORATION.  All rights reserved.
 */ 

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <mapmem.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <errno.h>
#include <linux/list.h>
#include <fs.h>
#include <asm/io.h>
#include <version.h>

#include "menu.h"
#include "cli.h"

#define DTS_OVERLAY_PROPERTY_LENGTH 10
#define DTS_OVERLAY_PREFIX "/overlays/"
#define DTS_PREFIX_LENGTH 10

#define MAX_OVERLAY_NAME_LENGTH 128
#define MAX_DTS_OVERLAY_NUMBER 16
//#define MAX_DTBO_PARAM_NUNBER 16

//struct dtbo_param_array
//{
//	int dtbo_param_number;
//	char dtbo_param_name[MAX_OVERLAY_NAME_LENGTH];
//}

struct dts_overlay_array
{
	int dtbo_number; //0 to (MAX_OVERLAY_NAME_LENGTH-1)
	char dtbo_name[MAX_OVERLAY_NAME_LENGTH];
//	int dtbo_param:1; // flag: 0 1
//	struct dtbo_param_array dtbo_param_name[MAX_DTBO_PARAM_NUNBER];
};

#if defined CONFIG_TARGET_ROCKPI4_RK3399
struct hw_config
{
	int valid;
	int pwm0;
	int pwm1;
	int uart2;
	int uart4;
	int spi1;
	int spi2;
	int i2c2;
	int i2c6;
	int i2c7;
	struct dts_overlay_array dts_overlay[MAX_DTS_OVERLAY_NUMBER];
	int dts_overlay_count;
};
#elif defined CONFIG_TARGET_ROCKPIS_RK3308
struct hw_config
{
	int valid;
	int i2c0;
	int i2c1;
	int i2c2;
	int i2c3;
	int pwm1;
	int pwm2;
	int pwm3;
	int spi0;
	int spi2;
	int uart0;
	int uart1;
	int uart2;
	struct dts_overlay_array dts_overlay[MAX_DTS_OVERLAY_NUMBER];
	int dts_overlay_count;
};
#else
struct hw_config
{
	int valid;
};
#endif

extern char *from_env(const char *envvar);
extern int get_relfile(cmd_tbl_t *cmdtp, const char *file_path, unsigned long file_addr);

static unsigned long hw_skip_comment(char *text)
{
	int i = 0;
	if(*text == '#')
	{
		while(*(text + i) != 0x00)
		{
			if(*(text + (i++)) == 0x0a)
				break;
		}
	}

	return i;
}

static unsigned long hw_skip_line(char *text)
{
	if(*text == 0x0a)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static unsigned long get_value(char *text, struct hw_config *hw_conf)
{
	int i = 0;
#if defined CONFIG_TARGET_ROCKPI4_RK3399
	if(memcmp(text, "pwm0=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->pwm0 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->pwm0 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "pwm1=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->pwm1 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->pwm1 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "uart2=", 6) == 0)
	{
		i = 6;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->uart2 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->uart2 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "uart4=", 6) == 0)
	{
		i = 6;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->uart4 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->uart4 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "spi1=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->spi1 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->spi1 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "spi2=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->spi2 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->spi2 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "i2c2=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->i2c2 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->i2c2 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}

	else if(memcmp(text, "i2c6=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->i2c6 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->i2c6 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "i2c7=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->i2c7 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->i2c7 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "dtoverlay=", 10) == 0)
	{
		static int dtbo_count = 0;
		int name_length;
		int j = DTS_OVERLAY_PROPERTY_LENGTH; //dts_overlay file offset
		i = j;

		while(*(text + j) != 0x00)
		{
			if(*(text + j) == 0x0a)
				break;
			j++;
		}

		name_length = j - i;
		i = j;

		//printf("dts_overlay name length = %d\n", name_length);
		if(name_length && name_length < MAX_OVERLAY_NAME_LENGTH)
		{
			if(dtbo_count < MAX_DTS_OVERLAY_NUMBER)
			{
				hw_conf->dts_overlay[dtbo_count].dtbo_number = dtbo_count;
				memcpy(hw_conf->dts_overlay[dtbo_count].dtbo_name, DTS_OVERLAY_PREFIX,
					DTS_PREFIX_LENGTH);
				memcpy(hw_conf->dts_overlay[dtbo_count].dtbo_name + DTS_PREFIX_LENGTH,
					text + DTS_OVERLAY_PROPERTY_LENGTH, name_length);
				memcpy(hw_conf->dts_overlay[dtbo_count].dtbo_name + DTS_PREFIX_LENGTH + name_length,
					".dtbo", 5);
				hw_conf->dts_overlay[dtbo_count].dtbo_name[DTS_PREFIX_LENGTH + name_length + 5] = 0x00;
				printf("dtoverlay number: %d, name:%s \n", \
								hw_conf->dts_overlay[dtbo_count].dtbo_number, \
								hw_conf->dts_overlay[dtbo_count].dtbo_name);
				dtbo_count++;
				hw_conf->dts_overlay_count = dtbo_count;
				//printf("The number of valid dtbo files : %d\n", hw_conf->dts_overlay_count);
			}
		}
		else
		{
			printf("Invalid dts overlay file name\n");
		}
	}
	else
		goto invalid_line;

	while(*(text + i) != 0x00)
	{
		if(*(text + (i++)) == 0x0a)
			break;
	}
	return i;

invalid_line:
	//It's not a legal line, skip it.
	printf("get_value: illegal line\n");
	while(*(text + i) != 0x00)
	{
		if(*(text + (i++)) == 0x0a)
			break;
	}
	return i;
#elif defined CONFIG_TARGET_ROCKPIS_RK3308
	if(memcmp(text, "pwm1=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->pwm1 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->pwm1 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "pwm2=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->pwm2 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->pwm2 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "pwm3=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->pwm3 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->pwm3 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "uart0=", 6) == 0)
	{
		i = 6;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->uart0 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->uart0 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "uart1=", 6) == 0)
	{
		i = 6;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->uart1 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->uart1 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "uart2=", 6) == 0)
	{
		i = 6;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->uart2 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->uart2 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "spi0=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->spi0 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->spi0 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "spi2=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->spi2 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->spi2 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "i2c0=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->i2c0 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->i2c0 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "i2c1=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->i2c1 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->i2c1 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "i2c2=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->i2c2 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->i2c2 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "i2c3=", 5) == 0)
	{
		i = 5;
		if(memcmp(text + i, "on", 2) == 0)
		{
			hw_conf->i2c3 = 1;
			i = i + 2;
		}
		else if(memcmp(text + i, "off", 3) == 0)
		{
			hw_conf->i2c3 = 0;
			i = i + 3;
		}
		else
			goto invalid_line;
	}
	else if(memcmp(text, "dtoverlay=", 10) == 0)
	{
		static int dtbo_count = 0;
		int name_length;
		int j = DTS_OVERLAY_PROPERTY_LENGTH; //dts_overlay file offset
		i = j;

		while(*(text + j) != 0x00)
		{
			if(*(text + j) == 0x0a)
				break;
			j++;
		}

		name_length = j - i;
		i = j;

		//printf("dts_overlay name length = %d\n", name_length);
		if(name_length && name_length < MAX_OVERLAY_NAME_LENGTH)
		{
			if(dtbo_count < MAX_DTS_OVERLAY_NUMBER)
			{
				hw_conf->dts_overlay[dtbo_count].dtbo_number = dtbo_count;
				memcpy(hw_conf->dts_overlay[dtbo_count].dtbo_name, DTS_OVERLAY_PREFIX,
					DTS_PREFIX_LENGTH);
				memcpy(hw_conf->dts_overlay[dtbo_count].dtbo_name + DTS_PREFIX_LENGTH,
					text + DTS_OVERLAY_PROPERTY_LENGTH, name_length);
				memcpy(hw_conf->dts_overlay[dtbo_count].dtbo_name + DTS_PREFIX_LENGTH + name_length,
					".dtbo", 5);
				hw_conf->dts_overlay[dtbo_count].dtbo_name[DTS_PREFIX_LENGTH + name_length + 5] = 0x00;
				printf("dtoverlay number: %d, name:%s \n", \
								hw_conf->dts_overlay[dtbo_count].dtbo_number, \
								hw_conf->dts_overlay[dtbo_count].dtbo_name);
				dtbo_count++;
				hw_conf->dts_overlay_count = dtbo_count;
				//printf("The number of valid dtbo files : %d\n", hw_conf->dts_overlay_count);
			}
		}
		else
		{
			printf("Invalid dts overlay file name\n");
		}
	}
	else
		goto invalid_line;

	while(*(text + i) != 0x00)
	{
		if(*(text + (i++)) == 0x0a)
			break;
	}
	return i;

invalid_line:
	//It's not a legal line, skip it.
	printf("get_value: illegal line\n");
	while(*(text + i) != 0x00)
	{
		if(*(text + (i++)) == 0x0a)
			break;
	}
	return i;
#endif
	return i;
}

static unsigned long hw_parse_property(char *text, struct hw_config *hw_conf)
{
	int i = 0;
	if(memcmp(text, "intfc:", 6) == 0)
	{
		i = 6;
		i = i + get_value(text + i, hw_conf);
	}
	else
	{
		printf("hw_parse_property: illegal line\n");
		//It's not a legal line, skip it.
		while(*(text + i) != 0x00)
		{
			if(*(text + (i++)) == 0x0a)
				break;
		}
	}
	return i;
}

void parse_hw_config(cmd_tbl_t *cmdtp, struct hw_config *hw_conf)
{
	unsigned long file_addr, size, count, offset = 0;
	char *envaddr, *conf_size;

	int valid = 0;

	envaddr = from_env("hw_conf_addr_r");

	if (!envaddr)
		goto end;

	if (strict_strtoul(envaddr, 16, &file_addr) < 0)
		goto end;

	if(get_relfile(cmdtp, "/hw_intfc.conf", file_addr) < 0)
		goto end;

	conf_size = from_env("filesize");
	if (strict_strtoul(conf_size, 16, &size) < 0)
		goto end;
	valid = 1;
	//printf("hw_conf size = %ld\n", size);

	*((char *)file_addr + size) = 0x00;

	while(offset != size)
	{
		count = hw_skip_comment((char *)(file_addr + offset));
		if(count > 0)
		{
			offset = offset + count;
			//printf("find comment = %ld\n", offset);
			continue;
		}
		count = hw_skip_line((char *)(file_addr + offset));
		if(count > 0)
		{
			offset = offset + count;
			//printf("find line %ld\n", offset);
			continue;
		}
		count = hw_parse_property((char *)(file_addr + offset), hw_conf);
		if(count > 0)
		{
			offset = offset + count;
			continue;
		}
	}
	//printf("offset = %ld\n", offset);
end:
	hw_conf->valid = valid;
}
