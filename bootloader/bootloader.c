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

#include "bootloader.h"

int bootloader_env_set(const char *name, const char *value)
{
#ifdef CONFIG_BOOTLOADER_UBOOT
	return fw_set_one_env(name, value);
#endif /* CONFIG_BOOTLOADER_UBOT */

#ifdef CONFIG_BOOTLOADER_GRUB
	return grubenv_set(name, value);
#endif /* CONFIG_BOOTLOADER_GRUB */
}

int bootloader_env_unset(const char *name)
{
#ifdef CONFIG_BOOTLOADER_UBOOT
	return fw_set_one_env(name, "");
#endif /* CONFIG_BOOTLOADER_UBOT */

#ifdef CONFIG_BOOTLOADER_GRUB
	return grubenv_unset(name);
#endif /* CONFIG_BOOTLOADER_GRUB */
}

int bootloader_apply_list(const char *script)
{
#ifdef CONFIG_BOOTLOADER_UBOOT
	return fw_parse_script((char *)BOOTLOADER_SCRIPT, fw_env_opts);
#endif /* CONFIG_BOOTLOADER_UBOT */

#ifdef CONFIG_BOOTLOADER_GRUB
	return grubenv_apply_list(script);
#endif /* CONFIG_BOOTLOADER_GRUB */
}

char *bootloader_env_get(const char *name)
{
#ifdef CONFIG_BOOTLOADER_UBOOT
	return fw_get_one_env(name);
#endif /* CONFIG_BOOTLOADER_UBOT */

#ifdef CONFIG_BOOTLOADER_GRUB
	return grubenv_get(name);
#endif /* CONFIG_BOOTLOADER_GRUB */
}
