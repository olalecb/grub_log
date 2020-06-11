#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#define BL_SIZE_INIT 4096
#define GRUB_PACKED __attribute__((packed))
#define GRUB_ERR_NONE 0
#define GRUB_ERR_OUT_OF_MEMORY 3
#define GRUB_ERR_BUG 38

struct bootloader_log_msg
{
  uint32_t level;
  uint32_t facility;
  char type[];
  /* char msg[]; */
} GRUB_PACKED;
typedef struct bootloader_log_msg bootloader_log_msg_t;

struct bootloader_log
{
  uint32_t version;
  uint32_t producer;
  uint32_t size;
  uint32_t next_off;
  bootloader_log_msg_t msgs[];
} GRUB_PACKED;
typedef struct bootloader_log bootloader_log_t;

static bootloader_log_t *grub_log;

int
grub_log_init (void)
{
  grub_log = calloc (1, BL_SIZE_INIT);
 
  if (grub_log == NULL)
    return GRUB_ERR_OUT_OF_MEMORY;
 
  grub_log->version = 1;
  grub_log->producer = 1; 
  grub_log->size = BL_SIZE_INIT;
  grub_log->next_off = sizeof (*grub_log); 
 
  return 0;
}

int
grub_log_realloc (void)
{
  bootloader_log_t *realloc_log;
  uint32_t size;  

  /* printf ("Realloc\n"); */

  size = grub_log->size + BL_SIZE_INIT;
  realloc_log = realloc (grub_log, size);
  
  if (realloc_log == NULL)
    return GRUB_ERR_OUT_OF_MEMORY; 

  grub_log = realloc_log;
  memset ((uint8_t *) grub_log + grub_log->size, 0, BL_SIZE_INIT);
  grub_log->size = size;  

  return 0;
}

uint32_t
grub_log_write_msg (char *msg, va_list prev_args, const char *fmt, ...)
{
  va_list args;
  uint32_t max_len;
  uint32_t act_len;

  do
    {
      if (prev_args == NULL)
        va_start (args, fmt);
      else
        va_copy (args, prev_args);

      max_len = grub_log->size - grub_log->next_off;
      act_len = vsnprintf (msg, max_len, fmt, args);
      va_end (args);

      if (act_len >= max_len)
        {
  	  if (grub_log_realloc () != GRUB_ERR_NONE)
	    return -1;

	  msg = (char *) ((uint8_t *) grub_log + grub_log->next_off);
        }
    }
  while (act_len >= max_len);

  return act_len;
}

int
grub_log_add_msg (uint32_t level, const char *file, const int line, const char *fmt, ...)
{
  bootloader_log_msg_t *msgs;
  va_list args;
  uint32_t act_len;
  uint32_t type_off;

  if (grub_log == NULL)
    return GRUB_ERR_BUG;

  if (grub_log->next_off + sizeof (*msgs) >= grub_log->size)
    grub_log_realloc ();

  msgs = (bootloader_log_msg_t *) ((uint8_t *) grub_log + grub_log->next_off);
  msgs->level = level;
  /* msgs->facility = 0; */
  grub_log->next_off += sizeof (*msgs);

  /* printf ("Writing type\n"); */
  act_len = grub_log_write_msg (msgs->type, NULL, "%s", file);
  if (act_len == -1)
    return GRUB_ERR_OUT_OF_MEMORY;
  
  grub_log->next_off += act_len + 1;
  type_off = act_len + 1;

  /* printf ("Writing msg beginning\n"); */
  act_len = grub_log_write_msg (msgs->type + type_off, NULL, "%d: ", line);
  if (act_len == -1)
    return GRUB_ERR_OUT_OF_MEMORY;

  grub_log->next_off += act_len;
  type_off += act_len;

 /*  printf ("Writing variable format\n"); */
  va_start (args, fmt);
  act_len = grub_log_write_msg (msgs->type + type_off, args, fmt);
  va_end (args);
  if (act_len == -1)
    return GRUB_ERR_OUT_OF_MEMORY;

  grub_log->next_off += act_len + 1;

  return 0;
}

