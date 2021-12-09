#include <sys/ioctl.h>
#include <sys/param.h>

#include <err.h>
#include <libutil.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <libinput.h>
#include <input-event-codes.h>
#include <acpiio.h>

#define SLEEP_BTN_DEV "/dev/input/event3"
#define ACPI_DEV "/dev/acpi"
#define PID_FILE "/var/run/nt535sleepd.pid"

static int open_restricted(const char *, int, void *);
static void close_restricted(int, void *);
static void acpi_init();
static void acpi_suspend();
static void sleep_button_init();
static void sleep_button_event_loop();
static void sleep_buton_destroy();

static struct sleep_button
{
	struct libinput *context;
	struct libinput_device *device;
} sb;

static int acpi_fd;
static const int acpi_mode = 3;

static void signal_handler(int sig)
{
	sleep_buton_destroy();
	exit(sig);
}

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

	acpi_fd = open(ACPI_DEV, O_RDWR);
	if (acpi_fd == -1)
		exit(errno);
}

static void acpi_suspend()
{
	int ret = ioctl(acpi_fd, ACPIIO_REQSLPSTATE, &acpi_mode);
	if (ret == -1)
		exit(errno);
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

	sb.context = li;
	sb.device = device;
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
		if (libinput_event_keyboard_get_key(key) != KEY_SLEEP)
			continue;

		libinput_event_destroy(event);
		free(event);
		free(key);

		acpi_suspend();
	}
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
	struct pidfh *pfh;
	pid_t otherpid, childpid;

	pfh = pidfile_open(PID_FILE, 0600, &otherpid);
	if (pfh == NULL)
	{
		if (errno == EEXIST)
		{
			errx(EXIT_FAILURE, "Daemon already running, pid: %jd.",
				 (intmax_t)otherpid);
		}
		warn("Cannot open or create pidfile");
	}

	if (daemon(0, 0) == -1)
	{
		warn("Cannot daemonize");
		pidfile_remove(pfh);
		exit(EXIT_FAILURE);
	}
	pidfile_write(pfh);

	sleep_button_init();
	acpi_init();
	sleep_button_event_loop();

	pidfile_remove(pfh);
	return 0;
}
