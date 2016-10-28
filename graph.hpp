#include "graph.h"
#include <unistd.h>

//munmap unaligned page size, fails --> that's a bug.
#define _HUGEPAGE_
#ifdef _HUGEPAGE_
#include <sys/mman.h>
#endif

template<typename data_t,
	typename index_t>
data_t *read_in_hugepage(
		FILE *file,
		index_t nmem
){
	data_t *tmp = (data_t*)mmap(NULL,
			sizeof(data_t)*(nmem),
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB ,
			0,0);
	perror("comm: mmap");
	
	index_t ret=fread(tmp, sizeof(data_t), nmem, file);
	assert(ret==nmem);
	return tmp;
}

template<typename data_t,
	typename index_t>
data_t *read_in_normalpage(
		FILE *file,
		index_t nmem
){
	data_t *tmp=NULL;
	if(posix_memalign((void **)&tmp, getpagesize(),
				sizeof(data_t)*nmem))
		perror("posix_memalign");

	index_t ret=fread(tmp, sizeof(data_t), nmem, file);
	assert(ret==nmem);
	return tmp;
}

template<
typename file_vert_t, typename file_index_t, typename file_weight_t,
typename new_vert_t, typename new_index_t, typename new_weight_t>
graph<file_vert_t,file_index_t, file_weight_t,
new_vert_t,new_index_t,new_weight_t>
::graph(
		const char *beg_file,
		const char *csr_file,
		const char *weight_file)
{
	double tm=wtime();
	FILE *file=NULL;
	file_index_t ret;
	
	vert_count=fsize(beg_file)/sizeof(file_index_t) - 1;
	edge_count=fsize(csr_file)/sizeof(file_vert_t);
	
	file=fopen(beg_file, "rb");
	if(file!=NULL)
	{
		if(sizeof(file_index_t)==sizeof(new_index_t))
			beg_pos=
				read_in_hugepage
				<new_index_t, file_index_t>
				(file,vert_count+1);
		else
		{
			file_index_t *tmp_beg_pos=
				read_in_normalpage
				<file_index_t, file_index_t>
				(file, vert_count+1);
			
			beg_pos = (new_index_t*)mmap(NULL,
					sizeof(new_index_t)*(vert_count+1),
					PROT_READ | PROT_WRITE,
					MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB ,
					0,0);
			perror("beg_pos: mmap");

			for(new_index_t i=0;i<vert_count+1;++i)
				beg_pos[i]=(new_index_t)tmp_beg_pos[i];
			
			delete[] tmp_beg_pos;
		}
		fclose(file);
		
	}else std::cout<<"beg file cannot open\n";

	file=fopen(csr_file, "rb");
	if(file!=NULL)
	{
		if(sizeof(file_vert_t)==sizeof(new_vert_t))
			csr=
				read_in_hugepage
				<new_vert_t, file_index_t>
				(file,edge_count);
		else
		{
			file_vert_t *tmp_csr=
				read_in_normalpage
				<file_vert_t, file_index_t>
				(file, edge_count);
			
			csr = (new_vert_t*)mmap(NULL,
					sizeof(new_vert_t)*(edge_count),
					PROT_READ | PROT_WRITE,
					MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB ,
					0,0);
			perror("csr: mmap");

			for(new_index_t i=0;i<edge_count;++i)
				csr[i]=(new_vert_t)tmp_csr[i];
			
			delete[] tmp_csr;
		}
		
		fclose(file);
	}else std::cout<<"CSR file cannot open\n";


	file=fopen(weight_file, "rb");
	if(file!=NULL)
	{
		if(sizeof(file_weight_t)==sizeof(new_weight_t))
			weight=
				read_in_hugepage
				<new_weight_t, file_index_t>
				(file,edge_count);
		else
		{
			file_weight_t *tmp_weight=
				read_in_normalpage
				<file_weight_t, file_index_t>
				(file, edge_count);
			
			weight = (new_weight_t*)mmap(NULL,
					sizeof(new_weight_t)*(edge_count),
					PROT_READ | PROT_WRITE,
					MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB ,
					0,0);
			perror("weight: mmap");

			for(new_index_t i=0;i<edge_count;++i)
				weight[i]=(new_weight_t)tmp_weight[i];
			
			delete[] tmp_weight;
		}
		
		fclose(file);

	}
	else std::cout<<"Weight file cannot open\n";

	std::cout<<"Graph load (success): "<<vert_count<<" verts, "
		<<edge_count<<" edges "<<wtime()-tm<<" second(s)\n";
}

