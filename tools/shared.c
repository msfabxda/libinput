/*
 * Copyright © 2014 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <config.h>

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libudev.h>

#include <libevdev/libevdev.h>
#include <libinput-util.h>

#include "shared.h"

LIBINPUT_ATTRIBUTE_PRINTF(3, 0)
static void
log_handler(struct libinput *li,
	    enum libinput_log_priority priority,
	    const char *format,
	    va_list args)
{
	static int is_tty = -1;

	if (is_tty == -1)
		is_tty = isatty(STDOUT_FILENO);

	if (is_tty) {
		if (priority >= LIBINPUT_LOG_PRIORITY_ERROR)
			printf(ANSI_RED);
		else if (priority >= LIBINPUT_LOG_PRIORITY_INFO)
			printf(ANSI_HIGHLIGHT);
	}

	vprintf(format, args);

	if (is_tty && priority >= LIBINPUT_LOG_PRIORITY_INFO)
		printf(ANSI_NORMAL);
}

void
tools_init_options(struct tools_options *options)
{
	memset(options, 0, sizeof(*options));
	options->tapping = -1;
	options->tap_map = -1;
	options->drag = -1;
	options->drag_lock = -1;
	options->natural_scroll = -1;
	options->left_handed = -1;
	options->middlebutton = -1;
	options->dwt = -1;
	options->click_method = -1;
	options->scroll_method = -1;
	options->scroll_button = -1;
	options->speed = 0.0;
	options->profile = LIBINPUT_CONFIG_ACCEL_PROFILE_NONE;
}

int
tools_parse_option(int option,
		   const char *optarg,
		   struct tools_options *options)
{
	switch(option) {
		case OPT_TAP_ENABLE:
			options->tapping = 1;
			break;
		case OPT_TAP_DISABLE:
			options->tapping = 0;
			break;
		case OPT_TAP_MAP:
			if (!optarg)
				return 1;

			if (streq(optarg, "lrm")) {
				options->tap_map = LIBINPUT_CONFIG_TAP_MAP_LRM;
			} else if (streq(optarg, "lmr")) {
				options->tap_map = LIBINPUT_CONFIG_TAP_MAP_LMR;
			} else {
				return 1;
			}
			break;
		case OPT_DRAG_ENABLE:
			options->drag = 1;
			break;
		case OPT_DRAG_DISABLE:
			options->drag = 0;
			break;
		case OPT_DRAG_LOCK_ENABLE:
			options->drag_lock = 1;
			break;
		case OPT_DRAG_LOCK_DISABLE:
			options->drag_lock = 0;
			break;
		case OPT_NATURAL_SCROLL_ENABLE:
			options->natural_scroll = 1;
			break;
		case OPT_NATURAL_SCROLL_DISABLE:
			options->natural_scroll = 0;
			break;
		case OPT_LEFT_HANDED_ENABLE:
			options->left_handed = 1;
			break;
		case OPT_LEFT_HANDED_DISABLE:
			options->left_handed = 0;
			break;
		case OPT_MIDDLEBUTTON_ENABLE:
			options->middlebutton = 1;
			break;
		case OPT_MIDDLEBUTTON_DISABLE:
			options->middlebutton = 0;
			break;
		case OPT_DWT_ENABLE:
			options->dwt = LIBINPUT_CONFIG_DWT_ENABLED;
			break;
		case OPT_DWT_DISABLE:
			options->dwt = LIBINPUT_CONFIG_DWT_DISABLED;
			break;
		case OPT_CLICK_METHOD:
			if (!optarg)
				return 1;

			if (streq(optarg, "none")) {
				options->click_method =
				LIBINPUT_CONFIG_CLICK_METHOD_NONE;
			} else if (streq(optarg, "clickfinger")) {
				options->click_method =
				LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER;
			} else if (streq(optarg, "buttonareas")) {
				options->click_method =
				LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS;
			} else {
				return 1;
			}
			break;
		case OPT_SCROLL_METHOD:
			if (!optarg)
				return 1;

			if (streq(optarg, "none")) {
				options->scroll_method =
				LIBINPUT_CONFIG_SCROLL_NO_SCROLL;
			} else if (streq(optarg, "twofinger")) {
				options->scroll_method =
				LIBINPUT_CONFIG_SCROLL_2FG;
			} else if (streq(optarg, "edge")) {
				options->scroll_method =
				LIBINPUT_CONFIG_SCROLL_EDGE;
			} else if (streq(optarg, "button")) {
				options->scroll_method =
				LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN;
			} else {
				return 1;
			}
			break;
		case OPT_SCROLL_BUTTON:
			if (!optarg) {
				return 1;
			}
			options->scroll_button =
			libevdev_event_code_from_name(EV_KEY,
						      optarg);
			if (options->scroll_button == -1) {
				fprintf(stderr,
					"Invalid button %s\n",
					optarg);
				return 1;
			}
			break;
		case OPT_SPEED:
			if (!optarg)
				return 1;
			options->speed = atof(optarg);
			break;
		case OPT_PROFILE:
			if (!optarg)
				return 1;

			if (streq(optarg, "adaptive"))
				options->profile = LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
			else if (streq(optarg, "flat"))
			      options->profile = LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT;
			else
			      return 1;
			break;
	}

	return 0;
}

static int
open_restricted(const char *path, int flags, void *user_data)
{
	bool *grab = user_data;
	int fd = open(path, flags);

	if (fd < 0)
		fprintf(stderr, "Failed to open %s (%s)\n",
			path, strerror(errno));
	else if (*grab && ioctl(fd, EVIOCGRAB, (void*)1) == -1)
		fprintf(stderr, "Grab requested, but failed for %s (%s)\n",
			path, strerror(errno));

	return fd < 0 ? -errno : fd;
}

static void
close_restricted(int fd, void *user_data)
{
	close(fd);
}

static const struct libinput_interface interface = {
	.open_restricted = open_restricted,
	.close_restricted = close_restricted,
};

static struct libinput *
tools_open_udev(const char *seat, bool verbose, bool grab)
{
	struct libinput *li;
	struct udev *udev = udev_new();

	if (!udev) {
		fprintf(stderr, "Failed to initialize udev\n");
		return NULL;
	}

	li = libinput_udev_create_context(&interface, &grab, udev);
	if (!li) {
		fprintf(stderr, "Failed to initialize context from udev\n");
		goto out;
	}

	if (verbose) {
		libinput_log_set_handler(li, log_handler);
		libinput_log_set_priority(li, LIBINPUT_LOG_PRIORITY_DEBUG);
	}

	if (libinput_udev_assign_seat(li, seat)) {
		fprintf(stderr, "Failed to set seat\n");
		libinput_unref(li);
		li = NULL;
		goto out;
	}

out:
	udev_unref(udev);
	return li;
}

static struct libinput *
tools_open_device(const char *path, bool verbose, bool grab)
{
	struct libinput_device *device;
	struct libinput *li;

	li = libinput_path_create_context(&interface, &grab);
	if (!li) {
		fprintf(stderr, "Failed to initialize context from %s\n", path);
		return NULL;
	}

	if (verbose) {
		libinput_log_set_handler(li, log_handler);
		libinput_log_set_priority(li, LIBINPUT_LOG_PRIORITY_DEBUG);
	}

	device = libinput_path_add_device(li, path);
	if (!device) {
		fprintf(stderr, "Failed to initialized device %s\n", path);
		libinput_unref(li);
		li = NULL;
	}

	return li;
}

struct libinput *
tools_open_backend(enum tools_backend which,
		   const char *seat_or_device,
		   bool verbose,
		   bool grab)
{
	struct libinput *li;

	switch (which) {
	case BACKEND_UDEV:
		li = tools_open_udev(seat_or_device, verbose, grab);
		break;
	case BACKEND_DEVICE:
		li = tools_open_device(seat_or_device, verbose, grab);
		break;
	default:
		abort();
	}

	return li;
}

void
tools_device_apply_config(struct libinput_device *device,
			  struct tools_options *options)
{
	if (options->tapping != -1)
		libinput_device_config_tap_set_enabled(device, options->tapping);
	if (options->tap_map != (enum libinput_config_tap_button_map)-1)
		libinput_device_config_tap_set_button_map(device,
							  options->tap_map);
	if (options->drag != -1)
		libinput_device_config_tap_set_drag_enabled(device,
							    options->drag);
	if (options->drag_lock != -1)
		libinput_device_config_tap_set_drag_lock_enabled(device,
								 options->drag_lock);
	if (options->natural_scroll != -1)
		libinput_device_config_scroll_set_natural_scroll_enabled(device,
									 options->natural_scroll);
	if (options->left_handed != -1)
		libinput_device_config_left_handed_set(device, options->left_handed);
	if (options->middlebutton != -1)
		libinput_device_config_middle_emulation_set_enabled(device,
								    options->middlebutton);

	if (options->dwt != -1)
		libinput_device_config_dwt_set_enabled(device, options->dwt);

	if (options->click_method != (enum libinput_config_click_method)-1)
		libinput_device_config_click_set_method(device, options->click_method);

	if (options->scroll_method != (enum libinput_config_scroll_method)-1)
		libinput_device_config_scroll_set_method(device,
							 options->scroll_method);
	if (options->scroll_button != -1)
		libinput_device_config_scroll_set_button(device,
							 options->scroll_button);

	if (libinput_device_config_accel_is_available(device)) {
		libinput_device_config_accel_set_speed(device,
						       options->speed);
		if (options->profile != LIBINPUT_CONFIG_ACCEL_PROFILE_NONE)
			libinput_device_config_accel_set_profile(device,
								 options->profile);
	}
}

static char*
find_device(const char *udev_tag)
{
	struct udev *udev;
	struct udev_enumerate *e;
	struct udev_list_entry *entry;
	struct udev_device *device;
	const char *path, *sysname;
	char *device_node = NULL;

	udev = udev_new();
	e = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(e, "input");
	udev_enumerate_scan_devices(e);

	udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
		path = udev_list_entry_get_name(entry);
		device = udev_device_new_from_syspath(udev, path);
		if (!device)
			continue;

		sysname = udev_device_get_sysname(device);
		if (strncmp("event", sysname, 5) != 0) {
			udev_device_unref(device);
			continue;
		}

		if (udev_device_get_property_value(device, udev_tag))
			device_node = strdup(udev_device_get_devnode(device));

		udev_device_unref(device);

		if (device_node)
			break;
	}
	udev_enumerate_unref(e);
	udev_unref(udev);

	return device_node;
}

bool
find_touchpad_device(char *path, size_t path_len)
{
	char *devnode = find_device("ID_INPUT_TOUCHPAD");

	if (devnode) {
		snprintf(path, path_len, "%s", devnode);
		free(devnode);
	}

	return devnode != NULL;
}

bool
is_touchpad_device(const char *devnode)
{
	struct udev *udev;
	struct udev_device *dev = NULL;
	struct stat st;
	bool is_touchpad = false;

	if (stat(devnode, &st) < 0)
		return false;

	udev = udev_new();
	dev = udev_device_new_from_devnum(udev, 'c', st.st_rdev);
	if (!dev)
		goto out;

	is_touchpad = udev_device_get_property_value(dev, "ID_INPUT_TOUCHPAD");
out:
	if (dev)
		udev_device_unref(dev);
	udev_unref(udev);

	return is_touchpad;
}

static inline void
setup_path(void)
{
	const char *path = getenv("PATH");
	char new_path[PATH_MAX];

	snprintf(new_path,
		 sizeof(new_path),
		 "%s:%s",
		 LIBINPUT_TOOL_PATH,
		 path ? path : "");
	setenv("PATH", new_path, 1);
}

int
tools_exec_command(const char *prefix, int real_argc, char **real_argv)
{
	char *argv[64] = {NULL};
	char executable[128];
	const char *command;
	int rc;

	assert((size_t)real_argc < ARRAY_LENGTH(argv));

	command = real_argv[0];

	rc = snprintf(executable,
		      sizeof(executable),
		      "%s-%s",
		      prefix,
		      command);
	if (rc >= (int)sizeof(executable)) {
		fprintf(stderr, "Failed to assemble command.\n");
		return EXIT_FAILURE;
	}

	argv[0] = executable;
	for (int i = 1; i < real_argc; i++)
		argv[i] = real_argv[i];

	setup_path();

	rc = execvp(executable, argv);
	fprintf(stderr,
		"Failed to execute '%s' (%s)\n",
		command,
		strerror(errno));

	return EXIT_FAILURE;
}
