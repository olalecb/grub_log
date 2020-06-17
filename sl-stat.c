#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>

//Regs
#include <getopt.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <errno.h>

#define TXT_PUB_CONFIG_REGS_BASE 0xfed30000
#define TXT_PRIV_CONFIG_REGS_BASE 0xfed20000
#define TXT_CONFIG_REGS_SIZE (TXT_PUB_CONFIG_REGS_BASE - \
				 TXT_PRIV_CONFIG_REGS_BASE)
#define TXTCR_STS 0x0000
#define TXTCR_ESTS 0x0008
#define TXTCR_ERRORCODE 0x0030
#define TXTCR_VER_FSBIF 0x0100
#define TXTCR_DIDVID 0x0110
#define TXTCR_VER_QPIIF 0x0200
#define TXTCR_SINIT_BASE 0x0270
#define TXTCR_SINIT_SIZE 0x0278
#define TXTCR_HEAP_BASE 0x0300
#define TXTCR_HEAP_SIZE 0x0308
#define TXTCR_DPR 0x330
#define TXTCR_PUBLIC_KEY 0x0400
#define TXTCR_E2STS 0x08f0

#define PACKED __attribute__((packed))

struct bootloader_log_msg
{
	uint32_t level;
	uint32_t facility;
	char type[];
	/* char msg[] */
} PACKED;
typedef struct bootloader_log_msg bootloader_log_msg_t;

struct bootloader_log
{
	uint32_t version;
	uint32_t producer;
	uint32_t size;
	uint32_t next_off;
	bootloader_log_msg_t msgs[];
} PACKED;
typedef struct bootloader_log bootloader_log_t;

static bootloader_log_t *log;

void read_file(char *filename)
{
	uint32_t log_fd;
	uint32_t file_size;

	log_fd = open(filename, O_RDONLY);

	if (log_fd < 0)
		return;

	file_size = lseek(log_fd, 0, SEEK_END);
	lseek(log_fd, 0, SEEK_SET);

	log = malloc(file_size);

	read(log_fd, log, file_size);

	close(log_fd);
}

/*
 * REGISTERS
 */
typedef void txt_heap_t;

typedef union {
	uint64_t _raw;
	struct {
		uint64_t senter_done_sts : 1;
		uint64_t sexit_done_sts : 1;
		uint64_t reserved1 : 4;
		uint64_t mem_config_lock_sts : 1;
		uint64_t private_open_sts : 1;
		uint64_t reserved2 : 7;
		uint64_t locality_1_open_sts : 1;
		uint64_t locality_2_open_sts : 1;
	};
} txt_sts_t;

typedef union {
	uint64_t _raw;
	struct {
		uint64_t txt_reset_sts : 1;
	};
} txt_ests_t;

typedef union {
	uint64_t _raw;
	struct {
		uint64_t reserved : 1;
		uint64_t secrets_sts : 1;
	};
} txt_e2sts_t;

typedef union {
	uint64_t _raw;
	struct {
		uint16_t vendor_id;
		uint16_t device_id;
		uint16_t revision_id;
		uint16_t reserved;
	};
} txt_didvid_t;

typedef union {
	uint64_t _raw;
	struct {
		uint64_t lock : 1;
		uint64_t reserved1 : 3;
		uint64_t top : 8;
		uint64_t reserved2 : 8;
		uint64_t size : 12;
		uint64_t reserved3 : 32; 
	};
} txt_dpr_t;

static inline const char * bit_to_str(uint64_t b)
{
	return b ? "TRUE" : "FALSE";
}

static inline uint64_t read_txt_config_reg(void *config_regs_base, uint32_t reg)
{
	/* these are MMIO so make sure compiler doesn't optimize */
	return *(volatile uint64_t *)(config_regs_base + reg);
}

void print_hex(const char* prefix, const void *start, size_t len)
{
	int i;
	const void *end = start + len;
	while (start < end) {
		printf("%s", prefix);
		for (i = 0; i < 16; i++) {
			if (start < end)
				printf("%02x ", *(uint8_t *)start);
			start++;
		}
		printf("\n");
	}
}

