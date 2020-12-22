#include "pml_hash.h"
#include <iostream>
#include <time.h>
using namespace std;

#define FILE_PATH "/mnt/pmemdir/pml_hash_file"

int main() {
    // char LOAD_RUN_PATH[][50] = {"benchmark/10w-rw-0-100-load.txt", "benchmark/10w-rw-0-100-run.txt",
    //                             "benchmark/10w-rw-25-75-load.txt", "benchmark/10w-rw-25-75-run.txt",
    //                             "benchmark/10w-rw-50-50-load.txt", "benchmark/10w-rw-50-50-run.txt",
    //                             "benchmark/10w-rw-75-25-load.txt", "benchmark/10w-rw-75-25-run.txt",
    //                             "benchmark/10w-rw-100-0-load.txt", "benchmark/10w-rw-100-0-run.txt"};
    // PMLHash hash(FILE_PATH);
    // char a[20];
    // int ret;
    // uint64_t key, cnt;
    // clock_t start ,end;
    
    // for(int i = 0; i < 2; i++){
    //     cnt = 0;
    //     FILE *fp = fopen(LOAD_RUN_PATH[i], "r");
    //     if(fp == NULL){
    //         perror("open file failed");
    //         exit(1);
    //     }
    //     printf("Open file successfully!\n");
    //     start = clock();
    //     while(fscanf(fp, "%s %lu", a, &key) != EOF){
    //         cnt++;
    //         if(!strcmp(a, "INSERT")){
    //             ret = hash.insert(key, key);
    //         }
    //         else if(!strcmp(a, "READ")){
    //             uint64_t val;
    //             ret = hash.search(key, val);
    //         }
    //         if(ret != 0){
    //             perror("operation failed");
    //             exit(1);
    //         }
    //     }
    //     end = clock();
    //     printf("opreation num:%ld time:%f\n", cnt, (double)(end - start)/CLOCKS_PER_SEC);
    //     printf("ops = %ld\n", cnt*CLOCKS_PER_SEC/(end - start));
    //     fclose(fp);
    // }
    
    PMLHash hash(FILE_PATH);
    // for (uint64_t i = 1; i <= HASH_SIZE * TABLE_SIZE; i++) {
    //     hash.insert(i, i);
    // }
    // for (uint64_t i = 1; i <= HASH_SIZE; i++) {
    //     uint64_t val;
    //     hash.search(i, val);
    //     cout << "key: " << i << "\nvalue: " << val << endl;
    // }

    // for (uint64_t i = HASH_SIZE * TABLE_SIZE + 1; 
    //      i <= (HASH_SIZE + 1) * TABLE_SIZE; i++) {
    //     hash.insert(i, i);
    // }
    // for (uint64_t i = HASH_SIZE * TABLE_SIZE + 1;
    //      i <= (HASH_SIZE + 1) * TABLE_SIZE; i++) {
    //     uint64_t val;
    //     hash.search(i, val);
    //     cout << "key: " << i << "\nvalue: " << val << endl;
    // }
    hash.display_table();
    return 0;
}