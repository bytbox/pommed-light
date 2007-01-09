/***************************************************************************
 *   (C) Copyright 2006 Alastair Poole.  netstar@gatheringofgray.com)      *   
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "of_api.h"

void of_init(void)
{
	OF_ROOT="/proc/device-tree";
}

void of_init_root(char *path)
{
	uint32_t len;

	if(!path)
		of_error("of_init_root() NULL path");
	
	len = strlen(path);
	if(!len)
		of_error("of_init_root() Invalid path");
		
	if(path[len-1] == '/')
		path[len-1] = '\0';
		
	OF_ROOT=path;	
}


int of_test_property(struct device_node *node, const char *name)
{
	struct stat fstats;
	char buf[PATH_MAX]={0};
	
	strcat(strcat(buf, node->full_path), name);
	
	if(stat(buf, &fstats) < 0)
		return 0;
	else
		return 1;
}

void *of_find_property(struct device_node *node, const char *name, int *plen)
{
	char buf[PATH_MAX]={0};
	uint8_t *property;
	uint32_t size;
	struct stat fstats;
	int fd;
	

	strcat(strcat(buf, node->full_path), name);

	if (stat(buf, &fstats) < 0)
		return NULL;

	size = fstats.st_size;

	property = malloc(size);
	if(!property)
		of_error("malloc()");

	if ((fd = open(buf, O_RDONLY)) < 0)
		of_error("open()");

	if ((*plen = read(fd, property, size)) != size)
		of_error("read()");
		
	close(fd);

	return property;
}

int of_property_to_n_uint64(uint64_t *val, void *prop, uint32_t len, int n)
{
	*val = 0;
	int i=0;
	uint8_t *data = prop;
	
	if((n*8 > len) || (n == 0))
		return 0;
	
	for(i=0; i < (n*8)-1; i++);
	
	*val = 		    ((uint64_t) data[i - 7] << 56) +
			    ((uint64_t) data[i - 6] << 48) +
			    ((uint64_t) data[i - 5] << 40)
			    + ((uint64_t) data[i - 4] << 32) +
			    ((uint64_t) data[i - 3] << 24) +
			    ((uint64_t) data[i - 2] << 16)
			    + ((uint64_t) data[i - 1] << 8) + (uint64_t) data[i];
	
	return 1;
}

int of_property_to_n_uint32(uint32_t *val, void *prop, uint32_t len, int n)
{
	*val = 0;
	int i=0;
	uint8_t *data = prop;
	
	if((n*4 > len) || (n == 0))
		return 0;
	
	for(i=0; i < (n*4)-1; i++);
	
	*val = (data[i-3] << 24) + (data[i-2] << 16) +
	       (data[i-1] << 8) + (data[i]);

	return 1;
}


int of_property_to_uint32(uint32_t *val, void *prop, uint32_t len)
{
	*val=0;
	uint8_t *data = prop;

	if(len != 4)
		return 0;

	*val =	    (data[0] << 24) + (data[1] << 16) +
		    (data[2] << 8) + data[3];

	return 1;
}

struct device_node *of_get_parent(struct device_node *node)
{
	struct device_node *tmp;
	char *p;
	char *ptr;
	
	if((!node->path) || (!node) || (strlen(node->path)==1))
		return NULL;
	
	ptr=strdup(node->path);
	
	p=strrchr(ptr, '/');
	*p=0;
	p=strrchr(ptr, '/');
	*p=0;
	
	tmp=of_find_node_by_path(ptr);
	
	free(ptr);
	
	return tmp;
}

struct device_node *of_find_node_by_name(const char *name, int type)
{
	if (!_n_sem)
		_of_find_node_by_parse(OF_ROOT, name, OF_SEARCH_NAME, type);

	return _of_return_nodes(_n_array, &_n_idx, &_n_sem, type);
}

struct device_node *of_find_type_devices(const char *device_type)
{
	struct device_node *list = calloc(1, sizeof(struct device_node));
	struct device_node *tmp = NULL;
	struct device_node *ptr = list;

	while ((tmp = of_find_node_by_type(device_type, 1)) != NULL) {
		ptr->next = tmp;
		ptr = ptr->next;
		ptr->next = NULL;
	}

	return list;
}

void of_find_type_devices_free(struct device_node *root)
{
	struct device_node *cursor = root;
	struct device_node *fwd;

	while (cursor) {
		fwd = cursor->next;
		of_free_node(cursor);
		cursor = fwd;
	}
}

struct device_node *of_find_node_by_path(const char *path)
{
	struct device_node *tmp = NULL;
	char buf[PATH_MAX];
	struct stat fstats;
	
	_of_make_compat_path(path, buf);

	if (stat(buf, &fstats) < 0)
		return NULL;

	tmp = _of_populate_node(buf, NULL);

	return tmp;
}

struct device_node *of_find_node_by_phandle(uint32_t phandle)
{
	uint32_t *ptr = malloc(4);
	if(!ptr)
		of_error("malloc()");

	*ptr = phandle;

	_of_find_node_by_parse(OF_ROOT, (void *)ptr, OF_SEARCH_PHDL, 0);

	free(ptr);

	return _of_return_nodes(_p_array, &_p_idx, &_t_sem, 0);
}

struct device_node *of_find_node_by_type(const char *device_type, int type)
{
	if (!_t_sem)
		_of_find_node_by_parse(OF_ROOT, device_type, OF_SEARCH_TYPE, type);

	return _of_return_nodes(_t_array, &_t_idx, &_t_sem, type);
}

