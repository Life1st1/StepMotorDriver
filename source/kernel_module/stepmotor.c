#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/pwm.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/counter.h>

#include "stepmotor.h"

#define DEVICE_NAME "stepmotor"
#define CLASS_NAME  "stepmotor_class"
#define MAX_MOTORS  3

struct motor_data {
    struct pwm_device *pwm;
    int speed_hz;
    int direction;
    int steps;
    int running;
    int id;
};

extern struct counter_device** ti_eqep_get_counter(void);

static struct class *stepmotor_class;
static dev_t dev_num;
static struct cdev stepmotor_cdev;
static struct motor_data motors[MAX_MOTORS];
static int motor_count;


static int stepmotor_open(struct inode *inode, struct file *file)
{
    int minor = iminor(inode);
    if (minor >= motor_count)
        return -ENODEV;
    file->private_data = &motors[minor];
    return 0;
}

static long stepmotor_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int period;
    struct motor_command user_cmd;
    struct motor_data *motor = file->private_data;
    
    if (copy_from_user(&user_cmd, (void __user *)arg, sizeof(user_cmd)))
        return -EFAULT;

    switch (cmd) {
        case IOCTL_SETDIR:
            printk("stepmotor: set direction %d\n", motor->id);
            motor->direction = user_cmd.direction;
            break;
        case IOCTL_SETSPEED:
            printk("stepmotor: set speed %d\n", motor->id);
            motor->speed_hz = user_cmd.value;
            break;
        case IOCTL_SETROTATE:
            printk("stepmotor: set position %d\n", motor->id);
            motor->steps = user_cmd.value;
            break;
        case IOCTL_START:
            printk("stepmotor: start motor %d\n", motor->id);
            period = 1000000000 / motor->speed_hz;
            pwm_config(motor->pwm, period / 2, period);
            pwm_enable(motor->pwm);
            motor->running = 1;
            break;
        case IOCTL_STOP:
            printk("stepmotor: stop motor %d\n", motor->id);
            pwm_disable(motor->pwm);
            motor->running = 0;
            break;
        case IOCTL_STATUS:
            user_cmd.value = motor->running ? MOTOR_RUNNING : MOTOR_STOPPED;
            if (copy_to_user((void __user *)arg, &user_cmd, sizeof(user_cmd)))
                return -EFAULT;
            break;
        default:
            return -EINVAL;
    }

    return 0;
}

static const struct file_operations stepmotor_fops = {
    .owner = THIS_MODULE,
    .open = stepmotor_open,
    .unlocked_ioctl = stepmotor_ioctl,
};

static int stepmotor_probe(struct platform_device *pdev)
{
    struct device_node *child;
    struct motor_data *motor;
    struct pwm_device *pwm;
    int ret, id = 0;

    motor_count = of_get_child_count(pdev->dev.of_node);
    if (motor_count > MAX_MOTORS)
        motor_count = MAX_MOTORS;

    ret = alloc_chrdev_region(&dev_num, 0, motor_count, DEVICE_NAME);
    if (ret)
        return ret;
    cdev_init(&stepmotor_cdev, &stepmotor_fops);
    cdev_add(&stepmotor_cdev, dev_num, motor_count);

    stepmotor_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(stepmotor_class))
        return PTR_ERR(stepmotor_class);

    for_each_child_of_node(pdev->dev.of_node, child) {
        if (id >= MAX_MOTORS)
            break;

        pwm = of_pwm_get(&pdev->dev, child, "pwm");
        if (IS_ERR(pwm)) {
            dev_err(&pdev->dev, "Failed to get PWM for motor %d\n", id);
            return PTR_ERR(pwm);
        }

        motor = &motors[id];
        motor->id = id;
        motor->pwm = pwm;
        motor->speed_hz = 1000;
        motor->direction = 0;
        motor->steps = 0;
        motor->running = 0;

        device_create(stepmotor_class, &pdev->dev, MKDEV(MAJOR(dev_num), id),
                      NULL, "stepmotor%d", id);

        id++;
    }

    dev_info(&pdev->dev, "StepMotor driver probed with %d motors\n", id);
    return 0;
}

static int stepmotor_remove(struct platform_device *pdev)
{
    int i;

    for (i = 0; i < motor_count; i++) {
        pwm_disable(motors[i].pwm);
        pwm_put(motors[i].pwm);
        device_destroy(stepmotor_class, MKDEV(MAJOR(dev_num), i));
    }

    class_destroy(stepmotor_class);
    cdev_del(&stepmotor_cdev);
    unregister_chrdev_region(dev_num, motor_count);

    return 0;
}

static const struct of_device_id stepmotor_of_match[] = {
    { .compatible = "tecotec,stepmotor", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, stepmotor_of_match);

static struct platform_driver stepmotor_driver = {
    .driver = {
        .name = "stepmotor",
        .of_match_table = stepmotor_of_match,
    },
    .probe = stepmotor_probe,
    .remove = stepmotor_remove,
};

static int counter_init(void)
{
    struct counter_device** counter_dev;
    int i = 0;

    pr_info("stepmotor: counter init\n");
    counter_dev = ti_eqep_get_counter();
    if(!counter_dev)
        return -ENODEV;

    for(i = 0; i < MAX_MOTORS; i++)
    {
        counter_dev[i]->ops->function_set(counter_dev[i], counter_dev[i]->counts, 2);   /* RISING_EDGE */
    }
    pr_info("stepmotor: counter init done\n");

    return 0;
}

static int __init stepmotor_init(void)
{
    int ret;

    pr_info("stepmotor: init\n");
    ret = platform_driver_register(&stepmotor_driver);
    
    if (ret)
        pr_err("stepmotor: failed to register platform driver: %d\n", ret);
    ret = counter_init();

    return ret;
}

static void __exit stepmotor_exit(void)
{
    pr_info("stepmotor: exit\n");
    platform_driver_unregister(&stepmotor_driver);
}

module_init(stepmotor_init);
module_exit(stepmotor_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tecotec");
MODULE_DESCRIPTION("Step Motor Driver using PWM and Device Tree");