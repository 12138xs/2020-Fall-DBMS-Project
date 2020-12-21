#include "pml_hash.h"


bool* is_used;   // 1 when be used, 0 for not. For recycling overflow table
uint64_t overflow_table_num;       //the max num of overflow table 

/**
 * PMLHash::PMLHash 
 * 
 * @param  {char*} file_path : the file path of data file
 * if the data file exist, open it and recover the hash
 * if the data file does not exist, create it and initial the hash
 */

//dispaly the whole table, for test
void PMLHash::display_table(){
    
    for(int i = 0; i < meta->size; i++){
        
        for(int j = 0; j < TABLE_SIZE; j++){
            if(table_arr[i].kv_arr[j].key != -1){
                printf("%ld ",table_arr[i].kv_arr[j].key);
            }
        }
        if(table_arr[i].next_offset != -1){
            pm_table* temp = (pm_table*)((uint64_t)start_addr + table_arr[i].next_offset);
            while(temp->next_offset != -1){
            
                for(int j = 0; j < TABLE_SIZE; j++){
                    if(temp->kv_arr[j].key != -1){
                        printf("%ld ",temp->kv_arr[j].key);
                    }
                }
                temp = (pm_table*)(temp->next_offset + (uint64_t)start_addr);
            }
            for(int j = 0; j < TABLE_SIZE; j++){
                    if(temp->kv_arr[j].key != -1){
                        printf("%ld ",temp->kv_arr[j].key);
                    }
                }
                temp = (pm_table*)(temp->next_offset + (uint64_t)start_addr);
        }
        printf("\n");
    }
    printf("\n");
}

PMLHash::PMLHash(const char* file_path) {
    
    // int fp = open(file_path, O_RDWR,0);
    // if(fp == -1){
    //     printf("file doesn't exist,create file\n");
    //     fp = open(file_path, O_RDWR|O_CREAT,0);
    //     if(fp == -1){
    //         printf("create file failed\n");
    //         return;
    //     }
    //     start_addr = mmap(0, FILE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fp, 0);
    
         start_addr = (void*)malloc(FILE_SIZE);
    //initialize data for new file 
        if(start_addr == NULL){
            printf("get pointer error\n");
            return ;
        }
        else{
            printf("ojbk\n");
        }
        overflow_addr = (void *)((uint64_t)start_addr + (uint64_t)FILE_SIZE/2); 
        meta = (metadata*)((uint64_t)start_addr);
        meta->size = 2;
        meta->level = 1;
        meta->next = 0;
        meta->overflow_num = 0;
        table_arr = (pm_table*)( (uint64_t)meta + sizeof(metadata) );
        for(int i = 0; i < HASH_SIZE; i++){
            table_arr[i].fill_num = 0;
            table_arr[i].next_offset = -1;
            
            for(int j = 0; j < TABLE_SIZE; j++){
                table_arr[i].kv_arr[j].key = -1; 
                table_arr[i].kv_arr[j].value = -1; 
            }
        }
        overflow_table_num = (FILE_SIZE/2)/(sizeof(pm_table));
        // printf("%ld\n",overflow_table_num/8);
        is_used = (bool*)((uint64_t)overflow_addr - (uint64_t)overflow_table_num*sizeof(bool));
        uint64_t unsuit = (uint64_t)is_used - ((uint64_t)start_addr + sizeof(metadata) + 16*sizeof(pm_table));
        // printf("%ld", unsuit);
        if(unsuit < 0){
            printf("no enough memory for is_used list, HASH_SIZE should be smaller\n");
        }

        for(int i = 0; i < overflow_table_num; i++){
            is_used[i] = 0;
        }

    // }

}
/**
 * PMLHash::~PMLHash 
 * 
 * unmap and close the data file
 */
PMLHash::~PMLHash() {
    
    free(start_addr);
    start_addr = NULL;
    // pmem_unmap(start_addr, FILE_SIZE);
}
/**
 * PMLHash 
 * 
 * split the hash table indexed by the meta->next
 * update the metadata
 */
