#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)


static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

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
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs - 1 /* lol */);
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev;  /* Pointer to the previous _block of allcated memory   */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};


struct _block *freeList = NULL; /* Free list to track the _blocks available */

#if defined NEXT && NEXT == 0
 struct _block * LAST_NF_VISITED = NULL;
#endif

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = freeList;
#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   /* Best fit */
   /* Which is really just finding the min node of a linked list */

   /* Stores our lowest value */
   struct _block * best = NULL;

   /* Traverse the LL, if we find a new min, redefine "best" to the current iteration */
   while (curr != NULL) {
      
      bool can_store_block = curr->free && curr->size >= size;
      bool is_way_better = (best == NULL) || curr->size < best->size;

      if (can_store_block && is_way_better) {
         best = curr;

         /* optimal case where we have a perfect fitting block, we can exit early */
         if (best->size == size) {
            break;
         }
      }

      curr = curr->next;
   }

   curr = best;
#endif

#if defined WORST && WORST == 0
   /* Worst fit */
   /* Which is really just finding the max of a linked list */
   struct _block * worst = NULL;

   /* Traverse the LL, if we find a new min, redefine "best" to the current iteration */
   while (curr != NULL) {
      bool can_store_block = (curr->free) && (curr->size > size);
      bool is_so_much_worse = (worst == NULL) || (curr->size > worst->size);

      if (can_store_block && is_so_much_worse) {
         worst = curr;
      }

      curr = curr->next;
   }

   curr = worst;
#endif

#if defined NEXT && NEXT == 0
   /* Next fit */
   /* Next fit picks up where we last left off, so we have a global that tracks the last exit block
      and start from there. It's no different from FF */

   if (LAST_NF_VISITED != NULL) {
      curr = LAST_NF_VISITED;
   }

   /* do FF */ 
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }

   LAST_NF_VISITED = curr;

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

   num_requested++;
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

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   /* Look for free _block */
   struct _block *last = freeList;
   struct _block *next = findFreeBlock(&last, size);

   /* If a free block is larger than the requested size then split the block into two. */ 
   if (next != NULL && next->size > size) {

      /* make a new block of size next->size - size, the amount of extra unused space */
      struct _block * new = (struct _block *)sbrk(sizeof(struct _block) + (next->size - size));
      new->size = next->size - size;
      new->free = true;

      /* if next has no subsequent block, then new has no subsequent block */
      /* if next has a subsequent block, that block belongs to new now */
      if (next->next == NULL) {
         new->next = NULL;
      } else {
         new->next = next->next;
      }

      /* next's subsequent block is now new */
      next->next = new;

      num_blocks++;
      num_grows++;
      num_splits++;
      num_requested++;
   }

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
      num_blocks++;
      num_grows++;

      max_heap += size;
   } else {
      /* a free block was found and can be repurposed */
      num_reuses++;
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark _block as in use */
   next->free = false;
   
   /* it worked */
   num_mallocs++;

   /* Return data address associated with _block */
   return BLOCK_DATA(next);
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
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;
   
   /* Coalesce two adjacent blocks */

   curr = freeList;
   /* traverse the LL
      if the current block is free, and the next one is free
      mash them together by setting the next block to the current one iteratively.
    */
   while (curr) {
      if ((curr && curr->next) && (curr->free && curr->next->free)) {

         curr->size = curr->next->size;
         curr->next = curr->next->next;

         /* when we coalesces, we reduce number of block allocated */
         num_coalesces++;
         num_blocks--;
      }
      curr = curr->next;
   }

   num_frees++;

}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/

