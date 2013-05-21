#include "paging.h"

page_directory_t page_directory;
page_table_t* pd_ptr=page_directory.page_tables;

extern page_table_t* _page_tbl_kernel;

page_t empty_page(void)
{
	page_t ret;
	ret.value=0;
	return ret;
}

page_table_t empty_page_table(void)
{
	page_table_t ptbl;
	for(int i=0; i<PAGECOUNT; ++i)
	{
		ptbl.pages[i]=empty_page();
	}
	return ptbl;
}

void setup_paging(void)
{
	for(int i=0; i<PAGECOUNT; ++i)
	{
		pd_ptr[i]=empty_page_table();
	}
	pd_ptr[768]=*_page_tbl_kernel;
}