void PMLHash::split() {
    // whether it can be split
    if(meta->size == HASH_SIZE){
        return;
    }
    //if it can be split
    // printf("%ld\n",meta->next);
    if(meta->next == 0){
        meta->level++;
    }
    int temp = 1;
    for(int i = 0; i < meta->level-1; i++){
        temp = temp * 2;
    }
    
    //determine split table
    pm_table* split_table = (pm_table*)((uint64_t)start_addr + sizeof(metadata) + sizeof(pm_table)*(meta->next));
    //fill split table
    while(split_table->next_offset != -1){
        for(int i = 0; i < TABLE_SIZE; i++){
            meta->size ++;
            uint64_t temp_h_key = hashFunc(split_table->kv_arr[i].key,meta->size);
            if(temp_h_key/temp == 0){
                meta->size--;
                continue;
            }
            else{
                insert(split_table->kv_arr[i].key,split_table->kv_arr[i].value);
                meta->size --;
                remove(split_table->kv_arr[i].key);
            }
        }
        split_table = (pm_table*)((uint64_t)start_addr + split_table->next_offset);
    }
    for(int i = 0; i < TABLE_SIZE; i++){
        if(split_table->kv_arr[i].key == -1){
            continue;
        }
        meta->size++;
        uint64_t temp_h_key = hashFunc(split_table->kv_arr[i].key, meta->size);
        //if the key should be kept
        if(temp_h_key/temp == 0){
            meta->size--;
            continue;
        }
        //if the key belong to the split table
        else{
            insert(split_table->kv_arr[i].key,split_table->kv_arr[i].value);
            meta->size--;
            remove(split_table->kv_arr[i].key); 
        }
    }
    //update meta
    meta->size++;
    meta->next++;
    if(meta->next == temp ){
        meta->next = 0;
    }

   return ;
}
/**
 * PMLHash 
 * 
 * @param  {uint64_t} key     : key
 * @param  {size_t} hash_size : the N in hash func: idx = hash % N
 * @return {uint64_t}         : index of hash table array
 * 
 * need to hash the key with proper hash function first
 * then calculate the index by N module
 */
uint64_t PMLHash::hashFunc(const uint64_t &key, const size_t &hash_size) {
    
    uint64_t temp_size = 2;
    while(temp_size < hash_size){
        temp_size *= 2;
    }
    uint64_t h_key = key * 1 % temp_size;
    if(h_key >= hash_size){
        temp_size /= 2;
        h_key = h_key%temp_size;
    }
    return h_key;
}

/**
 * PMLHash 
 * 
 * @param  {uint64_t} offset : the file address offset of the overflow hash table
 *                             to the start of the whole file
 * @return {pm_table*}       : the virtual address of new overflow hash table
 */
pm_table* PMLHash::newOverflowTable(uint64_t &offset) {
    pm_table* result = (pm_table*)((uint64_t)start_addr + offset);
    result->fill_num = 0;
    result->next_offset = -1;
    for(int i = 0; i < TABLE_SIZE; i++){
        result->kv_arr[i].key = -1;
        result->kv_arr[i].value = -1;
    }
    uint64_t num = (uint64_t)(offset - FILE_SIZE/2)/sizeof(pm_table);
    is_used[num] = 1;
    return result;
}

/**
 * PMLHash 
 * 
 * @param  {uint64_t} key   : inserted key
 * @param  {uint64_t} value : inserted value
 * @return {int}            : success: 0. fail: -1
 * 
 * insert the new kv pair in the hash
 * 
 * always insert the entry in the first empty slot
 * 
 * if the hash table is full then split is triggered
 */
int PMLHash::insert(const uint64_t &key, const uint64_t &value) {
    uint64_t h_key = hashFunc(key, meta->size);
    // printf("%ld\n",h_key);
    pm_table* table = (pm_table*)((uint64_t)start_addr + sizeof(metadata) + h_key * sizeof(pm_table) );
    
    int split_flag = 0;     //cheak if the table is full
    while(table->fill_num == TABLE_SIZE){
        split_flag = 1;
        // printf("%ld\n",(uint64_t)table->next_offset);
        if(table->next_offset != -1){
            table = (pm_table*)((uint64_t)start_addr + table->next_offset);
        }
        else{
            int i;
            for (i = 0; i < overflow_table_num; i++){
                if(is_used[i] != 1){
                    
                    uint64_t a = FILE_SIZE/2 + (uint64_t)(i*sizeof(pm_table));
                    table->next_offset = a;
                    table = newOverflowTable(a);
                     break;
                }
            }
            if(i == overflow_table_num){
                printf("no enough overflow table for insert\n");
                return -1;
            }
            
        }
    }
    for(int i = 0; i < TABLE_SIZE; i++){
        if(table->kv_arr[i].key == -1){
            table->kv_arr[i].key = key;
            table->kv_arr[i].value = value;
            table->fill_num++;
            break;
        }
    }
    //if the table is full, split
    if(split_flag == 1){
        split();
    }
    // pmem_persist();
    return 0;
}

