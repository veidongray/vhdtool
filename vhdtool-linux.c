#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>

static struct VHD_Hard_Disk_Footer_Format {
    /* Use union to solve the problem of large and small side endianness. */
    union {
        uint64_t value[2];
        unsigned char data[16];
    } Unique_Id;
    union {
        uint64_t value;
        unsigned char data[8];
    } Cookie;
    union {
        uint64_t value;
        unsigned char data[8];
    } Original_Size;
    union {
        uint64_t value;
        unsigned char data[8];
    } Current_Size;
    union {
        uint64_t value;
        unsigned char data[8];
    } Data_Offset;
    union {
        uint32_t value;
        unsigned char data[4];
    } Features;
    union {
        uint32_t value;
        unsigned char data[4];
    } File_Format_Version;
    union {
        uint32_t value;
        unsigned char data[4];
    } Time_Stamp;
    union {
        uint32_t value;
        unsigned char data[4];
    } Creator_Application;
    union {
        uint32_t value;
        unsigned char data[4];
    } Creator_Version;
    union {
        uint32_t value;
        unsigned char data[4];
    } Creator_Host_OS;
    union {
        uint32_t value;
        unsigned char data[4];
    } Disk_Geometry;
    union {
        uint32_t value;
        unsigned char data[4];
    } Disk_Type;
    union {
        uint32_t value;
        unsigned char data[4];
    } Checksum;
    uint8_t Saved_State_and_Reserved[428];
} *vhdff;

static struct Disk_Geometry_Info {
    uint16_t Cylinder;
    uint8_t Heads;
    uint8_t Sectors_Per_Track_in_Cylinder;
} *dgi;

#define FOOTER_FORMAT_SIZE 512
#define PERSECTSIZE 512
#define FUNCTION_FAILED -1
#define FUNCTION_SUCCESS 0
#define READ_BINARY_FUNCTION 2
#define VHD_BLACK_SIZE 51

static char DiskTypeName[][32] = {
    "None",
    "Reserved0(deprecated)",
    "Fixed Hard Disk",
    "Dynamic Hard Disk",
    "Differencing Hard Disk",
    "Reserved1(deprecated)",
    "Reserved2(deprecated)"
};

static struct stat * VHD_statbuff;
static struct stat * Bin_statbuff;
static struct stat * Red_statbuff;

int Get_vhd_Disk_Geometry_Field_Info(void);
int Get_vhd_Footer_Format_Info(FILE *, uint32_t);
int Console_Parameter_Check(int, char *[]);
int Read_Binary(char *);
int Write_Bin_to_VHD(char *[]);

int main(int argc, char * argv[])
{
    int Console_Parameter_Check_Ret = 0;
    Console_Parameter_Check_Ret = Console_Parameter_Check(argc, argv);
    if (Console_Parameter_Check_Ret == FUNCTION_FAILED) {
        return FUNCTION_FAILED;
    }
    else if (Console_Parameter_Check_Ret == READ_BINARY_FUNCTION
             || Console_Parameter_Check_Ret == FUNCTION_SUCCESS) {
        return FUNCTION_SUCCESS;
    }

    return FUNCTION_SUCCESS;
}

int Console_Parameter_Check(int argc_t, char * argv_t[])
{
    if (argc_t < 2) {
        printf("Have not option.\n");
        return FUNCTION_FAILED;
    }
    if (strncmp(argv_t[1], "-w", 2) && strncmp(argv_t[1], "-h", 2) && strncmp(argv_t[1], "-r", 2)) {
        printf("Error options!(Try to use -h)\n");
        return FUNCTION_FAILED;
    }
    else if (!strncmp(argv_t[1], "-r", 2)) {
        if (Read_Binary(argv_t[2]) == READ_BINARY_FUNCTION) {
            // Read_Binary() success.
            return READ_BINARY_FUNCTION;
        }
        else {
            return FUNCTION_FAILED;
        }
    }
    else if (!strncmp(argv_t[1], "-w", 2)) {
        if (!(Write_Bin_to_VHD(argv_t) == FUNCTION_SUCCESS)) {
            return FUNCTION_FAILED;
        }
        else {
            return FUNCTION_SUCCESS;
        }
    }
    else if (!strncmp(argv_t[1], "-h", 2)) {
        printf("%s -w [.Bin file] [.VHD file]\n", argv_t[0]);
        printf("%s -r [file name]\n", argv_t[0]);
        return FUNCTION_SUCCESS;
    }

    return FUNCTION_SUCCESS;
}

