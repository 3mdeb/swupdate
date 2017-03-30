/*
 * Author: Maciej Pijanowski, maciej.pijanowski@3mdeb.com
 * Copyright (C) 2017, 3mdeb
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.
 */

#define GRUBENV_SIZE 1024 /* bytes */
#define GRUBENV_HEADER "# GRUB Environment Block\n"
#define GRUBENV_DEFAULT_PATH "/boot/efi/EFI/BOOT/grub/grubenv"

#ifdef CONFIG_GRUBENV
#define GRUBENV_PATH CONFIG_GRUBENV
#else
#define GRUBENV_PATH GRUBENV_DEFAULT_PATH
#endif

/* Internal functions */
/* Exposed here for unit tests during development */
char *grubenv_open(const char *grubenv_file);
char *grubenv_find(const char *grubenv, const char *name);
void grubenv_remove(char *grubenv, char *ptrline, int space);
void grubenv_close(char *grubenv);
int grubenv_append(char *grubenv, const char *name, const char *value,
		   int space);
int grubenv_space(char *grubenv);
int grubenv_write(const char *grubenv_file, const char *grubenv);
int grubenv_llen(char *ptrline);

/* only these should be called from external */
int grubenv_set(const char *grubenv_file, const char *name, const char *value);
int grubenv_unset(const char *grubenv_file, const char *name);
