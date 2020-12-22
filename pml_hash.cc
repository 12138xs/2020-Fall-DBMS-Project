#include "pml_hash.h"

bitset<BITSET_SIZE> overflow_used;
bool* is_used;   // bitmap : 1 when be used, 0 for not. For recycling overflow table
uint64_t overflow_table_num;       //the max num of overflow table 


/**
 * PMLHash::display_table
 * 
 * dispaly the whole table, for test
 */
void PMLHash::display_table(){
    for(int i = 0; i < meta->size; i++){
        pm_table* temp = &table_arr[i];
        while(true){
            printf("Table %d: ", i);
            for(int j = 0; j < temp->fill_num; j++){
                    printf("%ld ", temp->kv_arr[j].key);
            }
            printf("\n");
            if(temp->next_offset == -1) break;
            else{
                temp = (pm_table*)(temp->next_offset + (uint64_t)overflow_addr);
            }
        }
        printf("\n");
    }
}


/**
 * PMLHash::PMLHash 
 * 
 * @param  {char*} file_path : the file path of data file
 * if the data file exist, open it and recover the hash
 * if the data file does not exist, create it and initial the hash
 */
PMLHash::PMLHash(const char* file_path) {
    size_t mapped_len;
    int is_pmem;
    char op[5];

    // get address
    if((start_addr = pmem_map_file(file_path, FILE_SIZE, PMEM_FILE_CREATE,
                0666, &mapped_len, &is_pmem)) == NULL) {
        perror("pmem_map_file");
        exit(1);
    }
    // printf("is_pmem = %d. Hello, pesistent memory!\n", is_pmem);
    overflow_addr = (void *)((uint64_t)start_addr + (uint64_t)FILE_SIZE/2); 
    meta = (metadata*)start_addr;
    table_arr = (pm_table*)((uint64_t)meta + sizeof(metadata));
    is_used = (bool *)((uint64_t)overflow_addr - BITSET_SIZE);
    overflow_table_num = (FILE_SIZE/2)/(sizeof(pm_table));

    // printf("0. Load\n");
    // printf("1. Run\n");
    // printf("Your choice(0-1): ");
    // scanf("%s", op);
    // if(!strcmp(op, "0")){
        // initial data
        meta->size = TABLE_INIT;
        meta->level = 0;
        meta->next = 0;
        meta->overflow_num = 0;
        for(int i = 0; i < HASH_SIZE; i++){
            table_arr[i].fill_num = 0;
            table_arr[i].next_offset = -1;
            for(int j = 0; j < TABLE_SIZE; j++)
                table_arr[i].kv_arr[j].key = table_arr[i].kv_arr[j].value = -1; 
        }
        memset(is_used, 0, BITSET_SIZE);
    // }
}


/**
 * PMLHash::~PMLHash 
 * 
 * unmap and close the data file
 */
PMLHash::~PMLHash() {
    pmem_persist(start_addr, FILE_SIZE);
    pmem_unmap(start_addr, FILE_SIZE);
}


/**
 * PMLHash 
 * 
 * split the hash table indexed by the meta->next
 * update the metadata
 */
