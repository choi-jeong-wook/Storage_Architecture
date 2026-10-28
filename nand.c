/*
 * Lab #1 : NAND Simulator
 *  - Storage Architecture, SSE3069
 *
 * TA: Eunji Song, Jeongryul Kim
 * Prof: Dongkun Shin
 * Intelligent Embedded Systems Laboratory
 * Sungkyunkwan University
 * http://nyx.skku.ac.kr
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nand.h"

 /*
  * define your own data structure for NAND flash implementation
  */
typedef struct {
	unsigned char data[PAGE_DATA_SIZE];
	unsigned char spare[PAGE_SPARE_SIZE];
	int written;
} nand_page_t;

static nand_page_t* nand = NULL;
static int* next_page = NULL;

static int total_banks = 0;
static int blocks_per_bank = 0;
static int pages_per_block = 0;

/* get the index of a page in the 1-D NAND array */
static int get_page_index(int bank, int blk, int page)
{
	return (bank * blocks_per_bank + blk) * pages_per_block + page;
}

/* get the index of a block in the next_page array */
static int get_block_index(int bank, int blk)
{
	return bank * blocks_per_bank + blk;
}

/*
 * initialize the NAND flash memory
 * @nbanks: number of bank
 * @nblks: number of blocks per bank
 * @npages: number of pages per block
 *
 * Returns:
 *   0 on success
 *   NAND_ERR_INVALID if given dimension is invalid
 */
int nand_init(int nbanks, int nblks, int npages)
{
	int i;
	int num_pages;
	int num_blocks;

	if (nbanks <= 0 || nblks <= 0 || npages <= 0)
		return NAND_ERR_INVALID;

	num_blocks = nbanks * nblks;
	num_pages = num_blocks * npages;

	nand = (nand_page_t*)malloc(sizeof(nand_page_t) * num_pages);
	next_page = (int*)malloc(sizeof(int) * num_blocks);

	if (nand == NULL || next_page == NULL) {
		free(nand);
		free(next_page);
		nand = NULL;
		next_page = NULL;
		return NAND_ERR_INVALID;
	}

	total_banks = nbanks;
	blocks_per_bank = nblks;
	pages_per_block = npages;

	for (i = 0; i < num_pages; i++) {
		memset(nand[i].data, 0xFF, PAGE_DATA_SIZE);
		memset(nand[i].spare, 0xFF, PAGE_SPARE_SIZE);
		nand[i].written = 0;
	}

	for (i = 0; i < num_blocks; i++)
		next_page[i] = 0;

	return NAND_SUCCESS;
}

/*
 * write data and spare into the NAND flash memory page
 *
 * Returns:
 *   0 on success
 *   NAND_ERR_INVALID if target flash page address is invalid
 *   NAND_ERR_OVERWRITE if target page is already written
 *   NAND_ERR_POSITION if target page is empty but not the position to be written
 */
int nand_write(int bank, int blk, int page, void* data, void* spare)
{
	int page_idx;
	int block_idx;

	if (bank < 0 || bank >= total_banks ||
		blk < 0 || blk >= blocks_per_bank ||
		page < 0 || page >= pages_per_block)
		return NAND_ERR_INVALID;

	page_idx = get_page_index(bank, blk, page);
	block_idx = get_block_index(bank, blk);

	if (nand[page_idx].written)
		return NAND_ERR_OVERWRITE;

	if (page != next_page[block_idx])
		return NAND_ERR_POSITION;

	memcpy(nand[page_idx].data, data, PAGE_DATA_SIZE);
	memcpy(nand[page_idx].spare, spare, PAGE_SPARE_SIZE);

	nand[page_idx].written = 1;
	next_page[block_idx]++;

	return NAND_SUCCESS;
}


/*
 * read data and spare from the NAND flash memory page
 *
 * Returns:
 *   0 on success
 *   NAND_ERR_INVALID if target flash page address is invalid
 *   NAND_ERR_EMPTY if target page is empty
 */
int nand_read(int bank, int blk, int page, void* data, void* spare)
{
	int page_idx;

	if (bank < 0 || bank >= total_banks ||
		blk < 0 || blk >= blocks_per_bank ||
		page < 0 || page >= pages_per_block)
		return NAND_ERR_INVALID;

	page_idx = get_page_index(bank, blk, page);

	if (!nand[page_idx].written)
		return NAND_ERR_EMPTY;

	memcpy(data, nand[page_idx].data, PAGE_DATA_SIZE);
	memcpy(spare, nand[page_idx].spare, PAGE_SPARE_SIZE);

	return NAND_SUCCESS;
}

/*
 * erase the NAND flash memory block
 *
 * Returns:
 *   0 on success
 *   NAND_ERR_INVALID if target flash block address is invalid
 *   NAND_ERR_EMPTY if target block is already erased
 */
int nand_erase(int bank, int blk)
{
	int i;
	int first_page;
	int block_idx;
	int has_written_page = 0;

	if (bank < 0 || bank >= total_banks ||
		blk < 0 || blk >= blocks_per_bank)
		return NAND_ERR_INVALID;

	block_idx = get_block_index(bank, blk);
	first_page = block_idx * pages_per_block;

	for (i = 0; i < pages_per_block; i++) {
		if (nand[first_page + i].written) {
			has_written_page = 1;
			break;
		}
	}

	if (!has_written_page)
		return NAND_ERR_EMPTY;

	for (i = 0; i < pages_per_block; i++) {
		memset(nand[first_page + i].data, 0xFF, PAGE_DATA_SIZE);
		memset(nand[first_page + i].spare, 0xFF, PAGE_SPARE_SIZE);
		nand[first_page + i].written = 0;
	}

	next_page[block_idx] = 0;

	return NAND_SUCCESS;
}
