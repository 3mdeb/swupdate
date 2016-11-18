#include "grub_env.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

grub_envblk_t
grub_envblk_open (char *buf, size_t size)
{
  grub_envblk_t envblk;

  if (size < sizeof (GRUB_ENVBLK_SIGNATURE)
      || memcmp (buf, GRUB_ENVBLK_SIGNATURE,
                      sizeof (GRUB_ENVBLK_SIGNATURE) - 1))
    {
      fprintf(stderr, "GRUB: invalid environment block");
      return 0;
    }

  envblk = malloc (sizeof (*envblk));
  if (envblk)
    {
      envblk->buf = buf;
      envblk->size = size;
      envblk->fname = GRUB_ENVBLK_PATH;
    }

  return envblk;
}


void
grub_envblk_close (grub_envblk_t envblk)
{
  free (envblk->buf);
  free (envblk);
}

static int
escaped_value_len (const char *value)
{
  int n = 0;
  char *p;

  for (p = (char *) value; *p; p++)
    {
      if (*p == '\\' || *p == '\n')
        n += 2;
      else
        n++;
    }

  return n;
}

static char *
find_next_line (char *p, const char *pend)
{
  while (p < pend)
    {
      if (*p == '\\')
        p += 2;
      else if (*p == '\n')
        break;
      else
        p++;
    }

  return p + 1;
}

int
grub_envblk_set (grub_envblk_t envblk, const char *name, const char *value)
{
  char *p, *pend;
  char *space; // free space (chars) left in grubenv
  int found = 0;
  int nl; // length of name
  int vl; // length of value
  int i;

  nl = strlen (name);
  vl = escaped_value_len (value);
  /*set pointer at the begging of the first variable name */
  p = envblk->buf + sizeof (GRUB_ENVBLK_SIGNATURE) - 1;
  pend = envblk->buf + envblk->size;

  /* First, look at free space. Empty byte is saved as '#' */
  for (space = pend - 1; *space == '#'; space--)
    ;

  if (*space != '\n')
    /* Broken.  */
    return 0;

  space++;

  while (p + nl + 1 < space)
    {
      if (memcmp (p, name, nl) == 0 && p[nl] == '=')
        {
          int len;

          /* Found the same name.  */
          /* set pointer at corresponding value */
          p += nl + 1;

          /* Check the length of the current value.  */
          len = 0;
          while (p + len < pend && p[len] != '\n')
            {
              if (p[len] == '\\')
                len += 2;
              else
                len++;
            }

          if (p + len >= pend)
            /* Broken.  */
            return 0;

          if (pend - space < vl - len)
            /* No space.  */
            return 0;

          if (vl < len)
            {
              /* Move the following characters backward, and fill the new
                 space with harmless characters.  */
              memmove (p + vl, p + len, pend - (p + len));
              memset (space + len - vl, '#', len - vl);
            }
          else
            /* Move the following characters forward.  */
            memmove (p + vl, p + len, pend - (p + vl));

          found = 1;
          break;
        }

      p = find_next_line (p, pend);
    }

  if (! found)
    {
      /* Append a new variable.  */

      if (pend - space < nl + 1 + vl + 1)
        /* No space.  */
        return 0;

      memcpy (space, name, nl);
      p = space + nl;
      *p++ = '=';
    }

  /* Write the value.  */
  for (i = 0; value[i]; i++)
    {
      if (value[i] == '\\' || value[i] == '\n')
        *p++ = '\\';

      *p++ = value[i];
    }

  *p = '\n';
  return 1;
}

void
grub_envblk_delete (grub_envblk_t envblk, const char *name)
{
  char *p, *pend;
  int nl; // length of name

  nl = strlen (name);
  p = envblk->buf + sizeof (GRUB_ENVBLK_SIGNATURE) - 1;
  pend = envblk->buf + envblk->size;

  while (p + nl + 1 < pend)
    {
      if (memcmp (p, name, nl) == 0 && p[nl] == '=')
        {
          /* Found.  */
          int len = nl + 1;

          while (p + len < pend)
            {
              if (p[len] == '\n')
                break;
              else if (p[len] == '\\')
                len += 2;
              else
                len++;
            }

          if (p + len >= pend)
            /* Broken.  */
            return;

          len++;
          memmove (p, p + len, pend - (p + len));
          memset (pend - len, '#', len);
          break;
        }

      p = find_next_line (p, pend);
    }
}

static grub_envblk_t
open_envblk_file (char *fname)
{
  FILE *fp;
  char *buf;
  size_t size;
  grub_envblk_t envblk;

  fp = fopen (fname, "rb");
  if (! fp)
    {
    // should we create env file if it's missing ?
    //  /* Create the file implicitly.  */
    //  grub_util_create_envblk_file (name);
    //  fp = grub_util_fopen (name, "rb");
    //  if (! fp)
    //    grub_util_error (_("cannot open `%s': %s"), name,
	//		 strerror (errno));
        fprintf(stderr, "GRUB: grubenv file is missing\n");
    }

  if (fseek (fp, 0, SEEK_END) < 0)
    fprintf(stderr, "GRUB: cannot seek %s", fname);

  size = (size_t) ftell (fp);

  if (fseek (fp, 0, SEEK_SET) < 0)
    fprintf(stderr, "GRUB: cannot seek %s", fname);

  buf=malloc(size);
  if (!buf)
  {
        fprintf(stderr, "No memory: malloc failed\n");
        return NULL;
  }

  if (fread (buf, 1, size, fp) != size)
    fprintf(stderr, "cannot read %s", fname);

  fclose (fp);

  envblk = grub_envblk_open (buf, size);
  if (! envblk)
    fprintf(stderr, "invalid environment block");

  return envblk;
}

static void
write_envblk (const char *fname, grub_envblk_t envblk)
{
  FILE *fp;

  fp = fopen (fname, "wb");
  if (! fp)
    fprintf(stderr, "cannot open %s", fname);

  if (fwrite (grub_envblk_buffer (envblk), 1, grub_envblk_size (envblk), fp)
      != grub_envblk_size (envblk))
    fprintf(stderr, "cannot write to %s", fname);

  static int allow_fd_syncs = 1;

  fflush (fp);
  if (allow_fd_syncs) fsync (fileno (fp));

  fclose (fp);
}

static void
set_variables (char *fname, char *name, char *value)
{
  grub_envblk_t envblk;

  envblk = open_envblk_file (fname);
  //while (argc)
  //  {
  //    char *p;

  //    p = strchr (argv[0], '=');
  //    if (! p)
  //      grub_util_error (_("invalid parameter %s"), argv[0]);

  //    *(p++) = 0;

  //    if (! grub_envblk_set (envblk, argv[0], p))
  //      grub_util_error ("%s", _("environment block too small"));

  //    argc--;
  //    argv++;
  //  }
  //
  if (!grub_envblk_set (envblk, name, value))
      fprintf(stderr, "environment block too small");

  write_envblk (name, envblk);
  grub_envblk_close (envblk);
}

static void
unset_variables (char *fname, char *name, char *value)
{
  grub_envblk_t envblk;

  envblk = open_envblk_file (fname);
      grub_envblk_delete (envblk, name);
  write_envblk (name, envblk);
  grub_envblk_close (envblk);
}
