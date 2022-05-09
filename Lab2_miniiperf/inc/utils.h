#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define 	BPS  123
#define 	KBPS 456
#define		MBPS 777
#define 	GBPS 953

int getMetric(char *str);
uint32_t convertToBps(uint32_t size, int metric);

/*
	Generic arraylist implementation
*/
#define CAPACITY 1000


typedef struct arrayList{
	void **data;
	size_t currentLength;
	size_t maxSize;
	size_t elementSize;

}arrayList_t;

arrayList_t *newArrayList(size_t elem_size);
void 		freeArrayList(arrayList_t *arrList);
int 		arrayListadd(arrayList_t *arr, void *inp);
void 		resize(arrayList_t *arr);
void 		arrayListRemoveAll(arrayList_t *arr);
void		*arrayListGetIndex(arrayList_t *arr, size_t index);

#endif
