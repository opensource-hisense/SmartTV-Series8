/*
 ***************************************************************************
 * MediaTek Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 1997-2012, MediaTek, Inc.
 *
 * All rights reserved. MediaTek source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of MediaTek. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of MediaTek Technology, Inc. is obtained.
 ***************************************************************************

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int bin2h(char *infname, char *outfname, char *fw_name)
{
    int ret = 0;
    FILE *infile, *outfile;    
    unsigned char c;
    int i=0;

    infile = fopen(infname, "r");

    if (infile == (FILE *) NULL) 
    {
        printf("Can't read file %s \n", infname);
        return -1;
    }

    outfile = fopen(outfname, "w");
    
    if (outfile == (FILE *) NULL) 
    {
        printf("Can't open write file %s \n", outfname);
           return -1;
    }
    
    fputs("/* AUTO GEN PLEASE DO NOT MODIFY IT */ \n", outfile);
    fputs("/* AUTO GEN PLEASE DO NOT MODIFY IT */ \n", outfile);
    fputs("\n", outfile);
    fputs("\n", outfile);

    fprintf(outfile, "unsigned char %s[] = {\n", fw_name);

    while(1) 
    {
        char cc[3];    

        c = getc(infile);
    
        if (feof(infile))
               break;
    
        memset(cc,0,2);
    
        if (i >= 16) 
        {
               fputs("\n", outfile);    
               i = 0;
        }
    
        fputs("0x", outfile); 
        sprintf(cc,"%02x",c);
        fputs(cc, outfile);
        fputs(", ", outfile);
        i++;
    } 
    
    fputs("} ;\n", outfile);
    fclose(infile);
    fclose(outfile);
}    

int main(int argc ,char *argv[])
{
    char infname[512], in_rom_patch[512];
    char outfname[512], out_rom_patch[512];
    char chipsets[1024];
    char fw_name[128], rom_patch[128];
    char *dir;
    char *chipset, *token;
    int is_bin2h_fw = 0, is_bin2h_rom_patch = 0;
   
    dir = (char *)getenv("DIR");
    chipset = (char *)getenv("CHIPSET");
    memcpy(chipsets, chipset, strlen(chipset));

    if(!dir) 
    {
        printf("Environment value \"DIR\" not export \n");
         return -1;
    }

    if(!chipset) 
    {
        printf("Environment value \"CHIPSET\" not export \n");
        return -1;
    }        
    
    if (strlen(dir) > (sizeof(infname)-100)) 
    {
        printf("Environment value \"DIR\" is too long!\n");
        return -1;
    }
    
    chipset = strtok(chipsets, " ");

    while (chipset != NULL) 
    {
        memset(infname, 0, 512);
        memset(outfname, 0, 512);
        memset(in_rom_patch, 0, 512);
        memset(out_rom_patch, 0, 512);
        memset(fw_name, 0, 128);
        memset(rom_patch, 0, 128);
        strcat(infname, dir);
        strcat(outfname, dir);
        strcat(in_rom_patch, dir);
        strcat(out_rom_patch, dir);
        is_bin2h_fw = 0;
        is_bin2h_rom_patch = 0;

        if (strncmp(chipset, "mt7662t", 7) == 0) 
        {
            strcat(in_rom_patch, "/mcu/bin/mt7662t_patch_e1_hdr.bin");
            strcat(out_rom_patch,"/mt7662t_rom_patch.h");
            strcat(rom_patch, "mt7662t_rom_patch");
            is_bin2h_rom_patch = 1;
            printf("chipset = %s (%s)\n", chipset, out_rom_patch);
        }
        else if ((strncmp(chipset, "mt7662u", 7) == 0) || (strncmp(chipset, "mt7632u", 7) == 0)) 
        {
            strcat(in_rom_patch, "/mcu/bin/mt7662_patch_e3_hdr.bin");
            strcat(out_rom_patch,"/mt7662_rom_patch.h");
            strcat(rom_patch, "mt7662_rom_patch");
            is_bin2h_rom_patch = 1;
            printf("chipset = %s (%s)\n", chipset, out_rom_patch);
        } 
        else 
        {
            printf("unknown chipset = %s\n", chipset);
        }

        if (is_bin2h_fw)
             bin2h(infname, outfname, fw_name);

        if (is_bin2h_rom_patch)
            bin2h(in_rom_patch, out_rom_patch, rom_patch);

        chipset = strtok(NULL, " ");
    }

    exit(0);
}    
