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
#include "grubenv.h"


/* read environment from storage into RAM */
char *grubenv_open(const char *grubenv_file)
{
	char *grubenv;
	FILE *fp;
	size_t size;

	fp = fopen(grubenv_file, "rb");
	if (!fp) {
		ERROR("Failed to open grubenv file: %s\n", grubenv_file);
		return NULL;
	}

	if (fseek(fp, 0, SEEK_END)) {
		ERROR("Failed to seek end grubenv file: %s\n", grubenv_file);
		return NULL;
	}

	size = (size_t)ftell(fp);

	if (size != GRUBENV_SIZE) {
		ERROR("Ivalid grubenv file size: %d\n", (int)size);
		return NULL;
	}

	if (fseek(fp, 0, SEEK_SET)) {
		ERROR("Failed to seek set grubenv file: %s\n", grubenv_file);
		return NULL;
	}

	grubenv = malloc(size);
	if (!grubenv) {
		ERROR("Not enough memory for environment\n");
		return NULL;
	}

	if (fread(grubenv, 1, size, fp) != size) {
		ERROR("Failed to read %s\n", grubenv);
		return NULL;
	}

	fclose(fp);

	if (memcmp(grubenv, GRUBENV_HEADER, sizeof(GRUBENV_HEADER) -1)) {
		ERROR("Invalid grubenv header\n");
		return NULL;
	}

	return grubenv;
}

char *grubenv_find(const char *grubenv, const char *name)
{
	char *ptrline, *string;

	/* form string "name=" */
	/* space for '=' and '\0' */
	string = malloc(strlen(name) + 2);
	if (!string) {
		ERROR("Not enough memory\n");
		return NULL;
	}
	/* copy with '\0' */
	strncpy(string, name, strlen(name) + 1);
	/* '\0' is always appended */
	strncat(string, "=", 1);
	ptrline = strstr(grubenv, string);
	free(string);

	/* return pointer to the beginning of found line */
	return ptrline;
}

int grubenv_llen(const char *grubenv, char *ptrline)
{
	char *ptrstart = ptrline;
	while (*ptrline != '\n') {
		ptrline++;
	}
	return ptrline - ptrstart + 1;
}

void grubenv_remove(char *grubenv, char *ptrline, int space)
{
	int llen = grubenv_llen(grubenv, ptrline);
	char *ptrend = grubenv + GRUBENV_SIZE - 1;

	/* copy block after given line at the beginning of this line */
	memmove(ptrline, ptrline + llen, ptrend - ptrline - llen);
	/* fill space after last variable with # */
	memset(ptrend - space - llen + 1, '#', space + llen);
}

int grubenv_space(char *grubenv)
{
	char *ptr, *ptrend;
	int space;

	/* pointer to the last character */
	ptrend = grubenv + GRUBENV_SIZE - 1;
	ptr = ptrend;

	/* empty space in bytes - actually it's number of # characters */
	space = 0;
	while(*ptr == '#') {
		ptr--;
		space++;
	}

	return space;
}

/* append single variable at the end of envblock stored in RAM */
int grubenv_append(char *grubenv, const char *name, const char *value,
	           int space)
{
	int nlen, vlen, llen, counter;
	char *ptr, *line;

	if (!name) {
		ERROR("Invalid variable name: %s\n", name);
		return -1;
	}

	/* 2 additional bytes: '=' and '\n' */
	llen = strlen(name) + strlen(value) + 2;

	/* ptr points now at first non '#' character */
	ptr = grubenv + GRUBENV_SIZE - 1 - space;

	/* it should be '\n' */
	if (*ptr != '\n') {
		ERROR("Invalid environment block\n");
		return -1;
	}

	/* one byte forward - beginning of a new line */
	ptr++;

	/* formatted name=value string */
	line = malloc(llen);
	if (!line) {
		ERROR("Not enough memory\n");
		return -ENOMEM;
	}
	snprintf(line, llen+1, "%s=%s\n", name, value);
	memcpy(ptr, line, llen);
	free(line);

	return 0;
}

int grubenv_write(const char *grubenv_file, const char *grubenv)
{
	FILE *fp;
	char *grubenv_file_new;
	int ret;

	/* 5 bytes for '.new' and '\0' */
	grubenv_file_new = malloc(strlen(grubenv_file) + 5);
	if (!grubenv_file_new) {
		ERROR("Not enough memory\n");
		return -ENOMEM;
	}
	/* copy with '\0' */
	strncpy(grubenv_file_new, grubenv_file, strlen(grubenv_file) + 1);
	/* copy '.new' string, '\0' is always appended */
	strncat(grubenv_file_new, ".new", 4);

	fp = fopen(grubenv_file_new, "wb");
	if (!fp) {
		ERROR("Failed to open file: %s", grubenv_file_new);
		return -1;
	}

	ret = fwrite(grubenv, 1, GRUBENV_SIZE, fp);
	if (ret != GRUBENV_SIZE) {
		ERROR("Failed to write file: %s. Bytes written: %d\n",
			grubenv_file_new, ret);
		return -1;
	}

	fclose(fp);

	if (rename(grubenv_file_new, grubenv_file)) {
		ERROR("Failed to move environment: %s into %s\n",
			grubenv_file_new, grubenv_file);
		return -1;
	}

	free(grubenv_file_new);
	return 0;
}

int grubenv_set(const char *grubenv_file, const char *name, const char *value)
{
	char *grubenv;
	char *found;
	int llen, space, ret;

	grubenv = grubenv_open(grubenv_file);
	if (!grubenv) {
		ret = -1;
		goto fail;
	}
	space = grubenv_space(grubenv);

	found = grubenv_find(grubenv, name);
	/* variable already exists in environment */
	if (found) {
		/* length of line to be removed */
		llen = grubenv_llen(grubenv, found);
		/* just remove line with found variable */
		grubenv_remove(grubenv, found, space);
		space += llen;
	}
	/* length of line to be added */
	/* 2 additional bytes: '=' and '\n' */
	llen = strlen(name) + strlen(value) + 2;

	if (llen < space){
		/* append new variable at the end */
		ret = grubenv_append(grubenv, name, value, space);
		if (ret)
			goto fail;
		ret = grubenv_write(grubenv_file, grubenv);
		if (ret)
			goto fail;
	}
	else {
		ret = -1;
		ERROR("Not enough free space in envblk file. Available: %d \
			, required %d", space, llen);
		goto fail;
	}

	grubenv_close(grubenv);
	return 0;

	fail:
		grubenv_close(grubenv);
		return ret;
}

int grubenv_unset(const char *grubenv_file, const char *name)
{
	char *grubenv;
	char *found;
	int space, ret;

	grubenv = grubenv_open(grubenv_file);
	if (!grubenv) {
		ret = -1;
		goto fail;
	}

	found = grubenv_find(grubenv, name);
	/* variable already exists in environment */
	if (found) {
		/* how many '#' characters to set after var removal */
		space = grubenv_space(grubenv);
		space += grubenv_llen(grubenv, found);
		/* remove line with found variable */
		grubenv_remove(grubenv, found, space);
		ret = grubenv_write(grubenv_file, grubenv);
		if (ret)
			goto fail;
	}
	/*
	else {
		 log if variable not found ?
	}
	*/

	grubenv_close(grubenv);
	return 0;

	fail:
		grubenv_close(grubenv);
		return ret;
}

/* free environment data from RAM */
/* ATM it's just for completeness */
void grubenv_close(char *grubenv)
{
	free(grubenv);
}
