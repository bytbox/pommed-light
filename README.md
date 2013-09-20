About pommed-light
------------------

This is a stripped-down version of pommed with client, dbus, and ambient light
sensor support removed, optimized for use with dwm and the like.

README for pommed
-----------------

 - Kernel version requirements
 - Supported machines
 - Using pommed
 - Beeper feature
 - When things go wrong


System requirements:
--------------------

Pommed expects standard development headers for libc to be installed, as well
as libpci.

 pommed requires at least a 2.6.25 kernel, due to the use of the new timerfd
 interface that was released as stable with this version.

 February and October 2008 machines require a 2.6.28 kernel for full support.


Using pommed
------------

Launch pommed at startup, a simple init script will do. Your distribution
should take care of this. A standard init script and a systemd service file are
provided.


Keyboard backlight on PowerMac machines
---------------------------------------

The keyboard backlight on PowerMac machines (except the very first ones) is
driven through i2c. You need the i2c-dev kernel module loaded on your system
for pommed to work properly; you can add i2c-dev to /etc/modules to have it
loaded automatically at system startup.


Beeper feature
--------------

The beeper feature relies on the uinput kernel module being loaded. You can
check for its availability by checking for the uinput device node, which is
either one of:
 - /dev/input/uinput
 - /dev/uinput
 - /dev/misc/uinput

Or by checking the output of 
 $ lsmod | grep uinput

If the module is not loaded, load it manually with
 # modprobe uinput
then restart pommed. You'll need to ensure the module is loaded before pommed
starts; to achieve that, add uinput to /etc/modules.

For the curious, as I've been asked a couple times already: pommed uses the
uinput facility to create a userspace input device which handles the console
beep. Once this device is set up, the kernel happily passes down beep events
to pommed through this device, and pommed only needs to ... well, *beep*.


When things go wrong
--------------------

First and foremost: don't panic!

If something doesn't work (or so it appears), there's usually a good reason to
that, and pommed should be able to provide some insight as to what is going
wrong if only you ask it.

By default, pommed uses syslog to log warnings and errors, so check your
system logs. If you can't find anything, running pommed in the foreground
will help a lot; in this mode, pommed will log everything to stderr instead
of syslog, so you'll see every message.

First, stop pommed. Then run
 # pommed -f

Use Ctrl-C to stop pommed, fix the problem, and restart it.

If you still can't see what's wrong, ask for more output by running pommed in
debug mode. Be warned: in this mode, pommed is very chatty.

First, stop pommed. Then run
 # pommed -d

Use Ctrl-C to stop pommed, fix the problem, and restart it.

If the debug mode doesn't offer any hint as to what's going on, then contact
me with the details of your problem and I'll be able to help.


