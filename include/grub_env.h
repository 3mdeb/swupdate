#define GRUB_ENVBLK_SIGNATURE	"# GRUB Environment tBlock\n"
#define GRUB_DEFAULT_ENVBLK_PATH "/boot/efi/EFI/BOOT/grub/"

#ifdef CONFIG_GRUB_ENV
#define GRUB_ENVBLK_PATH CONFIG_GRUB_ENV
#else
#define GRUB_ENVBLK_PATH GRUB_DEFAULT_ENVBLK_PATH
#endif

struct grub_envblk
{
  char *buf;
  size_t size;
  char *fname;
};

//int grub_parse_script(char *name);
typedef struct grub_envblk *grub_envblk_t;
grub_envblk_t grub_envblk_open (char *buf, size_t size);
int grub_envblk_set (grub_envblk_t envblk, const char *name, const char *value);
void grub_envblk_delete (grub_envblk_t envblk, const char *name);
void grub_envblk_close (grub_envblk_t envblk);

static grub_envblk_t grub_open_envblk_file (char *fname);
static void grub_write_envblk (char *fname, grub_envblk_t envblk);
static void grub_set_variables (char *fname, char *name, char value);
static void grub_unset_variables (char *fname, char *name, char *value);

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

