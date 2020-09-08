/*
 * Author: Daniel Kopta
 * Updated by: Erin Parker
 * CS 4400, University of Utah
 *
 * Simulator handout
 * A simple x86-like processor simulator.
 * Read in a binary file that encodes instructions to execute.
 * Simulate a processor by executing instructions one at a time and appropriately 
 * updating register and memory contents.
 *
 * Some code and pseudo code has been provided as a starting point.
 *
 * Completed by: Pranav Rajan
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "instruction.h"

// Forward declarations for helper functions
unsigned int get_file_size(int file_descriptor);
unsigned int *load_file(int file_descriptor, unsigned int size);
instruction_t *decode_instructions(unsigned int *bytes, unsigned int num_instructions);
unsigned int execute_instruction(unsigned int program_counter, instruction_t *instructions,
                                 int *registers, unsigned char *memory);
void print_instructions(instruction_t *instructions, unsigned int num_instructions);
void error_exit(const char *message);

// 17 registers
#define NUM_REGS 17
// 1024-byte stack
#define STACK_SIZE 1024

int main(int argc, char **argv)
{
  // Make sure we have enough arguments
  if (argc < 2)
    error_exit("must provide an argument specifying a binary file to execute");

  // Open the binary file
  int file_descriptor = open(argv[1], O_RDONLY);
  if (file_descriptor == -1)
    error_exit("unable to open input file");

  // Get the size of the file
  unsigned int file_size = get_file_size(file_descriptor);
  // Make sure the file size is a multiple of 4 bytes
  // since machine code instructions are 4 bytes each
  if (file_size % 4 != 0)
    error_exit("invalid input file");

  // Load the file into memory
  // We use an unsigned int array to represent the raw bytes
  // We could use any 4-byte integer type
  unsigned int *instruction_bytes = load_file(file_descriptor, file_size);
  close(file_descriptor);

  unsigned int num_instructions = file_size / 4;

  /****************************************/
  /**** Begin code to modify/implement ****/
  /****************************************/

  // Allocate and decode instructions (left for you to fill in)
  instruction_t *instructions = decode_instructions(instruction_bytes, num_instructions);

  // Optionally print the decoded instructions for debugging
  // Will not work until you implement decode_instructions
  // Do not call this function in your submitted final version
  // print_instructions(instructions, num_instructions);

  // Once you have completed Part 1 (decoding instructions), uncomment the below block

  // Allocate and initialize registers
  int *registers = (int *)calloc(NUM_REGS, sizeof(int));
  registers[6] = 1024;

  // Stack memory is byte-addressed, so it must be a 1-byte type
  // TODO allocate the stack memory. Do not assign to NULL.
  unsigned char *memory = (unsigned char *)calloc(STACK_SIZE, sizeof(char));

  // Run the simulation
  unsigned int program_counter = 0;

  // program_counter is a byte address, so we must multiply num_instructions by 4
  // to get the address past the last instruction
  while (program_counter != num_instructions * 4)
  {
    program_counter = execute_instruction(program_counter, instructions, registers, memory);
  }

  return 0;
}

/*
 * Decodes the array of raw instruction bytes into an array of instruction_t
 * Each raw instruction is encoded as a 4-byte unsigned int
*/
instruction_t *decode_instructions(unsigned int *bytes, unsigned int num_instructions)
{
  // TODO: Don't return NULL
  // instruction_t* retval = NULL;
  instruction_t *retval = (instruction_t *)calloc(num_instructions, sizeof(instruction_t));

  /*
  int i;
  for(i = ...){
    retval[i] = (fill in fields based on raw bits);
  */
  int i;
  for (i = 0; i < num_instructions; i++)
  {
    retval[i].opcode = bytes[i] >> 27;
    retval[i].first_register = (bytes[i] >> 22) & 0x1F;
    retval[i].second_register = (bytes[i] >> 17) & 0x1F;
    retval[i].immediate = bytes[i] & 0xFFFF;
  }

  return retval;
}

