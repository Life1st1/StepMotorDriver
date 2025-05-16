#ifndef _STEPMOTOR_H
#define _STEPMOTOR_H

#define IOCTL_SETDIR     _IOW('m', 1, struct motor_command)
#define IOCTL_SETSPEED   _IOW('m', 2, struct motor_command)
#define IOCTL_SETROTATE  _IOW('m', 3, struct motor_command)
#define IOCTL_START      _IO('m', 4)
#define IOCTL_STOP       _IO('m', 5)
#define IOCTL_STATUS     _IOR('m', 6, struct motor_command)

#define DIR_LEFT  0
#define DIR_RIGHT 1

#define MOTOR_STOPPED 0
#define MOTOR_RUNNING 1

struct motor_command {
    int direction;
    int value;
};

#endif