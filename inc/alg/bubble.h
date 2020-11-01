#ifndef BUBBLE_HANDLE
#define BUBBLE_HANDLE

template<typename T>
void bubble(T nums[], const int length)
{
	if((nullptr == nums) || (length <= 0))
	{
		return ;
	}

	for(int i = 0; i < length; ++i)
	{
		int len = length - i - 1;

		for(int k = 0; k < len; ++k)
		{
			if(nums[k] > nums[k + 1])
			{
				T tmp = nums[k];
				nums[k] = nums[k + 1];
				nums[k + 1] = tmp;
			}
		}
	}
}

#endif