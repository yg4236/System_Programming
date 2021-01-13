#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define LED1 17
#define LED2 18
#define VALUE_MAX 256
#define MAGNETIC 27
#define ZODO1 3
#define ZODO2 0

#define DIRECTION_MAX 35
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
#define BUFFER_MAX 3
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/export", O_WRONLY);
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

static int GPIODirection(int pin, int dir)
{
    static const char s_directions_str[] = "in\0out";

    //char path[DIRECTION_MAX] = "/sys/class/gpio/gpio24/direction";
    char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
    int fd;

    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio direction for writing!\n");
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

static int GPIOWrite(int pin, int value)
{
    static const char s_values_str[] = "01";

    char path[VALUE_MAX];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open gpio value for writing!\n");
        return (-1);
    }
    if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1))
    {
        fprintf(stderr, "Failed to write value!\n");
        return (-1);
    }
    close(fd);
    return (0);

    printf("Write value!!!!!!!!!!!!!!!\n");
}

static int GPIOUnexport(int pin)
{
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open unexport for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
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

int to_pi1(int argc, char **argv)
{

    char state[3];
    int light = 0;
    int serv_sock, clnt_sock = -1;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    //Enable GPIO pins
    if (-1 == GPIOExport(MAGNETIC))
        return (1);
    sleep(1);

    //Set GPIO directions
    if (-1 == GPIODirection(MAGNETIC, IN))
        return (2);
    //Enable GPIO pins
    if (-1 == GPIOExport(LED1))
        return (1);
    sleep(1);

    //Set GPIO directions
    if (-1 == GPIODirection(LED1, OUT))
        return (2);

    if (argc != 3)
    {
        printf("Usage : %s <port>\n", argv[0]);
    }
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    while (1)
    {
        if (clnt_sock < 0)
        {
            if (listen(serv_sock, 5) == -1)
                error_handling("listen() error");
            printf("waiting...\n");
            clnt_addr_size = sizeof(clnt_addr);
            clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr,
                               &clnt_addr_size);
            if (clnt_sock == -1)
                error_handling("accept() error");
            else
            {
                printf("Connect!\n");
                break;
            }
        }
        usleep(500 * 100);
    }
    while (1)
    {

        if (-1 != read(clnt_sock, state, sizeof(state)))
        {
            if (!strcmp(state, "1"))
            {
                GPIOWrite(LED1, HIGH);
                break;
            }
            else if (!strcmp(state, "0"))
            {
                close(clnt_sock);
                close(serv_sock);

                return 0;
            }
        }

        usleep(500 * 100);
    }
    sleep(10);
    while (1)
    {
        if (GPIORead(27) == 0)
        {
            write(clnt_sock, "1", 1);
            close(clnt_sock);
            close(serv_sock);
            return 3;
        }
        else
        {
            write(clnt_sock, "0", 1);
        }
        sleep(2);
    }

    close(clnt_sock);
    close(serv_sock);
    return 0;
}

int to_pi3(int argc, char **argv)
{

    char state[2];
    int light = 0;
    int serv_sock, clnt_sock = -1;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;

    //Enable GPIO pins
    if (-1 == GPIOExport(LED1))
        return (1);
    usleep(10000);

    //Set GPIO directions
    if (-1 == GPIODirection(LED1, OUT))
        return (2);
    usleep(10000);
    GPIOWrite(LED1, HIGH);

    if (-1 == GPIOExport(LED2))
        return (1);
    sleep(1);
    //Set GPIO directions
    if (-1 == GPIODirection(LED2, OUT))
        return (2);
    GPIOWrite(LED2, HIGH);

    int fd = open(DEVICE, O_RDWR);
    if (fd <= 0)
    {
        printf("Device %s not found\n", DEVICE);
        return -1;
    }
    if (argc != 3)
    {
        printf("Usage : %s <port>\n", argv[0]);
    }
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    while (1)
    {
        if (clnt_sock < 0)
        {
            if (listen(serv_sock, 5) == -1)
                error_handling("listen() error");
            printf("waiting...\n");

            clnt_addr_size = sizeof(clnt_addr);
            clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr,
                               &clnt_addr_size);
            if (clnt_sock == -1)
                error_handling("accept() error");
            else
                printf("connect!\n");
        }

        if (-1 != read(clnt_sock, state, sizeof(state)))
        {
            if (prepare(fd) == -1)
            {
                return -1;
            }
            int i = 0;

            light = readadc(fd, ZODO2);
            printf("%d\n", light);
            if (light > 300)
            {
                if (-1 == GPIOWrite(LED2, LOW))
                    return (3);
            }
            light = readadc(fd, ZODO1);
            printf("%d\n", light);
            if (light > 300)
            {
                if (-1 == GPIOWrite(LED1, LOW))
                    return (3);
            }
            close(fd);
            break;
        }
        printf("waiting\n");
        usleep(500 * 100);
    }
    close(clnt_sock);
    close(serv_sock);
    return 0;
}

int main(int argc, char **argv)
{
    to_pi1(argc, argv);
    to_pi3(argc, argv);
    return 0;
}