int Get_vhd_Footer_Format_Info(FILE * fp, uint32_t footer_offset)
{
    fseek(fp, footer_offset, 0);
    fread(&vhdff->Cookie, sizeof(uint64_t), 1, fp);
    if (strncmp((const char *)&vhdff->Cookie, "conectix", 8)) {
        /* If the Cookie's value is not "conectix" */
        /* Then out error and return. */
        printf("Cookie ERROR!\n");
        return FUNCTION_FAILED;
    }
    fread(&vhdff->Features.value, sizeof(uint32_t), 1, fp);
    fread(&vhdff->File_Format_Version.value, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Data_Offset.value, sizeof(uint64_t), 1, fp);
    fread(&vhdff->Time_Stamp.value, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Creator_Application.value, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Creator_Version.value, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Creator_Host_OS.value, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Original_Size.value, sizeof(uint64_t), 1, fp);
    fread(&vhdff->Current_Size.value, sizeof(uint64_t), 1, fp);
    fread(&vhdff->Disk_Geometry.value, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Disk_Type.value, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Checksum.value, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Unique_Id.value[0], sizeof(uint64_t), 1, fp);
    fread(&vhdff->Unique_Id.value[1], sizeof(uint64_t), 1, fp);
    for (int i = 0; i < 428; i++) {
        fread(&vhdff->Saved_State_and_Reserved[i], 1, 1, fp);
    }
    return FUNCTION_SUCCESS;
}

int Get_vhd_Disk_Geometry_Field_Info(void)
{
    int i = 0;
    uint64_t Disk_Size = 0;
    for (i = 0; i < 8; i++) {
        Disk_Size += vhdff->Current_Size.data[i];
        Disk_Size <<= 8;
    }
    dgi->Cylinder = (((vhdff->Disk_Geometry.value & 0xff) << 8)
                    | ((vhdff->Disk_Geometry.value & 0xff00) >> 8)) & 0xffff;
    dgi->Heads = ((vhdff->Disk_Geometry.value & 0xff0000) >> 16) & 0xff;
    dgi->Sectors_Per_Track_in_Cylinder = ((vhdff->Disk_Geometry.value & 0xff000000) >> 24) & 0xff;
    printf("Cylinder: %u\nHeads: %u\nSectors per track: %u\n",
           dgi->Cylinder, dgi->Heads, dgi->Sectors_Per_Track_in_Cylinder);
    printf("Host: %s\n", vhdff->Creator_Host_OS.data[0] == 0x57 ? "Win" : "Mac");
    printf("Disk type: %s\n", DiskTypeName[vhdff->Disk_Type.data[3]]);
    return FUNCTION_SUCCESS;
}

