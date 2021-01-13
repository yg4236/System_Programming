#include <stdint.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define BUFFER_MAX 3
#define DIRECTION_MAX 35
#define VALUE_MAX 256
#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define MO1 24
#define MO2 26
#define Motor 18
//for sound sensor
#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

static const char *DEVICE = "/dev/spidev0.0";
static uint8_t MODE = SPI_MODE_0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
static int GPIOExport(int pin)
{
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open export for writing! \n");
        return (-1);
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}
static int GPIOUnexport(int pin)
{
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open unexport for writing! \n");
        return (-1);
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}
static int GPIODirection(int pin, int dir)
{
    static const char s_directions_str[] = "in\0out";
    char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
    int fd;
    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio%d direction for writing!\n", pin);
        return (-1);
    }
    if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3))
    {
        fprintf(stderr, "Failed to set direction!\n");
        return (-1);
    }
    close(fd);
    return (0);
}
static int GPIORead(int pin)
{
    char path[VALUE_MAX];
    char value_str[3];
    int fd;
    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio value for reading!\n");
        return (-1);
    }
    if (-1 == read(fd, value_str, 3))
    {
        fprintf(stderr, "Failed to read value!\n");
        return (-1);
    }
    close(fd);
    return (atoi(value_str));
}
static int PWMExport(int pin)
{
#define BUFFER_MAX 3
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;
    fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open export for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}
static int PWMEnable(int pin)
{
#define PWMDIRECTION_MAX 300
    char path[PWMDIRECTION_MAX];
    int fd;

    snprintf(path, PWMDIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm0/enable");
    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to wirte enable!\n");
        return (-1);
    }
    write(fd, "1", 1);
    close(fd);
    return (0);
}

static int PWMWritePeriod(int pin, int value)
{
    char path[VALUE_MAX];
    char real_value[VALUE_MAX];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/period");
    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to write period!\n");
        return (-1);
    }
    sprintf(real_value, "%d", value);
    if (1 != write(fd, real_value, strlen(real_value)))
    {
        close(fd);
        return (-1);
    }
    close(fd);
    return (0);
}

static int PWMWriteDutyCycle(int pin, int value)
{
    char path[VALUE_MAX];
    char real_value[VALUE_MAX];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/duty_cycle");
    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to write duty cycle!\n");
        return (-1);
    }
    sprintf(real_value, "%d", value);
    if (1 != write(fd, real_value, strlen(real_value)))
    {
        close(fd);
        return (-1);
    }

    close(fd);
    return (0);
}

/*
 * Ensure all settings are correct for the ADC
 */
static int prepare(int fd)
{

    if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1)
    {
        perror("Can't set MODE");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1)
    {
        perror("Can't set number of BITS");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1)
    {
        perror("Can't set write CLOCK");
        return -1;
    }

    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1)
    {
        perror("Can't set read CLOCK");
        return -1;
    }

    return 0;
}

/*
 * (SGL/DIF = 0, D2=D1=D0=0)
 */
uint8_t control_bits_differential(uint8_t channel)
{
    return (channel & 7) << 4;
}

/*
 * (SGL/DIF = 1, D2=D1=D0=0)
 */
uint8_t control_bits(uint8_t channel)
{
    return 0x8 | control_bits_differential(channel);
}

/*
 * Given a prep'd descriptor, and an ADC channel, fetch the
 * raw ADC value for the given channel.
 */
int readadc(int fd, uint8_t channel)
{
    uint8_t tx[] = {1, control_bits(channel), 0};
    uint8_t rx[3];

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = ARRAY_SIZE(tx),
        .delay_usecs = DELAY,
        .speed_hz = CLOCK,
        .bits_per_word = BITS,
    };

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1)
    {
        perror("IO Error");
        abort();
    }

    return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}