static void display_config_regs(void *txt_config_base)
{
	printf("Intel(r) TXT Configuration Registers:\n");

	/* STS */
	txt_sts_t sts;
	sts._raw = read_txt_config_reg(txt_config_base, TXTCR_STS);
	printf("\tSTS: 0x%08jx\n", sts._raw);
	printf("\t    senter_done: %s\n", bit_to_str(sts.senter_done_sts));
	printf("\t    sexit_done: %s\n", bit_to_str(sts.sexit_done_sts));
	printf("\t    mem_config_lock: %s\n", bit_to_str(sts.mem_config_lock_sts));
	printf("\t    private_open: %s\n", bit_to_str(sts.private_open_sts));
	printf("\t    locality_1_open: %s\n", bit_to_str(sts.locality_1_open_sts));
	printf("\t    locality_2_open: %s\n", bit_to_str(sts.locality_2_open_sts));

	/* ESTS */
	txt_ests_t ests;
	ests._raw = read_txt_config_reg(txt_config_base, TXTCR_ESTS);
	printf("\tESTS: 0x%02jx\n", ests._raw);
	printf("\t    txt_reset: %s\n", bit_to_str(ests.txt_reset_sts));

	/* E2STS */
	txt_e2sts_t e2sts;
	e2sts._raw = read_txt_config_reg(txt_config_base, TXTCR_E2STS);
	printf("\tE2STS: 0x%016jx\n", e2sts._raw);
	printf("\t    secrets: %s\n", bit_to_str(e2sts.secrets_sts));

	/* ERRORCODE */
	printf("\tERRORCODE: 0x%08jx\n", read_txt_config_reg(txt_config_base,
							 TXTCR_ERRORCODE));

	/* DIDVID */
	txt_didvid_t didvid;
	didvid._raw = read_txt_config_reg(txt_config_base, TXTCR_DIDVID);
	printf("\tDIDVID: 0x%016jx\n", didvid._raw);
	printf("\t    vendor_id: 0x%x\n", didvid.vendor_id);
	printf("\t    device_id: 0x%x\n", didvid.device_id);
	printf("\t    revision_id: 0x%x\n", didvid.revision_id);

	/* FSBIF */
	uint64_t fsbif;
	fsbif = read_txt_config_reg(txt_config_base, TXTCR_VER_FSBIF);
	printf("\tFSBIF: 0x%016jx\n", fsbif);

	/* QPIIF */
	uint64_t qpiif;
	qpiif = read_txt_config_reg(txt_config_base, TXTCR_VER_QPIIF);
	printf("\tQPIIF: 0x%016jx\n", qpiif);

	/* SINIT.BASE/SIZE */
	printf("\tSINIT.BASE: 0x%08jx\n", read_txt_config_reg(txt_config_base,
							TXTCR_SINIT_BASE));
	printf("\tSINIT.SIZE: %juB (0x%jx)\n",
		read_txt_config_reg(txt_config_base, TXTCR_SINIT_SIZE),
		read_txt_config_reg(txt_config_base, TXTCR_SINIT_SIZE));

	/* HEAP.BASE/SIZE */
	printf("\tHEAP.BASE: 0x%08jx\n", read_txt_config_reg(txt_config_base,
							TXTCR_HEAP_BASE));
	printf("\tHEAP.SIZE: %juB (0x%jx)\n",
		read_txt_config_reg(txt_config_base, TXTCR_HEAP_SIZE),
		read_txt_config_reg(txt_config_base, TXTCR_HEAP_SIZE));

	/* DPR.BASE/SIZE */
	txt_dpr_t dpr;
	dpr._raw = read_txt_config_reg(txt_config_base, TXTCR_DPR);
	printf("\tDPR: 0x%016jx\n", dpr._raw);
	printf("\t    lock: %s\n", bit_to_str(dpr.lock));
	printf("\t    top: 0x%08x\n", (uint32_t)dpr.top << 20);
	printf("\t    size: %uMB (%uB)\n", dpr.size, dpr.size*1024*1024);

	/* PUBLIC.KEY */
	uint8_t key[256/8];
	unsigned int i = 0;
	do {
		*(uint64_t *)&key[i] = read_txt_config_reg(txt_config_base,
							TXTCR_PUBLIC_KEY + i);
		i += sizeof(uint64_t);
	} while (i < sizeof(key));
	printf("\tPUBLIC.KEY:\n");
	print_hex("\t    ", key, sizeof(key)); printf("\n");

	/* easy-to-see status of TXT and secrets */
	printf("***********************************************************\n");
	printf("\t TXT measured launch: %s\n", bit_to_str(sts.senter_done_sts));
	printf("\t secrets flag set: %s\n", bit_to_str(e2sts.secrets_sts));
	printf("***********************************************************\n");
}
//HEAP
/*
typedef struct __packed {
	uint32_t type;
	uint32_t size;
	uint8_t data[];
} heap_ext_data_element_t;

typedef struct __packed {
	uint32_t version;
	uint32_t bios_sinit_size;
	uint64_t lcp_pd_base;
	uint64_t lcp_pd_size;
	uint32_t num_logical_procs;
	uint64_t flags;
	heap_ext_data_element_t ext_data_elts[];
} bios_data_t;

static inline uint64_t get_bios_data_size(const txt_heap_t *heap)
{
	return *(uint64_t *)heap;
}

static inline bios_data_t *get_bios_data_start(const txt_heap_t *heap)
{
	return (bios_data_t *)((char *)heap + sizeof(uint64_t));
}

static void print_bios_data(const bios_data_t *bios_data, uint64_t size)
{
    printk(TBOOT_DETA"bios_data (@%p, %jx):\n", bios_data,
           *((uint64_t *)bios_data - 1));
    printk(TBOOT_DETA"\t version: %u\n", bios_data->version);
    printk(TBOOT_DETA"\t bios_sinit_size: 0x%x (%u)\n", bios_data->bios_sinit_size,
           bios_data->bios_sinit_size);
    printk(TBOOT_DETA"\t lcp_pd_base: 0x%jx\n", bios_data->lcp_pd_base);
    printk(TBOOT_DETA"\t lcp_pd_size: 0x%jx (%ju)\n", bios_data->lcp_pd_size,
           bios_data->lcp_pd_size);
    printk(TBOOT_DETA"\t num_logical_procs: %u\n", bios_data->num_logical_procs);
    if ( bios_data->version >= 3 )
        printk(TBOOT_DETA"\t flags: 0x%08jx\n", bios_data->flags);
    if ( bios_data->version >= 4 && size > sizeof(*bios_data) + sizeof(size) )
        print_ext_data_elts(bios_data->ext_data_elts);
}

static void display_heap(txt_heap_t *heap)
{
	uint64_t size = get_bios_data_size(heap);
	bios_data_t *bios_data = get_bios_data_start(heap);
	print_bios_data(bios_data, size);
}
*/
static int fd_mem;
static void *buf_config_regs_read;
static void *buf_config_regs_mmap;