int Read_Binary(char * File_Name)
{
    /*====================Read file data.=======================*/
    int i = 0;
    int show_flag = 0;      /* 0 show ......; 1 don't.*/
    uint64_t count = 0;
    uint64_t addr = 0;
    uint64_t addr_index = 0;
    uint32_t Red_File_Size = 0;
    FILE *fp = fopen(File_Name, "rb+");
    if (fp == NULL) {
        perror(File_Name);
        return FUNCTION_FAILED;
    }

    Red_statbuff = (struct stat *)malloc(sizeof(struct stat));
    memset((struct stat *)Red_statbuff, 0, sizeof(struct stat));
    stat(File_Name, Red_statbuff);
    Red_File_Size = Red_statbuff->st_size;
    unsigned int size = 0;
    unsigned int temp_size = 0;
    printf("File name: %s\n", File_Name);
    printf("%s: %uBytes\n", File_Name, Red_File_Size);
    while (1) {
        printf("Enter size((MIN)16B - (MAX)%dBytes):", Red_File_Size);
        scanf("%d", &size);
        if (size > Red_File_Size) {
            printf("Size too large, try again.\n");
            continue;
        } else if (size < 16) {
            printf("Size too small, try again.\n");
            continue;
        } else {
            break;
        }
    }
    fseek(fp, 0, 0);
    /*
    if ((size + (16 - (size % 16))) > Red_File_Size) {
        size -= size % 16;
        printf("size - size %% 16: %uBytes\n", size);
    } else {
        size += (16 - (size % 16));
        printf("size + size %% 16: %uBytes\n", size);
    }
    */
    unsigned char * buff = (unsigned char *)malloc(size * sizeof(unsigned char));
    memset((unsigned char *)buff, 0, size);
    fread((unsigned char *)buff, 1, size, fp);

    temp_size = size / 16;
    printf("<----------------------------------->\n");
    for (addr = 0, addr_index = 0; addr_index < temp_size; addr += 16, addr_index++) {
        for (i = 0, count = 0; i < 16; i++) {
            count += buff[addr + i];
            count <<= 8;
        }
        if (count) {
            show_flag = 1;      /* If next count value is zero then show ........*/
            printf("%.8lX: ", addr);
            printf("%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X  %.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X | ",
                    buff[addr], buff[addr + 1], buff[addr + 2], buff[addr + 3],
                    buff[addr + 4], buff[addr + 5], buff[addr + 6], buff[addr + 7],
                    buff[addr + 8], buff[addr + 9], buff[addr + 10], buff[addr + 11],
                    buff[addr + 12], buff[addr + 13], buff[addr + 14], buff[addr + 15]);
            for (int i = addr; i < (addr + 16); i++) {
                if (buff[i] < '~' && buff[i] > ' ') {
                    printf("%c", buff[i]);
                }
                else {
                    printf(".");
                }
            }
        } else if (show_flag) {
            printf("........\n\n");
            show_flag = 0;      /* Just show only once*/
            continue;
        } else {
            continue;
        }
        printf("\n");
    }

    if (size % 16) {
        /* Output the remaining characters after the pair. */
        temp_size = (size % 16) + addr;
        printf("%.8lX: ", addr);
        for (; addr < temp_size; addr++) {
            printf("%.2X ", buff[addr]);
            if ((addr + 1) % 8 == 0) {
                printf(" ");
            }
        }
        for (i = 0; i < 16 - (temp_size % 16); i++) {
            printf("   ");
        }
        printf("| ");

        for (i = addr - (size % 16); i < temp_size; i++) {
            if (buff[i] < '~' && buff[i] > ' ') {
                printf("%c", buff[i]);
            }
            else {
                printf(".");
            }
        }
        for (i = 0; i < 16 - (temp_size % 16); i++) {
            printf(" ");
        }
        printf("\n");
    }

    free(buff);
    fclose(fp);
    return READ_BINARY_FUNCTION;
}

