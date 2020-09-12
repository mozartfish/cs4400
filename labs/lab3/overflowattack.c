/* Lab 3, 9/5/19 */

#include <stdio.h>
#include <stdlib.h>

// Ask the user input some numbers, then return their sum
int sum_vals() {
  long vals_to_sum[4];
  int num_vals; // How many numbers to ask for?
  int i; // Loop iterator
  int total = 0;

  printf("How many numbers to sum?\n");
  
  // Ask the user how many numbers they will input
  // Surely the user will never type a number > 4 here
  scanf("%d", &num_vals); 
  
  // Read specified number of values from the keyboard
  for(i = 0; i < num_vals; i++) 
    scanf("%ld", &vals_to_sum[i]);
  
  for(i = 0; i < num_vals; i++)
    total += vals_to_sum[i];

  return total;
}


// Why is this function here? It's never called.
void nevercalled() {
  printf("How did you get here?!?!\n");
  exit(1);
}


int main() {
  int sum = sum_vals();
  printf("Sum of values: %d\n", sum);
}


