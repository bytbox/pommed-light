/***************************************************************************
 *   (C) Copyright 2006 Alastair Poole.  netstar@gatheringofgray.com)      *   
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "of_api.h"

void of_error(char *error)
{
	fprintf(stderr, "Error: %s\n", error);
	exit(EXIT_FAILURE);
}

void of_free_node(struct device_node *node)
{
	if (node->name)
		free(node->name);
	if (node->path)
		free(node->path);
	if (node->full_path)
		free(node->full_path);
	if (node->type)
		free(node->type);
	if (node->linux_phandle.data)
		free(node->linux_phandle.data);
	if (node->data)
		free(node->data);
	if (node)
		free(node);
}
