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
#define VALUE_MAX 30
#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define POUT 23 //ultra out
#define PIN 24 //ultra in
#define PINM 17 //magnetic in
#define POUTB 18 //buzzer out

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
}

void error_handling( char *message){
 fputs(message,stderr);
 fputc( '\n',stderr);
 exit( 1);
}

static int magnetic()
{
    char path[VALUE_MAX];
    char value_str[3];
    int fd;
    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio17/value");
    fd = open(path, O_RDONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "M_Failed to open gpio value for reading!\n");
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

static int butcher(int value)
{
    static const char s_values_str[] = "01";
    char path[VALUE_MAX];
    int fd;
    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio18/value");
    fd = open(path, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "B_Failed to open gpio value for writing!\n");
        return (-1);
    }
    if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1))
    {
        fprintf(stderr, "B_Failed to write value!\n");
        return (-1);
    }
    close(fd);
    return (0);
}

int main(int argc, char *argv[])
{
    
    //Enable GPIO pins
    if (-1 == GPIOExport(PIN))
        return(1);
    //Enable GPIO pins
    if (-1 == GPIOExport(POUT))
        return(1);
    //Enable GPIO pins
    if (-1 == GPIOExport(PINM))
        return(1);
    //Enable GPIO pins
    if (-1 == GPIOExport(POUTB))
        return(1);

     //Wait GPIOExport
     usleep(10000);
    
    //Set GPIO directions
    if (-1 == GPIODirection(PIN, IN))
        return(2);
    //Set GPIO directions
    if (-1 == GPIODirection(POUT, OUT))
        return(2);
    //Set GPIO directions
    if (-1 == GPIODirection(PINM, IN))
        return(2);
    //Set GPIO directions
    if (-1 == GPIODirection(POUTB, OUT))
        return(2);

    int repeat = 99;
    clock_t start_t, end_t;
    double time;

    //init ultrawave trigger
    GPIOWrite(POUT, 0);
    usleep(10000);
    
    //start
    // outcheck is chaged 1 if magnectic() is 0
    int outcheck = 0;
        
    do{
        if(outcheck == 1 && magnetic() > 0){
            butcher(0);
            break;
        }
        
        while (magnetic() > 0)
        {
            butcher(0);
            printf("waiting...\n");
            usleep(2000000);
        }
        
        if(magnetic() == 0){
            printf("opening\n");
            outcheck = 1;
        }
        
        if (-1 == GPIOWrite(POUT, 1))
        {
            printf("gpio write/trigger err\n");
            return (3);
        }
        else
            printf("check started");

        //1sec == 1000000ultra_sec, 1ms = 1000ultra_sec
        usleep(10);
        GPIOWrite(POUT, 0);

        while (GPIORead(PIN) == 0)
        {
            start_t = clock();
        }
        while (GPIORead(PIN) == 1)
        {
            end_t = clock();
        }

        time = (double)(end_t - start_t) / CLOCKS_PER_SEC; //ms
        printf("time : %.4lf\n", time);
        printf("distance : %.2lfcm\n", 340 * time * 50);

        if (340 * time * 50 > 100)
        {
            butcher(0);
        }
        else
        {
            butcher(1);
        }
        usleep(500000);
        
    } while (repeat--);
    
    //connect part start
    int sock;
    struct sockaddr_in serv_addr;
    char msg[2];

    if(argc!=3){
        printf("Usage : %s <IP> <port>\n",argv[0]);
        exit(1);
    }

    while(1) {
        sock = socket(PF_INET, SOCK_STREAM, 0);
        if(sock == -1)
            error_handling("socket() error");

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
        serv_addr.sin_port = htons(atoi(argv[2]));

        if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
            error_handling("connect() error");
        else
            printf("Connect!\n");
        
        snprintf(msg, 2, "%d", 1);
        write(sock, msg, sizeof(msg));
        
        close(sock);
    }
    
    //Disable GPIO pins
    if (-1 == GPIOUnexport(POUT) || -1 == GPIOUnexport(PIN))
        return (4);

    if (-1 == GPIOUnexport(POUTB) || -1 == GPIOUnexport(PINM))
        return (4);
    printf("complete\n");
    return (0);
}
