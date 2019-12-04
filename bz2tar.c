// Defines
#define FILENAME_MAX 1000
#define _XOPEN_SOURCE 500

//Standart
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Using on mkdir
#include <sys/stat.h>
#include <sys/types.h>

// Files
#include <dirent.h>
#include <ftw.h>

// structs
static int one (const struct dirent *unused){
    return 1;
}


// Global scope
char folder_name[FILENAME_MAX];


// Funcs
int create_dest_folder(char *dest_name){
    memcpy(folder_name, dest_name, strlen(dest_name) - 3); 
    folder_name[strlen(dest_name) - 4] = '\0';

    int check = mkdir(folder_name, 0777); 
  
    // check if directory is created or not 
    if (check){
        printf("Unable to create directory\n"); 
        exit(1); 
    } 

    return 1;

}


int nftw_copy_func(const char *in_path, const struct stat *stat, int tflags, struct FTW *ftwbuf){
    if(tflags == FTW_D){
        printf("%s\n", in_path);
        char out_path[FILENAME_MAX];
        strcpy(out_path, folder_name);
        strcat(out_path, "/");
        strcat(out_path, in_path);
        mkdir(out_path, 0777); 

    }

    if(tflags == FTW_F){
        printf("%s\n", in_path);
        char out_path[FILENAME_MAX];
        strcpy(out_path, folder_name);
        strcat(out_path, "/");
        strcat(out_path, in_path);
        
        char buff[BUFSIZ];
        FILE    *in, *out;
        size_t  n;

        in  = fopen(in_path, "rb");
        out = fopen(out_path, "wb");
        while ( (n=fread(buff,1,BUFSIZ,in)) != 0 ) {
            fwrite( buff, 1, n, out );
        }

        fclose(in);
        fclose(out);

    }
    return 0;
}

void copy_files(char path[]){
    int fd_limit = 5;
    int flags = FTW_CHDIR & FTW_DEPTH;
    int ret;

    ret = nftw(path, nftw_copy_func, fd_limit, flags);
}


int nftw_zip_func(const char *in_path, const struct stat *stat, int tflags, struct FTW *ftwbuf){
    if(tflags == FTW_F){
        char cmd[FILENAME_MAX] = "bzip2 ";
        strcat(cmd, in_path);
        FILE *fp = popen(cmd, "w");
        pclose(fp);
    }
    return 0;
}

void bzip2_files(char path[]){
    int fd_limit = 5;
    int flags = FTW_CHDIR & FTW_DEPTH;
    int ret;

    ret = nftw(path, nftw_zip_func, fd_limit, flags);
}



int nftw_rm_func(const char *in_path, const struct stat *stat, int tflags, struct FTW *ftwbuf){
    int rv = remove(in_path);
    if (rv)
        perror(in_path);

    return rv;
}

void rmrf_files(char path[]){
    int fd_limit = 5;
    int flags = FTW_DEPTH;
    int ret;

    ret = nftw(path, nftw_rm_func, fd_limit, flags);
}

void compress_file(char filename[]){
    char cmd[FILENAME_MAX] = "tar cf ";
    strcat(cmd, filename);
    strcat(cmd, " ");
    strcat(cmd, folder_name);

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

    // Create folder .bz2
    create_dest_folder(dest_name);

    // Copy files to .bz2 folder
    copy_files(orig_dir);

    // Run bzip2 on files
    bzip2_files(folder_name);


    // Read orig dir recursively
    compress_file(dest_name);

    // Delete .bz2 folder
    rmrf_files(folder_name);


    return 0;
}