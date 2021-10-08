#ifndef __VHD_TOOL__
#define __VHD_TOOL__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#pragma pack(1)
static struct VHD_Hard_Disk_Footer_Format {
    uint64_t Unique_Id[2];
    uint64_t Cookie;
    uint64_t Original_Size;
    uint64_t Current_Size;
    uint64_t Data_Offset;
    uint32_t Features;
    uint32_t File_Format_Version;
    uint32_t Time_Stamp;
    uint32_t Creator_Application;
    uint32_t Creator_Version;
    uint32_t Creator_Host_OS;
    uint32_t Disk_Geometry;
    uint32_t Disk_Type;
    uint32_t Checksum;
    uint8_t Saved_State_and_Reserved[428];
} *vhdff;

static struct Disk_Geometry_Info {
    uint16_t Cylinder;
    uint8_t Heads;
    uint8_t Sectors_Per_Track_in_Cylinder;
} *dgi;
#pragma pack()
#define FOOTER_FORMAT_SIZE 512
#define FUNCTION_FAILED -1
#define FUNCTION_SUCCESS 0
#define READ_BINARY_FUNCTION 2
#define VHD_BLACK_SIZE 512

static struct stat * VHD_statbuff;
static struct stat * Bin_statbuff;

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
    if (strncmp(argv_t[1], "-x", 2) && strncmp(argv_t[1], "-h", 2) && strncmp(argv_t[1], "-v", 2)) {
        printf("Error options!(Try to use -h)\n");
        return FUNCTION_FAILED;
    }
    else if (!strncmp(argv_t[1], "-v", 2)) {
        if (Read_Binary(argv_t[2]) == READ_BINARY_FUNCTION) {
            // Read_Binary() success.
            return READ_BINARY_FUNCTION;
        }
        else {
            return FUNCTION_FAILED;
        }
    }
    else if (!strncmp(argv_t[1], "-x", 2)) {
        if (!(Write_Bin_to_VHD(argv_t) == FUNCTION_SUCCESS)) {
            return FUNCTION_FAILED;
        }
        else {
            return FUNCTION_SUCCESS;
        }
    }
    else if (!strncmp(argv_t[1], "-h", 2)) {
        printf("%s -x [.Bin file] [.VHD file]\n", argv_t[0]);
        printf("%s -v [file name]\n", argv_t[0]);
        return FUNCTION_SUCCESS;
    }

    return FUNCTION_SUCCESS;
}

int Get_vhd_Footer_Format_Info(FILE * fp, uint32_t HDFFS)
{
    fseek(fp, HDFFS, 0);
    fread(&vhdff->Cookie, sizeof(uint64_t), 1, fp);
    fread(&vhdff->Features, sizeof(uint32_t), 1, fp);
    fread(&vhdff->File_Format_Version, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Data_Offset, sizeof(uint64_t), 1, fp);
    fread(&vhdff->Time_Stamp, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Creator_Application, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Creator_Version, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Creator_Host_OS, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Original_Size, sizeof(uint64_t), 1, fp);
    fread(&vhdff->Current_Size, sizeof(uint64_t), 1, fp);
    fread(&vhdff->Disk_Geometry, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Disk_Type, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Checksum, sizeof(uint32_t), 1, fp);
    fread(&vhdff->Unique_Id[0], sizeof(uint64_t), 1, fp);
    fread(&vhdff->Unique_Id[1], sizeof(uint64_t), 1, fp);
    for (int i = 0; i < 428; i++) {
        fread(&vhdff->Saved_State_and_Reserved[i], 1, 1, fp);
    }
    return FUNCTION_SUCCESS;
}

int Get_vhd_Disk_Geometry_Field_Info(void)
{
    dgi->Cylinder = (((vhdff->Disk_Geometry & 0xff) << 8)
                    | ((vhdff->Disk_Geometry & 0xff00) >> 8)) & 0xffff;
    dgi->Heads = ((vhdff->Disk_Geometry & 0xff0000) >> 16) & 0xff;
    dgi->Sectors_Per_Track_in_Cylinder = ((vhdff->Disk_Geometry & 0xff000000) >> 24) & 0xff;
    printf("Cylinder: %u\nHeads: %u\nSectors per track: %u\n",
           dgi->Cylinder, dgi->Heads, dgi->Sectors_Per_Track_in_Cylinder);
    return FUNCTION_SUCCESS;
}

