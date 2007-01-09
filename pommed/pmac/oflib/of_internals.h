/***************************************************************************
 *   (C) Copyright 2006 Alastair Poole.  netstar@gatheringofgray.com)      *   
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
 
#ifndef __OF_INTERNALS__
#define __OF_INTERNALS__
#include "of_api.h"

void _of_find_node_by_parse(char *path, const void *search, uint16_t type, int full);
struct device_node *_of_return_nodes(struct device_node **array, int *idx,
				     int *sem, int type);
struct device_node *_of_get_name(const char *path, const char *name);
struct device_node *_of_get_type(const char *path, const char *type);
struct device_node *_of_populate_node(const char *path, const char *name);
struct device_node *_of_get_phandle(const char *path, const uint32_t * phandle);

void _of_read_linux_phandle(const char *path, struct device_node *node);
void _of_read_type(const char *path, struct device_node *node);

void _of_remove_filename(char *path);
void _of_get_path(char *path);
void _of_make_compat_path(const char *path, char *buf);

#endif
