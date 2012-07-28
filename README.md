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


Kernel version requirements:
----------------------------

 pommed requires at least a 2.6.25 kernel, due to the use of the new timerfd
 interface that was released as stable with this version.

 February and October 2008 machines require a 2.6.28 kernel for full support.


Supported machines:
-------------------

 - Intel machines
   * MacBook Pro Core Duo 15" (January 2006)
   * MacBook Pro Core Duo 17" (April 2006)
   * MacBook Pro Core2 Duo 15" (October 2006, June 2007, February 2008, October 2008)
   * MacBook Pro Core2 Duo 17" (October 2006, June 2007, February 2008, October 2008)
   * MacBook Pro 13", 15", 17" (June 2009)
   * MacBook Pro Core i5/i7 15", 17" (April 2010)
   * MacBook Pro Core2 Duo 13" (April 2010)
   * MacBook Pro Core i5/i7 13", 15", 17" (Early 2011)
   * MacBook Core Duo (May 2006)
   * MacBook Core2 Duo (November 2006 & May 2007)
   * MacBook Core2 Duo Santa Rosa (November 2007)
   * MacBook Core2 Duo (February 2008, October 2008, October 2009, April 2010)
   * MacBook Air Core2 Duo (January 2008, October 2008)
   * MacBook Air Core2 Duo 11" & 13" (October 2010)
   * MacBook Air Core i5/i7 11" & 13" (Mid 2011)

   If your MacBook Pro/MacBook Air/MacBook is not listed here, please contact us
   and include the content of /sys/class/dmi/id/product\_name in your mail. Thanks.

 - PowerMac machines
   * PowerBook G4 Titanium 15" (December 2000)
   * PowerBook G4 Titanium 15" (October 2001)
   * PowerBook G4 Titanium 15" (April 2002)
   * PowerBook G4 Titanium 15"
   * PowerBook G4 Aluminium 17"
   * PowerBook G4 Aluminium 15" (September 2003)
   * PowerBook G4 Aluminium 17" (September 2003)
   * PowerBook G4 Aluminium 15" (April 2004)
   * PowerBook G4 Aluminium 17" (April 2004)
   * PowerBook G4 Aluminium 15" (February 2005)
   * PowerBook G4 Aluminium 17" (February 2005)
   * PowerBook G4 Aluminium 15"
   * PowerBook G4 Aluminium 17"
   * PowerBook G4 12" (January 2003)
   * PowerBook G4 12" (September 2003)
   * iBook G4 (October 2003)
   * PowerBook G4 12" (April 2004)
   * iBook G4 (October 2004)
   * iBook G4
   * PowerBook G4 12"

   If your PowerBook/iBook is not listed here, please contact us and include
   the content of /proc/device-tree/model in your mail. Thanks.


Using pommed
------------

Launch pommed at startup, a simple init script will do. Your distribution
should take care of this.


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


