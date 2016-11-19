#include <stdlib.h>

#define GRUB_ENVBLK_SIGNATURE "# GRUB Environment Block\n"
#define GRUB_DEFAULT_ENVBLK_PATH "/boot/efi/EFI/BOOT/grub/grubenv"

#ifdef CONFIG_GRUB_ENV
#define GRUB_ENVBLK_PATH CONFIG_GRUB_ENV
#else
#define GRUB_ENVBLK_PATH GRUB_DEFAULT_ENVBLK_PATH
#endif

struct grub_envblk
{
  char *buf;
  size_t size;
};

//int grub_parse_script(char *name);
typedef struct grub_envblk *grub_envblk_t;

/* only 'set' and 'unset' are callable from external */
void grub_set_variable (char *name, char *value);
void grub_unset_variable (char *name);

static inline char *
grub_envblk_buffer (const grub_envblk_t envblk)
{
  return envblk->buf;
}

static inline size_t
grub_envblk_size (const grub_envblk_t envblk)
{
  return envblk->size;
}
