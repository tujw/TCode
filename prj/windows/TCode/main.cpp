#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "alg/binarySearch.h"

int main(int argc, char* argv[])
{
	int nums[] = {1, 2, 3, 4, 5, 6, 7, 8};
	int target = 0;
	printf("please input target search number\n");
	std::cin >> target;
	int low = 0, high = sizeof(nums) / sizeof(int) - 1;
	int index = binarySearch(nums, low, high, target);
	printf("binary search target = %d index = %d", target, index);

	getchar();
	getchar();
	return 0;
}