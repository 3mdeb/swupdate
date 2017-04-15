/*
 * Author: Maciej Pijanowski maciej.pijanowski@3mdeb.com
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "util.h"
#include "grubenv.h"
#include "swupdate_dict.h"


/* read environment from storage into RAM */
static int grubenv_open(struct grubenv_t *grubenv)
{
	FILE *fp = NULL;
	size_t size;
	int ret = 0, counter = 0;
	char *buf = NULL, *key, *value;

	fp = fopen(GRUBENV_PATH, "rb");
	if (!fp) {
		ERROR("Failed to open grubenv file: %s\n", GRUBENV_PATH);
		ret = -1;
		goto cleanup;
	}

	if (fseek(fp, 0, SEEK_END)) {
		ERROR("Failed to seek end grubenv file: %s\n", GRUBENV_PATH);
		ret = -1;
		goto cleanup;
	}

	size = (size_t)ftell(fp);

	if (size != GRUBENV_SIZE) {
		ERROR("Ivalid grubenv file size: %d\n", (int)size);
		ret = -1;
		goto cleanup;
	}

	if (fseek(fp, 0, SEEK_SET)) {
		ERROR("Failed to seek set grubenv file: %s\n", GRUBENV_PATH);
		ret = -1;
		goto cleanup;
	}

	buf = malloc(size);
	if (!buf) {
		ERROR("Not enough memory for environment\n");
		fclose(fp);
		ret = -ENOMEM;
		goto cleanup;
	}

	if (fread(buf, 1, size, fp) != size) {
		ERROR("Failed to read file %s\n", GRUBENV_PATH);
		ret = 1;
		goto cleanup;
	}

	if (memcmp(buf, GRUBENV_HEADER, sizeof(GRUBENV_HEADER) -1)) {
		ERROR("Invalid grubenv header\n");
		ret = -1;
		goto cleanup;
	}

	/* truncate header, prepare buf for further splitting */
	strtok(buf, "\n");

	while (counter < 1024) {
		key = strtok(NULL, "=");
		value = strtok(NULL, "\n");
		counter++;
		if (value != NULL && key != NULL) {
			ret = dict_set_value(&grubenv->vars, key, value);
			if (ret) {
				ERROR("Adding pair [%s] = %s into dictionary \
						list failed\n", key, value);
				return ret;
			}
			DEBUG("Added to grubenv dict list: [%s] = %s\n", key,
					value);
		}
		else
			break;
	}

cleanup:
	if (fp) fclose(fp);
	if (buf) free(buf);
	return ret;
}

static int grubenv_parse_script(struct grubenv_t *grubenv, const char *script)
{
	FILE *fp;
	int ret = 0;
	char *line, *key, *value;
	size_t len = 0;

	/* open script generated during sw-description parsing */
	fp = fopen(script, "rb");
	if (!fp) {
		ERROR("Failed to open grubenv script file: %s\n", script);
		ret = -1;
		goto cleanup;
	}

	/* load  varname-value pairs from script into grubenv dictlist */
	while ((getline(&line, &len, fp)) != -1) {
		key = strtok(line, " \t\n");
		value = strtok(NULL, " \t\n");
		if (value != NULL && key != NULL) {
			ret = dict_set_value(&grubenv->vars, key, value);
			if (ret) {
				ERROR("Adding pair [%s] = %s into dictionary \
						list failed\n", key, value);
				goto cleanup;
			}
			DEBUG("Added to grubenv dict list: [%s] = %s\n", key,
					value);
		}
	}

cleanup:
	if (fp) fclose(fp);
	if (line) free(line);
	return ret;
}

/* I'm not sure about size member of grubenv_t struct
 * It seems to me that it is enough if we check size of env stored in dict list
 * only once, just before writing into grubenv file.
 */
static inline void grubenv_update_size(struct grubenv_t *grubenv)
{
	int size = 0;
	struct dict_entry *grubvar;

	LIST_FOREACH(grubvar, &grubenv->vars, next) {
		size = strlen(grubvar->varname) + strlen(grubvar->value) + 2;
	}
	size += strlen(GRUBENV_HEADER);
	grubenv->size = size;
}

