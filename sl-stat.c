#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>

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

void print_msgs(void)
{
	uint32_t offset;
	uint32_t type_len;
	uint32_t msg_len;
	bootloader_log_msg_t *msgs;

	if(log == NULL)
		return;

	offset = sizeof(*log);

	while (offset < log->next_off)
	{
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
	read_file("log_file");
	print_msgs(); 

	return 0;
}