void
grub_log_print (void)
{
  uint32_t offset;
  uint32_t type_len;
  uint32_t msg_len;
  bootloader_log_msg_t *msgs;

  if (grub_log == NULL)
    return;

  printf ("Version: %i\n", grub_log->version);
  printf ("Producer: %i\n", grub_log->producer);
  printf ("Size: %i\n", grub_log->size);
  printf ("Next_off: %i\n\n", grub_log->next_off);

  offset = sizeof (*grub_log);

  while (offset < grub_log->next_off)
    {
      msgs = (bootloader_log_msg_t *) ((uint8_t *) grub_log + offset);

      printf ("Level: %i\n", msgs->level);
      printf ("Facility: %i\n", msgs->facility);
      offset += sizeof (*msgs);

      printf ("Type: %s\n", msgs->type);
      type_len = strlen (msgs->type) + 1;

      printf ("Msg: %s\n\n", msgs->type + type_len);
      msg_len = strlen (msgs->type + type_len) + 1;
      offset += type_len + msg_len;
    }
}

void
grub_log_write_file (char *filename)
{
  uint32_t log_fd;

  if (grub_log == NULL)
    return;

  log_fd = open (filename, O_WRONLY | O_TRUNC | O_CREAT);
  
  if (log_fd < 0)
    return;

  write (log_fd, grub_log, grub_log->size);

  close (log_fd);
}

void
grub_log_read_file (char *filename)
{
  uint32_t log_fd;
  uint32_t file_size;
 
  log_fd = open (filename, O_RDONLY);

  if (log_fd < 0)
    return;

  file_size = lseek (log_fd, 0, SEEK_END);
  lseek (log_fd, 0, SEEK_SET);

  grub_log = malloc (file_size);

  read (log_fd, grub_log, file_size);

  close (log_fd);
}

int 
main (void)
{
  bootloader_log_msg_t *msgs;
  int i; 

  //grub_log_read_file ("log_file");

  grub_log_init ();
   
  for (i = 0; i<50; i++)
    grub_log_add_msg (i, __FILE__, __LINE__, "%s %s:%i", "Hello", "World!", i);

  grub_log_add_msg (0, __FILE__, __LINE__, "%s %s:%i", "Hello", "World!", __LINE__);
  grub_log_add_msg (0, __FILE__, __LINE__, "%s %s:%i", "Hello", "World!", __LINE__);
  
  grub_log_write_file ("log_file");

  grub_log_print ();

  /*printf ("Log Buffer Header Before\n");
  printf ("Version: %i\n", grub_log->version);
  printf ("Producer: %i\n", grub_log->producer);
  printf ("Size: %i\n", grub_log->size);
  printf ("Next_off: %i\n\n", grub_log->next_off);

  msgs = (bootloader_log_msg_t *) ((uint8_t *) grub_log + grub_log->next_off);
  grub_log_add_msg (1, "Linux", 20, "Hello World!");

  printf ("Log Buffer 1\n");
  printf ("Level: %i\n", msgs->level);
  printf ("Facility: %i\n", msgs->facility);
  printf ("Type: %s\n", msgs->type);
  printf ("Msg: %s\n\n", msgs->type + strlen (msgs->type) + 1);

  msgs = (bootloader_log_msg_t *) ((uint8_t *) grub_log + grub_log->next_off); 
  grub_log_add_msg (2, "TBOOT", 177, "%s %s:%i", "Hello", "World!", 2);
  printf ("Log Buffer 2\n");
  printf ("Level: %i\n", msgs->level);
  printf ("Facility: %i\n", msgs->facility);
  printf ("Type: %s\n", msgs->type); 
  printf ("Msg: %s\n\n", msgs->type + strlen (msgs->type) + 1);

  printf ("Log Buffer Header After\n");
  printf ("Version: %i\n", grub_log->version);
  printf ("Producer: %i\n", grub_log->producer);
  printf ("Size: %i\n", grub_log->size);
  printf ("Next_off: %i\n\n", grub_log->next_off);
  
  printf("DONE!\n");*/
  return 0;
}
