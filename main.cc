#include "pml_hash.h"
#include <iostream>
#include <time.h>
using namespace std;

#define FILE_PATH "/mnt/pmemdir/pml_hash_file"

int main() {
    char LOAD_RUN_PATH[][100] = {"benchmark/10w-rw-0-100-load.txt",
                                "benchmark/10w-rw-0-100-run.txt",
                                "benchmark/10w-rw-25-75-load.txt",
                                "benchmark/10w-rw-25-75-run.txt",
                                "benchmark/10w-rw-50-50-load.txt",
                                "benchmark/10w-rw-50-50-run.txt",
                                "benchmark/10w-rw-75-25-load.txt",
                                "benchmark/10w-rw-75-25-run.txt",
                                "benchmark/10w-rw-100-0-load.txt",
                                "benchmark/10w-rw-100-0-run.txt"
                                };

    PMLHash hash(FILE_PATH);
    char a[20],b[20],temp[20];
    int ret;
    uint64_t key;
    uint64_t cnt;
    clock_t start ,end;
    
    for(int i = 0; i < 2; i++){
        cnt = 0;
        start = 0;
        end = 0;
        FILE *fp = fopen(LOAD_RUN_PATH[i], "r");
        if(fp == NULL){
            perror("open");
            exit(0);
        }
        printf("open file successfully\n");
        start = clock();
        while(fscanf(fp, "%s %s", a, b)!=EOF){
            cnt++;
            strncpy(temp, b, 9);
            temp[8] = '\0';
            key = (uint64_t)atoi(temp);
            if(!strcmp(a, "INSERT")){
                ret = hash.insert(key, key);
            }
            else if(!strcmp(a, "READ")){
                uint64_t val;
                ret = hash.search(key, val);
            }
        }
        end = clock();
        printf("opreation num:%ld time:%f\n",cnt, double(end - start)/CLOCKS_PER_SEC);
        fclose(fp);
    }
    



    // int ret;
    // for (uint64_t i = 1; i <= HASH_SIZE * TABLE_SIZE; i++) {
    //     ret = hash.insert(i, i);
        
    //     if(ret == 0){
    //         // printf("insert %ld successful\n",i);
           
    //     }
    // }
    //  hash.display_table();
    // for (uint64_t i = 1; i <= HASH_SIZE*TABLE_SIZE; i++) {
    //     uint64_t val;
    //     ret = hash.remove(i);
    //     if(ret == 0){
    //         printf("\n");
    //     }
    //     cout << "key: " << i << "\nvalue: " << val << endl;
    // }

    // for (uint64_t i = HASH_SIZE * TABLE_SIZE + 1; i <= (HASH_SIZE + 1) * TABLE_SIZE; i++) {
    //     ret = hash.insert(i, i);
    //     ret = hash.update(i,i*2);
    //     if(ret == 0){
    //         printf("\n");
    //     }
    // }
    //  hash.display_table();
    // for (uint64_t i = HASH_SIZE * TABLE_SIZE + 1;
    //      i <= (HASH_SIZE + 1) * TABLE_SIZE; i++) {
    //     uint64_t val;
    //     ret = hash.search(i, val);
    //     if(ret == 0){
    //         printf("\n");
    //     }
    //     cout << "key: " << i << "\nvalue: " << val << endl;
    // }    
    // hash.~PMLHash();
    // return 0;
}