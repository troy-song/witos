#include <g-bios.h>
#include <flash/flash.h>
#include <flash/part.h>
#include <getopt.h>
#include <sysconf.h>
#include <string.h>
#include <bar.h>
#include <app.h>

struct cmd_info flash_cmd_info[] =
{
	[0] =
	{
		.cmd   = "dump",
		.opt   = "b:p:a:h",
		.flags = 2,
	},
	[1] =
	{
		.cmd  = "erase",
		.opt  = "b:a:d:m:l",
		.flags = 2,
	},
	[2] =
	{
		.cmd  = "parterase",
		.opt  = "p:d",
		.flags = 2,
	},
	[3] =
	{
		.cmd  = "partshow",
		.flags = 2,
	},
	[4] =
	{
		.cmd  = "scanbb",
		.flags = 2,
	},
	[5] = {},
};


/* flash dump */
static void flash_dump_usage(void)
{
	printf("Usage: flash dump [options <value>]\n"
		"\noptions:\n"
		"  -b\tset flash block number\n"
		"  -p\tset flash page number\n"
		"  -a\tset flash address\n");
}


static int dump(int num, char *string[])
{
	int ch, ret, flag;
	u8  *p, *buff;
	u32 start, size;
	struct flash_chip *flash;
	struct partition *curr_part;
	char *optarg;

	// fixme: flash location
	curr_part = part_open(PART_CURR, OP_RDONLY);
	BUG_ON(NULL == curr_part);

	flash = curr_part->host;
	BUG_ON(NULL == flash);

	size  = flash->write_size + flash->oob_size;
	start = part_get_base(curr_part);

	part_close(curr_part);

	flag = 0;
	while ((ch = getopt(num, string, flash_cmd_info[0].opt, &optarg)) != -1)
	{
		switch (ch)
		{
		case 'b':
			if (flag || string2value(optarg, &start) < 0)
			{
				printf("Invalid argument: \"%s\"\n", optarg);
				flash_dump_usage();

				return -EINVAL;
			}

			start = start * flash->block_size;

			if (start >= flash->chip_size)
			{
				printf("Block number 0x%08x overflow!\n", start);
				return -EINVAL;
			}

			flag = 1;

			break;

		case 'a':
			if (flag || string2value(optarg, &start) < 0)
			{
				printf("Invalid argument: \"%s\"\n", optarg);

				flash_dump_usage();

				return -EINVAL;
			}

			if (start >= flash->chip_size)
			{
				printf("Address 0x%08x overflow!\n", start);
				return -EINVAL;
			}

			flag = 1;

			break;

		case 'p':
			if (flag || string2value(optarg, &start) < 0)
			{
				printf("Invalid argument: \"%s\"\n", optarg);
				flash_dump_usage();

				return -EINVAL;
			}

			start = start * flash->page_size;

			if (start >= flash->chip_size)
			{
				printf("Page number 0x%08x overflow!\n", start);
				return -EINVAL;
			}

			flag = 1;

			break;

		case 'h':
			flash_dump_usage();
			return 0;

		default:
			flash_dump_usage();
			return -EINVAL;
		}
	}

	buff = (u8 *)malloc(size);
	if (NULL == buff)
	{
		return -ENOMEM;
	}

	start &= ~(flash->write_size - 1);

	ret = flash_ioctl(flash, FLASH_IOCS_OOB_MODE, (void *)FLASH_OOB_RAW);
	// if ret < 0
	ret = flash_read(flash, buff, start, size);
	if (ret < 0)
	{
		printf("%s(): line %d execute flash_read_raw() error!\n"
			"error = %d\n", __FUNCTION__, __LINE__, ret);

		free(buff);

		return ret;
	}

	DPRINT("Flash 0x%08x ==> RAM 0x%08x, Expected length 0x%08x, Real length 0x%08x\n\n",
		   start, buff, size, ret);

	p = buff;

	while (size > 0)
	{
		int i;

		printf("Page @ 0x%08x:\n", start);

		i = flash->write_size >> 4;

		while (i--)
		{
			printf( "\t%02x %02x %02x %02x %02x %02x %02x %02x"
				"  %02x %02x %02x %02x %02x %02x %02x %02x\n",
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
				p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);

			p += 16;
		}

		printf("OOB @ 0x%08x + 0x08x:\n", start, flash->write_size);

		i = flash->oob_size >> 4;

		while (i--)
		{
			printf( "\t%02x %02x %02x %02x %02x %02x %02x %02x"
				"  %02x %02x %02x %02x %02x %02x %02x %02x\n",
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
				p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);

			p += 16;
		}

		start += flash->write_size + flash->oob_size;
		size  -= flash->write_size + flash->oob_size;

		puts("\n");
	}

	free(buff);

	flash_close(flash);

	return 0;
}


