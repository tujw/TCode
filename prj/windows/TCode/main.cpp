#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include "alg\bubble.h"
#include "alg/binarySearch.h"

int main(int argc, char* argv[])
{
	int nums[5] = {0};

	int num = 0;
	for(int i = 0; i < (sizeof(nums) / sizeof(int)); ++i)
	{
		printf("please input a number:\n");
		std::cin >> num;

		nums[i] = num;
	}

	printf("begin sort\n");
	std::for_each(nums, nums + 5, [](const int& num){printf("%d\t", num);});
	printf("\n");
	bubble(nums, sizeof(nums) / sizeof(int));
	printf("sorted\n");
	std::for_each(nums, nums + 5, [](const int& num){printf("%d\t", num);});
	getchar();
	return 0;
}