// Defines
#define FILENAME_MAX 1000
#define _XOPEN_SOURCE 500
#define MAX_AMOUNT 200010
#define NUMTHREAD 4      /* number of threads */
#define BUFLEN 24

//Standart
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

// Using on mkdir
#include <sys/stat.h>
#include <sys/types.h>

// Using on file system
#include <dirent.h>
#include <ftw.h>

// structs
static int one (const struct dirent *unused){
    return 1;
}

typedef struct FilePath{
    char in_filename[FILENAME_MAX];
    char out_filename[FILENAME_MAX];
    int type;
} file_path;


// Global scope
int count_path = 0;
char folder_name[FILENAME_MAX];
int thread_id[NUMTHREAD]  = {0,1,2,3};
struct FilePath file_paths[MAX_AMOUNT];
struct FilePath buffer[BUFLEN];
int buffer_index = 0;
int aux = 0;
int aux_ = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t can_consume  = PTHREAD_COND_INITIALIZER;
pthread_cond_t can_produce  = PTHREAD_COND_INITIALIZER;

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


int nftw_dirs_func(const char *fpath, const struct stat *stat, int tflags, struct FTW *ftwbuf){
    char in_path[FILENAME_MAX], parent_path[FILENAME_MAX];
    strcpy(in_path, fpath);
    strcpy(parent_path, fpath);

    const char delim[2] = "/";
    char *rest;
    rest = strtok(parent_path, delim);    
    int p_strlen = strlen(parent_path)+1;

    memmove(in_path, in_path+p_strlen, strlen(in_path));


    if(strlen(in_path) > 0){

        if(tflags == FTW_D){
            char out_path[FILENAME_MAX];
            strcpy(out_path, folder_name);
            strcat(out_path, "/");
            strcat(out_path, in_path);

            strcpy(file_paths[count_path].in_filename,fpath);
            strcpy(file_paths[count_path].out_filename, out_path);
            file_paths[count_path].type = 1;
            count_path++;

        }

        if(tflags == FTW_F){
            char out_path[FILENAME_MAX];
            strcpy(out_path, folder_name);
            strcat(out_path, "/");
            strcat(out_path, in_path);

            strcpy(file_paths[count_path].in_filename,fpath);
            strcpy(file_paths[count_path].out_filename, out_path);
            file_paths[count_path].type = 0;
            count_path++;
        }

    }


    return 0;
}

void fill_list_of_files(char path[]){
    int fd_limit = 5;
    int flags = FTW_CHDIR & FTW_DEPTH;
    int ret;

    ret = nftw(path, nftw_dirs_func, fd_limit, flags);
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


void* producer(void *arg) {
    while(aux < count_path){
        pthread_mutex_lock(&mutex);

        if(buffer_index == BUFLEN) { // full
            // wait until some elements are consumed
            pthread_cond_wait(&can_produce, &mutex);
        }

        // Copy folder
        if(file_paths[aux].type == 0){
            // copy file
            char fbuff[BUFSIZ];
            FILE *in, *out;
            size_t  n;

            in  = fopen(file_paths[aux].in_filename, "rb");
            out = fopen(file_paths[aux].out_filename, "wb");

            while ( (n=fread(fbuff,1,BUFSIZ,in)) != 0 ) {
                fwrite( fbuff, 1, n, out );
            }

            fclose(in);
            fclose(out);

        }else{
            // create dir
            mkdir(file_paths[aux].out_filename, 0777); 

        }

        // append data to the buffer
        buffer[buffer_index] = file_paths[aux];

        ++buffer_index;
        ++aux;



        // signal the fact that new items may be consumed
        pthread_cond_signal(&can_consume);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}


void* consumer(void *arg) {
    while(1){
        pthread_mutex_lock(&mutex);
        aux_++;
        if(aux_ >= count_path) { // empty and produced all
            pthread_mutex_unlock(&mutex);
            break;
        }

        while(buffer_index == 0) { // full
            // wait until some elements are consumed
            pthread_cond_wait(&can_consume, &mutex);
        }


        pthread_mutex_unlock(&mutex);


        --buffer_index;
        struct FilePath buffer_aux = buffer[buffer_index];
        if(buffer_aux.type == 0){
            char cmd[FILENAME_MAX] = "bzip2 ";
            strcat(cmd, buffer_aux.out_filename);
            FILE *fp = popen(cmd, "w");
            pclose(fp);
        }



        pthread_cond_signal(&can_produce);
    }



    // never reached
    return NULL;
}

int main(int argc, char *argv[]){
    char *orig_dir, *dest_name;
    pthread_t prod, cons1, cons2, cons3;

    // Read input from cli
    if(argc == 3){
        orig_dir = argv[1];
        dest_name = argv[2];
    }

    // Create folder .bz2
    create_dest_folder(dest_name);
    fill_list_of_files(orig_dir);
    
    pthread_create(&prod, NULL, producer, (void*)&thread_id[0]);
    pthread_create(&cons1, NULL, consumer, (void*)&thread_id[1]);
    pthread_create(&cons2, NULL, consumer, (void*)&thread_id[2]);
    pthread_create(&cons3, NULL, consumer, (void*)&thread_id[3]);

    pthread_join(prod, NULL); // will wait forever
    pthread_join(cons1, NULL);
    pthread_join(cons2, NULL);
    pthread_join(cons3, NULL);

    // Read orig dir recursively
    compress_file(dest_name);

    // // // Delete .bz2 folder
    rmrf_files(folder_name);

    return 0;
}