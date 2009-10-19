/*
 * pommed - video-client.h
 */
#ifndef __MBP_VIDEO_CLIENT_H__
#define __MBP_VIDEO_CLIENT_H__


#define VIDEO_SWITCH_SYSTEM      "/etc/pommed/videoswitch"
#define VIDEO_SWITCH_USER        "/.videoswitch"


int
mbp_get_x_vtnum(Display *dpy);

void
mbp_video_switch(void);


#endif /* !__MBP_VIDEO_CLIENT_H__ */
