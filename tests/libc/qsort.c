//
// Copyright 2015 Jeff Bush
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <stdlib.h>


#define NUM_ELEMENTS 16

int cmpint(const void *a, const void *b)
{
    return *((int*)a) - *((int*)b);
}

int main(void)
{
    int i;
    int sortArray[] = { 11, 6, 10, 9, 13, 12, 2, 15, 0, 14, 3, 1, 8, 4, 5, 7 };
    qsort(sortArray, NUM_ELEMENTS, sizeof(int), cmpint);
    for (i = 0; i < NUM_ELEMENTS; i++)
        printf("%d ", sortArray[i]);

    // CHECK: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
}
