/***************************************************************************
 *   (C) Copyright 2006 Alastair Poole.  netstar@gatheringofgray.com)      *   
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __OF_USERSPACE__
#define __OF_USERSPACE__

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

char *OF_ROOT;

struct node_property_t {
	uint8_t *data;
	uint32_t len;
};

struct device_node {
	char *name;
	char *path;
	char *full_path;
	char *type;
	struct node_property_t linux_phandle;
	struct device_node *next;
	void *data;
	uint32_t len;
};

#include "of_standard.h"
#include "of_internals.h"
#include "of_externals.h"

#endif