int Read_Binary(char * File_Name)
{
    FILE *fp = fopen(File_Name, "rb+");
    if (fp == NULL) {
        printf("Have not file.\n");
        return FUNCTION_FAILED;
    }

    unsigned int size = 0;
    printf("File name: %s\n", File_Name);
    printf("Enter size(4B~1,073,741,824B):");
    scanf("%d", &size);
    if (size > 1073741824) {
        printf("Size is too large.\n");
        return FUNCTION_FAILED;
    }
    fseek(fp, 0, 0);

    unsigned char * buff = (unsigned char *)malloc(size * sizeof(unsigned char));
    memset((unsigned char *)buff, 0, size);
    fread((unsigned char *)buff, 1, size, fp);

    printf("================================\n");
    for (uint64_t addr = 0; addr < size; addr += 16) {
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
        printf("\n");
    }

    free(buff);
    fclose(fp);
    return READ_BINARY_FUNCTION;
}

int Write_Bin_to_VHD(char * argv_x[])
{
    FILE * Bin_File_Point = fopen(argv_x[2], "rb+");
    if (Bin_File_Point == NULL) {
        printf("Bin fopen No!\n");
        return FUNCTION_FAILED;
    }
    FILE * VHD_File_Point = fopen(argv_x[3], "rb+");
    if (VHD_File_Point == NULL) {
        printf("VHD fopen No!\n");
        return FUNCTION_FAILED;
    }

    printf("BIN File: %s\nVHD File: %s\n", argv_x[2], argv_x[3]);

    uint32_t Hard_Disk_Footer_Format_Start = 0; //Footer Format start address.
    uint32_t VHD_File_Size = 0;
    uint32_t Bin_File_Size = 0;
    uint32_t Start_Sector = 0;
    uint32_t From_BIN_Input_Buff[1024];
    uint32_t All_Sector_Number = 0;

    dgi = (struct Disk_Geometry_Info *)malloc(sizeof(struct Disk_Geometry_Info));
    vhdff = (struct VHD_Hard_Disk_Footer_Format *)malloc(sizeof(struct VHD_Hard_Disk_Footer_Format));
    VHD_statbuff = (struct stat *)malloc(sizeof(struct stat));
    Bin_statbuff = (struct stat *)malloc(sizeof(struct stat));
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
    Get_vhd_Footer_Format_Info(VHD_File_Point, Hard_Disk_Footer_Format_Start);

    // back to file header.
    fseek(VHD_File_Point, 0, 0);

    // get vhd Disk Geometry field.
    Get_vhd_Disk_Geometry_Field_Info();
    All_Sector_Number = Hard_Disk_Footer_Format_Start / FOOTER_FORMAT_SIZE;
    printf("All sectors: %u\n", All_Sector_Number);

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
    printf("Writing......\n");
    if (Bin_File_Size >= 1024) {
        for (int i = 0; i < (Bin_File_Size % 1024) - 1024; i++) {
            for (int j = 0; j < 1024; j++) {
                From_BIN_Input_Buff[j] = fgetc(Bin_File_Point);
                fputc(From_BIN_Input_Buff[j], VHD_File_Point);
            }
        }
        for (int index = 0; index < Bin_File_Size - (Bin_File_Size % 1024); index++) {
            From_BIN_Input_Buff[index] = fgetc(Bin_File_Point);
            fputc(From_BIN_Input_Buff[index], VHD_File_Point);
        }
    }
    else {
        for (int j = 0; j < Bin_File_Size; j++) {
            From_BIN_Input_Buff[j] = fgetc(Bin_File_Point);
            fputc(From_BIN_Input_Buff[j], VHD_File_Point);
        }
    }
    printf("Done!\n");

    fseek(Bin_File_Point, 0, 0);
    fseek(VHD_File_Point, 0, 0);
    free(dgi);
    free(VHD_statbuff);
    free(vhdff);
    free(Bin_statbuff);
    fclose(VHD_File_Point);
    fclose(Bin_File_Point);
    return FUNCTION_SUCCESS;
}

#endif // __VHD_TOOL__