/*
 * Executes a single instruction and returns the next program counter
*/
unsigned int execute_instruction(unsigned int program_counter, instruction_t *instructions, int *registers, unsigned char *memory)
{
  // program_counter is a byte address, but instructions are 4 bytes each
  // divide by 4 to get the index into the instructions array
  instruction_t instr = instructions[program_counter / 4];

  int sum;
  long long int difference;

  switch (instr.opcode)
  {

  // opcode 0
  case subl:
    registers[instr.first_register] = registers[instr.first_register] - instr.immediate;
    break;

  // opcode 1
  case addl_reg_reg:
    registers[instr.second_register] = registers[instr.first_register] + registers[instr.second_register];
    break;

  // opcode 2
  case addl_imm_reg:
    registers[instr.first_register] = registers[instr.first_register] + instr.immediate;
    break;

  // opcode 3
  case imull:
    registers[instr.second_register] = registers[instr.first_register] * registers[instr.second_register];
    break;

  // opcode 4
  case shrl:
    registers[instr.first_register] = registers[instr.first_register] >> 1;
    break;

  // opcode 5
  case movl_reg_reg:
    registers[instr.second_register] = registers[instr.first_register];
    break;

  // opcode 6
  case movl_deref_reg:
    registers[instr.second_register] = memory[registers[instr.first_register] + instr.immediate];
    break;

  // opcode 7
  case movl_reg_deref:
    memory[registers[instr.second_register] + instr.immediate] = registers[instr.first_register];
    break;

  // opcode 8
  case movl_imm_reg:
    registers[instr.first_register] = instr.immediate;
    break;

  // opcode 9
  case cmpl:
    // sum for keeping track of all the different values 
    sum = 0; 
    difference = 0;

    // difference for use with the overflow flag
    difference = registers[instr.second_register] - registers[instr.first_register];
    // CF Check
    if ((uint32_t)registers[instr.second_register] < (uint32_t)registers[instr.first_register])
    {
      // set CF flag - bit 0 
      sum += 1;
    }
    // ZF Check
    if (registers[instr.second_register] == registers[instr.first_register])
    {
      // set ZF Flag - bit 6
      sum += 64;
    }

    // SF Check
    if ((difference >> 30) & 0x1 == 1)
    {
      // set SF Flag - bit 7
      sum += 128;
    }

    // OF Check
    // OF = SF^OF if register 2 < register 1
    // CASE 1: register 2 < 0 and register 1 > 0 and result > 0
    if ((registers[instr.second_register] < 0 && registers[instr.first_register] > 0) && (difference > INT64_MAX))
    {
      // set OF Flag - bit 11
      sum += 2048;
    }
    // CASE 2: register 2 > 0 and register 1 < 0 and result < 0
    if ((registers[instr.second_register] > 0 && registers[instr.first_register] < 0) && (difference < INT64_MIN))
    {
      // set OF Flag - bit 11;
      sum += 2048;
    }

    registers[16] = sum;

    break;

  // opcode 10
  case je:
    if ((registers[16] & 0x0040) != 0)
    {
      return program_counter + 4 + instr.immediate;
    }
    break;

  // opcode 11
  case jl:
    if ((registers[16] & 0x0080)^(registers[16] & 0x0800) != 0)
    {
      return program_counter + 4 + instr.immediate;
    }
    break;

  // //opcode 12
  // case jle:
  //   if (((registers[16] & 0x0080) ^ (registers[16] & 0x0800) != 0) || (registers[16] & 0x0040) != 0))
  //   {
      
  //   }
  //   break;

  // // opcode 13
  // case jge:
  //   break;

  // opcode 14
  case jbe:
    if (((registers[16] & 0x0001) != 0) || ((registers[16] & 0x0040) != 0))
    {
      return program_counter + 4 + instr.immediate;
    }
    break;

  // opcode 15
  case jmp:
    return program_counter + 4 + instr.immediate;
    break;

  // opcode 16
  case call:
    registers[6] = registers[6] - 4;
    memory[registers[6]] = program_counter + 4;
    return program_counter + 4 + instr.immediate;
    break;

  // opcode 17
  case ret:
    if (registers[6] == 1024)
    {
      exit(0);
    }
    else
    {
      program_counter = memory[registers[6]];
      registers[6] = registers[6] + 4;
    }
    break;

  // opcode 18
  case pushl:
    registers[6] = registers[6] - 4;
    memory[registers[6]] = registers[instr.first_register];
    break;

  // opcode 19
  case popl:
    registers[instr.first_register] = memory[registers[6]];
    registers[6] = registers[6] + 4;
    break;

  // opcode 20
  case printr:
    printf("%d (0x%x)\n", registers[instr.first_register], registers[instr.first_register]);
    break;

  // opcode 21
  case readr:
    scanf("%d", &(registers[instr.first_register]));
    break;

    // TODO: Implement remaining instructions
  }

  // TODO: Do not always return program_counter + 4
  //       Some instructions jump elsewhere

  // program_counter + 4 represents the subsequent instruction
  return program_counter + 4;
}

/*********************************************/
/****  DO NOT MODIFY THE FUNCTIONS BELOW  ****/
/*********************************************/

/*
 * Returns the file size in bytes of the file referred to by the given descriptor
*/
unsigned int get_file_size(int file_descriptor)
{
  struct stat file_stat;
  fstat(file_descriptor, &file_stat);
  return file_stat.st_size;
}

/*
 * Loads the raw bytes of a file into an array of 4-byte units
*/
unsigned int *load_file(int file_descriptor, unsigned int size)
{
  unsigned int *raw_instruction_bytes = (unsigned int *)malloc(size);
  if (raw_instruction_bytes == NULL)
    error_exit("unable to allocate memory for instruction bytes (something went really wrong)");

  int num_read = read(file_descriptor, raw_instruction_bytes, size);

  if (num_read != size)
    error_exit("unable to read file (something went really wrong)");

  return raw_instruction_bytes;
}

/*
 * Prints the opcode, register IDs, and immediate of every instruction, 
 * assuming they have been decoded into the instructions array
*/
void print_instructions(instruction_t *instructions, unsigned int num_instructions)
{
  printf("instructions: \n");
  unsigned int i;
  for (i = 0; i < num_instructions; i++)
  {
    printf("op: %d, reg1: %d, reg2: %d, imm: %d\n",
           instructions[i].opcode,
           instructions[i].first_register,
           instructions[i].second_register,
           instructions[i].immediate);
  }
  printf("--------------\n");
}

/*
 * Prints an error and then exits the program with status 1
*/
void error_exit(const char *message)
{
  printf("Error: %s\n", message);
  exit(1);
}