static int grubenv_write(struct grubenv_t *grubenv)
{
	FILE *fp = NULL;
	char *buf = NULL, *ptr;
	struct dict_entry *grubvar;
	int ret = 0, nlen = 0, vlen = 0;

	grubenv_update_size(grubenv);
	if (grubenv->size > GRUBENV_SIZE) {
		ERROR("Not enough free space in envblk file, %ld",
			grubenv->size);
		ret = -1;
		goto cleanup;
	}

	fp = fopen(GRUBENV_PATH_NEW, "wb");
	if (!fp) {
		ERROR("Failed to open file: %s", GRUBENV_PATH_NEW);
		ret = -1;
		goto cleanup;
	}

	buf = malloc(GRUBENV_SIZE);
	if (!buf) {
		ERROR("Not enough memory for environment\n");
		ret = -ENOMEM;
		goto cleanup;
	}

	memcpy(buf, GRUBENV_HEADER, strlen(GRUBENV_HEADER));
	ptr = buf + strlen(GRUBENV_HEADER);

	LIST_FOREACH(grubvar, &grubenv->vars, next) {
		nlen = strlen(grubvar->varname);
		vlen = strlen(grubvar->value);
		memcpy(ptr, grubvar->varname, nlen);
		ptr += nlen;
		memcpy(ptr, "=", 1);
		ptr++;
		memcpy(ptr, grubvar->value, vlen);
		ptr += vlen;
		memcpy(ptr, "\n", 1);
		ptr++;
	}

	/* fill with '#' from current ptr position up to the end of block */
	memset(ptr, '#', buf + GRUBENV_SIZE - 1 - ptr);
	DEBUG("number of chars + number of #'s = %ld\n", grubenv->size +
			(buf + GRUBENV_SIZE - 1 - ptr));

	ret = fwrite(buf , 1, GRUBENV_SIZE, fp);
	if (ret != GRUBENV_SIZE) {
		ERROR("Failed to write file: %s. Bytes written: %d\n",
			GRUBENV_PATH_NEW, ret);
		ret = -1;
		goto cleanup;
	}

	if (rename(GRUBENV_PATH_NEW, GRUBENV_PATH)) {
		ERROR("Failed to move environment: %s into %s\n",
			GRUBENV_PATH_NEW, GRUBENV_PATH);
		ret = -1;
		goto cleanup;
	}

	ret = 0;

cleanup:
	if (fp) fclose(fp);
	if (buf) free(buf);
	return ret;
}

/* I'm not sure what would be the proper method to free memory from dict list
 * allocation */
static inline void grubenv_close(struct grubenv_t *grubenv)
{
	struct dict_entry *grubvar;

	LIST_FOREACH(grubvar, &grubenv->vars, next) {
		dict_remove(&grubenv->vars, grubvar->varname);
	}
}

int grubenv_set(const char *name, const char *value)
{
	static struct grubenv_t grubenv;
	int ret;

	/* read env into dictionary list in RAM */
	if (ret = grubenv_open(&grubenv))
		goto cleanup;

	/* set new variable or change value of existing one */
	if (ret = dict_set_value(&grubenv.vars, (char *)name, (char *)value))
		goto cleanup;

	/* form grubenv format out of dictionary list and save it to file */
	if (ret = grubenv_write(&grubenv))
		goto cleanup;

cleanup:
	grubenv_close(&grubenv);
	return ret;
}

int grubenv_unset(const char *name)
{
	static struct grubenv_t grubenv;
	int ret = 0;

	/* read env into dictionary list in RAM */
	if (ret = grubenv_open(&grubenv))
		goto cleanup;

	/* remove entry from dictionary list */
	dict_remove(&grubenv.vars, (char *)name);

	/* form grubenv format out of dictionary list and save it to file */
	if (ret = grubenv_write(&grubenv))
		goto cleanup;

cleanup:
	grubenv_close(&grubenv);
	return ret;
}

char *grubenv_get(const char *name)
{
	static struct grubenv_t grubenv;
	char *value;
	int ret = 0;

	/* read env into dictionary list in RAM */
	if (ret = grubenv_open(&grubenv))
		goto cleanup;

	/* retrieve value of given variable from dictionary list */
	value = dict_get_value(&grubenv.vars, (char *)name);

	/* form grubenv format out of dictionary list and save it to file */
	if (ret = grubenv_write(&grubenv))
		goto cleanup;

	grubenv_close(&grubenv);
	return value;

cleanup:
	grubenv_close(&grubenv);
	return NULL;
}

int grubenv_apply_list(const char *script)
{
	static struct grubenv_t grubenv;
	int ret = 0;

	/* read env into dictionary list in RAM */
	if (ret = grubenv_open(&grubenv))
		goto cleanup;

	/* add variables from sw-description into dict list */
	if (ret = grubenv_parse_script(&grubenv, script))
		goto cleanup;

	/* form grubenv format out of dictionary list and save it to file */
	if (ret = grubenv_write(&grubenv))
		goto cleanup;

cleanup:
	grubenv_close(&grubenv);
	return ret;
}
