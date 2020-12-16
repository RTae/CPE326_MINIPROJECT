#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
void readfile(long unsigned int *dma, long unsigned int *dma32, long unsigned int *normal)
{
    FILE *fp;
    fp = fopen("/proc/zoneinfo", "r");
    if (fp != NULL)
    {
        char zone[50];
        int zoneNumber = 0;
        char startPageFrame[30];

        while (!feof(fp))
        {
            char buffer[255];
            fgets(buffer, 255, fp);
            if (sscanf(buffer, "Node 0, zone%s\n", zone) == 1)
            {
                if (strcmp(zone, "DMA") == 0)
                {
                    zoneNumber = 0;
                }
                else if (strcmp(zone, "DMA32") == 0)
                {
                    zoneNumber = 1;
                }
                else if (strcmp(zone, "Normal") == 0)
                {
                    zoneNumber = 2;
                }
                else
                {
                    zoneNumber = -1;
                }
            }
            if (sscanf(buffer, " start_pfn:%s\n", startPageFrame) == 1)
            {
                if (zoneNumber == 0)
                {
                    *dma = strtoul(startPageFrame, NULL, 0);
                    //printf("DMA =  %ld\n", *dma);
                }
                else if (zoneNumber == 1)
                {
                    *dma32 = strtoul(startPageFrame, NULL, 0);
                    //printf("DMA32 =  %ld\n", *dma32);
                }
                else if (zoneNumber == 2)
                {
                    *normal = strtoul(startPageFrame, NULL, 0);
                    //printf("Normal %ld\n",*norm);
                }
            }
        }
        fclose(fp);
    }
}

int hexadecimalToDecimal(char hex[])
{
    int decimal, place;
    int i = 0, val, len;

    decimal = 0;
    place = 1;

    /* Find the length of total number of hex digit */
    len = strlen(hex);
    len--;

    /*
     * Iterate over each hex digit
     */
    for (i = 0; hex[i] != '\0'; i++)
    {

        /* Find the decimal representation of hex[i] */
        if (hex[i] >= '0' && hex[i] <= '9')
        {
            val = hex[i] - 48;
        }
        else if (hex[i] >= 'a' && hex[i] <= 'f')
        {
            val = hex[i] - 97 + 10;
        }
        else if (hex[i] >= 'A' && hex[i] <= 'F')
        {
            val = hex[i] - 65 + 10;
        }

        decimal += val * pow(16, len);
        len--;
    }

    return decimal;
}

long zone(char *start, char *end, char *PID, long *dma_size, long *dma32_size, long *norm_size, int memSize)
{
    char path[64] = "sudo ./pagemap ";
    strcat(path, PID);
    strcat(path, " 0x");
    strcat(path, start);
    strcat(path, " 0x");
    strcat(path, end);
    printf("start: 0x%s end: 0x%s size: %d [Kb]\n", start, end, memSize / 1024);
    long unsigned int dma = 0;
    long unsigned int dma32 = 0;
    long unsigned int norm = 0;
    int fault = 0;
    FILE *fp = popen(path, "r");
    if (fp != NULL)
    {
        char path[1024];
        while (fgets(path, sizeof(path) - 1, fp) != NULL)
        {
            if (path[0] == '1')
            {
                unsigned long long pfn = -1;
                readfile(&dma, &dma32, &norm);
                sscanf(path + 2, "%llx", &pfn);
                if (pfn < dma32)
                {
                    *dma_size = *dma_size + 1;
                }
                else if (pfn < norm)
                {
                    *dma32_size = *dma32_size + 1;
                }
                else if (pfn >= norm)
                {
                    *norm_size = *norm_size + 1;
                }
            }
        }
    }
    else
    {
        printf("PID doesn't exist");
    }
}

void getVA(char *PID)
{
    FILE *fp;
    char path[64] = "/proc/";
    strcat(path, PID);
    strcat(path, "/maps");
    fp = fopen(path, "r");
    if (fp != NULL)
    {
        char start[4095][20];
        char end[4095][20];
        int startDecimal[100];
        int endDecimal[100];
        int memSize[100];
        int k = 0;
        while (!feof(fp))
        {
            char buff[511];
            fgets(buff, 511, fp);
            for (int i = 0; i < 30; i++)
            {
                if (buff[i] == '-')
                {
                    buff[i] = ' ';
                }
            }
            sscanf(buff, "%s %s", start[k], end[k]);
            startDecimal[k] = hexadecimalToDecimal(start[k]);
            endDecimal[k] = hexadecimalToDecimal(end[k]);

            // printf("start :%s end : %s\n",start[k],end[k]);
            // printf("startDec :%d endDec : %d\n",startDecimal[k],endDecimal[k]);
            memSize[k] = endDecimal[k] - startDecimal[k];
            // printf("%d\n",memSize[k] / 1024 );
            k++;
        }
        long dma_size = 0;
        long dma32_size = 0;
        long norm_size = 0;
        for (int i = 0; i < k - 1; i++)
        {
            long temp = zone(start[i], end[i], PID, &dma_size, &dma32_size, &norm_size, memSize[i]);
        }
        printf("dma_size = %lu KB dma32_size = %lu KB normal_size = %lu KB\n", dma_size, dma32_size, norm_size);
        fclose(fp);
    }
    else
    {
        printf("PID doesn't exist");
    }
}

void main(int argc, char *argv[])
{
    system("ps");
    char PID[20];
    printf("input your PID : ");
    scanf("%s", PID);
    getVA(PID);
}