void PMLHash::split() {
    if(meta->size >= HASH_SIZE) return ;    // whether it can be split
    meta->size++;

    uint64_t temp = TABLE_INIT * pow(2, meta->level);
    pm_table* split_table = (pm_table*)((uint64_t)start_addr + sizeof(metadata) + sizeof(pm_table)*(meta->next));       // find split table
    pm_table* new_table = (pm_table*)((uint64_t)start_addr + sizeof(metadata) + sizeof(pm_table)*(meta->next + temp));  // find new table
    pm_table* fill_table = split_table;
    int temp1 = 0;
    while(true){    // fill split table
        int temp2 = split_table->fill_num;
        for(int i = 0; i < temp2; i++){
            uint64_t h_key = hashFunc(split_table->kv_arr[i].key, meta->size);
            if(h_key == meta->next){
                split_table->fill_num--;
                uint64_t temp_key = split_table->kv_arr[i].key;
                uint64_t temp_value = split_table->kv_arr[i].value;
                split_table->kv_arr[i].key = -1;
                split_table->kv_arr[i].value = -1;

                fill_table->kv_arr[temp1].key = temp_key;
                fill_table->kv_arr[temp1].value = temp_value;
                temp1++;
                fill_table->fill_num++;  
                if(temp1 == TABLE_SIZE-1){
                    fill_table = (pm_table*)((uint64_t)overflow_addr + fill_table->next_offset);
                    temp1 = 0;
                }  
                continue;
            }
            new_table->kv_arr[new_table->fill_num].key = split_table->kv_arr[i].key;
            new_table->kv_arr[new_table->fill_num].value = split_table->kv_arr[i].value;
            split_table->kv_arr[i].key = -1;
            split_table->kv_arr[i].value = -1;
            split_table->fill_num--;
            new_table->fill_num++;
            if(new_table->fill_num == TABLE_SIZE){
                for(i = 0; i < overflow_table_num; i++){
                    if(is_used[i] != 1){
                        new_table->next_offset = (uint64_t)(i*sizeof(pm_table));
                        new_table = newOverflowTable(new_table->next_offset);
                        is_used[i] = 1;
                        break;
                    }
                }
            }

            // else{
            //     insert(split_table->kv_arr[i].key, split_table->kv_arr[i].value);
            //     meta->size--;
            //     remove(split_table->kv_arr[i].key);
            //     i--, meta->size++;
            // }
        }
        if(split_table->next_offset == -1)  break;
        split_table = (pm_table*)((uint64_t)overflow_addr + split_table->next_offset);
    }
    
    while(fill_table->next_offset != -1){
        pm_table* temp_table = (pm_table*)((uint64_t)overflow_addr + fill_table->next_offset);
        fill_table->next_offset = -1;
        fill_table = temp_table;
    }


    //update meta
    meta->next++;
    if(meta->next == temp){
        meta->next = 0;
        meta->level++;
    }
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
    // N_level = N * 2^level
    uint64_t n_level = TABLE_INIT * pow(2, meta->level+1);
    uint64_t h_key = key * 1 % n_level;
    if(h_key >= hash_size){
        n_level /= 2;
        h_key = h_key % n_level;
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
    pm_table* result = (pm_table*)((uint64_t)overflow_addr + offset);
    result->fill_num = 0;
    result->next_offset = -1;
    for(int i = 0; i < TABLE_SIZE; i++)
        result->kv_arr[i].key = result->kv_arr[i].value = -1;
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
    pm_table* table = (pm_table *)((uint64_t)start_addr + sizeof(metadata) + h_key * sizeof(pm_table));
    
    int split_flag = 0, i;     //cheak if the table is full
    while(table->fill_num == TABLE_SIZE){
        split_flag = 1;
        // printf("%ld\n",(uint64_t)table->next_offset);
        if(table->next_offset != -1){
            table = (pm_table *)((uint64_t)overflow_addr + table->next_offset);
        }
        else{
            for(i = 0; i < overflow_table_num; i++){
                if(is_used[i] != 1){
                    table->next_offset = (uint64_t)(i*sizeof(pm_table));
                    table = newOverflowTable(table->next_offset);
                    is_used[i] = 1;
                    break;
                }
            }
            if(i == overflow_table_num){
                printf("no enough overflow table for insert\n");
                return -1;
            }
        }
    }
    table->kv_arr[table->fill_num].key = key;
    table->kv_arr[table->fill_num].value = value;
    table->fill_num++;
    //if the table is full, split
    if(split_flag == 1)
        split();
    pmem_persist(start_addr,FILE_SIZE);
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
    pm_table* table = (pm_table *)((uint64_t)start_addr + sizeof(metadata) + h_key * sizeof(pm_table));
    
    while(true){
        for(int i = 0; i < table->fill_num; i++){
            if(table->kv_arr[i].key == key){
                value = table->kv_arr[i].value;
                return 0;
            }
        }
        if(table->next_offset == -1)    break;
        table = (pm_table*)((uint64_t)overflow_addr + table->next_offset);
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
    pm_table* table = (pm_table*)((uint64_t)start_addr + sizeof(metadata) + h_key * sizeof(pm_table));
    pm_table* last = table;

    while(true){
        for(int i = 0; i < table->fill_num; i++){
            if(table->kv_arr[i].key == key){
                for(int j = i; j < table->fill_num-1; j++){
                    table->kv_arr[j].key = table->kv_arr[j+1].key;
                    table->kv_arr[j].value = table->kv_arr[j+1].value;
                }
                while(table->next_offset != -1){
                    last = table;
                    table = (pm_table*)((uint64_t)overflow_addr + table->next_offset);
                    last->kv_arr[TABLE_SIZE-1].key = table->kv_arr[0].key;
                    last->kv_arr[TABLE_SIZE-1].value = table->kv_arr[0].value;
                    for(int j = 0; j < table->fill_num-1; j++){
                        table->kv_arr[j].key = table->kv_arr[j+1].key;
                        table->kv_arr[j].value = table->kv_arr[j+1].value;
                    }
                }
                table->fill_num--;
                //remove the empty table
                if(table->fill_num == 0 && last != table){
                    last->next_offset = -1;
                    meta->overflow_num--;
                    int temp = ((uint64_t)table - (uint64_t)overflow_addr)/sizeof(pm_table);
                    is_used[temp] = 0;
                }
                pmem_persist(start_addr,FILE_SIZE);
                return 0;
            }
        }
        if(table->next_offset == -1)    break;
        last = table;
        table = (pm_table*)((uint64_t)overflow_addr + table->next_offset);
    }
    printf("couldn't find the key, failed to remove\n");
    return -1;
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
    pm_table* table = (pm_table*)((uint64_t)start_addr + sizeof(metadata) + h_key * sizeof(pm_table));

    //while this table may be not the final table
    while(true){
        for(int i = 0; i < table->fill_num; i++){
            if(table->kv_arr[i].key == key){
                table->kv_arr[i].value = value;
                pmem_persist(start_addr,FILE_SIZE);
                return 0;
            }
        }
        //if no more overflow table
        if(table->next_offset == -1)    break;
        table = (pm_table*)((uint64_t)overflow_addr + table->next_offset);
    }
    printf("couldn't find the key, failed to update\n");
    return -1;
}