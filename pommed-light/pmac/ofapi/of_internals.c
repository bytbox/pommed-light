/***************************************************************************
 *   (C) Copyright 2006 Alastair Poole.  netstar@gatheringofgray.com)      *   
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "of_api.h"

#include <errno.h>

struct device_node *_of_return_nodes(struct device_node **array, int *idx,
				     int *sem, int type)
{
	while (*idx >= 0) {

		if (*idx == 0 || type == 0)
			*sem = 0;

		return *idx > 0 ? array[(*idx)--] : NULL;
	}
	
	return NULL;
}

char *_of_read_name(const char *path, struct device_node *node)
{
	int fd;
	char *tmp;
	char buf[PATH_MAX];
	struct stat fstats;
	uint32_t size;
	
	snprintf(buf, sizeof(buf), "%s%s", path, "name");

	if (stat(buf, &fstats) < 0)
		return NULL;

	size = fstats.st_size;

	if ((fd = open(buf, O_RDONLY)) < 0)
		return NULL;

	if (node) {
		node->name = calloc(1, size);
		if(!node->name)
			of_error("calloc()");

		tmp = node->name;
	} else {
		tmp = calloc(1, size);
		if(!tmp)
			of_error("calloc()");
	}

	if (read(fd, tmp, size) != size)
		of_error("read()");

	close(fd);

	return tmp;
}

void _of_read_type(const char *path, struct device_node *node)
{
	int fd;
	char buf[PATH_MAX];
	struct stat fstats;
	uint32_t size;
	
	snprintf(buf, sizeof(buf),"%s%s", path, "device_type");

	if (stat(buf, &fstats) < 0)
		return;

	size = fstats.st_size;

	if ((fd = open(buf, O_RDONLY)) < 0)
		return;

	node->type = calloc(1, size);
	if(!node->type)
		of_error("calloc()");

	if (read(fd, node->type, size) != size)
		of_error("read()");

	close(fd);
}

void _of_read_linux_phandle(const char *path, struct device_node *node)
{
	int fd;
	char buf[PATH_MAX];
	struct stat fstats;
	uint32_t size;
	
	snprintf(buf, sizeof(buf), "%s%s", path, "linux,phandle");

	if (stat(buf, &fstats) < 0)
		return;

	size = fstats.st_size;
	if(!size)
		return;

	if ((fd = open(buf, O_RDONLY)) < 0)
		return;

	node->linux_phandle.len = size;
	node->linux_phandle.data = calloc(1, size);

	if (read(fd, node->linux_phandle.data, size) != size)
		of_error("read()");

	close(fd);
}

void _of_get_path(char *path)
{
	char *ptr;
	char *p;
	int i;
	
	i = strlen(OF_ROOT);
	ptr = strdup(path);
	p = &ptr[i];

	sprintf(path, "%s", p);

	free(ptr);
}

struct device_node *_of_populate_node(const char *path, const char *name)
{
	struct device_node *tmp = calloc(1, sizeof(struct device_node));
	char *p = strdup(path);

	_of_remove_filename(p);

	tmp->full_path = strdup(p);

	if (name) 
		tmp->name = strdup(name);
	else
		_of_read_name(path, tmp);

	_of_get_path(p);

	tmp->path = strdup(p);
	free(p);

	_of_read_linux_phandle(tmp->full_path, tmp);
	_of_read_type(tmp->full_path, tmp);

	return tmp;
}

struct device_node *_of_get_type(const char *path, const char *type)
{
	int fd;
	char buf[PATH_MAX];
	char *name;
	char *ptr;
	struct device_node *tmp = NULL;
	struct stat fstats;
	uint32_t size;

	if (stat(path, &fstats) < 0) {
		exit(EXIT_FAILURE);
	}

	size = fstats.st_size;
	if(!size)
		return NULL;
	
	if ((fd = open(path, O_RDONLY)) < 0)
		of_error("open()");

	if (read(fd, buf, size) != size)
		of_error("read()");

	if (memcmp(buf, type, size))
		goto out;

	ptr = strdup(path);

	_of_remove_filename(ptr);

	name = _of_read_name(ptr, NULL);

	tmp = _of_populate_node(path, name);

	free(ptr);
	free(name);
	
      out:

	close(fd);

	return tmp;
}

void _of_remove_filename(char *path)
{
	char *ptr;
	ptr = strrchr(path, '/');
	*(ptr + 1) = '\0';
}

uint32_t _of_phandle_to_int(struct node_property_t phandle)
{
	uint32_t tmp = 0;

	if (phandle.len == 4)
		tmp =
		    (phandle.data[0] << 24) + (phandle.data[1] << 16) +
		    (phandle.data[2] << 8) + phandle.data[3];

	return tmp;
}

void _of_make_compat_path(const char *path, char *buf)
{
	size_t slen = strlen(path);
	int changed = 0;

	if (*path != '/')
		changed = 1;

	if(!strlen(path)) {
		snprintf(buf, PATH_MAX, "%s/", OF_ROOT);
		return;
	}

	if (path[slen - 1] != '/') {
		if (changed)
			snprintf(buf, PATH_MAX, "%s/%s/", OF_ROOT, path);
		else
			snprintf(buf, PATH_MAX, "%s%s/", OF_ROOT, path);
	} else 
		snprintf(buf, PATH_MAX, "%s%s", OF_ROOT, path);
}

struct device_node *_of_get_phandle(const char *path, const uint32_t * phandle)
{
	struct node_property_t props;
	struct device_node *tmp = NULL;
	uint32_t size, val = 0;
	struct stat fstats;
	int fd;
	char *ptr;
	char *name;

	if (stat(path, &fstats) < 0)
		return NULL;

	size = fstats.st_size;
	if(!size)
		return NULL;

	props.data = malloc(size);
	if(!props.data)
		of_error("calloc()");

	props.len = size;

	if ((fd = open(path, O_RDONLY)) < 0)
		of_error("open()");

	if (read(fd, props.data, size) != size)
		of_error("read()");

	close(fd);

	val = _of_phandle_to_int(props);
	free(props.data);

	if (val == *phandle) {
		ptr = strdup(path);
		_of_remove_filename(ptr);
		name = _of_read_name(ptr, NULL);
		tmp = _of_populate_node(path, name);
		free(ptr);
		free(name);
	}

	return tmp;
}
struct device_node *_of_get_name(const char *path, const char *name)
{
	int fd;
	uint32_t size;
	char buf[PATH_MAX];
	struct device_node *tmp = NULL;
	struct stat fstats;

	if (stat(path, &fstats) < 0)
		return NULL;
	
	size = fstats.st_size;
	if(!size)
		return NULL;

	if ((fd = open(path, O_RDONLY)) < 0)
		of_error("open()");

	if (read(fd, buf, size) != size)
		of_error("read()");

	if (memcmp(buf, name, size))
		goto out;		

	tmp = _of_populate_node(path, name);

      out:

	close(fd);

	return tmp;
}

void _of_find_node_by_parse(char *path, const void *search, uint16_t type, int full)
{
	DIR *dir;
	struct dirent *tmp = NULL;
	char *directories[8192] = { NULL };
	char fullpath[PATH_MAX];
	int x = 0;
	struct stat fstats;
	struct device_node *node = NULL;
	
	lstat(path, &fstats);

	if (S_ISLNK(fstats.st_mode))
		return;

	if ((dir = opendir(path)) == NULL)
		return;

	while ((tmp = readdir(dir)) != NULL) {

		if ((strcmp(tmp->d_name, ".")) && (strcmp(tmp->d_name, ".."))) {

			if (!strcmp(path, "/"))
				strcat(strcpy(fullpath, "/"), tmp->d_name);
			else
				strcat(strcat(strcpy(fullpath, path), "/"),
				       tmp->d_name);

			if (type == OF_SEARCH_NAME) {
				if (!strcmp(tmp->d_name, "name")) {
					if ((node =
					     _of_get_name(fullpath,
							  search)) != NULL) {
						_n_array[++_n_idx] = node;
						_n_sem = 1;
						if(full==0)
							goto out;
					}
				}
			}

			if (type == OF_SEARCH_TYPE) {
				if (!strcmp(tmp->d_name, "device_type")) {
					if ((node =
					     _of_get_type(fullpath,
							  search)) != NULL) {
						_t_array[++_t_idx] = node;
						_t_sem = 1;
						if(full==0)
							goto out;
					}
				}
			}

			if (type == OF_SEARCH_PHDL) {
				if (!strcmp(tmp->d_name, "linux,phandle")) {
					if ((node =
					     _of_get_phandle(fullpath,
							     search)) != NULL) {
						_p_array[++_p_idx] = node;
						_p_sem = 1;
						goto out;
					}
				}
			}

			lstat(fullpath, &fstats);
			if (S_ISDIR(fstats.st_mode))
				directories[x++] = strdup(fullpath);
		}
	}

	x = 0;

	while (directories[x] != NULL) {
		_of_find_node_by_parse(directories[x], search, type, full);
	      out:
		free(directories[x++]);
	}

	closedir(dir);
}

