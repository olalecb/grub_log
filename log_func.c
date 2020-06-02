#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>

#define BL_SIZE_INIT 4096
#define GRUB_PACKED __attribute__((packed))

struct bootloader_log_msgs
{
  uint32_t level;
  uint32_t facility;
  char type[];
  /*char msg[];*/
}GRUB_PACKED;

struct bootloader_log
{
  uint32_t version;
  uint32_t producer;
  uint32_t size;
  uint32_t next_off;
  struct bootloader_log_msgs msgs[];
}*grub_log GRUB_PACKED;

void
grub_log_init()
{
  /*zalloc(size) should be the same as calloc(1, size)*/
  grub_log = calloc(1, BL_SIZE_INIT);
  grub_log->version = 1;
  grub_log->producer = 1; 
  grub_log->size = BL_SIZE_INIT;
  grub_log->next_off = sizeof(*grub_log);
  
}

void
grub_log_add_msg(int level, const char *file, const int line, const char *fmt, ...)
{
  struct bootloader_log_msgs *msgs;
  va_list args;
  uint32_t max_len;
  int act_len;
  int type_off;

  msgs = (struct bootloader_log_msgs *) ((uint8_t *)grub_log + grub_log->next_off);
  msgs->level = level;
  msgs->facility = 0;
  grub_log->next_off += sizeof(*msgs);
 
  printf("Writing type\n");
  do
    {
      max_len = grub_log->size - grub_log->next_off;
      act_len = snprintf(msgs->type, max_len, "%s", file);

      /*Check if more space is needed*/
      if(act_len >= max_len)
      {
        grub_log = realloc(grub_log, grub_log->size + BL_SIZE_INIT);
        grub_log->size = grub_log->size + BL_SIZE_INIT;
      }  
    }
  while(act_len >= max_len);
 
  grub_log->next_off += act_len + 1;
  type_off = act_len;

  printf("Writing msg beginning\n");
  do
    {
      max_len = grub_log->size - grub_log->next_off;
      act_len = snprintf(msgs->type, max_len, "%s:%d: ", file, line);

      /*Check if more space is needed*/
      if(act_len >= max_len)
      {
        grub_log = realloc(grub_log, grub_log->size + BL_SIZE_INIT);
        grub_log->size = grub_log->size + BL_SIZE_INIT;
      }  
    }
  while(act_len >= max_len);
 
  grub_log->next_off += act_len;
  type_off = act_len;

  printf("Writing variable format\n");
  do
    {
      max_len = grub_log->size - grub_log->next_off;
      va_start(args, fmt);
      act_len = vsnprintf(msgs->type + type_off, max_len, fmt, args);
      va_end(args);    

      /*Check if more space is needed*/
      if(act_len >= max_len)
      {
        grub_log = realloc(grub_log, grub_log->size + BL_SIZE_INIT);
        grub_log->size = grub_log->size + BL_SIZE_INIT;
      }  
    }
  while(act_len >= max_len);
 
  grub_log->next_off += act_len + 1; 
}

int 
main()
{
  struct bootloader_log_msgs *msgs;
  
  grub_log_init();
  msgs = (struct bootloader_log_msgs *) ((uint8_t *)grub_log + grub_log->next_off);
  
  grub_log_add_msg(1, "Linux", 20, "%s %s", "Hello", "World!");
  grub_log_add_msg(2, "TBOOT", 145, "%s %s", "Hello2 ", "World2!");
  
  printf("Log Buffer Header\n");
  printf("Version: %i\n", grub_log->version);
  printf("Producer: %i\n", grub_log->producer);
  printf("Size: %i\n", grub_log->size);
  printf("Next_off: %i\n\n", grub_log->next_off);

  printf("Log Buffer1\n");
  printf("Level: %i\n", msgs->level);
  printf("Facility: %i\n", msgs->facility);
  printf("Type: %s\n\n", msgs->type);
  //printf("Msg: %s\n", grub_log.log_msg[0].msg);

  msgs = (struct bootloader_log_msgs *) ((uint8_t *)grub_log + grub_log->next_off);
  printf("Log Buffer2\n");
  printf("Level: %i\n", msgs->level);
  printf("Facility: %i\n", msgs->facility);
  printf("Type: %s\n", msgs->type);
  //printf("Msg: %s\n", grub_log.log_msg[1].msg); 

  printf("DONE!\n");
  return 0;
}