/* flash erase */
static void FlashEraseUsage(void)
{
	printf("Usage: flash erase [options <value>] [flags]\n" );
	printf("\noptions:\n"
#if 0
		"  -p   \t\terase current partition \n"
#endif
		"  -a   \t\tset erase unit to byte and specify starting address\n"
		"  -b   \t\tset erase unit to block and specify starting block number\n"
		"  -d   \t\tset allow bad block\n"
		"  -m   \t\tset flash clean mark\n"
		"  -l   \t\tset length,default is 0\n"
#if 0
		"       \t\t-p, -a,  -b conflict with other\n"
#else
		"		\t\t-a,  -b conflict with other\n"
#endif
		 );

    printf("\nexamples:\n"
#if 0
	"  flash erase -p\n"
#endif
	"  flash erase -a 1M -l 32K\n"
	"  flash erase -b 100 -l 16 -d \n"
	);
}


// fixme: bad logic!
static int erase(int num, char *string[])
{
	struct flash_chip *flash	   = NULL;
	struct partition *pCurPart = NULL;
	u32 nRet 		= 0;
	u32 nAddress  	= 0;
	u32 nEraseStart = 0;
	u32 nLen        = 0;
	u32 nEraseLen;
	u32 flags  	    = 0;
	int c           = 0;
	u8 bFlag;
	char *optarg;

	if (1 == num)
	{
		FlashEraseUsage();
		return -EINVAL;
	}

	pCurPart = part_open(PART_CURR, OP_RDWR);
	BUG_ON(NULL == pCurPart);

	flash = pCurPart->host;
	BUG_ON(NULL == flash);

	nEraseLen = flash->erase_size;

	part_close(pCurPart);

	bFlag = 0;

	while ((c = getopt(num, string, flash_cmd_info[1].opt, &optarg)) != -1)
	{
		switch (c)
		{
		case 'b':
			if (string2value(optarg, &nAddress) < 0 || bFlag)
			{
				FlashEraseUsage();
				return -EINVAL;
			}

			bFlag = 1;

			nEraseStart = nAddress << flash->erase_shift;

			break;

		case 'a':
			if (string2value(optarg, &nAddress) < 0 || bFlag)
			{
				FlashEraseUsage();
				return -EINVAL;
			}

			bFlag = 1;

			nEraseStart = nAddress;

			break;

		case 'l':
			if (string2value(optarg, &nLen) < 0)
			{
				FlashEraseUsage();
				return -EINVAL;
			}
			nEraseLen = nLen;
			break;

		case 'p':
			nEraseStart = pCurPart->attr->part_base;
			nEraseLen   = pCurPart->attr->part_size;

			break;

		case 'm':
			flags |= EDF_JFFS2;
			break;

		case 'd':
			flags |= EDF_ALLOWBB;
			break;

		case '?':
		case ':':
		case 'h':
		default:
			FlashEraseUsage();
			return -EINVAL;

		}
	}

	if (flash->chip_size < nEraseStart + nEraseLen)
	{
		printf("Out of chip size!\n");
		return -EINVAL;
	}

	//aligned:
	ALIGN_UP(nEraseStart, flash->write_size);
	ALIGN_UP(nEraseLen, flash->block_size);

	printf("[0x%08x : 0x%08x]\n", nEraseStart, nEraseLen);
	nRet = flash_erase(flash, nEraseStart, nEraseLen, flags);

	flash_close(flash);

	return nRet;
}


/* part erase */
struct flash_parterase_param
{
	FLASH_HOOK_PARAM hParam;
	struct process_bar *pBar;
};


static void flash_parterase_usage(void)
{
	printf("Usage: flash parterase <subcommands> [options <value>] [flags]\n"
			"options:\n"
			"  -p   \t\tset partition\n"
			"  -d   \t\tignore bad block\n"
//			"  -j   \t\tJffs2 file system.\n"
		  );
}


static int flash_parterase_process(struct flash_chip *flash, FLASH_HOOK_PARAM *pParam)
{
	struct flash_parterase_param *pErInfo;

	pErInfo = OFF2BASE(pParam, struct flash_parterase_param, hParam);

	progress_bar_set_val(pErInfo->pBar, pParam->nBlockIndex);

	return 0;
}


