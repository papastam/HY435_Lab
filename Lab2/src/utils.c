#include "utils.h"


void removeEndingCharacter(char *str){
	str[strlen(str) - 1] = '\0';
}

int getMetric(char *str){
	char c = str[strlen(str) - 1];
	if(c == 'M' || c == 'm' ){
		removeEndingCharacter(str);
		return MBPS;
	}
	else if(c == 'K' || c == 'k'){
		removeEndingCharacter(str);
		return KBPS;
	}
	else if(c == 'G' || c == 'g'){
		removeEndingCharacter(str);
		return GBPS;
	}
	else
		return BPS;
}

uint32_t convertToBps(uint32_t size, int metric){
	if(metric == BPS)
		return size;
	else if(metric == KBPS){
		return size * 1024;
	}else if(metric == MBPS){
		return size * (1024*1024);
	}else if(metric == GBPS){
		return size * (1024*1024*1024);
	}
} 

arrayList_t *newArrayList(size_t elem_size){
	arrayList_t *arr = malloc(sizeof(struct arrayList ));
	if(!arr){
		exit(EXIT_FAILURE);
	}
	arr->currentLength = 0;
	arr->maxSize = CAPACITY;
	arr->elementSize = elem_size;
	arr->data = calloc(CAPACITY, sizeof(void * ));
	if(!arr->data){
		free(arr);
		exit(EXIT_FAILURE);
	}
	return arr;

}

void freeArrayList(arrayList_t *arrList){
	if(arrList == NULL ) 
		return;
	free(arrList->data);
	free(arrList);

}

int arrayListadd(arrayList_t *arr, void *inp){
	if(arr != NULL && inp != NULL ){
		if(arr->currentLength >= arr->maxSize ){
			resize(arr);
		}
		arr->data[arr->currentLength] = inp;
		arr->currentLength++;
		return 1;
	}
	return -1;
	
}

/* increases the size of the arraylist */ 
void resize(arrayList_t *arr){
	size_t new_size = (arr->maxSize * 2);
	void *tmp = realloc(arr->data, new_size);
	
	if(!tmp){
		free(arr);
		exit(EXIT_FAILURE);	
	}
	arr->maxSize = new_size;
}



/* 	memsets to 0 all data from the arrayList and
	shrinks it's size to the default capacity 1000 elements */
void arrayListRemoveAll(arrayList_t *arr){

	size_t new_length = CAPACITY * arr->elementSize;
	void *new_data = realloc(arr->data, new_length);
	if(!new_data){
		exit(EXIT_FAILURE);
	}
	memset(new_data, 0, new_length);
	arr->data = new_data;
}

void* arrayListGetIndex(arrayList_t *arr, size_t index){
	return (arr->data[index]);
}


