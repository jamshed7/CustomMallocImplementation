/*
  Name: Adarsh Pai
  ID: 1001530167

  Name: Jamshed Jahangir
  ID: 1001366821
*/

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s) (((((s)-1) >> 2) << 2) + 4)
#define BLOCK_DATA(b) ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr)-1)

static int atexit_registered = 0;
static int num_mallocs = 0;
static int num_frees = 0;
static int num_reuses = 0;
static int num_grows = 0;
static int num_splits = 0;
static int num_coalesces = 0;
static int num_blocks = 0;
static int num_requested = 0;
static int max_heap = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics(void)
{
   printf("\nheap management statistics\n");
   printf("mallocs:\t%d\n", num_mallocs);
   printf("frees:\t\t%d\n", num_frees);
   printf("reuses:\t\t%d\n", num_reuses);
   printf("grows:\t\t%d\n", num_grows);
   printf("splits:\t\t%d\n", num_splits);
   printf("coalesces:\t%d\n", num_coalesces);
   printf("blocks:\t\t%d\n", num_blocks);
   printf("requested:\t%d\n", num_requested);
   printf("max heap:\t%d\n", max_heap);
}

struct _block
{
   size_t size;         /* Size of the allocated _block of memory in bytes */
   struct _block *next; /* Pointer to the next _block of allcated memory   */
   bool free;           /* Is this _block free?                     */
   char padding[3];
};

struct _block *freeList = NULL; /* Free list to track the _blocks available */
struct _block *next_fit_store = NULL;//this must be set to NULL
/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size)
{
   struct _block *curr = freeList;

#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   //printf("TODO: Implement best fit here\n");
   while (curr)
   {
      //curr->next = freeList;   //may be redundant, check later

      if (curr->next->free && curr->next->size >= size && curr->next->size < curr->size)
      {
         *last = curr->next;
         curr = curr->next;
      }
      else if (!(curr->free) && curr->next == NULL)
      {
         *last = curr->next;
         curr = curr->next->next;
      }

      if (curr != NULL && !(curr->free))
      {
         curr = curr->next->next;
      }
   }
#endif

#if defined WORST && WORST == 0
   struct _block *worst_fit = freeList;
   while (worst_fit)
   {
      if (worst_fit->free && worst_fit->size >= size && worst_fit->size > curr->size)
      {
         *last = curr = worst_fit;
      }
      else if (!(curr->free) && (worst_fit->next == NULL))
      {
         *last = curr->next;
         curr = worst_fit->next;
      }
      worst_fit = worst_fit->next;
   }

   if (curr != NULL && !(curr->free))
   {
      curr = worst_fit->next;
   }
#endif

#if defined NEXT && NEXT == 0
   //printf("TODO: Implement next fit here\n");
   if (next_fit_store != NULL){
      curr = next_fit_store;
   }
   while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr = curr->next;
   }
   next_fit_store = curr;
#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size)
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1)
   {
      return NULL;
   }

   /* Update freeList if not set */
   if (freeList == NULL)
   {
      freeList = curr;
   }

   /* Attach new _block to prev _block */
   if (last)
   {
      last->next = curr;
   }

   /* Update _block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */
void *malloc(size_t size)
{
   num_mallocs += 1;
   if (atexit_registered == 0)
   {
      atexit_registered = 1;
      atexit(printStatistics);
   }

   /* Align to multiple of 4 */
   size_t prevSize = size;
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0)
   {
      return NULL;
   }

   /* Look for free _block */
   struct _block *last = freeList;
   struct _block *next = findFreeBlock(&last, size);

   /* TODO: Split free _block if possible */

   if (next != NULL && next->size > size)
   {
      num_grows += 1;
      num_splits += 1;
      size_t diff = next->size - size;
      next->size = next->size - size;
      struct _block *new_block = (struct _block *)sbrk(sizeof(struct _block) + diff);
      new_block->next = next->next == NULL ? NULL : next->next;
      next->next = new_block;
      num_blocks += 1;
      new_block->size = diff;
      new_block->free = true;
   }

   /* Could not find free _block, so grow heap */
   if (next == NULL)
   {
      next = growHeap(last, size);
      max_heap += prevSize;
      num_blocks += 1;
      num_grows += 1;
   }
   else
   {
      num_reuses += 1;
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL)
   {
      return NULL;
   }

   /* Mark _block as in use */
   next->free = false;

   /* Return data address associated with _block */
   return BLOCK_DATA(next);
}

void *calloc(size_t nmemb, size_t size)
{
   struct _block *ptr = malloc(nmemb * size);
   memset(ptr, 0, sizeof(ptr));
   return ptr;
}

void *realloc(void *ptr, size_t size){
   struct _block *new_ptr = malloc(size);
   memcpy(new_ptr,ptr,size);
   free(ptr);
   return new_ptr;
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr)
{
   if (ptr == NULL)
   {
      return;
   }

   /* Make _block as free */
   num_frees += 1;
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   curr = freeList;

   while (curr)
   {
      struct _block *currNext = curr->next;
      if (curr && currNext && curr->free & currNext->free)
      {
         num_coalesces++;
         num_blocks--;
         curr->size = currNext->size;
         curr->next = currNext->next;
      }
      curr = curr->next;
   }
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
