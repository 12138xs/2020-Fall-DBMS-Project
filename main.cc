#include "pml_hash.h"
#include <iostream>
#include <time.h>
using namespace std;

#define FILE_PATH "/mnt/pmemdir/pml_hash_file"

int main() {
    char LOAD_RUN_PATH[][50] = {"benchmark/10w-rw-0-100-load.txt", "benchmark/10w-rw-0-100-run.txt",
                                "benchmark/10w-rw-25-75-load.txt", "benchmark/10w-rw-25-75-run.txt",
                                "benchmark/10w-rw-50-50-load.txt", "benchmark/10w-rw-50-50-run.txt",
                                "benchmark/10w-rw-75-25-load.txt", "benchmark/10w-rw-75-25-run.txt",
                                "benchmark/10w-rw-100-0-load.txt", "benchmark/10w-rw-100-0-run.txt"};
    char opt[20];
    
    for(int j = 0; j < 10; j += 2){
        printf("# Test %d\n", j/2);
        PMLHash hash(FILE_PATH);
        clock_t start ,end;

        for(int i = 0; i < 2; i++){
            if(i == 0)  printf("* Load file\n");
            else        printf("* Run file\n");
            int ret, insert_cnt = 0, read_cnt = 0;
            uint64_t key, cnt = 0;

            start = clock();
            FILE *fp = fopen(LOAD_RUN_PATH[j+i], "r");
            if(fp == NULL){
                perror("open file failed");
                exit(1);
            }
            
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
            fclose(fp);
            end = clock();
            printf(" Total %ld opertions. Insert %d times. Read %d times\n", cnt, insert_cnt, read_cnt);
            printf(" cost time = %f\n", (double)(end - start)/CLOCKS_PER_SEC);
            printf(" OPS = %ld\n", cnt*CLOCKS_PER_SEC/(end - start));
        }
        printf("\n");
    }
    

    // PMLHash hash(FILE_PATH);
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
    // hash.display_table();
    return 0;
}