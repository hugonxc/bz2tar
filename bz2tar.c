#define _XOPEN_SOURCE 500
#define FILENAME_MAX 1000
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <bzlib.h>
#include <ftw.h>

static int one (const struct dirent *unused){
    return 1;
}

void cp_files(char *filename){
    FILE *fptr_orig, *fptr_dest; 
    char c;  
  
    // Open one file for reading 
    fptr_orig = fopen(filename, "r"); 
    if (fptr_orig == NULL){ 
        printf("Cannot open file %s \n", filename); 
        exit(0); 
    } 
    
    // Open another file for writing
    char dest_tmp[FILENAME_MAX] = "./t/";
    strcat(dest_tmp, filename);

    fptr_dest = fopen(dest_tmp, "w"); 
    if (fptr_dest == NULL){ 
        printf("Cannot open file %s \n", filename); 
        exit(0); 
    } 
  
    // Read contents from file 
    c = fgetc(fptr_orig); 
    while (c != EOF){ 
        fputc(c, fptr_dest); 
        c = fgetc(fptr_orig); 
    } 
  
    fclose(fptr_orig); 
    fclose(fptr_dest); 
}

int nftwfunc(const char *fpath, const struct stat *stat, int tflags, struct FTW *ftwbuf){
    if(tflags == FTW_D){
        printf("| %s\n", fpath);
    }

    if(tflags == FTW_F){
        printf("path %s\n", fpath);
        const char token = '/';
        char *ret;
        ret = strrchr(fpath, token);
        ret = ret + 1;

        printf("ret %s\n", ret);
        cp_files(ret);

        // char cmd[FILENAME_MAX] = "bzip2 ";
        // strcat(cmd, ret);
        // FILE *fp = popen(cmd, "w");
        // pclose(fp);
        // printf("PASSED\n");
    }


    return 0;
}


int list_dirs (char path[]){
    int fd_limit = 5;
    int flags = FTW_CHDIR;
    int ret;

    ret = nftw(path, nftwfunc, fd_limit, flags);
}


void compress_file(char filename[], char orig_filename[]){
    char cmd[FILENAME_MAX] = "tar cf ";
    strcat(cmd, filename);
    strcat(cmd, " ");
    strcat(cmd, orig_filename);
    // strcat(cmd, ".bz2");

    printf("%s\n", cmd);
    FILE *fp = popen(cmd, "w");
    pclose(fp);
}





int main(int argc, char *argv[]){
    char *orig_dir, *dest_name;
    
    // Read input from cli
    if(argc == 3){
        orig_dir = argv[1];
        dest_name = argv[2];
    }
    

    // Read orig dir recursively
    list_dirs(orig_dir);
    // compress_file(dest_name, orig_dir);
    // do_stuff();


    return 0;
}