#include "pml_hash.h"
#include <iostream>
#include <time.h>
using namespace std;

/*
* pmem file path
* NVM
* The folder with the file system ext4-dax mounted
*/
#define FILE_PATH "/mnt/pmemdir/pml_hash_file"

int main() {
    char LOAD_RUN_PATH[][50] = {"benchmark/10w-rw-0-100-load.txt", "benchmark/10w-rw-0-100-run.txt",
                                "benchmark/10w-rw-25-75-load.txt", "benchmark/10w-rw-25-75-run.txt",
                                "benchmark/10w-rw-50-50-load.txt", "benchmark/10w-rw-50-50-run.txt",
                                "benchmark/10w-rw-75-25-load.txt", "benchmark/10w-rw-75-25-run.txt",
                                "benchmark/10w-rw-100-0-load.txt", "benchmark/10w-rw-100-0-run.txt"};
    char opt[20];

    for(int j = 0; j < 10; j += 2){
        printf("\033[1;%dm# Test %d\033[0m\n", 33, j/2);
        PMLHash hash(FILE_PATH);    // create linear hash
        clock_t start ,end;         // counting time

        for(int i = 0; i < 2; i++){
            if(i == 0)  printf("\033[1;%dm* Load file\033[0m\n", 34);
            else        printf("\033[1;%dm* Run file\033[0m\n", 34);
            int ret, insert_cnt = 0, read_cnt = 0;
            uint64_t key, cnt = 0;

            FILE *fp = fopen(LOAD_RUN_PATH[j+i], "r");  // open the benchmark file
            if(fp == NULL){
                perror("open file failed");
                exit(1);
            }

            start = clock();
            while(fscanf(fp, "%s %lu", opt, &key) != EOF){
                cnt++;
                if(!strcmp(opt, "INSERT")){
                    ret = hash.insert(key, key);
                    insert_cnt++;
                }
                else if(!strcmp(opt, "READ")){
                    uint64_t val;
                    ret = hash.search(key, val);
                    read_cnt++;
                }
            }
            end = clock();
            fclose(fp);
            
            printf(" Total %ld opertions. Insert %d times. Read %d times\n", cnt, insert_cnt, read_cnt);
            printf(" cost time = %f\n", (double)(end - start)/CLOCKS_PER_SEC);
            printf(" OPS = %ld\n", cnt*CLOCKS_PER_SEC/(end - start));
        }
        printf("\n");
    }

    // Test for insert
    // PMLHash hash(FILE_PATH);
    // for (uint64_t i = 1; i <= (HASH_SIZE+1) * TABLE_SIZE; i++) {
    //     hash.insert(i, i);
    // }
    // hash.display_table();

    // Test for remove
    // printf("remove key: ");
    // for (uint64_t i = 1, j = 0; i <= HASH_SIZE * TABLE_SIZE; j++, i += j) {
    //     printf("%ld ", i);
    //     hash.remove(i);
    // }
    // printf("\n");
    // hash.display_table();

    // Test for search
    // int ret;
    // for (uint64_t i = 1, j = 0; i <= (HASH_SIZE+5) * TABLE_SIZE; j++, i += j) {
    //     uint64_t val;
    //     ret = hash.search(i, val);
    //     if(!ret)
    //         cout << "key: " << i << ", value: " << val << endl;
    //     else 
    //         cout << "no such key: " << i << endl;
    // }
    // hash.display_table();

    // Test for update
    // printf("remove key: ");
    // for (uint64_t i = 1, j = 0; i <= (HASH_SIZE+5) * TABLE_SIZE; j++, i += j) {
    //     printf("%ld ", i);
    //     hash.update(i, i*100);
    // }
    // printf("\n");
    // hash.display_table();

    
    return 0;
}