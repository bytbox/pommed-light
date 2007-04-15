/***************************************************************************
 *   (C) Copyright 2006 Alastair Poole.  netstar@gatheringofgray.com)      *   
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __OF_DEVICE_BY__
#define __OF_DEVICE_BY__

#define OF_SEARCH_NAME 0x01
#define OF_SEARCH_TYPE 0x02
#define OF_SEARCH_PHDL 0x03

struct device_node *_n_array[256];
int _n_idx;
int _n_sem;

struct device_node *_t_array[256];
int _t_idx;
int _t_sem;

struct device_node *_p_array[2];
int _p_sem;
int _p_idx;

void of_init(void);
void of_init_root(char *path);

struct device_node *of_find_node_by_type(const char *device_type, int type);
struct device_node *of_find_node_by_name(const char *name, int type);
struct device_node *of_find_node_by_path(const char *path);
struct device_node *of_find_node_by_phandle(uint32_t phandle);
struct device_node *of_get_parent(struct device_node *node);

void *of_find_property(struct device_node *node, const char *name, int *plen);
int of_test_property(struct device_node *node, const char *name);
int of_property_to_uint32(uint32_t *val, void *prop, uint32_t len);
int of_property_to_n_uint32(uint32_t *val, void *prop, uint32_t len, int n);
int of_property_to_n_uint64(uint64_t *val, void *prop, uint32_t len, int n);

struct device_node *of_find_type_devices(const char *device_type);
void of_find_type_devices_free(struct device_node *root);

#endif
