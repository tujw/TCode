#ifndef BINARY_SEARCH_HANDLE
#define BINARY_SEARCH_HANDLE

template<typename T>
int binarySearch(const T arr[], int low, int high, const T& target)
{
	if(nullptr == arr)
	{
		return -1;
	}

	if(low > high)
	{
		return -1;
	}

	while(low <= high)
	{
		int mid = low + (high - low) / 2;
		if(arr[mid] < target)
		{
			low = mid + 1;
		}
		else if(arr[mid] > target)
		{
			high = mid - 1;
		}
		else
		{
			return mid;
		}

	}

	return -1;
}

#endif
