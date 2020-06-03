#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>

#define BL_SIZE_INIT 4096
#define GRUB_PACKED __attribute__((packed))

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

void
grub_log_init (void)
{
  /* zalloc(size) should be the same as calloc(1, size) */
  grub_log = calloc (1, BL_SIZE_INIT);
  if (grub_log == NULL)
  {
    /* Handle NULL case */
    return;
  }
  grub_log->version = 1;
  grub_log->producer = 1; 
  grub_log->size = BL_SIZE_INIT;
  grub_log->next_off = sizeof (*grub_log); 
}

uint32_t
grub_log_write_msg (char msg[], const char *fmt, va_list args)
{
  bootloader_log_t *realloc_log;
  uint32_t max_len;
  uint32_t act_len;
  uint32_t size;

  do
    {
      max_len = grub_log->size - grub_log->next_off;
      act_len = vsnprintf (msg, max_len, fmt, args);
     
      if (act_len >= max_len)
      {
        size = grub_log->size + BL_SIZE_INIT;
        realloc_log = realloc (grub_log, size);
       
	 if (realloc_log == NULL)
        {
	  /* Handle NULL case */
        }
	else
	{
	  grub_log = realloc_log;
	  memset (grub_log + grub_log->size, 0, BL_SIZE_INIT);
	  grub_log->size = size;
	}
      }
    }
  while (act_len >= max_len);

  return act_len;
}

uint32_t
grub_log_vwrite_msg (char msg[], const char *fmt, ...)
{
  va_list args;
  uint32_t act_len;

  va_start (args, fmt);
  act_len = grub_log_write_msg (msg, fmt, args);
  va_end (args);

  return act_len;
}

void
grub_log_add_msg (uint32_t level, const char *file, const int line, const char *fmt, ...)
{
  if (grub_log == NULL)
  {
    /* Handle NULL case */
    return;
  }

  bootloader_log_msg_t *msgs;
  va_list args;
  uint32_t act_len;
  uint32_t type_off;

  msgs = (bootloader_log_msg_t *) ((uint8_t *) grub_log + grub_log->next_off);
  msgs->level = level;
  /* msgs->facility = 0; */
  grub_log->next_off += sizeof (*msgs);
 
  printf ("Writing type\n");
  act_len = grub_log_vwrite_msg (msgs->type, "%s", file);
  grub_log->next_off += act_len + 1;
  type_off = act_len + 1;

  printf ("Writing msg beginning\n");
  act_len = grub_log_vwrite_msg (msgs->type + type_off, "%s:%d: ", file, line);
  grub_log->next_off += act_len;
  type_off += act_len;

  printf("Writing variable format\n");
  va_start (args, fmt);
  act_len = grub_log_write_msg (msgs->type + type_off, fmt, args);
  va_end (args);
  grub_log->next_off += act_len + 1; 
}

int 
main()
{
  bootloader_log_msg_t *msgs;
  
  grub_log_init ();
 
  printf ("Log Buffer Header Before\n");
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
  grub_log_add_msg (2, "TBOOT", 145, "%s %s:%d", "Hello", "World", 2);

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
  
  printf("DONE!\n");
  return 0;
}