int Write_Bin_to_VHD(char * argv_x[])
{
    int i = 0, j = 0;
    int ret = FUNCTION_SUCCESS;
    FILE * Bin_File_Point = fopen(argv_x[2], "rb+");
    if (Bin_File_Point == NULL) {
        perror(argv_x[2]);
        return FUNCTION_FAILED;
    }
    FILE * VHD_File_Point = fopen(argv_x[3], "rb+");
    if (VHD_File_Point == NULL) {
        perror(argv_x[3]);
        return FUNCTION_FAILED;
    }

    printf("BIN File: %s\nVHD File: %s\n", argv_x[2], argv_x[3]);

    uint32_t Hard_Disk_Footer_Format_Start = 0; //Footer Format start address.
    uint32_t VHD_File_Size = 0;
    uint32_t Bin_File_Size = 0;
    uint32_t Start_Sector = 0;
    char *From_BIN_Input_Buff = NULL;
    uint32_t All_Sector_Number = 0;

    dgi = (struct Disk_Geometry_Info *)malloc(sizeof(struct Disk_Geometry_Info));
    vhdff = (struct VHD_Hard_Disk_Footer_Format *)malloc(sizeof(struct VHD_Hard_Disk_Footer_Format));
    VHD_statbuff = (struct stat *)malloc(sizeof(struct stat));
    Bin_statbuff = (struct stat *)malloc(sizeof(struct stat));
    From_BIN_Input_Buff = (char *)malloc(SHRT_MAX * sizeof(char));
    memset(dgi, 0, sizeof(struct Disk_Geometry_Info));
    memset((struct VHD_Hard_Disk_Footer_Format *)vhdff, 0, sizeof(struct VHD_Hard_Disk_Footer_Format));
    memset((struct stat *)VHD_statbuff, 0, sizeof(struct stat));
    memset((struct stat *)Bin_statbuff, 0, sizeof(struct stat));

    //================================.Bin file to .VHD file================================
    // get vhd and bin file size.
    stat(argv_x[3], VHD_statbuff);
    VHD_File_Size = VHD_statbuff->st_size;
    stat(argv_x[2], Bin_statbuff);
    Bin_File_Size = Bin_statbuff->st_size;
    // Hard_Disk_Footer_Format_Start is realy File size.
    Hard_Disk_Footer_Format_Start = VHD_File_Size - FOOTER_FORMAT_SIZE;

    // get vhd footer format info.
    ret = Get_vhd_Footer_Format_Info(VHD_File_Point, Hard_Disk_Footer_Format_Start);
    if (ret == FUNCTION_FAILED) {
        goto fail;
    }

    // back to file header.
    fseek(VHD_File_Point, 0, 0);

    // get vhd Disk Geometry field.
    Get_vhd_Disk_Geometry_Field_Info();
    All_Sector_Number = Hard_Disk_Footer_Format_Start / PERSECTSIZE;
    printf("All sectors: %u\n", All_Sector_Number);
    printf("Disk size: %uMB\n", VHD_File_Size / 1024 / 1024);

    // enter start sector.
    printf("Enter start sector(1~%u): ", All_Sector_Number);
    scanf("%u", &Start_Sector);

    // back to BIN file header.
    if (fseek(Bin_File_Point, 0, 0)) {
        printf("Fseek bin error!\n");
        return FUNCTION_FAILED;
    }
    if (fseek(VHD_File_Point, (Start_Sector * VHD_BLACK_SIZE) - VHD_BLACK_SIZE, 0)) {
        printf("Fseek vhd error!\n");
        return FUNCTION_FAILED;
    }

    //write from .bin to input to VHD.
    //per write 1024bytes.
    printf("[");
    for (i = 0, j = 0; i < Bin_File_Size; i++) {
        From_BIN_Input_Buff[i] = (char)fgetc(Bin_File_Point);
        fputc((int)From_BIN_Input_Buff[i], VHD_File_Point);
        if (i % (Bin_File_Size / 10) == 0) {
            printf("_%d%%", j);
            j += 10;
        }
    }
    printf("] Done!\n");

fail:
    fseek(Bin_File_Point, 0, 0);
    fseek(VHD_File_Point, 0, 0);
    free(dgi);
    free(From_BIN_Input_Buff);
    free(VHD_statbuff);
    free(vhdff);
    free(Bin_statbuff);
    fclose(VHD_File_Point);
    fclose(Bin_File_Point);
    if (ret == FUNCTION_FAILED) {
        return FUNCTION_FAILED;
    }
    return FUNCTION_SUCCESS;
}
