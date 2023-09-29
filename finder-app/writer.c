#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    char* writeFile;
    char* writeStr;
    int numBytes;
    int fd;

    openlog(NULL, 0, LOG_USER);

    if ((argc<2) || argv[2] == NULL)
    {
        syslog(LOG_ERR, "Invalid number of arguments: %d", argc);
        return 1;
    }

    writeFile = argv[1];
    writeStr = argv[2];
    int dataLen = strlen(writeStr);

    fd = open(writeFile, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);

    if (fd == -1)
    {
        syslog(LOG_ERR, "Can not create file: %s", writeFile);
        return 1;
    }
    else
    {
        numBytes = write(fd, writeStr, dataLen);

        if (numBytes == -1)
            syslog(LOG_ERR, "Unable to write to file: %s", writeFile);
        else if (numBytes != dataLen)
            syslog(LOG_ERR, "Write file not complete, expected %d bytes but only wrote %d bytes", dataLen, numBytes);
        else
            syslog(LOG_DEBUG, "Writing %s to %s", writeStr, writeFile);
    }

    close(fd);

    return 0;
}

