#include<stdlib.h>
#include<stdio.h>
#include<string.h>

struct bootloader_log_msgs
{
	int level;
	int facility;
	char *type;
	char *msg;
};

struct bootloader_log
{
	int version;
	int bootloader;
	int size;
	int next_off;
	struct bootloader_log_msgs *log_msg;
}grub_log;

void
grub_log_init(int version, int bootloader, int size)
{
	grub_log.version = version;
	grub_log.bootloader = bootloader;
	grub_log.size = size;
	grub_log.next_off = 0;
	grub_log.log_msg = (struct bootloader_log_msgs *)
			   malloc(size*sizeof(struct bootloader_log_msgs));
}

void
grub_log_addmsg(int level, int facility, const char *file, const int line, const char *fmt)
{	
	int offset = grub_log.next_off;
	/* Determine the strlen of the line int. */
	int line_tmp = line;
	int line_strlen = 1;
	while(line_tmp>9)
	{
		line_strlen++;
		line_tmp/=10;
	}
	/*
 	 * Get the total message length from the file, line and fmt. 
 	 * Also add 3 for formating.
 	 */
	int msg_len = strlen(file) + line_strlen + strlen(fmt) + 3;
	printf("Message is len: %i\n", msg_len);
	grub_log.log_msg[offset].msg = (char *)malloc(msg_len*sizeof(char));
	char msg[msg_len];
	sprintf(msg, "%s:%d: %s", file, line, fmt);
	printf("%s\n\n", msg);
	grub_log.log_msg[offset].level = level;
	grub_log.log_msg[offset].facility = facility;
	grub_log.log_msg[offset].type = file; 
	grub_log.log_msg[offset].msg = msg;
	grub_log.next_off = offset + 1;
}

int 
main()
{
	grub_log_init(1, 2, 5);
	grub_log_addmsg(1, 2, "Linux", 20, "Test Happened Here");
	
	printf("Log Buffer Header\n");
	printf("Version: %i\n", grub_log.version);
	printf("Bootloader: %i\n", grub_log.bootloader);
	printf("Size: %i\n", grub_log.size);
	printf("Next_off: %i\n\n", grub_log.next_off);

	printf("Log Buffer\n");
	printf("Level: %i\n", grub_log.log_msg[0].level);
	printf("Facility: %i\n", grub_log.log_msg[0].facility);
	printf("Type: %s\n", grub_log.log_msg[0].type);
	printf("Msg: %s\n", grub_log.log_msg[0].msg);
	printf("DONE!\n");
	return 0;
}