/**
 * PMLHash 
 * 
 * @param  {uint64_t} key   : the searched key
 * @param  {uint64_t} value : return value if found
 * @return {int}            : 0 found, -1 not found
 * 
 * search the target entry and return the value
 */
int PMLHash::search(const uint64_t &key, uint64_t &value) {
    uint64_t h_key = hashFunc(key, meta->size);

    pm_table* table = (pm_table*)((uint64_t)start_addr + sizeof(metadata) + h_key * sizeof(pm_table) );
    
    while(table->next_offset != -1){
        for(int i = 0; i < TABLE_SIZE; i++){
            if(table->kv_arr[i].key!=-1 && table->kv_arr[i].key == key){
                value = table->kv_arr[i].value;
                return 0;
            }
        }
        table = (pm_table*)((uint64_t)start_addr + table->next_offset);
    }
    for(int i = 0; i < table->fill_num; i++){
        if(table->kv_arr[i].key == key){
            value = table->kv_arr[i].value;
            return 0;
        }
    }
    return -1;
}

/**
 * PMLHash 
 * 
 * @param  {uint64_t} key : target key
 * @return {int}          : success: 0. fail: -1
 * 
 * remove the target entry, move entries after forward
 * if the overflow table is empty, remove it from hash
 */
int PMLHash::remove(const uint64_t &key) {
    uint64_t h_key = hashFunc(key, meta->size);

    pm_table* table = (pm_table*)((uint64_t)start_addr + sizeof(metadata) + h_key * sizeof(pm_table) );
    pm_table* last = table;
    while(table->next_offset != -1){
        for(int i = 0; i < TABLE_SIZE; i++){
            if(table->kv_arr[i].key != -1 && table->kv_arr[i].key == key){
                table->kv_arr[i].key = -1;
                table->kv_arr[i].value = -1;
                table->fill_num --;
                //remove the empty table
                if(table->fill_num == 0){
                    last->next_offset = table->next_offset;
                    meta->overflow_num--;
                    if((uint64_t)table >= (uint64_t)overflow_addr){
                        int temp = ((uint64_t)table - (uint64_t)overflow_addr)/sizeof(pm_table);
                        is_used[temp] = 0;
                    }
                }
                // pmem_persist();
                return 0;
            }
        }
        last = table;
        table = (pm_table*)((uint64_t)start_addr + table->next_offset);
        
           
    }
    for(int i = 0; i < TABLE_SIZE; i++){
        if(table->kv_arr[i].key == key){
            table->kv_arr[i].key = -1;
            table->kv_arr[i].value = -1;
            table->fill_num --;
            if(table->fill_num == 0){
                last->next_offset = table->next_offset;
                if((uint64_t)table >= (uint64_t)overflow_addr){
                    uint64_t temp = ((uint64_t)table - (uint64_t)overflow_addr)/sizeof(pm_table);
                    is_used[temp] = 0;
                }
            }
            // pmem_persist();
            return 0;
        }
    }
    printf("couldn't find the key, failed to remove\n");
    return -1;

    // pmem_persist();
}

/**
 * PMLHash 
 * 
 * @param  {uint64_t} key   : target key
 * @param  {uint64_t} value : new value
 * @return {int}            : success: 0. fail: -1
 * 
 * update an existing entry
 */
int PMLHash::update(const uint64_t &key, const uint64_t &value) {
    uint64_t h_key = hashFunc(key, meta->size);

    pm_table* table = (pm_table*)((uint64_t)start_addr + sizeof(metadata) + h_key * sizeof(pm_table) );
    //while this table may be not the final table
    while(table->next_offset != -1){
        for(int i = 0; i < TABLE_SIZE; i++){
            if(table->kv_arr[i].key!=-1 && table->kv_arr[i].key == key){
                table->kv_arr[i].value = value;
                // pmem_persist();
                return 0;
            }
        }
        //if no more overflow table
        if(table->next_offset != -1){
            table = (pm_table*)((uint64_t)start_addr + table->next_offset);
        }
        else{
            printf("couldn't find the key, failed to update\n");
            return -1;
        }
    }

    for(int i = 0; i < table->fill_num; i++){
        if(table->kv_arr[i].key == key){
            table->kv_arr[i].value = value;
            // pmem_persist();
            return 0;
        }
    }

}