int main(int argc, char *argv[])
{
    int repeat = 5;
    int sound = 0;
    int alarm_check = 0;
    int motion, motion2;
    int move = 0;
    int door;

    //PWM
    PWMExport(0);
    sleep(1);
    PWMWritePeriod(0, 20000000);
    PWMWriteDutyCycle(0, 0);
    PWMEnable(0);
    //Enable GPIO pins
    if (-1 == GPIOExport(MO1) || -1 == GPIOExport(MO2) || -1 == GPIOExport(Motor))
    {
        printf("gpio export err\n");
        return (1);
    }
    //wait for writing to export file
    usleep(100000);
    //Set GPIO directions
    if (-1 == GPIODirection(MO1, IN) || -1 == GPIODirection(MO2, IN))
    {
        printf("gpio direction err\n");
        return (2);
    }

    /////////////////sound sensor////////////////
    int fd = open(DEVICE, O_RDWR);
    if (fd <= 0)
    {
        printf("Device %s not found\n", DEVICE);
        return -1;
    }
    if (prepare(fd) == -1)
    {
        return -1;
    }
    while (1)
    {
        sound = readadc(fd, 0);
        if (alarm_check > 5)
        {
            sound = 1;
            break;
        }

        if (sound > 250)
        {
            printf("sound %d\n", sound);
            alarm_check += 1;
            sleep(1);
        }
        else
        {
            printf("sound %d\n", sound);
            if (alarm_check < 1)
            {
                alarm_check = 0;
            }
            else
            {
                alarm_check -= 1;
            }
            sleep(1);
        }
    }
    close(fd);
    ///////////////motion&magnetic////////////
    do
    {
        motion = GPIORead(MO1);
        motion2 = GPIORead(MO2);
        if (motion == 0 && motion2 == 0)
        {
            printf("Both motion X\n");
            usleep(1000000);
        }
        else if (motion == 1 && motion2 == 0)
        {
            printf("motion1 Detected\n");
            usleep(1000000);
        }
        else if (motion == 0 && motion2 == 1)
        {
            printf("motion2 Detected\n");
            usleep(1000000);
        }
        else if (motion == 1 && motion2 == 1)
        {
            printf("Both motion detected\n");
            move = 1;
            break;
            usleep(1000000);
        }
    } while (repeat--);
    //////////////connect to pi2////////////////////
    int sock;
    struct sockaddr_in serv_addr;
    char msg[2];
    int light;
    if (move == 0)
    {
        sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock == -1)
            error_handling("socket() error");

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
        serv_addr.sin_port = htons(atoi(argv[2]));

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
            error_handling("connect() error");
        else
            printf("connect success\n");
        light = 1;
        snprintf(msg, 2, "%d", light);
        write(sock, msg, sizeof(msg));
        printf("Send message To Server : %s\n", msg);
    }
    else
    {
        sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock == -1)
            error_handling("socket() error");

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
        serv_addr.sin_port = htons(atoi(argv[2]));

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
            error_handling("connect() error");
        else
            printf("connect success\n");
        light = 0; // 여기서 light=0은 move=1의 의미로 pi2에 전달된다.
        snprintf(msg, 2, "%d", light);
        write(sock, msg, sizeof(msg));
        printf("Send message To Server : %s\n", msg);
    }
    repeat = 5;
    do //////////////////motion&magnetic////////////////////////
    {
        motion = GPIORead(MO1);
        motion2 = GPIORead(MO2);
        if (motion == 0 && motion2 == 0)
        {
            printf("Both motion X\n");
            usleep(1000000);
        }
        else if (motion == 1 && motion2 == 0)
        {
            printf("motion1 Detected\n");
            usleep(1000000);
        }
        else if (motion == 0 && motion2 == 1)
        {
            printf("motion2 Detected\n");
            usleep(1000000);
        }
        else if (motion == 1 && motion2 == 1)
        {
            printf("Both motion detected\n");
            move = 1;
            break;
            usleep(1000000);
        }
    } while (repeat--);
    if (move == 0)
    {
        while (1)
        { ///////////////////////servo motor ON/////////////////
            printf("servo On\n");
            door = read(sock, msg, 1);
            PWMWriteDutyCycle(0, 1000000);
            usleep(1000000);
            PWMWriteDutyCycle(0, 2000000);

            if (!strcmp(msg, "0"))
                printf("door closed\n");
            else
            {
                printf("door open\n");
                close(sock);
                break;
            }
        }
    }
    //Disable GPIO pins
    if (-1 == GPIOUnexport(MO1) || -1 == GPIOUnexport(MO2) || -1 == GPIOUnexport(Motor))
        return (4);
    printf("complete\n");
    return (0);
}