static inline uint64_t read_config_reg(uint32_t config_regs_base, uint32_t reg)
{
	uint64_t reg_val;
	void *buf;

	(void)config_regs_base;

	buf = buf_config_regs_read;
	if (buf == NULL)
		buf = buf_config_regs_mmap;
	if (buf == NULL)
		return 0;

	reg_val = read_txt_config_reg(buf, reg);
	return reg_val;
}

bool display_heap_optin = false;
static const char *short_option = "h";
static struct option longopts[] = {
	{"heap", 0, 0, 'p'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
};
static const char *usage_string = "txt-stat [--heap] [-h]";
static const char *option_strings[] = {
	"--heap:\t\tprint out heap info.\n",
	"-h, --help:\tprint out this help message.\n",
	NULL
};

//int print_regs(int argc, char *argv[])
int print_regs()
{	
	uint64_t heap = 0;
	uint64_t heap_size = 0;
	void *buf = NULL;
	off_t seek_ret = -1;
	size_t read_ret = 0;

	/*int c;
	while ((c = getopt_long(argc, (char **const)argv, short_option,
		longopts, NULL)) != -1) {
		switch (c) {
		case 'h':
			print_help(usage_string, option_strings);
			return 0;

		case 'p':
			display_heap_optin = true;
			break;

		default:
			return 1;
		}
	}
	*/
	fd_mem = open("/dev/mem", O_RDONLY);
	if (fd_mem == -1) {
		printf("ERROR: cannot open /dev/mem\n");
		return 1;
	}

	/*
	 * display public config regs
	 */
	seek_ret = lseek(fd_mem, TXT_PUB_CONFIG_REGS_BASE, SEEK_SET);
	if (seek_ret == -1)
		printf("ERROR: seeking public config registers failed: %s, try mmap\n",
			strerror(errno));
	else {
		buf = malloc(TXT_CONFIG_REGS_SIZE);
		if (buf == NULL)
			printf("ERROR: out of memory, try mmap\n");
		else {
			read_ret = read(fd_mem, buf, TXT_CONFIG_REGS_SIZE);
			if (read_ret != TXT_CONFIG_REGS_SIZE) {
				printf("ERROR: reading public config registers failed: %s,"
					"try mmap\n", strerror(errno));
				free(buf);
				buf = NULL;
			}
			else
				buf_config_regs_read = buf;
		}
	}

	/*
	 * try mmap to display public config regs,
	 * since public config regs should be displayed always.
	 */
	if (buf == NULL) {
		buf = mmap(NULL, TXT_CONFIG_REGS_SIZE, PROT_READ,
			MAP_PRIVATE, fd_mem, TXT_PUB_CONFIG_REGS_BASE);
		if (buf == MAP_FAILED) {
			printf("ERROR: cannot map config regs by mmap()\n");
			buf = NULL;
		}
		else
			buf_config_regs_mmap = buf;
	}

	if (buf) {
		display_config_regs(buf);
		heap = read_txt_config_reg(buf, TXTCR_HEAP_BASE);
		heap_size = read_txt_config_reg(buf, TXTCR_HEAP_SIZE);
	}

	/*
	 * display heap
	 */
	/*if (heap && heap_size && display_heap_optin) {
		seek_ret = lseek(fd_mem, heap, SEEK_SET);
		if (seek_ret == -1) {
			printf("ERROR: seeking TXT heap failed by lseek(): %s, try mmap\n",
				strerror(errno));
			goto try_mmap_heap;
		}
		buf = malloc(heap_size);
		if (buf == NULL) {
			printf("ERROR: out of memory, try mmap\n");
			goto try_mmap_heap;
		}
		read_ret = read(fd_mem, buf, heap_size);
		if (read_ret != heap_size) {
			printf("ERROR: reading TXT heap failed by read(): %s, try mmap\n",
				strerror(errno));
			free(buf);
			goto try_mmap_heap;
		}
		display_heap((txt_heap_t *)buf);
		free(buf);
		goto try_display_log;

	try_mmap_heap:

		buf = mmap(NULL, heap_size, PROT_READ, MAP_PRIVATE, fd_mem, heap);
		if (buf == MAP_FAILED)
			printf("ERROR: cannot map TXT heap by mmap()\n");
		else {
			display_heap((txt_heap_t *)buf);
			munmap(buf, heap_size);
		}
	}
*/
try_display_log:
	if (buf_config_regs_read)
		free(buf_config_regs_read);
	if (buf_config_regs_mmap)
		munmap(buf_config_regs_mmap, TXT_CONFIG_REGS_SIZE);

	close(fd_mem);

	return 0;
}

void print_msgs(void)
{
	uint32_t offset;
	uint32_t type_len;
	uint32_t msg_len;
	bootloader_log_msg_t *msgs;

	if (log == NULL)
		return;

	offset = sizeof(*log);

	while (offset < log->next_off) {
		msgs = (bootloader_log_msg_t *) ((uint8_t *) log + offset);

		printf("%s:", msgs->type);
		type_len = strlen(msgs->type) + 1;

		printf("%s\n", msgs->type + type_len);
		msg_len = strlen(msgs->type + type_len) + 1;

		offset += sizeof(*msgs) + type_len + msg_len;
	}
}

int main(void)
{
	print_regs();
	read_file("log_file");
	print_msgs(); 

	return 0;
}