static int parterase(int num, char *string[])
{
	int ret = 0, nDevNum;
	u32 size, start;
	u32 flags = EDF_NORMAL;
	struct flash_chip *flash;
	struct partition *part = NULL;
	u32 ch;
	struct flash_parterase_param erInfo;
	char *optarg;
	BOOL need_save = TRUE, for_jffs2 = FALSE;
	FLASH_CALLBACK callback;

	while ((ch = getopt(num, string, flash_cmd_info[2].opt, &optarg)) != -1)
	{
		switch (ch)
		{
		case 'p':
			if(string2value(optarg, (u32 *)&nDevNum) < 0)
			{
				flash_parterase_usage();
				goto L1;
			}

			part = part_open(nDevNum, OP_RDWR);
			if (NULL == part)
			{
				flash_parterase_usage();
				goto L1;
			}

			break;

		case 'd':
			flags |= EDF_ALLOWBB;
			break;

		case 'j':
			for_jffs2 = TRUE;
			break;

		case 's':
			need_save = FALSE;
			break;

		default:
			flash_parterase_usage();
			goto L1;
		}
	}

	if (NULL == part)
	{
		part = part_open(PART_CURR, OP_RDWR);

		if (NULL == part)
		{
			printf("fail to open current partition!\n");
			ret = -EACCES;
			goto L1;
		}
	}

	if (for_jffs2)
	{
		if (part->attr->part_type == PT_FS_JFFS2)
			flags |= EDF_JFFS2;
		else
			printf("not JFFS2 part (-j skipped)\n");
	}

	flash = part->host;

	start = part_get_base(part);
	size  = part_get_size(part);

	create_progress_bar(&erInfo.pBar,
				start >> flash->erase_shift,
				(start + size - 1) >> flash->erase_shift
				);

	callback.func = flash_parterase_process;
	callback.args = &erInfo.hParam;
	flash_ioctl(flash, FLASH_IOCS_CALLBACK, &callback);

	ret = flash_erase(flash, start, size, flags);
	printf("\n");

	delete_progress_bar(erInfo.pBar);

	part_set_image(part, "", 0);

	if (need_save)
	{
		ret = sysconf_save();
		if (ret < 0)
			printf("Can not save configure information to flash");
	}

	part_close(part);

L1:
	return ret;
}


/* part show */
static int partshow(int num, char *string[])
{
	struct flash_chip *flash;
	u32 flash_id;

	flash_id = 0;

	while ((flash = flash_open(flash_id)) != NULL)
	{
		part_show(flash);

		flash_close(flash);

		flash_id++;
	}

	return 0;
}


/* scan bb */
// TODO: add process bar
static int scanbb(int num, char *string[])
{
	int ret;
	struct flash_chip *flash;

	flash = flash_open(BOOT_FLASH_ID);
	if (NULL == flash)
	{
		ret = -ENODEV;
		goto L1;
	}

	if (FLASH_NANDFLASH == flash->type)
	{
		// fixme
	}

	flash_ioctl(flash, FLASH_IOC_SCANBB, NULL);

	flash_close(flash);
L1:
	return ret;
}


static void flash_usage(void)
{
	printf("Usage: flash command [options <value>]\n"
			"command:\n"
			"  dump      \tview one page\n"
			"  erase     \terase flash\n"
			"  parterase \terase flash partition\n"
			"  partshow  \tshow flash partition\n"
			"  scanbb    \tscan flash bad block\n"
//			"  -j   \t\tJffs2 file system.\n"
			);
}


static int get_cmd(int num, char *string)
{
	u32 i = 0;

	while (flash_cmd_info[i].cmd)
	{	if (0 == strcmp(string, flash_cmd_info[i].cmd))
			break;
		i++;
	}

	return i;
}


int main(int argc, char *argv[])
{
	u32 ch;

	if (argc < 2)
		ch = 0;
	else
		ch = get_cmd(argc - 1, argv[1]);

	switch (ch)
	{
	case 0:
		dump(argc - 1, argv + 1);
		break;

	case 1:
		erase(argc - 1, argv + 1);
		break;

	case 2:
		parterase(argc - 1, argv + 1);
		break;

	case 3:
		partshow(argc - 1, argv + 1);
		break;

	case 4:
		scanbb(argc - 1, argv + 1);
		break;

	default:
		flash_usage();
	}

	return 0;
}
