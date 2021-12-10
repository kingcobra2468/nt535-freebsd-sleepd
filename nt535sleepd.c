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

// input device that corresponds to the sleep button
#define SLEEP_BTN_DEV "/dev/input/event3"
#define ACPI_DEV "/dev/acpi"
#define INACTIVITY_COOLDOWN 2
// pidfile for the daemon
#define PID_FILE "/var/run/nt535sleepd.pid"

static int open_restricted(const char *, int, void *);
static void close_restricted(int, void *);
static void acpi_init();
static void acpi_suspend();
static void sleep_button_init();
static void sleep_button_event_loop();
static void sleep_button_destroy();

static struct sleep_button
{
	struct libinput *context;
	struct libinput_device *device;
} sb;

// fd for the acpi device
static int acpi_fd;
// always use ACPI s3 in order to put machine to sleep
static const int acpi_mode = 3;

// handler for system events
static void signal_handler(int sig)
{
	sleep_button_destroy();
	exit(sig);
}

// opens the sleep button device for libinput
static int open_restricted(const char *path, int flags, void *user_data)
{
	(void)user_data;
	int fd = open(path, flags);
	return fd < 0 ? -errno : fd;
}

// closes the sleep button device for libinput
static void close_restricted(int fd, void *user_data)
{
	(void)user_data;
	close(fd);
}

// opens the acpi device to get the fd for further operations
static void acpi_init()
{
	if (acpi_fd)
		return;

	acpi_fd = open(ACPI_DEV, O_RDWR);
	if (acpi_fd == -1)
		exit(errno);
}

// sends a S3 command to the machine to put machine to suspend state
static void acpi_suspend()
{
	int ret = ioctl(acpi_fd, ACPIIO_REQSLPSTATE, &acpi_mode);
	if (ret == -1)
		exit(errno);
}

// initializes the sleep button device for libinput
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

// starts the event loop to listen for sleep button presses
static void sleep_button_event_loop()
{
	struct libinput_event *event;
	struct libinput_event_keyboard *key;

	while (1)
	{
		libinput_dispatch(sb.context);
		event = libinput_get_event(sb.context);

		// check if a new event is in the queue
		if (event == NULL) {
			sleep(INACTIVITY_COOLDOWN);
			continue;
		}

		// check to see if the occured event that has occured is a "keyboard" aka button press event.
		if (libinput_event_get_type(event) != LIBINPUT_EVENT_KEYBOARD_KEY)
			continue;

		key = libinput_event_get_keyboard_event(event);
		// check if the event is for the correct key-type. This is a theortical case as in theory
		// the button should always propogate a keycode of 142 (sleep key).
		if (libinput_event_keyboard_get_key(key) != KEY_SLEEP)
			continue;

		libinput_event_destroy(event);
		free(event);
		free(key);

		// suspend the machine
		acpi_suspend();
	}
}

// destroys the handler of the sleep button for libinput
static void sleep_button_destroy()
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

	// attempt to open pidfile for holding onto daemon pid
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

	// daemonize this process
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
