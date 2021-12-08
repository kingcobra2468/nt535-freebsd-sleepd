#include <sys/ioctl.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <libinput.h>
#include <acpiio.h>

#define SLEEP_BTN_DEV "/dev/input/event3"
#define ACPI_DEV "/dev/acpi"

static int open_restricted(const char *, int, void *);
static void close_restricted(int, void *);

static struct sleep_button
{
	struct libinput *context;
	struct libinput_device *device;
};

static struct sleep_button sb;
static int acpi_fd;
static const int acpi_mode = 3;

static int open_restricted(const char *path, int flags, void *user_data)
{
	int fd = open(path, flags);
	return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data)
{
	close(fd);
}

static void acpi_init()
{
	if (acpi_fd)
		return;

	acpifd = open(ACPI_DEV, O_RDWR);
	if (acpi_fd == -1)
		exit(errno);
}

static void acpi_suspend()
{
	int ret = ioctl(acpifd, ACPIIO_REQSLPSTATE, &acpi_mode);
	if (ret == -1)
		exit(errno)
}

static void sleep_button_init()
{
	struct libinput *li;
	struct libinput_device *device;
	const struct libinput_interface interface = {
		.open_restricted = open_restricted,
		.close_restricted = close_restricted,
	};

	li = libinput_path_create_context(&interface, NULL);
	device = libinput_path_add_device(li, SLEEP_BTN_DEV);
	if (device == NULL)
	{
		exit(ENXIO);
	}
	libinput_device_ref(device);

	sb = {li, device};
}

static void sleep_button_event_loop()
{
	struct libinput_event *event;
	struct libinput_event_keyboard *key;

	while (1)
	{
		libinput_dispatch(sb.context);
		event = libinput_get_event(sb.context);

		if (event == NULL)
			continue;

		if (libinput_event_get_type(event) != LIBINPUT_EVENT_KEYBOARD_KEY)
			continue;

		key = libinput_event_get_keyboard_event(event);
		if (key != KEY_SLEEP)
			continue;

		libinput_event_destroy(event);
	}

	free(event);
	free(key);
}

static void sleep_buton_destroy()
{
	libinput_unref(sb.context);
	libinput_path_remove_device(sb.device);

	free(sb.context);
	free(sb.device);
}

int main(void)
{

	return 0;
}
